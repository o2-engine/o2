#include "o2/stdafx.h"
#include "o2/O2.h"
#include "o2/Application/Application.h"
#include <gtest/gtest.h>

extern void InitializeTypeso2TestsSupport();
extern void InitializeTypeso2SystemTests();

using namespace o2;

// Systems-tier test runner. Initializes o2 subsystems (Time, Assets, Input, Scene,
// Events, Scripting, ...) but skips window + render + UI styles. Use for tests that
// need o2Scene / Actor / Asset metadata / scripting but never draw.

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
    Application::SetHeadless(true);

    InitializeTypeso2TestsSupport();
    InitializeTypeso2SystemTests();
    INITIALIZE_O2;

    ::testing::InitGoogleTest(&argc, argv);

    bool listOnly = ::testing::GTEST_FLAG(list_tests);

    Ref<Application> app;
    if (!listOnly)
    {
        app = mmake<Application>();
        app->Initialize();
    }

    int result = RUN_ALL_TESTS();

    if (app)
        app->Deinitialize();

    return result;
}
