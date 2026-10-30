#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"

#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/AssetsLineDlg.h"

// The Pipeline settings window inside the whole editor: its menus, windows and their shortcuts listen to the keys too

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

namespace
{
    // A frame as the application runs it: the editor updated and drawn, then the input and the events
    void Step(EditorShell& shell, int frames = 1)
    {
        for (int i = 0; i < frames; i++)
        {
            {
                PushEditorScopeOnStack scope;
                o2Events.PostUpdate();
                shell.Update(1.0f/60.0f);
                o2Render.Begin();
                o2Render.SetCamera(Camera());
                shell.Draw();
                o2UI.DrawCurrentLayerTopWidgets();
                o2Render.End();
            }
            AppTestDriver::PumpFrames(1);
        }
    }

    // A dialog of its own for every test, destroyed after it, and a sync that asks no server. The sync lives as long as
    // the process, as the editor's does; the dialog scope opens after the set up, so it takes no singleton of the fixture
    struct AssetsLineDlgShell : EditorVectorUIFixture
    {
        std::unique_ptr<SingletonsScope<AssetsLineDlg>> dialogScope;

        void SetUp() override
        {
            EditorVectorUIFixture::SetUp();

            static Ref<AssetsLineSync> sync = AssetsLineSync::IsSingletonInitialzed() ? Ref(AssetsLineSync::InstancePtr()) :
                mmake<AssetsLineSync>();
            sync->rebuildAssets = []() {};
            sync->openUrl = [](const String&) {};

            dialogScope = std::make_unique<SingletonsScope<AssetsLineDlg>>();
            Singleton<AssetsLineDlg>::mInstance = nullptr;
        }

        void TearDown() override
        {
            dialogScope = nullptr;
            EditorVectorUIFixture::TearDown();
        }
    };

    void Press(EditorShell& shell, KeyboardKey key, bool down)
    {
        if (down)
            o2Input.OnKeyPressed(key);
        else
            o2Input.OnKeyReleased(key);

        Step(shell);
    }
}

TEST_F(AssetsLineDlgShell, TheServerAddressPastesInTheEditor)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
    Step(shell, 10);

    AssetsLineDlg::Show();
    shell.RaiseDialogs();
    Step(shell, 5);

    auto window = AssetsLineDlg::Instance().GetWindow();
    auto server = DynamicCast<EditBox>(window->FindChild("server"));
    ASSERT_TRUE(server);

    Vec2F point = server->layout->GetWorldRect().Center();
    o2Input.OnCursorMoved(point);
    Step(shell);
    o2Input.OnCursorPressed(point);
    Step(shell);
    o2Input.OnCursorReleased();
    Step(shell, 2);
    ASSERT_TRUE(server->IsFocused()) << "focused: " << (o2UI.GetFocusedWidget() ? o2UI.GetFocusedWidget()->GetName() : String("none"));

#if defined PLATFORM_MAC
    const KeyboardKey command = VK_COMMAND;
#else
    const KeyboardKey command = VK_CONTROL;
#endif

    // Select all, then paste over it
    WString saved = Clipboard::GetText();
    Clipboard::SetText("https://example.test");
    Press(shell, command, true);
    Press(shell, VK_A, true);
    Press(shell, VK_A, false);
    Press(shell, VK_V, true);
    Press(shell, VK_V, false);
    Press(shell, command, false);
    Clipboard::SetText(saved);

    EXPECT_TRUE(server->IsFocused()) << "focused: " << (o2UI.GetFocusedWidget() ? o2UI.GetFocusedWidget()->GetName() : String("none"));
    EXPECT_EQ((String)server->GetText(), "https://example.test");

    window->Hide(true);
}

// In the dark theme every text of the window reads on its panels: titles, values and hints
TEST_F(AssetsLineDlgShell, TextsReadInTheDarkTheme)
{
    EditorTheme::SetKind(EditorThemeKind::Dark);
    PipelineTheme::FollowEditor();
    BuildStyle(false);
    {
        SingletonsScope<AssetsLineDlg> dialogScope;
        Singleton<AssetsLineDlg>::mInstance = nullptr;
        EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
        AssetsLineDlg::Show();

        auto luminance = [](const Color4& c) { return 0.2126f*c.r + 0.7152f*c.g + 0.0722f*c.b; };
        Vector<Ref<Actor>> actors;
        AssetsLineDlg::Instance().GetWindow()->GetAllChildrenActors(actors);
        int labels = 0;
        for (auto& actor : actors)
        {
            if (auto label = DynamicCast<Label>(actor))
            {
                labels++;
                EXPECT_GT(luminance(label->GetColor()), 120.0f) << (String)label->GetText();
            }
        }
        EXPECT_GT(labels, 10);

        AssetsLineDlg::Instance().GetWindow()->Hide(true);
    }

    EditorTheme::SetKind(EditorThemeKind::Light);
    PipelineTheme::FollowEditor();
    BuildStyle(false);
}

// The window in both themes, for review (O2_EDITOR_THEME_OUT=<folder>)
TEST_F(AssetsLineDlgShell, Gallery)
{
    const char* out = getenv("O2_EDITOR_THEME_OUT");
    if (!out)
        GTEST_SKIP() << "O2_EDITOR_THEME_OUT is not set";

    for (auto kind : { EditorThemeKind::Light, EditorThemeKind::Dark })
    {
        EditorTheme::SetKind(kind);
        PipelineTheme::FollowEditor();
        BuildStyle(false);

        SingletonsScope<AssetsLineDlg> dialogScope;
        Singleton<AssetsLineDlg>::mInstance = nullptr;
        EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
        AssetsLineDlg::Show();
        shell.RaiseDialogs();
        Frame frame = shell.Settle();
        frame.bitmap->Save(String(out) + "/assetsline_dlg" + (kind == EditorThemeKind::Dark ? "_dark.png" : "_light.png"),
                           Bitmap::ImageType::Png);
        AssetsLineDlg::Instance().GetWindow()->Hide(true);
    }

    EditorTheme::SetKind(EditorThemeKind::Light);
    PipelineTheme::FollowEditor();
}
