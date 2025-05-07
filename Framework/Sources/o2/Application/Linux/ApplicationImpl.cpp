#include "o2/stdafx.h"

#ifdef PLATFORM_LINUX

#include "o2/Application/Application.h"
#include "o2/Events/EventSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#if !defined(O2_RENDER_GLES2)
#include <GL/glx.h>
#endif

namespace o2
{

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

#if !defined(O2_DISABLE_PLATFORM)
        if (mNeedPlatformInitialization)
        {
            mLog->Out("Initializing window..");

            mDisplay = XOpenDisplay(NULL);
            if (!mDisplay)
            {
                mLog->Error("Can't open display");
                return;
            }

            int screen = DefaultScreen(mDisplay);
            Window rootWindow = RootWindow(mDisplay, screen);

            GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, 0};
            mVisualInfo = glXChooseVisual(mDisplay, 0, att);

            Colormap cmap = XCreateColormap(mDisplay, rootWindow, mVisualInfo->visual, AllocNone);

            XSetWindowAttributes winAttrs;
            winAttrs.colormap = cmap;
            winAttrs.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
                                  ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask |
                                  Button1MotionMask | VisibilityChangeMask | FocusChangeMask;

            mWindow = XCreateWindow(mDisplay, rootWindow, mWindowedPos.x, mWindowedPos.y,
                mWindowedSize.x, mWindowedSize.y, 0, mVisualInfo->depth, InputOutput, mVisualInfo->visual, CWColormap | CWEventMask, &winAttrs);

            XStoreName(mDisplay, mWindow, "o2 application");
            XMapWindow(mDisplay, mWindow);

            mLog->Out("Window initialized!");
        }
