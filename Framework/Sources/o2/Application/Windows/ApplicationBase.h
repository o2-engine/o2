#pragma once

#ifdef PLATFORM_WINDOWS

#include <Windows.h>
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Application;

    // -------------------------------
    // Windows application base fields
    // -------------------------------
    class ApplicationBase
    {
    protected:
        HWND   mHWnd;            // Window handle
        UInt   mWndStyle;        // Window styles data
        bool   mWindowed;        // True if app in windowed mode, false if in fullscreen mode
        bool   mWindowResizible; // True, if window can be sized by user
        Vec2I  mWindowedPos;     // Position of window
        String mWndCaption;      // Window caption
        bool   mActive;          // True, if window is active

        HCURSOR mCurrentCursor;  // Current cursor type

        bool mNeedPlatformInitialization = true; // True, if need to initialize platform: window and render

    public:
        // Returns window handle
        HWND GetWindowHandle() const { return mHWnd; }

        friend class Render;
        friend class FileSystem;
    };
}

#endif // PLATFORM_WINDOWS
