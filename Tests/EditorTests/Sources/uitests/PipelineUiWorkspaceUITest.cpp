#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2Editor/Actions/ActionsList.h"

// The AI UI layout workspace driven through its window: the stage's rect tool (move keeps the anchor, resize, snapping, the
// anchor dragged keeps the rect, reorder inside a column), the hierarchy (drag and drop, rename), the inspector, the UIML
// text, undo / redo by action, the history, the keys, the window's geometry and the config written as the web writes it

namespace
{
    struct UiWorkspaceUiFixture: UiWorkspaceFixture
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
            editor->actionsListDelegate = nullptr;
            undo = nullptr;
            UiWorkspaceFixture::TearDown();
        }

        UiDoc::UiRect RectOf(const String& id) const
        {
            auto doc = DocNow();
            return UiDoc::SolveLayout(doc).Get(id);
        }

        const UiDoc::UiNode* NodeOf(const UiDoc::UiDoc& doc, const String& id) const { return UiDoc::FindNode(doc, id); }

        // The world offset of a design offset at the stage's zoom (y down in design, up in the world)
        Vec2F WorldDelta(float dx, float dy) const
        {
            float k = Window()->GetStage()->GetZoom();
            return Vec2F(dx*k, -dy*k);
        }
    };
}

// The window opens over the editor, the stage draws the screen once and not again while nothing changes, the hierarchy
// lists every block
TEST_F(UiWorkspaceUiFixture, OpensDrawsOnceAndListsTheBlocks)
{
    OpenWorkspace(false);
    auto window = Window();
    ASSERT_TRUE(window && window->IsShown());
    EXPECT_TRUE(UiWorkspace::IsOpenFor(layoutNode->id));
    EXPECT_GT(window->GetStage()->GetZoom(), 0.0f);

    int renders = window->GetStage()->GetRenderCount();
    EXPECT_GE(renders, 1);
    printf("ui workspace: the screen drawn at %dx%d in %.1f ms\n", window->GetStage()->GetRenderedSize().x,
           window->GetStage()->GetRenderedSize().y, window->GetStage()->GetLastRenderMs());
    Step(10);
    EXPECT_EQ(window->GetStage()->GetRenderCount(), renders) << "a frame without a change draws nothing";

    for (auto id : { "bg", "card", "title", "play", "list", "a", "b", "c", "coin" })
        EXPECT_GT(window->GetHierarchy()->RowRect(id).Width(), 0.0f) << id;

    UiWorkspace::Close();
    EXPECT_FALSE(UiWorkspace::IsOpen());
}

// A click selects the deepest block; a drag moves it as one hand action that keeps its anchor (top-right: offsets inwards)
TEST_F(UiWorkspaceUiFixture, StageMoveKeepsTheAnchor)
{
    OpenWorkspace(false);
    auto stage = Window()->GetStage();
    Vec2F coin = stage->BlockRect("coin").Center();
    Click(coin);
    EXPECT_EQ(Ctx()->selected, String("coin"));

    auto before = RectOf("coin");
    DragWith(coin, coin + WorldDelta(-100, 50), { VK_CONTROL });
    auto doc = DocNow();
    auto n = NodeOf(doc, "coin");
    ASSERT_TRUE(n);
    EXPECT_EQ(*n->anchor, String("top-right"));
    EXPECT_NEAR(*n->x, 124, 1.0);
    EXPECT_NEAR(*n->y, 74, 1.0);
    auto after = RectOf("coin");
    EXPECT_NEAR(after.x, before.x - 100, 1.0);
    EXPECT_NEAR(after.y, before.y + 50, 1.0);
    EXPECT_EQ(LastLabel(), String("coin: move"));
    EXPECT_EQ(undo->GetUndoActionsCount(), 1) << "one undo step of the editor";

    // Whole numbers are written as the web writes them
    String json = editor->GetGraph()->ToJsonString();
    EXPECT_TRUE(json.Contains("\"x\":" + UiDoc::JsNum(*n->x)) || json.Contains("\"x\": " + UiDoc::JsNum(*n->x))) << json.SubStr(0, 400).Data();
}

// The edges snap to a sibling: the coin's left edge pulled to within a few pixels of the card's right edge lands on it
TEST_F(UiWorkspaceUiFixture, StageMoveSnapsToASibling)
{
    OpenWorkspace(false);
    auto stage = Window()->GetStage();
    Vec2F coin = stage->BlockRect("coin").Center();
    Click(coin);
    // card: 160..920; the coin's left edge at 960 moves to 923
    Drag(coin, coin + WorldDelta(-37, 0));
    auto after = RectOf("coin");
    EXPECT_NEAR(after.x, 920, 0.01) << "snapped to the card's right edge";
    EXPECT_EQ(*NodeOf(DocNow(), "coin")->x, 64.0);
}

