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
int main(int argc, char** argv)
{
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
