#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsContext.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsSide.h"

// The layout agent's part of the workspace: the request field takes the keys (the workspace's shortcuts wait while it has
// the focus), points placed with the point tool become numbered tokens of the request at its caret and go with their
// tokens, and the agent's panel shows a run's acts live and under its step (rows unfold into words and pictures)

namespace
{
    struct UiLayoutAgentUiFixture: UiWorkspaceFixture
    {
        Ref<ActionsList> undo;

        void SetUp() override
        {
            UiWorkspaceFixture::SetUp();
            undo = mmake<ActionsList>();
            editor->actionsListDelegate = undo;
        }

        void TearDown() override
        {
            UiAgentState::ClearRun(layoutNode ? layoutNode->id : String());
            editor->actionsListDelegate = nullptr;
            undo = nullptr;
            UiWorkspaceFixture::TearDown();
        }

        // A key pressed and released as the keyboard sends it (through the window's listeners, not PressKey)
        void Key(int key)
        {
            o2Input.OnKeyPressed(key);
            Step();
            o2Input.OnKeyReleased(key);
            Step();
        }

        Ref<UiWsAgentTab> Agent() const { return Window()->GetSide()->GetAgent(); }
    };
}

// The workspace's shortcuts (Delete / Backspace, F, Cmd/Ctrl+Z) do not fire while the request field has the focus:
// they edit the request; with the focus back on the window they act on the layout again
TEST_F(UiLayoutAgentUiFixture, ShortcutsWaitWhileTheRequestFieldHasTheFocus)
{
    OpenWorkspace(false);
    Window()->GetSide()->OpenTab("agent");
    Ctx()->Select("coin");
    Step(2);
    auto field = Agent()->GetPromptField();
    field->SetText("make it gold");
    field->Focus();
    Step(2);
    ASSERT_TRUE(field->IsFocused()) << "the request field takes the focus";

    float zoom = Window()->GetStage()->GetZoom();
    double x = *UiDoc::FindNode(DocNow(), "coin")->x;
    int steps = StepsNow().Count();
    // (The arrows go through the same check; sent as real keys here they would also reach the scene's move tool, which needs
    // the editor's actions list the fixture does not have)
    for (int key : { (int)VK_DELETE, (int)VK_BACK, (int)VK_F })
        Key(key);
    o2Input.OnKeyPressed(VK_CTRL_CMD);
    Key(VK_Z);
    o2Input.OnKeyReleased(VK_CTRL_CMD);
    Step();

    EXPECT_TRUE(UiDoc::FindNode(DocNow(), "coin")) << "Delete / Backspace went to the field";
    EXPECT_EQ(*UiDoc::FindNode(DocNow(), "coin")->x, x);
    EXPECT_EQ(Window()->GetStage()->GetZoom(), zoom) << "F went to the field";
    EXPECT_EQ(StepsNow().Count(), steps) << "Cmd/Ctrl+Z went to the field";
    EXPECT_EQ(Ctx()->selected, String("coin"));

    // The focus back on the window: Delete removes the selected block
    o2UI.FocusWidget(Window());
    Step(2);
    Key(VK_DELETE);
    Step(2);
    EXPECT_FALSE(UiDoc::FindNode(DocNow(), "coin"));
}

