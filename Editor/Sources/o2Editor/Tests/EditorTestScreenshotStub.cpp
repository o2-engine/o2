#if !defined(PLATFORM_MAC) && (defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX))

#include "EditorTestScreenshot.h"
#include "o2/Utils/Debug/Debug.h"

namespace Editor::Tests
{
    bool SaveScreenshot(const o2::String& filePath)
    {
        // TODO: implement Windows/Linux screenshot (e.g. via Render render-target readback).
        o2Debug.LogWarningStr(o2::WString("[Test] SaveScreenshot is not implemented on this platform: ") +
                                  (o2::WString)filePath);
        return false;
    }
}

#endif
