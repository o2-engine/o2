#include "o2/stdafx.h"

#ifdef PLATFORM_WINDOWS

#include "o2/Application/Application.h"
#include "o2/Events/EventSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Debug/StackTrace.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include <Windows.h>

namespace o2
{
    class WndProcFunc
    {
    public:
        static LRESULT WndProc(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    };

    void Application::Initialize()
    {
        BasicInitialize();
    }

    void Application::InitializePlatform()
    {
        mWindowed = true;
        mWindowedSize = Vec2I(800, 600);
        mWindowedPos = Vec2I(0, 0);
        mWindowResizible = true;
        mActive = false;

        if (mNeedPlatformInitialization)
        {
            mLog->Out("Initializing window..");

            mWndStyle = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

            WNDCLASSEX wndClass;
            wndClass.cbSize = sizeof(WNDCLASSEX);
            wndClass.style = mWndStyle;
            wndClass.lpfnWndProc = (WNDPROC)WndProcFunc::WndProc;
            wndClass.cbClsExtra = 0;
            wndClass.cbWndExtra = 0;
            wndClass.hInstance = NULL;
            wndClass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
            if (!wndClass.hIcon)
                wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);

            wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            wndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
            wndClass.lpszMenuName = NULL;
            wndClass.lpszClassName = "o2App";
            wndClass.hIconSm = wndClass.hIcon;

            if (!RegisterClassEx(&wndClass))
            {
                mLog->Error("Can't register class");
                return;
            }

            if (!(mHWnd = CreateWindowEx(NULL, wndClass.lpszClassName, "o2 application",
                                         WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                         mWindowedPos.x, mWindowedPos.y, mWindowedSize.x, mWindowedSize.y,
                                         NULL, NULL, NULL, NULL)))
            {

                mLog->Error("Can't create window (CreateWindowEx)");
                return;
            }

            mLog->Out("Window initialized!");
        }
    }

    void Application::Shutdown()
    {
        DestroyWindow(mHWnd);
    }

    void Application::SetFullscreen(bool fullscreen /*= true*/)
    {
        if (fullscreen)
        {
            mLog->Out("Setting full screen");
        }
        else
        {
            mLog->Out("Setting windowed..");

            mWindowed = true;

            RECT rt = { mWindowedPos.x, mWindowedPos.y, mWindowedPos.x + mWindowedSize.x, mWindowedPos.y + mWindowedSize.y };
            AdjustWindowRect(&rt, mWndStyle, false);
            SetWindowPos(mHWnd, HWND_NOTOPMOST, mWindowedPos.x, mWindowedPos.y,
                         mWindowedSize.x, mWindowedSize.y, SWP_SHOWWINDOW);

            mLog->Out("Complete");
        }
    }

    void Application::CheckCursorInfiniteMode()
    {
        int threshold = 10;
        POINT p, lp;
        GetCursorPos(&p);
        lp = p;

        Vec2I resolution = GetScreenResolution();

        if (p.x > resolution.x - threshold)
            p.x = threshold;
        else if (p.x < threshold)
            p.x = resolution.x - threshold;

        if (p.y > resolution.y - threshold)
            p.y = threshold;
        else if (p.y < threshold)
            p.y = resolution.y - threshold;

        SetCursorPos(p.x, p.y);

        if (p.x != lp.x || p.y != lp.y)
            mCursorCorrectionDelta = Vec2F((float)(lp.x - p.x), (float)(lp.y - p.y));
    }

