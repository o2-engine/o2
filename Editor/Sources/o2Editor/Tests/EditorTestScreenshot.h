#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "o2/Utils/Types/String.h"

namespace Editor::Tests
{
    // Saves the editor window contents to a PNG file.
    // Returns true on success. On platforms without an implementation, logs and returns false.
    bool SaveScreenshot(const o2::String& filePath);
}

#endif
