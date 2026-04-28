#if !defined(PLATFORM_MAC) && (defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX))

#include "EditorTestMacLoopHelper.h"

namespace Editor::Tests
{
    void StartTestTickTimer(TickCallback) {}
}

#endif