// A corner resizes, the side not pulled stays; the size is written with the offsets of its anchor
TEST_F(UiWorkspaceUiFixture, StageResizeFromACorner)
{
    OpenWorkspace(false);
    auto stage = Window()->GetStage();
    Click(stage->BlockRect("play").Center());
    ASSERT_EQ(Ctx()->selected, String("play"));
    auto before = RectOf("play");
    Vec2F knob = stage->HandlePoint("se");
    DragWith(knob, knob + WorldDelta(40, 20), { VK_CONTROL });
    auto after = RectOf("play");
    EXPECT_NEAR(after.x, before.x, 1.0) << "the left edge stays";
    EXPECT_NEAR(after.y, before.y, 1.0) << "the top edge stays";
    EXPECT_NEAR(after.w, 440, 1.0);
    EXPECT_NEAR(after.h, 140, 1.0);
    EXPECT_EQ(*NodeOf(DocNow(), "play")->anchor, String("bottom-center"));
    EXPECT_EQ(LastLabel(), String("play: size"));
}

// The anchor's triangles dragged to the parent's top-left point: the anchor changes, the block stays where it is
TEST_F(UiWorkspaceUiFixture, StageAnchorDragKeepsTheRect)
{
    OpenWorkspace(false);
    auto stage = Window()->GetStage();
    Click(stage->BlockRect("play").Center());
    ASSERT_EQ(Ctx()->selected, String("play"));
    auto before = RectOf("play");
    auto card = RectOf("card");
    Vec2F grip = stage->AnchorGrip();
    Drag(grip, stage->DesignToWorld(card.x + 2, card.y + 2));
    auto playDoc = DocNow();
    auto n = NodeOf(playDoc, "play");
    ASSERT_TRUE(n);
    EXPECT_EQ(*n->anchor, String("top-left"));
    auto after = RectOf("play");
    EXPECT_NEAR(after.x, before.x, 0.01);
    EXPECT_NEAR(after.y, before.y, 0.01);
    EXPECT_NEAR(after.w, before.w, 0.01);
    EXPECT_NEAR(after.h, before.h, 0.01);
    EXPECT_EQ(LastLabel(), String("play: anchor"));
}

// Inside a column a drag reorders: a dropped below c lands after it
TEST_F(UiWorkspaceUiFixture, StageReordersInsideAColumn)
{
    OpenWorkspace(false);
    auto stage = Window()->GetStage();
    Vec2F a = stage->BlockRect("a").Center();
    Click(a);
    ASSERT_EQ(Ctx()->selected, String("a"));
    auto c = RectOf("c");
    auto aRect = RectOf("a");
    Drag(a, a + WorldDelta(0, (float)(c.y + c.h + 30 - (aRect.y + aRect.h*0.5))));
    auto doc = DocNow();
    auto list = NodeOf(doc, "list");
    ASSERT_TRUE(list);
    ASSERT_EQ(list->children.size(), 3u) << StepsNow().Last().data.Dump().Data() << "\n" << UiDoc::DocToUiml(doc).Data();
    EXPECT_EQ(list->children[0].id, String("b"));
    EXPECT_EQ(list->children[1].id, String("c"));
    EXPECT_EQ(list->children[2].id, String("a"));
    EXPECT_EQ(LastLabel(), String("a: order"));
}

// A row dragged onto the middle of a block's row goes inside it and keeps its place on the screen
TEST_F(UiWorkspaceUiFixture, HierarchyDropsIntoABlockKeepingThePlace)
{
    OpenWorkspace(false);
    // The design itself as the screen (the kit's device would add room: blocks at half pixels after the drop)
    Ctx()->SetUi("uiDevice", UiDocJson("design"));
    Step(2);
    auto hierarchy = Window()->GetHierarchy();
    auto shownRect = [&](const String& id) { return UiDoc::SolveLayout(DocNow(), Ctx()->preview.frame.w, Ctx()->preview.frame.h).Get(id); };
    auto before = shownRect("coin");
    Drag(hierarchy->RowRect("coin").Center(), hierarchy->RowRect("card").Center(), 12);
    auto doc = DocNow();
    auto parent = UiDoc::FindParent(doc, "coin");
    ASSERT_TRUE(parent);
    EXPECT_EQ(parent->id, String("card"));
    auto after = shownRect("coin");
    EXPECT_NEAR(after.x, before.x, 0.01);
    EXPECT_NEAR(after.y, before.y, 0.01);
    EXPECT_EQ(LastLabel(), String("coin \xE2\x86\x92 card"));

    // The top third of a row: above it
    UiDoc::Ws::UiDropAt at;
    RectF title = hierarchy->RowRect("title");
    ASSERT_TRUE(hierarchy->DropTarget("bg", Vec2F(title.Center().x, title.top - 2), at));
    EXPECT_EQ(at.id, String("title"));
    EXPECT_EQ(at.zone, String("before"));
}