// The point tool: a click on the screen puts the point's token into the request at the caret; a second point before it takes
// the first number; deleting a token from the text removes its point; Escape ends the tool
TEST_F(UiLayoutAgentUiFixture, PointsAreTokensOfTheRequest)
{
    OpenWorkspace(false);
    Window()->GetSide()->OpenTab("agent");
    Step(2);
    auto stage = Window()->GetStage();
    auto field = Agent()->GetPromptField();
    field->SetText("make  bigger");
    field->Focus();
    field->SetCaretPosition(5);
    Step();

    Ctx()->pinning = true;
    Click(stage->DesignToWorld(540, 960));
    ASSERT_EQ(Ctx()->points.Count(), 1);
    EXPECT_EQ(Ctx()->points[0].x, 540);
    // The space already after the caret is gone past, not doubled (PinInput.tsx insertPin)
    EXPECT_EQ(Ctx()->draft, String("make [1] bigger"));
    EXPECT_EQ(String(field->GetText()), String("make [1] bigger"));
    EXPECT_TRUE(field->IsFocused()) << "the field keeps the focus to write about the point";
    auto config = ConfigNow();
    ASSERT_TRUE(config.Get("uiPoints"));
    EXPECT_EQ(config.Get("uiPoints")->Count(), 1);

    // A point put in before it: the tokens are numbered in the order they stand
    field->SetCaretPosition(0);
    Step();
    Click(stage->DesignToWorld(100, 100));
    ASSERT_EQ(Ctx()->points.Count(), 2);
    EXPECT_EQ(Ctx()->draft, String("[1] make [2] bigger"));
    EXPECT_EQ(Ctx()->points[0].x, 100) << "the new point is the first";

    // Its token deleted from the text: the point goes, the other is [1] again
    ASSERT_TRUE(field->IsFocused());
    field->SetText("make [2] bigger");
    Step(2);
    ASSERT_EQ(Ctx()->points.Count(), 1);
    EXPECT_EQ(Ctx()->points[0].x, 540);
    EXPECT_EQ(String(field->GetText()), String("make [1] bigger"));

    // A pin dragged to another place
    Drag(stage->DesignToWorld(540, 960), stage->DesignToWorld(700, 1200));
    Step(2);
    EXPECT_NEAR(Ctx()->points[0].x, 700, 3);

    o2UI.FocusWidget(Window());
    Step();
    Key(VK_ESCAPE);
    EXPECT_FALSE(Ctx()->pinning) << "Escape ends the point tool first";
    EXPECT_TRUE(Window()->IsShown());
    Shot("agent-points");
}

// While the agent works its acts show in the panel as they come, with Stop; a finished step folds its run under a line that
// unfolds into the acts, and an act into its words
TEST_F(UiLayoutAgentUiFixture, TheAgentsActsShowLiveAndUnderItsStep)
{
    OpenWorkspace(false);
    Window()->GetSide()->OpenTab("agent");
    UiAgentState::LiveRun live;
    live.runId = "srunlive";
    live.prompt = "Build this screen from the reference picture.";
    live.startedMs = (double)UiDoc::NowMs() - 65000;
    UiDocJson::Parse(R"([
        {"id":"analysis","kind":"analysis","title":"Reading the reference","status":"ok","ms":4200,"note":"9 of 12 from the kit","data":{"elements":12,"kit":9,"looks":2,"texts":3}},
        {"id":"skill-turn-3","kind":"turn","title":"Turn 3: measure 60 270 650 770","status":"run","data":{"skill":1,"step":"turn-3","action":"measure","label":"Turn 3: measure 60 270 650 770"}}
    ])", live.acts);
    UiAgentState::SetRun(layoutNode->id, live);
    UiWorkspace::SetAgentBusy(layoutNode->id, true);
    Step(3);
    EXPECT_FALSE(Agent()->HotRect("stop").IsZero()) << "Stop while it runs";
    EXPECT_FALSE(Agent()->HotRect("act:srunlive:analysis").IsZero());
    Shot("agent-live");

    Agent()->ClickHot("stop");
    EXPECT_TRUE(UiAgentState::IsStopRequested(layoutNode->id));
    UiAgentState::ClearRun(layoutNode->id);
    UiWorkspace::SetAgentBusy(layoutNode->id, false);

    // The step it came to, with its run
    auto step = UiDoc::MakeAgentStep("Build this screen from the reference picture.", "Rebuilt from the reference with the UI kit.", {},
                                     UiDoc::Ws::NowMs(), "gpt-6.1-sol");
    UiDocJson run;
    UiDocJson::Parse(R"({"id":"srunlive","ms":420000,"turns":44,"acts":[
        {"id":"analysis","kind":"analysis","title":"Reading the reference","status":"ok","ms":4200,"note":"12 elements, most from the kit","data":{"elements":12,"kit":9,"looks":2}},
        {"id":"check","kind":"check","title":"Result against the reference","status":"ok","data":{"score":0.83,"from":0.6}}],
        "built":true,"reused":9,"made":2,"score":0.83,"scoreFrom":0.6})", run);
    step.data["run"] = run;
    UiWorkspace::AppendAgentStep(editor, layoutNode->id, step.data);
    Step(3);
    int last = StepsNow().Count() - 1;
    String fold = "acts:" + (String)last;
    ASSERT_FALSE(Agent()->HotRect(fold).IsZero()) << "the run's line under the step";
    EXPECT_TRUE(Agent()->HotRect("act:srunlive:analysis").IsZero()) << "folded";
    Agent()->ClickHot(fold);
    Step(2);
    ASSERT_FALSE(Agent()->HotRect("act:srunlive:analysis").IsZero());
    Agent()->ClickHot("act:srunlive:analysis");
    Step(2);
    Shot("agent-step-acts");
}
