#include "o2/stdafx.h"
#include "o2/O2.h"
#include "o2/Application/Application.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/ToolsPanel.h"
#include "o2Editor/Windows/WindowsManager.h"
#include <gtest/gtest.h>

using namespace o2;

DECLARE_SINGLETON(Editor::WindowsManager);
DECLARE_SINGLETON(Editor::EditorConfig);
DECLARE_SINGLETON(Editor::ToolsPanel);

extern void InitializeTypeso2Editor();


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

    INITIALIZE_O2;
    InitializeTypeso2Editor();

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
