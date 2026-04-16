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

namespace o2
{
    namespace
    {
        Vec2I gCanvasResolution = Vec2I(960, 640);

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
            if (Application::IsSingletonInitialzed())
                o2Input.OnKeyPressed(MapDomKeyToVK(e->code));
            return EM_TRUE;
        }

        EM_BOOL OnKeyUp(int, const EmscriptenKeyboardEvent* e, void*)
        {
            if (Application::IsSingletonInitialzed())
                o2Input.OnKeyReleased(MapDomKeyToVK(e->code));
            return EM_TRUE;
        }

        EM_BOOL OnMouseDown(int, const EmscriptenMouseEvent* e, void*)
        {
            if (!Application::IsSingletonInitialzed()) return EM_TRUE;
            Vec2F p = GetCanvasCursorPos(e->targetX, e->targetY);
            if (e->button == 0) o2Input.OnCursorPressed(p);
            else if (e->button == 2) o2Input.OnAltCursorPressed(p);
            return EM_TRUE;
        }

        EM_BOOL OnMouseUp(int, const EmscriptenMouseEvent* e, void*)
        {
            if (!Application::IsSingletonInitialzed()) return EM_TRUE;
            if (e->button == 0) o2Input.OnCursorReleased();
            else if (e->button == 2) o2Input.OnAltCursorReleased();
            return EM_TRUE;
        }

        EM_BOOL OnMouseMove(int, const EmscriptenMouseEvent* e, void*)
        {
            if (Application::IsSingletonInitialzed())
                o2Input.OnCursorMoved(GetCanvasCursorPos(e->targetX, e->targetY));
            return EM_TRUE;
        }

        EM_BOOL OnWheel(int, const EmscriptenWheelEvent* e, void*)
        {
            // Input API has OnCursorWheelDelta — optional; ignore for minimal MVP
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
