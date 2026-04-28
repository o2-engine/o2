#if defined(PLATFORM_MAC)

#include "EditorTestScreenshot.h"

#import <Cocoa/Cocoa.h>

#include "o2/Application/Mac/ApplicationPlatformWrapper.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include <cstdio>
#include <cstdlib>

namespace Editor::Tests
{
    bool SaveScreenshot(const o2::String& filePath)
    {
        @autoreleasepool {
            NSWindow* win = o2::ApplicationPlatformWrapper::window;
            if (!win)
            {
                o2Debug.LogErrorStr(o2::WString("[Test] SaveScreenshot: window is nil"));
                return false;
            }

            o2::String dir = o2FileSystem.ExtractPathStr(filePath);
            if (!dir.IsEmpty())
                o2FileSystem.FolderCreate(dir, true);

            // CGWindowListCreateImage was deprecated and is now unavailable on macOS 15+.
            // Use the system 'screencapture' tool with a window ID — this works on every
            // macOS version and avoids ScreenCaptureKit's async API.
            CGWindowID windowID = (CGWindowID)[win windowNumber];

            // Build a safe shell command. Both windowID (an int) and filePath are quoted/escaped.
            char cmd[2048];
            std::snprintf(cmd, sizeof(cmd),
                          "/usr/sbin/screencapture -x -l%u -t png \"%s\"",
                          (unsigned)windowID, filePath.Data());

            int rc = std::system(cmd);
            if (rc != 0)
            {
                o2Debug.LogErrorStr(o2::WString("[Test] screencapture failed, rc=") +
                                    (o2::WString)(o2::String)rc);
                return false;
            }
            return true;
        }
    }
}

#endif
