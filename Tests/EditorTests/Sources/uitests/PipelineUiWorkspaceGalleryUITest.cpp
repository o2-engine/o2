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

// The states between the plain views: the chat with hand edits, an answer with its check pass and a failed edit unfolded,
// the agent working, its error; the history's actions and a peek; the add menu; the kit graphic picker; a block dragged on
// the stage (guides, badge) and a row dragged in the hierarchy (the landing place, the label following the pointer)
TEST_F(UiWorkspaceGalleryFixture, States)
{
    PipelineTheme::SetKind(PipelineThemeKind::Dark);
    OpenWorkspace(true);
    auto window = Window();
    auto hierarchy = window->GetHierarchy();
    Ctx()->Select("coin");
    Step(2);
    hierarchy->ClickHot("hide:coin");
    Step(2);
    hierarchy->AddBlock("text");
    Step(2);

    Vector<UiDoc::UiOp> ops;
    UiDocJson set = UiDocJson::Object();
    set["h"] = 160;
    ops.Add(UiDoc::MakeUpdateOp("title", set));
    ops.Add(UiDoc::MakeRemoveOp({ "ghost" }));
    auto answer = UiDoc::MakeAgentStep("make the title bigger and drop the ghost", "Made the title 160 high; there was no ghost.", ops,
                                       UiDoc::Ws::NowMs(), "gpt-6.1-sol");
    answer.SetReview("Matched the play button to btn_green.", 1);
    UiWorkspace::AppendAgentStep(editor, layoutNode->id, answer.data);
    window->GetSide()->GetAgent()->ToggleEdits(2);
    UiWorkspace::SetAgentBusy(layoutNode->id, true);
    Step(3);
    Shot("states_agent_busy");

    UiWorkspace::SetAgentBusy(layoutNode->id, false);
    UiWorkspace::SetAgentError(layoutNode->id, "The agent could not answer: HTTP 500 from the provider");
    Step(3);
    Shot("states_agent_error");
    UiWorkspace::SetAgentError(layoutNode->id, "");

    window->GetSide()->OpenTab("history");
    Step(2);
    window->GetSide()->GetHistory()->ToggleActions(2);
    Ctx()->SetPeek(1);
    Step(3);
    window->GetStage()->FlushRender();
    Step(2);
    Shot("states_history_peek");
    Ctx()->SetPeek(-1);
    Step(2);

    Ctx()->Select("card");
    Step(2);
    hierarchy->OpenAddMenu();
    Step(3);
    Shot("states_add_menu");
    hierarchy->OpenAddMenu();
    Step(2);

    Ctx()->Select("play");
    window->GetSide()->OpenTab("inspect");
    Step(3);
    window->GetSide()->GetInspector()->ClickHot("kitpick:kit");
    Step(3);
    Shot("states_kit_grid");
    o2Input.OnKeyPressed(VK_ESCAPE);
    Step();
    o2Input.OnKeyReleased(VK_ESCAPE);
    Step(2);

    // A block held mid-drag on the stage
    auto stage = window->GetStage();
    Vec2F from = stage->BlockRect("play").Center();
    float k = stage->GetZoom();
    o2Input.OnCursorMoved(from, 0, false);
    Step();
    o2Input.OnCursorPressed(from);
    Step();
    for (int i = 1; i <= 6; i++)
    {
        o2Input.OnCursorMoved(from + Vec2F(14.0f*i*k, -6.0f*i*k), 0);
        Step();
    }
    Step(2);
    Shot("states_stage_drag");
    o2Input.OnCursorReleased();
    Step(2);

    // A row held mid-drag in the hierarchy, over the card
    Vec2F row = hierarchy->RowRect("text1").Center();
    Vec2F onto = hierarchy->RowRect("list").Center();
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    o2Input.OnCursorMoved(row, 0, false);
    Step();
    o2Input.OnCursorPressed(row);
    Step();
    for (int i = 1; i <= 8; i++)
    {
        o2Input.OnCursorMoved(Math::Lerp(row, onto, i/8.0f), 0);
        Step();
    }
    Step(2);
    Shot("states_hierarchy_drag");
    o2Input.OnCursorReleased();
    Step(2);
}