// Rename: the name typed replaces the id and the selection follows; a name another block has is refused
TEST_F(UiWorkspaceUiFixture, HierarchyRenames)
{
    OpenWorkspace(false);
    auto hierarchy = Window()->GetHierarchy();
    Ctx()->Select("play");
    hierarchy->StartRename("play");
    Step();
    ASSERT_TRUE(hierarchy->GetRenameField());
    hierarchy->GetRenameField()->SetText("start");
    hierarchy->EndRename(true);
    Step(2);
    EXPECT_TRUE(NodeOf(DocNow(), "start"));
    EXPECT_FALSE(NodeOf(DocNow(), "play"));
    EXPECT_EQ(Ctx()->selected, String("start"));
    EXPECT_EQ(LastLabel(), String("play \xE2\x86\x92 start"));

    hierarchy->StartRename("start");
    hierarchy->GetRenameField()->SetText("a");
    hierarchy->EndRename(true);
    Step(2);
    EXPECT_TRUE(NodeOf(DocNow(), "start")) << "a is taken";

    // The screen's row renames the screen
    hierarchy->StartRename(DocNow().root.id);
    hierarchy->GetRenameField()->SetText("Shop");
    hierarchy->EndRename(true);
    EXPECT_EQ(DocNow().name, String("Shop"));
}

// A number typed into the inspector applies while it is typed and the keys fold into one action
TEST_F(UiWorkspaceUiFixture, InspectorFieldsFoldIntoOneAction)
{
    OpenWorkspace(false);
    Ctx()->Select("title");
    Window()->GetSide()->OpenTab("inspect");
    Step(3);
    auto inspector = Window()->GetSide()->GetInspector();
    auto field = inspector->GetField("num:h");
    ASSERT_TRUE(field);
    int stepsBefore = StepsNow().Count();
    for (auto text : { "1", "15", "150" })
    {
        field->SetText(text);
        Step();
    }
    auto titleDoc = DocNow();
    auto n = NodeOf(titleDoc, "title");
    EXPECT_EQ(*n->h, 150.0);
    auto steps = StepsNow();
    ASSERT_EQ(steps.Count(), stepsBefore + 1);
    EXPECT_EQ(UiDoc::StepParts(steps.Last()).Count(), 1) << "typing folds";
    EXPECT_EQ(LastLabel(), String("title: h"));

    // The anchor presets: the block keeps its rect; Alt moves it to the anchor
    Ctx()->Select("coin");
    Step(2);
    auto before = RectOf("coin");
    inspector->Choose("anchor:center");
    EXPECT_EQ(*NodeOf(DocNow(), "coin")->anchor, String("center"));
    EXPECT_NEAR(RectOf("coin").x, before.x, 0.01);
    inspector->Choose("anchor:top-left", true);
    auto coinDoc = DocNow();
    auto coin = NodeOf(coinDoc, "coin");
    EXPECT_EQ(*coin->anchor, String("top-left"));
    EXPECT_FALSE(coin->x.has_value());
    EXPECT_FALSE(coin->y.has_value());

    // Segments and toggles
    Ctx()->Select("card");
    Step(2);
    inspector->Choose("seg:layout:row");
    EXPECT_EQ(*NodeOf(DocNow(), "card")->layout, String("row"));
    inspector->Choose("toggle:visible");
    EXPECT_FALSE(NodeOf(DocNow(), "card")->IsShown());
}

