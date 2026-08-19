#include "o2/stdafx.h"

#ifdef PLATFORM_WASM

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/WebAssembly/OpenGL.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <cstring>

// While a pointer is captured the browser keeps sending its move/up events to the canvas even
// outside the window — without it a mouseup beyond the window edge is never delivered at all
// and the cursor stays stuck "pressed". The capture auto-releases on pointerup.
// The right button is driven by pointer events with pointer capture instead of
// mouse events: macOS browsers swallow the button-2 mouseup around the
// contextmenu sequence, so an RMB press delivered via mousedown often never got
// its release. Captured pointerup is delivered reliably.
EM_JS(void, o2_EnablePointerCapture, (), {
    var canvas = document.getElementById('canvas');
    if (!canvas || !canvas.setPointerCapture)
        return;
    canvas.addEventListener('pointerdown', function(e) {
        try { canvas.setPointerCapture(e.pointerId); } catch (err) {}
        if (e.button === 2)
            _o2_web_alt_cursor_down(e.offsetX, e.offsetY);
    });
    var altUp = function(e) {
        if (e.button === 2)
            _o2_web_alt_cursor_up();
    };
    canvas.addEventListener('pointerup', altUp);
    canvas.addEventListener('pointercancel', altUp);
});

// Keyboard events are registered on the window; when the page has HTML inputs
// (the editor's git bar) typing there must not reach the engine
EM_JS(int, o2_IsHtmlInputFocused, (), {
    var el = document.activeElement;
    if (!el)
        return 0;
    var tag = el.tagName;
    return (tag === 'INPUT' || tag === 'TEXTAREA' || el.isContentEditable) ? 1 : 0;
});

namespace o2
{
    // Last character produced by each virtual key, captured from DOM keydown
    // events. EditBox consumes it via GetWasmUnicodeForKey — the browser is the
    // only place that knows the active layout (incl. Shift/AltGr/cyrillic).
    static UInt16 gVkUnicode[256] = {};

    UInt16 GetWasmUnicodeForKey(KeyboardKey code)
    {
        if (code < 0 || code >= 256)
            return 0;

        return gVkUnicode[code];
    }

    namespace
    {
        Vec2I gCanvasResolution = Vec2I(960, 640);

        void StoreKeyUnicode(KeyboardKey vk, const char* domKey)
        {
            if (vk < 0 || vk >= 256 || !domKey)
                return;

            // Single-character "key" values are the typed character; named keys
            // map to the control codes the desktop backends produce
            unsigned char c0 = (unsigned char)domKey[0];
            if (c0 != 0 && domKey[1] == 0)
            {
                gVkUnicode[vk] = (UInt16)c0; // ASCII
                return;
            }

            // Decode a single two-byte UTF-8 sequence (cyrillic and most latin
            // extras); longer sequences are rare on keyboards - skip them
            if ((c0 & 0xE0) == 0xC0 && domKey[1] != 0 && domKey[2] == 0)
            {
                gVkUnicode[vk] = (UInt16)(((c0 & 0x1F) << 6) | ((unsigned char)domKey[1] & 0x3F));
                return;
            }

            if (std::strcmp(domKey, "Enter") == 0)          { gVkUnicode[vk] = 13; return; }
            if (std::strcmp(domKey, "Backspace") == 0)      { gVkUnicode[vk] = 8; return; }

            gVkUnicode[vk] = 0;
        }

