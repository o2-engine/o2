#include "o2/stdafx.h"
#include "o2/O2.h"
#include "o2/Application/Application.h"
#include "o2/Sound/SoundSystem.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/ToolsPanel.h"
#include "o2Editor/UI/Style/EditorUIStyle.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/WindowsManager.h"
#include <gtest/gtest.h>
#include <filesystem>

using namespace o2;
using namespace Editor;

DECLARE_SINGLETON(Editor::WindowsManager);
DECLARE_SINGLETON(Editor::EditorConfig);
DECLARE_SINGLETON(Editor::ToolsPanel);

extern void InitializeTypeso2Editor();

// Non-headless editor test runner: brings up the Render system + editor UI styles so tests can build
// real property fields/viewers. Headless o2EditorTests can't (widgets need Render and the styles).
int main(int argc, char** argv)
{
    // cwd two levels under repo root so the relative "../../BuiltAssets/.." asset paths resolve
    if (argc > 0 && argv[0])
    {
        std::filesystem::path exe(argv[0]);
        if (exe.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::current_path(exe.parent_path() / ".." / "..", ec);
        }
    }

    INITIALIZE_O2;
    InitializeTypeso2Editor();

    ::testing::InitGoogleTest(&argc, argv);
    bool listOnly = ::testing::GTEST_FLAG(list_tests);

    // The suites bring up a real window: keep it out of the focus and the audio out of the speakers,
    // otherwise a test run makes the machine unusable
    Integration::SetBackgroundWindow(true);
    SoundSystem::SetSilent(true);

    Ref<Application> app;
    Ref<Properties> properties;
    Ref<SceneEditScreen> sceneScreen;
    if (!listOnly)
    {
        app = mmake<Application>();
        app->Initialize();

        EditorUIStyleBuilder().RebuildEditorUIManager("Editor UI styles", false, true);
        properties = mmake<Properties>();
        sceneScreen = mmake<SceneEditScreen>();
    }

    int result = RUN_ALL_TESTS();

    sceneScreen = nullptr;
    properties = nullptr;
    if (app)
        app->Deinitialize();

    return result;
}
