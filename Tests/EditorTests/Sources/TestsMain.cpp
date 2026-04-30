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

int main(int argc, char** argv)
{
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

    return RUN_ALL_TESTS();
}