        KeyboardKey MapDomKeyToVK(const char* code)
        {
            if (!code) return 0;

            if (std::strncmp(code, "Key", 3) == 0 && code[3] != 0)
                return (KeyboardKey)code[3];

            if (std::strncmp(code, "Digit", 5) == 0 && code[5] != 0)
                return (KeyboardKey)code[5];

            if (std::strcmp(code, "ArrowLeft") == 0)  return 0x25;
            if (std::strcmp(code, "ArrowUp") == 0)    return 0x26;
            if (std::strcmp(code, "ArrowRight") == 0) return 0x27;
            if (std::strcmp(code, "ArrowDown") == 0)  return 0x28;

            if (std::strcmp(code, "Space") == 0)     return 0x20;
            if (std::strcmp(code, "Enter") == 0)     return 0x0D;
            if (std::strcmp(code, "Escape") == 0)    return 0x1B;
            if (std::strcmp(code, "Backspace") == 0) return 0x08;
            if (std::strcmp(code, "Tab") == 0)       return 0x09;
            if (std::strcmp(code, "Delete") == 0)    return 0x2E;
            if (std::strcmp(code, "Home") == 0)      return 0x24;
            if (std::strcmp(code, "End") == 0)       return 0x23;
            if (std::strcmp(code, "PageUp") == 0)    return 0x21;
            if (std::strcmp(code, "PageDown") == 0)  return 0x22;
            if (std::strcmp(code, "Insert") == 0)    return 0x2D;

            if (std::strcmp(code, "ShiftLeft") == 0 || std::strcmp(code, "ShiftRight") == 0) return 0x10;
            if (std::strcmp(code, "ControlLeft") == 0 || std::strcmp(code, "ControlRight") == 0) return 0x11;
            if (std::strcmp(code, "AltLeft") == 0 || std::strcmp(code, "AltRight") == 0) return 0x12;
            if (std::strcmp(code, "MetaLeft") == 0 || std::strcmp(code, "MetaRight") == 0) return 0x11; // Cmd as Ctrl

            // Punctuation — Windows VK_OEM_* codes, matching the desktop backends
            if (std::strcmp(code, "Semicolon") == 0)    return 0xBA;
            if (std::strcmp(code, "Equal") == 0)        return 0xBB;
            if (std::strcmp(code, "Comma") == 0)        return 0xBC;
            if (std::strcmp(code, "Minus") == 0)        return 0xBD;
            if (std::strcmp(code, "Period") == 0)       return 0xBE;
            if (std::strcmp(code, "Slash") == 0)        return 0xBF;
            if (std::strcmp(code, "Backquote") == 0)    return 0xC0;
            if (std::strcmp(code, "BracketLeft") == 0)  return 0xDB;
            if (std::strcmp(code, "Backslash") == 0)    return 0xDC;
            if (std::strcmp(code, "BracketRight") == 0) return 0xDD;
            if (std::strcmp(code, "Quote") == 0)        return 0xDE;

            if (std::strncmp(code, "Numpad", 6) == 0)
            {
                const char* rest = code + 6;
                if (rest[0] >= '0' && rest[0] <= '9' && rest[1] == 0) return (KeyboardKey)(0x60 + (rest[0] - '0'));
                if (std::strcmp(rest, "Multiply") == 0) return 0x6A;
                if (std::strcmp(rest, "Add") == 0)      return 0x6B;
                if (std::strcmp(rest, "Subtract") == 0) return 0x6D;
                if (std::strcmp(rest, "Decimal") == 0)  return 0x6E;
                if (std::strcmp(rest, "Divide") == 0)   return 0x6F;
                if (std::strcmp(rest, "Enter") == 0)    return 0x0D;
            }

            if (std::strncmp(code, "F", 1) == 0 && code[1] >= '0' && code[1] <= '9')
            {
                int n = atoi(code + 1);
                if (n >= 1 && n <= 12) return (KeyboardKey)(0x6F + n); // F1 = 0x70
            }

            return 0;
        }

        Vec2F GetCanvasCursorPos(double x, double y)
        {
            return Vec2F((float)(x - gCanvasResolution.x * 0.5),
                         (float)(gCanvasResolution.y * 0.5 - y));
        }

        EM_BOOL OnKeyDown(int, const EmscriptenKeyboardEvent* e, void*)
        {
            if (o2_IsHtmlInputFocused())
                return EM_FALSE;

            if (Application::IsSingletonInitialzed())
            {
                KeyboardKey vk = MapDomKeyToVK(e->code);
                StoreKeyUnicode(vk, e->key);
                o2Input.OnKeyPressed(vk);
            }
            return EM_TRUE;
        }

        EM_BOOL OnKeyUp(int, const EmscriptenKeyboardEvent* e, void*)
        {
            if (o2_IsHtmlInputFocused())
                return EM_FALSE;

            if (Application::IsSingletonInitialzed())
                o2Input.OnKeyReleased(MapDomKeyToVK(e->code));
            return EM_TRUE;
        }

        // Button 2 is handled by the pointer-event path (o2_EnablePointerCapture)
        EM_BOOL OnMouseDown(int, const EmscriptenMouseEvent* e, void*)
        {
            if (!Application::IsSingletonInitialzed()) return EM_TRUE;
            Vec2F p = GetCanvasCursorPos(e->targetX, e->targetY);
            if (e->button == 0) o2Input.OnCursorPressed(p);
            return EM_TRUE;
        }

        EM_BOOL OnMouseUp(int, const EmscriptenMouseEvent* e, void*)
        {
            if (!Application::IsSingletonInitialzed()) return EM_TRUE;
            if (e->button == 0) o2Input.OnCursorReleased();
            return EM_TRUE;
        }

        EM_BOOL OnMouseMove(int, const EmscriptenMouseEvent* e, void*)
        {
            if (Application::IsSingletonInitialzed())
            {
                o2Input.OnCursorMoved(GetCanvasCursorPos(e->targetX, e->targetY));

                // A mouseup that happened outside the window may never arrive: a move with the
                // button bit cleared while the cursor is still held down is that missed release
                if ((e->buttons & 1) == 0 && o2Input.IsCursorDown())
                    o2Input.OnCursorReleased();
            }
            return EM_TRUE;
        }

        EM_BOOL OnWheel(int, const EmscriptenWheelEvent* e, void*)
        {
            if (Application::IsSingletonInitialzed())
            {
                // DOM deltaY is positive when scrolling down; o2 follows the
                // Windows convention (positive = up, ~120 per notch)
                float delta = (float)-e->deltaY;
                if (e->deltaMode == DOM_DELTA_LINE)
                    delta *= 40.0f;
                o2Input.OnMouseWheel(delta * 1.2f);
            }
            return EM_TRUE;
        }