    void Application::Launch()
    {
        ShowWindow(mHWnd, IsBackgroundWindow() ? SW_SHOWNOACTIVATE : SW_SHOW);

        mLog->Out("Application launched!");

        OnStarted();
        onStarted.Invoke();
        o2Events.OnApplicationStarted();

        MSG msg;
        memset(&msg, 0, sizeof(msg));

        //mTimer->Reset();

        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                ProcessFrame();
            }
        }

        o2Events.OnApplicationClosing();
        OnClosing();
        onClosing.Invoke();
        
        Deinitialize();
    }

    bool Application::IsFullScreen() const
    {
        return !mWindowed;
    }

    void Application::Maximize()
    {
        ShowWindow(mHWnd, SW_MAXIMIZE);
    }

    bool Application::IsMaximized() const
    {
        WINDOWPLACEMENT pl;
        GetWindowPlacement(mHWnd, &pl);
        return pl.showCmd == SW_MAXIMIZE;
    }

    void Application::SetResizible(bool resizible)
    {
        if (resizible == mWindowResizible)
            return;

        mWindowResizible = resizible;

        if (mWindowResizible)
            mWndStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
        else
            mWndStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX;

        mLog->Out("Set resizible: " + ((mWindowResizible ? (String)"true" : (String)"false")));

        SetWindowLong(mHWnd, GWL_STYLE, mWndStyle);
    }

    bool Application::IsResizible() const
    {
        return mWindowResizible;
    }

    void Application::SetWindowSizePlatform(const Vec2I& size)
    {
        mWindowedSize = size;
        mLog->Out("Set sindow size: %ix%i", mWindowedSize.x, mWindowedSize.y);
		SetFullscreen(!mWindowed);
    }

    Vec2I Application::GetWindowSize() const
    {
        return mWindowedSize;
    }

    void Application::SetWindowPosition(const Vec2I& position)
    {
        mWindowedPos = position;
        mLog->Out("Set window position: %i, %i", mWindowedPos.x, mWindowedPos.y);
        SetFullscreen(!mWindowed);
    }

    Vec2I Application::GetWindowPosition() const
    {
        return mWindowedPos;
    }

    void Application::SetWindowCaption(const String& caption)
    {
        mWndCaption = caption;
        SetWindowText(mHWnd, mWndCaption.Data());
    }

    String Application::GetWindowCaption() const
    {
        return mWndCaption;
    }

    void Application::SetApplicationIcon(const String& pngPath)
    {
        if (!mHWnd)
            return;

        Bitmap icon;
        if (!icon.Load(pngPath) || icon.GetFormat() != PixelFormat::R8G8B8A8)
        {
            mLog->WarningStr("Can't load the application icon " + pngPath);
            return;
        }

        // A top-down 32-bit BGRA DIB; the loaded rows go from the bottom
        Vec2I size = icon.GetSize();
        BITMAPV5HEADER header = {};
        header.bV5Size = sizeof(BITMAPV5HEADER);
        header.bV5Width = size.x;
        header.bV5Height = -size.y;
        header.bV5Planes = 1;
        header.bV5BitCount = 32;
        header.bV5Compression = BI_BITFIELDS;
        header.bV5RedMask = 0x00FF0000;
        header.bV5GreenMask = 0x0000FF00;
        header.bV5BlueMask = 0x000000FF;
        header.bV5AlphaMask = 0xFF000000;

        HDC dc = GetDC(NULL);
        void* bits = nullptr;
        HBITMAP color = CreateDIBSection(dc, (BITMAPINFO*)&header, DIB_RGB_COLORS, &bits, NULL, 0);
        ReleaseDC(NULL, dc);

        if (!color || !bits)
            return;

        const UInt8* pixels = icon.GetData();
        UInt8* target = (UInt8*)bits;
        for (int y = 0; y < size.y; y++)
        {
            const UInt8* row = pixels + (size_t)(size.y - 1 - y)*size.x*4;
            for (int x = 0; x < size.x; x++)
            {
                UInt8* out = target + ((size_t)y*size.x + x)*4;
                out[0] = row[x*4 + 2];
                out[1] = row[x*4 + 1];
                out[2] = row[x*4 + 0];
                out[3] = row[x*4 + 3];
            }
        }

        HBITMAP mask = CreateBitmap(size.x, size.y, 1, 1, NULL);

        ICONINFO info = {};
        info.fIcon = TRUE;
        info.hbmMask = mask;
        info.hbmColor = color;
        HICON hIcon = CreateIconIndirect(&info);

        DeleteObject(color);
        DeleteObject(mask);

        if (!hIcon)
            return;

        SendMessage(mHWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(mHWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    void Application::SetContentSize(const Vec2I& size)
    {
        Vec2I clientRectSize = size;

        RECT clientRect;
        GetClientRect(mHWnd, &clientRect);
        clientRect.right = clientRect.left + size.x;
        clientRect.bottom = clientRect.top + size.y;

        AdjustWindowRect(&clientRect, mWndStyle, false);

        mWindowedPos = Vec2I(clientRect.left, clientRect.top);
        mWindowedSize = Vec2I(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

        mLog->Out("Set Content Size: %ix%i", size.x, size.y);

        SetFullscreen(!mWindowed);

        mRender->OnFrameResized();
        onResizing();
    }

    Vec2I Application::GetContentSize() const
    {
        RECT clientRect;
        GetClientRect(mHWnd, &clientRect);
        return Vec2I(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    }

    Vec2I Application::GetScreenResolution() const
    {
        int hor = GetSystemMetrics(SM_CXSCREEN), ver = GetSystemMetrics(SM_CYSCREEN);
        return Vec2I(hor, ver);
    }

    void Application::SetCursor(CursorType type)
    {
        LPSTR cursorsIds[] = { IDC_APPSTARTING, IDC_ARROW, IDC_CROSS, IDC_HAND, IDC_HELP, IDC_IBEAM, IDC_ICON, IDC_NO,
            IDC_SIZEALL, IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE, IDC_SIZEWE, IDC_UPARROW, IDC_WAIT };

        mCurrentCursor = LoadCursor(NULL, cursorsIds[(int)type]);
        ::SetCursor(mCurrentCursor);
        SetClassLongPtr(mHWnd, (int)GCLP_HCURSOR, (LONG_PTR)mCurrentCursor);
    }

    void Application::SetCursorPosition(const Vec2F& position)
    {
        SetCursorPos((int)position.x, (int)position.y);
    }

    String Application::GetBinPath() const
    {
        TCHAR szFileName[MAX_PATH];
        GetModuleFileName(NULL, szFileName, MAX_PATH);
        return o2FileSystem.CanonicalizePath(o2FileSystem.GetParentPath((String)szFileName));
    }


    LRESULT WndProcFunc::WndProc(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto& app = o2Application;

        POINT pt;
        RECT rt;
        int key = 0;
        Vec2I size, pos;
        GetCursorPos(&pt);
        ScreenToClient(wnd, &pt);
        Vec2F cursorPos = Vec2F((float)pt.x, (float)-pt.y);

        if (app.mRender)
            cursorPos -= Vec2F(Math::Round(app.mRender->mResolution.x*0.5f),
                               Math::Round(-app.mRender->mResolution.y*0.5f));

        float wheelDelta;

        if (app.IsReady())
        {
            switch (uMsg)
            {
            case WM_LBUTTONDOWN:
            case WM_TOUCH:
            SetCapture(app.mHWnd);
            app.mInput->OnCursorPressed(cursorPos);
            break;

            case WM_LBUTTONUP:
            app.mInput->OnCursorReleased();
            ReleaseCapture();
            break;

            case WM_RBUTTONDOWN:
            SetCapture(app.mHWnd);
            app.mInput->OnAltCursorPressed(cursorPos);
            break;

            case WM_RBUTTONUP:
            app.mInput->OnAltCursorReleased();
            ReleaseCapture();
            break;

            case WM_MBUTTONDOWN:
            SetCapture(app.mHWnd);
            app.mInput->OnAlt2CursorPressed(cursorPos);
            break;

            case WM_MBUTTONUP:
            app.mInput->OnAlt2CursorReleased();
            ReleaseCapture();
            break;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            key = (int)wParam;
            app.mInput->OnKeyPressed(key);
            break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
            app.mInput->OnKeyReleased((int)wParam);
            break;

            case WM_MOUSEMOVE:
            app.mInput->OnCursorMoved(cursorPos, 0);
            app.mInput->GetCursor()->delta -= app.mCursorCorrectionDelta;
            app.mCursorCorrectionDelta = Vec2F();
            break;

            case WM_MOUSEWHEEL:
            wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            app.mInput->OnMouseWheel(wheelDelta);
            break;

            case WM_ACTIVATEAPP:
            case WM_ENABLE:
            if (wParam == TRUE)
            {
                app.mActive = true;
                app.OnActivated();
                app.onActivated.Invoke();
                o2Events.OnApplicationActivated();
            }
            else
            {
                app.mActive = false;
                app.OnDeactivated();
                app.onDeactivated.Invoke();
                o2Events.OnApplicationDeactivated();
            }
            break;

            case WM_SIZE:
            GetWindowRect(app.mHWnd, &rt);
            size.x = rt.right - rt.left; size.y = rt.bottom - rt.top;

            if (size.x > 0 && size.y > 0 && size != app.mWindowedSize)
                app.OnResized(size);

            app.ProcessFrame();

            break;

            case WM_MOVE:
            GetWindowRect(app.mHWnd, &rt);
            pos.x = rt.left; pos.y = rt.top;

            if (pos.x < 10000 && pos.y < 10000 && pos != app.mWindowedPos)
            {
                app.mWindowedPos = pos;
                app.OnMoved();
                app.onMoving.Invoke();
            }
            break;

            case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            break;
            }
        }

        return DefWindowProc(wnd, uMsg, wParam, lParam);
    }
}

#endif // PLATFORM_WINDOWS
