#pragma once

#include "o2/Utils/Types/String.h"

namespace o2::RenderedTests
{
    // Path to the sandbox assets directory used by the Rendered (non-headless) test
    // runner. Lives next to the test executable, is wiped and re-created at startup,
    // and is wired into o2 via SetAssetsPathOverride() so o2Assets.GetAssetsPath()
    // returns this directory. Tests can read/write into it freely.
    const o2::String& GetTestAssetsRoot();
}
