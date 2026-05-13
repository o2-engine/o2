#pragma once

#include "o2/Utils/Types/String.h"

namespace Editor::Tests
{
    // Per-PID sandbox assets directory used by o2EditorTests. Wiped and re-created
    // at startup, wired via SetAssetsPathOverride() so o2Assets.GetAssetsPath()
    // returns this directory. Tests touching o2Assets / asset actions must operate
    // against this root rather than the project's real Assets/.
    const o2::String& GetTestAssetsRoot();
}