// The UIML text applies as one action; a text with problems lists them and changes nothing
TEST_F(UiWorkspaceUiFixture, TextTabAppliesUiml)
{
    OpenWorkspace(false);
    Window()->GetSide()->OpenTab("text");
    Step(3);
    auto text = Window()->GetSide()->GetText();
    String uiml = UiDoc::DocToUiml(DocNow());
    EXPECT_EQ((String)text->GetField()->GetText(), uiml);
    EXPECT_FALSE(text->HasDraft());

    text->GetField()->SetText(uiml.ReplacedAll("\"PLAY\"", "\"GO\""));
    Step();
    EXPECT_TRUE(text->HasDraft());
    EXPECT_TRUE(text->Apply());
    Step(2);
    EXPECT_EQ(*NodeOf(DocNow(), "play")->text, String("GO"));
    EXPECT_EQ(LastLabel(), String("text edited"));

    int steps = StepsNow().Count();
    text->GetField()->SetText("screen Menu 1080x1920\n  button a\n      text deeper \"x\"\n  ??? !!!\n");
    Step();
    EXPECT_FALSE(text->Apply());
    EXPECT_FALSE(text->GetErrors().IsEmpty());
    EXPECT_EQ(StepsNow().Count(), steps);
    text->Reset();
    Step(2);
    EXPECT_FALSE(text->HasDraft());
}

// Undo takes the last hand action back (into the redo list, part of its step), redo puts it back into its step; the
// history reverts to a step and peeks at one
TEST_F(UiWorkspaceUiFixture, UndoRedoByActionAndHistory)
{
    OpenWorkspace(false);
    auto hierarchy = Window()->GetHierarchy();
    Ctx()->Select("coin");
    Step(2);
    hierarchy->ClickHot("hide:coin");
    Ctx()->Select("bg");
    Step(2);
    hierarchy->ClickHot("hide:bg");
    Step(2);
    auto steps = StepsNow();
    ASSERT_EQ(steps.Count(), 2);
    EXPECT_EQ(UiDoc::StepParts(steps.Last()).Count(), 2);
    EXPECT_FALSE(NodeOf(DocNow(), "bg")->IsShown());

    Window()->GetSide()->OpenTab("history");
    Step(2);
    auto history = Window()->GetSide()->GetHistory();
    history->ClickHot("undo");
    Step(2);
    EXPECT_TRUE(NodeOf(DocNow(), "bg")->IsShown());
    EXPECT_FALSE(NodeOf(DocNow(), "coin")->IsShown());
    auto redo = UiDoc::RedoOf(layoutNode->config);
    ASSERT_EQ(redo.Count(), 1);
    EXPECT_TRUE(redo[0].PartOf().has_value());

    history->ClickHot("redo");
    Step(2);
    EXPECT_FALSE(NodeOf(DocNow(), "bg")->IsShown());
    EXPECT_EQ(UiDoc::StepParts(StepsNow().Last()).Count(), 2) << "back into its step";
    EXPECT_TRUE(UiDoc::RedoOf(layoutNode->config).IsEmpty());

    // Peek at the empty screen, back to now
    history->ClickHot("step:0");
    Step(2);
    EXPECT_EQ(Ctx()->peek, 0);
    EXPECT_TRUE(Ctx()->Doc().root.children.empty());
    EXPECT_FALSE(Ctx()->Editable());
    Window()->GetStage()->ClickHot("peek:back");
    Step(2);
    EXPECT_EQ(Ctx()->peek, -1);

    // Revert to the screen after the agent's step: the hand edits go to redo
    history->ClickHot("revert:1");
    Step(2);
    EXPECT_EQ(StepsNow().Count(), 1);
    EXPECT_EQ(UiDoc::RedoOf(layoutNode->config).Count(), 1);
    EXPECT_TRUE(NodeOf(DocNow(), "coin")->IsShown());
}

// The keys: arrows nudge (by the anchor), Delete removes, F2 renames, F frames
TEST_F(UiWorkspaceUiFixture, KeysNudgeDeleteRenameFrame)
{
    OpenWorkspace(false);
    auto window = Window();
    Ctx()->Select("coin");
    Step(2);
    window->PressKey(VK_LEFT);
    Step();
    EXPECT_EQ(*NodeOf(DocNow(), "coin")->x, 25.0) << "top-right: left is inwards from the right edge";
    EXPECT_EQ(LastLabel(), String("coin: nudge"));

    float zoom = window->GetStage()->GetZoom();
    window->PressKey(VK_F);
    Step();
    EXPECT_GT(window->GetStage()->GetZoom(), zoom) << "F frames the selected block";

    window->PressKey(VK_DELETE);
    Step(2);
    EXPECT_FALSE(NodeOf(DocNow(), "coin"));
    EXPECT_EQ(Ctx()->selected, String());

    Ctx()->Select("title");
    Step();
    window->PressKey(VK_F2);
    Step();
    EXPECT_EQ(window->GetHierarchy()->GetRenaming(), String("title"));
    window->PressKey(VK_ESCAPE);
    Step();
    EXPECT_EQ(window->GetHierarchy()->GetRenaming(), String());
    EXPECT_TRUE(window->IsShown()) << "Escape ends the rename first";

    // Then the peek, then the window
    Ctx()->SetPeek(0);
    window->PressKey(VK_ESCAPE);
    Step();
    EXPECT_EQ(Ctx()->peek, -1);
    EXPECT_TRUE(window->IsShown());
    window->PressKey(VK_ESCAPE);
    Step();
    EXPECT_FALSE(window->IsShown());
}