        EM_BOOL OnTouchEvent(int eventType, const EmscriptenTouchEvent* e, void*)
        {
            if (!Application::IsSingletonInitialzed()) return EM_TRUE;

            for (int i = 0; i < e->numTouches; i++)
            {
                const auto& t = e->touches[i];
                if (!t.isChanged) continue;
                Vec2F p = GetCanvasCursorPos(t.targetX, t.targetY);
                CursorId id = (CursorId)t.identifier;
                if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART)      o2Input.OnCursorPressed(p, id);
                else if (eventType == EMSCRIPTEN_EVENT_TOUCHMOVE)  o2Input.OnCursorMoved(p, id);
                else /* end/cancel */                              o2Input.OnCursorReleased(id);
            }
            return EM_TRUE;
        }

        EM_BOOL OnResize(int, const EmscriptenUiEvent*, void*)
        {
            double w = 0, h = 0;
            emscripten_get_element_css_size("#canvas", &w, &h);
            Vec2I newSize((int)w, (int)h);
            if (newSize.x <= 0) newSize.x = 1;
            if (newSize.y <= 0) newSize.y = 1;

            if (newSize != gCanvasResolution)
            {
                gCanvasResolution = newSize;
                emscripten_set_canvas_element_size("#canvas", newSize.x, newSize.y);
                if (Application::IsSingletonInitialzed())
                    o2Application.SetWindowSize(newSize);
            }
            return EM_TRUE;
        }

        void MainLoopTick()
        {
            if (Application::IsSingletonInitialzed())
                o2Application.Update();
        }
    }

    extern "C" EMSCRIPTEN_KEEPALIVE void o2_web_alt_cursor_down(float x, float y)
    {
        if (Application::IsSingletonInitialzed())
            o2Input.OnAltCursorPressed(GetCanvasCursorPos(x, y));
    }

    extern "C" EMSCRIPTEN_KEEPALIVE void o2_web_alt_cursor_up()
    {
        if (Application::IsSingletonInitialzed())
            o2Input.OnAltCursorReleased();
    }

    void Application::Initialize()
    {
        BasicInitialize();
    }

    void Application::InitializePlatform()
    {
        double w = 0, h = 0;
        emscripten_get_element_css_size("#canvas", &w, &h);
        if (w <= 0 || h <= 0) { w = 960; h = 640; }
        gCanvasResolution = Vec2I((int)w, (int)h);
        emscripten_set_canvas_element_size("#canvas", gCanvasResolution.x, gCanvasResolution.y);

        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnKeyDown);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnKeyUp);
        emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, OnMouseDown);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnMouseUp);
        emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnMouseMove);
        emscripten_set_wheel_callback("#canvas", nullptr, EM_TRUE, OnWheel);
        emscripten_set_touchstart_callback("#canvas", nullptr, EM_TRUE, OnTouchEvent);
        emscripten_set_touchend_callback("#canvas", nullptr, EM_TRUE, OnTouchEvent);
        emscripten_set_touchmove_callback("#canvas", nullptr, EM_TRUE, OnTouchEvent);
        emscripten_set_touchcancel_callback("#canvas", nullptr, EM_TRUE, OnTouchEvent);
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnResize);

        o2_EnablePointerCapture();
    }

    void Application::Shutdown()
    {}

    void Application::Launch()
    {
        mLog->Out("Application launched!");

        OnStarted();
        onStarted.Invoke();
        o2Events.OnApplicationStarted();

        emscripten_set_main_loop(&MainLoopTick, 0, EM_TRUE);
    }

    void Application::Update()
    {
        ProcessFrame();
    }

    void Application::SetFullscreen(bool fullscreen /*= true*/) {}
    bool Application::IsFullScreen() const { return false; }
    void Application::Maximize() {}
    bool Application::IsMaximized() const { return true; }
    void Application::SetResizible(bool resizible) {}
    bool Application::IsResizible() const { return true; }
    void Application::CheckCursorInfiniteMode() {}

    void Application::SetWindowSizePlatform(const Vec2I& size) {}

    Vec2I Application::GetWindowSize() const
    {
        return gCanvasResolution;
    }

    void Application::SetWindowPosition(const Vec2I& position) {}

    Vec2I Application::GetWindowPosition() const
    {
        return Vec2I();
    }

    void Application::SetWindowCaption(const String& caption)
    {
        EM_ASM({ document.title = UTF8ToString($0); }, caption.Data());
    }

    String Application::GetWindowCaption() const { return ""; }

    void Application::SetContentSize(const Vec2I& size)
    {
        gCanvasResolution = size;
        emscripten_set_canvas_element_size("#canvas", size.x, size.y);
    }

    Vec2I Application::GetContentSize() const
    {
        return gCanvasResolution;
    }

    Vec2I Application::GetScreenResolution() const
    {
        int w = EM_ASM_INT({ return window.screen.width; });
        int h = EM_ASM_INT({ return window.screen.height; });
        return Vec2I(w, h);
    }

    void Application::SetCursor(CursorType type) {}
    void Application::SetCursorPosition(const Vec2F& position) {}

    String Application::GetBinPath() const { return "/"; }
}

#endif // PLATFORM_WASM