#endif
    }

    void Application::Shutdown()
    {
        //DestroyWindow(mHWnd);
    }

    void Application::SetFullscreen(bool fullscreen /*= true*/)
    {
//        if (fullscreen)
//        {
//            //mRenderSystem->FrameResized();
//            mLog->Out("Setting full screen");
//        }
//        else
//        {
//            mLog->Out("Setting windowed..");
//
//            mWindowed = true;
//
//            RECT rt = { mWindowedPos.x, mWindowedPos.y, mWindowedPos.x + mWindowedSize.x, mWindowedPos.y + mWindowedSize.y };
//            AdjustWindowRect(&rt, mWndStyle, false);
//            SetWindowPos(mHWnd, HWND_NOTOPMOST, mWindowedPos.x, mWindowedPos.y,
//                         mWindowedSize.x, mWindowedSize.y, SWP_SHOWWINDOW);
//
//            //mRenderSystem->FrameResized();
//            mLog->Out("Complete");
//        }
    }

    void Application::CheckCursorInfiniteMode()
    {
//        int threshold = 10;
//        POINT p, lp;
//        GetCursorPos(&p);
//        lp = p;
//
//        Vec2I resolution = GetScreenResolution();
//
//        if (p.x > resolution.x - threshold)
//            p.x = threshold;
//        else if (p.x < threshold)
//            p.x = resolution.x - threshold;
//
//        if (p.y > resolution.y - threshold)
//            p.y = threshold;
//        else if (p.y < threshold)
//            p.y = resolution.y - threshold;
//
//        SetCursorPos(p.x, p.y);
//
//        if (p.x != lp.x || p.y != lp.y)
//            mCursorCorrectionDelta = Vec2F((float)(lp.x - p.x), (float)(lp.y - p.y));
    }

    void Application::Launch()
    {
        mLog->Out("Application launched!");

        OnStarted();
        onStarted.Invoke();
        o2Events.OnApplicationStarted();

        //mTimer->Reset();
        Vec2I cursorPos;

#if !defined(O2_DISABLE_PLATFORM)
        Atom wmDeleteMessage = XInternAtom(mDisplay, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(mDisplay, mWindow, &wmDeleteMessage, 1);

        while (1)
        {
            bool breakCycle = false;
            while (XPending(mDisplay) > 0)
            {
                XEvent xev;
                XNextEvent( mDisplay, &xev );

                //mLog->Out("event type " + (String)xev.type);

                if(xev.type == ClientMessage && xev.xclient.data.l[0] == wmDeleteMessage)
                {
                    // Exit the main loop if the close button is clicked
                    breakCycle = true;
                }

                if(xev.type == KeyPress)
                {
                    int key = XLookupKeysym( &xev.xkey, 0 );
                    mInput->OnKeyPressed( key );
                }

                if (xev.type == KeyRelease)
                {
                    int key = XLookupKeysym( &xev.xkey, 0 );
                    mInput->OnKeyReleased( key );
                }

                if (xev.type == ButtonPress)
                {
                    int button = xev.xbutton.button;
                    if(button == Button1)
                        mInput->OnCursorPressed( cursorPos );
                    else if(button == Button2)
                        mInput->OnAlt2CursorPressed( cursorPos );
                    else if(button == Button3)
                        mInput->OnAltCursorPressed( cursorPos );
                    if (xev.xbutton.button == Button4)
                        mInput->OnMouseWheel(120);
                    else if (xev.xbutton.button == Button5)
                        mInput->OnMouseWheel(-120);
                }

                if (xev.type == ButtonRelease)
                {
                    int button = xev.xbutton.button;
                    if(button == Button1)
                        mInput->OnCursorReleased();
                    else if(button == Button2)
                        mInput->OnAlt2CursorReleased();
                    else if(button == Button3)
                        mInput->OnAltCursorReleased();
                }

                if (xev.type == MotionNotify)
                {
                    Vec2I pos( xev.xmotion.x, xev.xmotion.y );

                    cursorPos = o2::Vec2I(pos.x - mWindowedSize.x / 2, mWindowedSize.y / 2 - pos.y);
                    mInput->OnCursorMoved( cursorPos );
                }

                if(xev.type == ResizeRequest)
                {
                    OnResized( Vec2I( xev.xresizerequest.width, xev.xresizerequest.height ) );
                }
            }

            XWindowAttributes gwa;
            XGetWindowAttributes(mDisplay, mWindow, &gwa);
            Vec2I windowedSize(gwa.width, gwa.height);
            if (windowedSize != mWindowedSize)
                OnResized(windowedSize);

            ProcessFrame();

            if (breakCycle)
                break;
        }
#endif

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
        //ShowWindow(mHWnd, SW_MAXIMIZE);
    }

    bool Application::IsMaximized() const
    {
        return false;
    }

    void Application::SetResizible(bool resizible)
    {
        if (resizible == mWindowResizible)
            return;

        mWindowResizible = resizible;
        mLog->Out("Set resizible: " + ((mWindowResizible ? (String)"true" : (String)"false")));
    }

    bool Application::IsResizible() const
    {
        return mWindowResizible;
    }

    void Application::SetWindowSize(const Vec2I& size)
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
#if !defined(O2_DISABLE_PLATFORM)
        if (mDisplay && mWindow)
            XStoreName(mDisplay, mWindow, caption.Data());
#endif
    }

    String Application::GetWindowCaption() const
    {
        return mWndCaption;
    }

    void Application::SetContentSize(const Vec2I& size)
    {
        Vec2I clientRectSize = size;

        mWindowedSize = size;

//        mWindowedPos = Vec2I(clientRect.left, clientRect.top);
//        mWindowedSize = Vec2I(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

        mLog->Out("Set Content Size: %ix%i", size.x, size.y);

        SetFullscreen(!mWindowed);

        mRender->OnFrameResized();
        onResizing();
    }

    Vec2I Application::GetContentSize() const
    {
        return mWindowedSize;
    }

    Vec2I Application::GetScreenResolution() const
    {

#if !defined(O2_DISABLE_PLATFORM)
        int screen = DefaultScreen(mDisplay);
        return Vec2I(DisplayWidth(mDisplay, screen), DisplayHeight(mDisplay, screen));
#else
        return Vec2I(800, 600);
#endif
    }

    void Application::SetCursor(CursorType type)
    {
    }

    void Application::SetCursorPosition(const Vec2F& position)
    {
    }

    String Application::GetBinPath() const
    {
        return "";
    }
}

#endif // PLATFORM_LINUX
