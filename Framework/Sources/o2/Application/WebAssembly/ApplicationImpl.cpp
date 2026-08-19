#include "o2/stdafx.h"

#ifdef PLATFORM_WASM

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Application/WebAssembly/WasmKeyboard.h"
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
// The same click focuses the canvas, so key events reach the page even when the game runs
// inside an iframe.
// The right button is driven by pointer events with pointer capture instead of
// mouse events: macOS browsers swallow the button-2 mouseup around the
// contextmenu sequence, so an RMB press delivered via mousedown often never got
// its release. Captured pointerup is delivered reliably.
EM_JS(void, o2_EnablePointerCapture, (), {
    var canvas = document.getElementById('canvas');
    if (!canvas)
        return;
    canvas.addEventListener('pointerdown', function(e) {
        try { canvas.focus(); } catch (err) {}
        if (canvas.setPointerCapture)
        {
            try { canvas.setPointerCapture(e.pointerId); } catch (err) {}
        }
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

        // Single-character "key" values are the typed character; named keys map to the
        // control codes the desktop backends produce. EditBox consumes this via
        // GetWasmUnicodeForKey - the browser is the only place that knows the layout
        void StoreKeyUnicode(KeyboardKey vk, const char* domKey)
        {
            if (vk < 0 || vk >= 256 || !domKey)
                return;

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

            if (std::strcmp(domKey, "Enter") == 0)     { gVkUnicode[vk] = 13; return; }
            if (std::strcmp(domKey, "Backspace") == 0) { gVkUnicode[vk] = 8; return; }

            gVkUnicode[vk] = 0;
        }

        Vec2F GetCanvasCursorPos(double x, double y)
        {
            return Vec2F((float)(x - gCanvasResolution.x * 0.5),
                         (float)(gCanvasResolution.y * 0.5 - y));
        }

        // Unhandled keys are left to the browser (EM_FALSE): consuming them would kill the
        // page shortcuts, while the handled ones must be consumed so arrows and space don't
        // scroll the page under the canvas
        EM_BOOL OnKeyDown(int, const EmscriptenKeyboardEvent* e, void*)
        {
            // Typing into the page's HTML inputs must not reach the engine
            if (o2_IsHtmlInputFocused())
                return EM_FALSE;

            KeyboardKey key = DomKeyCodeToKeyboardKey(e->code);
            if (key == 0 || !Application::IsSingletonInitialzed())
                return EM_FALSE;

            StoreKeyUnicode(key, e->key);
            o2Input.OnKeyPressed(key);
            return EM_TRUE;
        }

        EM_BOOL OnKeyUp(int, const EmscriptenKeyboardEvent* e, void*)
        {
            if (o2_IsHtmlInputFocused())
                return EM_FALSE;

            KeyboardKey key = DomKeyCodeToKeyboardKey(e->code);
            if (key == 0 || !Application::IsSingletonInitialzed())
                return EM_FALSE;

            o2Input.OnKeyReleased(key);
            return EM_TRUE;
        }

        // Leaving the tab never delivers the keyup, so everything held stays pressed forever
        EM_BOOL OnFocusLost(int, const EmscriptenFocusEvent*, void*)
        {
            if (!Application::IsSingletonInitialzed())
                return EM_FALSE;

            for (auto& key : o2Input.GetDownKeys())
                o2Input.OnKeyReleased(key.keyCode);

            return EM_FALSE;
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
        emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnFocusLost);
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