// The add menu puts a block into the selected container, or after a selected leaf
TEST_F(UiWorkspaceUiFixture, AddPutsBlocksWhereTheWebDoes)
{
    OpenWorkspace(false);
    auto hierarchy = Window()->GetHierarchy();
    Ctx()->Select("card");
    Step();
    hierarchy->AddBlock("button");
    auto doc = DocNow();
    auto parent = UiDoc::FindParent(doc, "button1");
    ASSERT_TRUE(parent);
    EXPECT_EQ(parent->id, String("card"));
    EXPECT_EQ(Ctx()->selected, String("button1"));
    EXPECT_EQ(LastLabel(), String("+ button1"));

    Ctx()->Select("title");
    Step();
    hierarchy->AddBlock("text");
    doc = DocNow();
    auto card = NodeOf(doc, "card");
    ASSERT_TRUE(card);
    EXPECT_EQ(card->children[0].id, String("title"));
    EXPECT_EQ(card->children[1].id, String("text1")) << "after the selected leaf";
}

// The config is written as the web writes it: a hand step {id, by, prompt, ops, at, parts}, uiRedo emptied, the view keys
// written without an undo step and out of the node's signature
TEST_F(UiWorkspaceUiFixture, ConfigWrittenAsTheWebWrites)
{
    OpenWorkspace(false);
    auto signatureOf = [&]() { return editor->GetGraph()->ComputeSignatures()[layoutNode->id]; };
    String signature = signatureOf();
    Ctx()->Select("coin");
    Window()->GetSide()->OpenTab("history");
    Ctx()->SetUi("uiShowIds", UiDocJson(true));
    Step(2);
    EXPECT_EQ(signatureOf(), signature) << "view keys never change what the node renders";
    EXPECT_EQ(undo->GetUndoActionsCount(), 0) << "view keys are no undo steps";

    Window()->GetHierarchy()->ClickHot("hide:coin");
    Step(2);
    EXPECT_NE(signatureOf(), signature);
    EXPECT_EQ(undo->GetUndoActionsCount(), 1);

    auto config = ConfigNow();
    Vector<String> keys;
    for (auto& m : config.Members())
        keys.Add(m.first);
    int stepsAt = keys.IndexOf("steps"), redoAt = keys.IndexOf("uiRedo");
    ASSERT_GE(stepsAt, 0);
    ASSERT_GE(redoAt, 0);
    EXPECT_EQ(config.Get("uiRedo")->Dump(), String("[]"));
    EXPECT_EQ(config.Get("uiSelected")->Str(), String("coin"));
    EXPECT_EQ(config.Get("uiTab")->Str(), String("history"));
    EXPECT_TRUE(config.Get("uiShowIds")->Bool());

    const UiDocJson* step = config.Get("steps")->At(1);
    ASSERT_TRUE(step);
    Vector<String> stepKeys;
    for (auto& m : step->Members())
        stepKeys.Add(m.first);
    EXPECT_EQ(stepKeys, Vector<String>({ "id", "by", "prompt", "ops", "at", "parts" }));
    EXPECT_EQ(step->Get("by")->Str(), String("user"));
    EXPECT_EQ(step->Get("prompt")->Str(), String("coin: Hide"));
    EXPECT_EQ(step->Get("ops")->Dump(), String("[{\"op\":\"update\",\"id\":\"coin\",\"set\":{\"visible\":false}}]"));
    const UiDocJson* part = step->Get("parts")->At(0);
    EXPECT_EQ(part->Get("label")->Str(), String("coin: Hide"));
    EXPECT_EQ(part->Get("n")->Dump(), String("1"));
    EXPECT_EQ(part->Get("at")->Str(), step->Get("at")->Str());
    EXPECT_TRUE(step->Get("at")->Str().EndsWith("Z"));
}

