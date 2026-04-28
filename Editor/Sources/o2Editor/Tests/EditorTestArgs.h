#pragma once

#include "o2/Utils/Types/String.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

namespace Editor::Tests
{
    struct EditorTestArgs
    {
        bool        runTests          = false;
        o2::String  testFilter;
        o2::String  testsDir;
        o2::String  outputDir;
        o2::String  argv0;             // Captured argv[0] used to resolve default paths.
        bool        verbose           = false;
        bool        screenshotOnFail  = true;
        int         warmupFrames      = 5;

        static bool ParseFromArgv(int argc, char** argv, EditorTestArgs& out);

        o2::String ResolveTestsDir() const;
        o2::String ResolveOutputDir() const;
    };
}

#endif
