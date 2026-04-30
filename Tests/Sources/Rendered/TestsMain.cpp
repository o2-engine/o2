#include "o2/stdafx.h"
#include "o2/O2.h"
#include "o2/Application/Application.h"
#include <gtest/gtest.h>

extern void InitializeTypeso2TestsSupport();
extern void InitializeTypeso2RenderTests();

using namespace o2;

// Rendered-tier test runner. Full Application::Initialize — window, render device,
// FreeType, UI styles. Use for Render/Sprite/Camera/Material tests and UI widget
// tests that require the styles asset to be loaded.
int main(int argc, char** argv)
{
    InitializeTypeso2TestsSupport();
    InitializeTypeso2RenderTests();
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