// The agent's Send goes to the handler with the request and the selected block; busy holds the hand edits; the answer is
// appended as the web appends it
TEST_F(UiWorkspaceUiFixture, AgentSendAndAppend)
{
    OpenWorkspace(false);
    String sentPrompt, sentSelected, sentNode;
    UiWorkspace::SetAgentSendHandler([&](const Ref<PipelineEditor>&, const String& nodeId, const String& prompt, const String& selected)
    {
        sentNode = nodeId;
        sentPrompt = prompt;
        sentSelected = selected;
    });
    Ctx()->Select("card");
    Window()->GetSide()->OpenTab("agent");
    Step(2);
    auto agent = Window()->GetSide()->GetAgent();
    agent->GetPromptField()->SetText("make the card wider");
    EXPECT_TRUE(agent->Send());
    EXPECT_EQ(sentNode, layoutNode->id);
    EXPECT_EQ(sentPrompt, String("make the card wider"));
    EXPECT_EQ(sentSelected, String("card"));
    EXPECT_EQ(ConfigNow().Get("uiDraft")->Str(), String("make the card wider"));

    UiWorkspace::SetAgentBusy(layoutNode->id, true);
    Step();
    EXPECT_FALSE(agent->Send()) << "one request at a time";
    int steps = StepsNow().Count();
    Window()->GetHierarchy()->ClickHot("hide:card");
    Step();

    UiDoc::UiOp op = UiDoc::MakeUpdateOp("card", [] { UiDocJson s = UiDocJson::Object(); s["w"] = 900; return s; }());
    auto answer = UiDoc::MakeAgentStep("make the card wider", "Wider.", { op }, UiDoc::Ws::NowMs(), "gpt-6.1-sol");
    EXPECT_TRUE(UiWorkspace::AppendAgentStep(editor, layoutNode->id, answer.data));
    UiWorkspace::SetAgentBusy(layoutNode->id, false);
    Step(2);
    EXPECT_EQ(StepsNow().Count(), steps + 2) << "the hand edit and the answer";
    EXPECT_EQ(*NodeOf(DocNow(), "card")->w, 900.0);
    EXPECT_EQ(ConfigNow().Get("uiDraft")->Str(), String(""));
    EXPECT_EQ(ConfigNow().Get("uiRedo")->Dump(), String("[]"));
    UiWorkspace::SetAgentSendHandler({});
}

// The screen picker writes the view keys: a device, turned, a custom screen (w, h, dpi in that order); the kit's device is
// the default, every row names its aspect ratio and the canvas it gives in the layout's pixels
TEST_F(UiWorkspaceUiFixture, DevicePickerWritesTheViewKeys)
{
    OpenWorkspace(false);
    auto picker = Window()->GetStage()->GetDevicePicker();
    int steps = StepsNow().Count();
    EXPECT_EQ(Ctx()->preview.kind, String("kit"));
    EXPECT_EQ(Ctx()->preview.name, String("iPhone 16"));
    EXPECT_EQ(Ctx()->preview.frame.w, 1080.0);
    EXPECT_EQ(Ctx()->preview.frame.h, 2341.0) << "the design fitted into the device, the room left over added";
    picker->ToggleMenu();
    Step(2);
    ASSERT_TRUE(picker->IsMenuOpen());
    auto menu = picker->GetMenuPanel();
    EXPECT_EQ(menu->RowText("kit"), String("Kit's device \xC2\xB7 iPhone 16 9:19.5 \xC2\xB7 1080\xC3\x97" "2341"));
    EXPECT_EQ(menu->RowText("ipad-pro-13"), String("iPad Pro 13\xE2\x80\xB3 3:4 \xC2\xB7 1440\xC3\x97" "1920"));
    EXPECT_EQ(menu->RowText("design"), String("Design 9:16 \xC2\xB7 1080\xC3\x97" "1920"));
    EXPECT_EQ(menu->RowText("reference"), String("Reference \xC2\xB7 connect a picture"));
    picker->ToggleMenu();
    Step(2);
    picker->Pick("iphone-15");
    Step(2);
    EXPECT_EQ(ConfigNow().Get("uiDevice")->Str(), String("iphone-15"));
    EXPECT_EQ(Ctx()->preview.id, String("iphone-15"));
    EXPECT_NE(Ctx()->preview.frame.h, 1920.0);

    picker->ToggleLandscape();
    Step(2);
    EXPECT_TRUE(ConfigNow().Get("uiLandscape")->Bool());
    EXPECT_GT(Ctx()->preview.screen.w, Ctx()->preview.screen.h);

    picker->SetCustom("w", 800);
    Step(2);
    EXPECT_EQ(ConfigNow().Get("uiDevice")->Str(), String("custom"));
    EXPECT_EQ(ConfigNow().Get("uiDeviceCustom")->Dump(), String("{\"w\":800,\"h\":1920,\"dpi\":480}"));

    EXPECT_EQ(StepsNow().Count(), steps);

    picker->ToggleMenu();
    Step(2);
    ASSERT_TRUE(picker->IsMenuOpen());
    EXPECT_EQ(menu->RowText("ipad-pro-13"), String("iPad Pro 13\xE2\x80\xB3 4:3 \xC2\xB7 2560\xC3\x97" "1920")) << "turned";
    menu->ClickHot("device:design");
    Step(2);
    EXPECT_EQ(ConfigNow().Get("uiDevice")->Str(), String("design"));
    menu->ClickHot("device:kit");
    Step(2);
    EXPECT_EQ(ConfigNow().Get("uiDevice")->Str(), String("kit"));
    EXPECT_EQ(StepsNow().Count(), steps) << "view keys only";
}

