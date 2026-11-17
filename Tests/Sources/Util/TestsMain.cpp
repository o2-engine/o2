#include <gtest/gtest.h>

extern void InitializeTypeso2UtilTests();

// Utility-tier test runner. No Application, no o2 subsystems, no asset tree load.
// Tests linked here must depend only on header-only / value-type code from o2Framework
// (Math, Vec2F, Color, Pool, Vector, Map, Curve...). Anything that touches Time, Scene,
// Assets, Input, Render, etc. belongs in o2SystemTests or o2RenderTests.

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// A Debug CRT check (an index out of range, ...) prints to stderr: its message box ended the run silently on a machine
// with no one to answer it
static void ReportCrtChecksToStderr()
{
#if defined(_MSC_VER) && defined(_DEBUG)
    for (int type : { _CRT_ERROR, _CRT_ASSERT })
    {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
#endif
}

int main(int argc, char** argv)
{
    ReportCrtChecksToStderr();
    InitializeTypeso2UtilTests();

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
