#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"

// The workspace's look in both themes: every tab, the coverage popover, the screen menu, a block selected with its handles,
// the kit view and line art. Shots go to $O2_PIPELINE_SHOTS/uiworkspace (else Work/Pipelines/shots/uiworkspace)

namespace
{
    struct UiWorkspaceGalleryFixture: UiWorkspaceFixture
    {
        String WorkDir() const override { return "../../Work/Pipelines/uitest-uiworkspace-gallery/"; }

        void Gallery(const String& theme)
        {
            OpenWorkspace(true);
            auto window = Window();
            ASSERT_TRUE(window && window->IsShown());
            Ctx()->Select("play");
            Step(3);
            window->GetStage()->FlushRender();
            Step(2);
            Shot(theme + "_agent");

            window->GetSide()->OpenTab("inspect");
            Step(3);
            Shot(theme + "_inspector");

            Ctx()->Select("coin");
            Step(3);
            Shot(theme + "_inspector_coin");

            Ctx()->Select("");
            Step(3);
            Shot(theme + "_inspector_screen");

            window->GetSide()->OpenTab("text");
            Step(3);
            Shot(theme + "_text");

            window->GetSide()->OpenTab("history");
            Step(3);
            Shot(theme + "_history");

            window->ToggleCoverage();
            Step(3);
            Shot(theme + "_coverage");
            window->ToggleCoverage();
            Step(3);

            window->GetStage()->GetDevicePicker()->ToggleMenu();
            Step(3);
            Shot(theme + "_devices");
            window->GetStage()->GetDevicePicker()->ToggleMenu();
            Step(3);

            Ctx()->SetUi("uiView", UiDocJson("lines"));
            Ctx()->Select("card");
            window->GetSide()->OpenTab("agent");
            Step(3);
            window->GetStage()->FlushRender();
            Step(2);
            Shot(theme + "_lines");

            // The agent's model menu (the editor's) opens over the workspace
            window->GetSide()->GetAgent()->ClickModelField();
            Step(3);
            Shot(theme + "_model");
            if (auto picker = editor->GetModelPicker())
                picker->Close();
            Step(2);
        }
    };
}

TEST_F(UiWorkspaceGalleryFixture, DarkTheme)
{
    PipelineTheme::SetKind(PipelineThemeKind::Dark);
    Gallery("dark");
}

// A theme switched while the window is open: the panels are made again in the new colours
TEST_F(UiWorkspaceGalleryFixture, ThemeSwitchedWhileOpen)
{
    PipelineTheme::SetKind(PipelineThemeKind::Dark);
    OpenWorkspace(true);
    Ctx()->Select("title");
    Window()->GetSide()->OpenTab("inspect");
    Step(3);
    PipelineTheme::SetKind(PipelineThemeKind::Light);
    Step(4);
    Window()->GetStage()->FlushRender();
    Step(2);
    EXPECT_TRUE(Window()->GetSide()->GetInspector()->GetField("num:h"));
    Shot("switched_light");
}

TEST_F(UiWorkspaceGalleryFixture, LightTheme)
{
    PipelineTheme::SetKind(PipelineThemeKind::Light);
    Gallery("light");
}