// The window: at least 860 x 520, maximized by its button and remembered (with the panels' widths) after it is made again
TEST_F(UiWorkspaceUiFixture, WindowGeometryIsKept)
{
    o2FileSystem.FileDelete(PipelinePairLayout::GetViewPrefsPath());
    // In its device's pixels: an older layout's notice in the title bar would take the header's middle
    OpenWorkspace(false, String(workspaceScreen).ReplacedAll("screen Menu 1080x1920", "screen Menu 1080x1920 dpi=460"));
    auto window = Window();
    Vec2F screen = (Vec2F)o2Render.GetResolution();
    window->SetFrame(10, 10, 300, 200);
    auto f = window->GetFrame();
    EXPECT_GE(f.w, Math::Min(860.0f, screen.x - 16));
    EXPECT_GE(f.h, Math::Min(520.0f, screen.y - 16));

    // The header drags the window
    window->SetFrame(20, 20, 860, 520);
    Step(2);
    float x = window->GetFrame().x;
    RectF whole = window->layout->GetWorldRect();
    Vec2F head(whole.left + 20 + 400, whole.top - 20 - 24);
    Drag(head, head + Vec2F(30, -10));
    EXPECT_NEAR(window->GetFrame().x, x + 30, 1.0);

    // A double-click on the header maximizes, another restores
    for (int n = 0; n < 2; n++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(350));
        Vec2F at(whole.left + window->GetFrame().x + 300, whole.top - window->GetFrame().y - 24);
        o2Input.OnCursorMoved(at, 0, false);
        Step();
        for (int i = 0; i < 2; i++)
        {
            o2Input.OnCursorPressed(at);
            Step();
            o2Input.OnCursorReleased();
            Step();
        }
        Step();
        EXPECT_EQ(window->IsMaximized(), n == 0);
    }

    window->SetPanelWidth("left", 200);
    EXPECT_EQ(window->GetLeftWidth(), 200.0f);
    window->ToggleMaximized();
    EXPECT_TRUE(window->IsMaximized());
    Step(2);

    UiWorkspace::Reset();
    UiWorkspace::Open(editor, layoutNode->id);
    Step(2);
    EXPECT_TRUE(Window()->IsMaximized());
    EXPECT_EQ(Window()->GetLeftWidth(), 200.0f);
    Window()->SetPanelWidth("left", 0);
    EXPECT_NE(Window()->GetLeftWidth(), 200.0f) << "a double-click on the divider gives the default back";
}

// With a kit: the coverage popover lists the blocks without a graphic, a click selects one and opens the inspector
TEST_F(UiWorkspaceUiFixture, CoverageListsBlocksWithoutAGraphic)
{
    OpenWorkspace(true, String(workspaceScreen) + "  progress hp anchor=bottom-left pos=40,40 size=400x40\n");
    ASSERT_TRUE(Ctx()->hasKit);
    bool listed = Ctx()->report.missing.Any([](const Pair<String, String>& m) { return m.first == "hp"; });
    EXPECT_TRUE(listed);
    Window()->ToggleCoverage();
    Step(3);
    Window()->GetCoveragePanel()->ClickHot("block:hp");
    Step(2);
    EXPECT_EQ(Ctx()->selected, String("hp"));
    EXPECT_EQ(Ctx()->tab, String("inspect"));
}

// The screen picker on its own (as a card places it): bound to the node, it reads the node itself, follows a change made
// elsewhere and writes the view keys
TEST_F(UiWorkspaceUiFixture, DevicePickerStandsAlone)
{
    OpenWorkspace(false);
    UiWorkspace::Close();
    PushEditorScopeOnStack scope;
    auto picker = mmake<UiDevicePicker>();
    *picker->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(360, 28), Vec2F(40, -40));
    EditorUIRoot.AddWidget(picker);
    picker->Bind(editor, layoutNode->id);
    Step(2);
    EXPECT_GT(picker->GetPreferredWidth(), 100.0f);
    EXPECT_EQ(picker->ctx->preview.kind, String("kit"));

    picker->Pick("pixel-8");
    Step(2);
    EXPECT_EQ(ConfigNow().Get("uiDevice")->Str(), String("pixel-8"));

    // A change made elsewhere is noticed
    UiDoc::SetConfigJson(layoutNode->config, "uiDevice", UiDocJson("ipad-mini"));
    Step(30);
    EXPECT_EQ(picker->ctx->preview.id, String("ipad-mini"));

    picker->ToggleMenu();
    Step(2);
    EXPECT_TRUE(picker->IsMenuOpen());
    // A custom side applies on Enter or when its field is left, not while it is typed
    auto field = picker->GetMenuPanel()->GetCustomField("w");
    ASSERT_TRUE(field);
    field->Focus();
    field->SetText("1");
    Step(2);
    EXPECT_EQ(ConfigNow().Get("uiDevice")->Str(), String("ipad-mini")) << "not live";
    field->SetText("1000");
    field->Unfocus();
    Step(2);
    EXPECT_EQ(ConfigNow().Get("uiDevice")->Str(), String("custom"));
    EXPECT_EQ(ConfigNow().Get("uiDeviceCustom")->Get("w")->Num(), 1000.0);
    picker->ToggleMenu();
    Step(2);
    EditorUIRoot.RemoveWidget(picker);
}

// Cmd+Z while the window has the focus undoes the layout's last hand action (the shortcut is the window's, not the editor's);
// Cmd+Shift+Z redoes it
TEST_F(UiWorkspaceUiFixture, CmdZUndoesTheLayout)
{
    OpenWorkspace(false);
    Ctx()->Select("coin");
    Step(2);
    Window()->GetHierarchy()->ClickHot("hide:coin");
    Step(2);
    ASSERT_FALSE(NodeOf(DocNow(), "coin")->IsShown());
    Click(Window()->GetStage()->DesignToWorld(-30, 300)); // the board beside the screen: the window takes the focus
    int undoBefore = undo->GetUndoActionsCount();

    auto press = [&](const Vector<int>& keys)
    {
        for (int key : keys)
        {
            o2Input.OnKeyPressed(key);
            Step();
        }
        Step();
        for (int i = keys.Count() - 1; i >= 0; i--)
            o2Input.OnKeyReleased(keys[i]);
        Step(2);
    };
#if defined(PLATFORM_MAC)
    int cmd = VK_COMMAND;
#else
    int cmd = VK_CONTROL;
#endif
    press({ cmd, VK_Z });
    ASSERT_TRUE(NodeOf(DocNow(), "coin"));
    EXPECT_TRUE(NodeOf(DocNow(), "coin")->IsShown());
    EXPECT_EQ(UiDoc::RedoOf(layoutNode->config).Count(), 1);
    EXPECT_EQ(undo->GetUndoActionsCount(), undoBefore + 1) << "the layout's undo is itself one step of the editor";
    press({ cmd, VK_SHIFT, VK_Z });
    ASSERT_TRUE(NodeOf(DocNow(), "coin"));
    EXPECT_FALSE(NodeOf(DocNow(), "coin")->IsShown());
    EXPECT_TRUE(UiDoc::RedoOf(layoutNode->config).IsEmpty());
}

// A double-click on a row renames the block; the window closes when its node is deleted
TEST_F(UiWorkspaceUiFixture, DoubleClickRenamesAndDeletingTheNodeCloses)
{
    OpenWorkspace(false);
    auto hierarchy = Window()->GetHierarchy();
    Vec2F row = hierarchy->RowRect("title").Center();
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    o2Input.OnCursorMoved(row, 0, false);
    Step();
    for (int i = 0; i < 2; i++)
    {
        o2Input.OnCursorPressed(row);
        Step();
        o2Input.OnCursorReleased();
        Step();
    }
    Step(2);
    EXPECT_EQ(hierarchy->GetRenaming(), String("title"));
    EXPECT_EQ(Ctx()->selected, String("title"));
    hierarchy->EndRename(false);
    Step();

    editor->GetGraph()->nodes.Remove(editor->GetGraph()->FindNode(layoutNode->id));
    Step(20);
    EXPECT_FALSE(UiWorkspace::IsOpen());
}
