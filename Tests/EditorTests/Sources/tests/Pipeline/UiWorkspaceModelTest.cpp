#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiDocTestKit.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspaceModel.h"

// The workspace's logic without a window (port of uiLayoutState.ts and the logic parts of UiWorkspace.tsx): the step queue
// written into a node's config as the web writes it, the timeline, the preview screens, the hierarchy's add / drop / rename
// and the inspector's edits

namespace
{
    using namespace UiDocTest;
    namespace Ws = Editor::UiDoc::Ws;

    const char* const screen =
        "screen Menu 1080x1920\n"
        "  panel card anchor=center size=760x900\n"
        "    text title anchor=top-stretch pos=0,40 h=120 \"TITLE\"\n"
        "    button play anchor=bottom-center pos=0,60 size=400x120 \"PLAY\"\n"
        "  column list anchor=top-left pos=40,80 size=300x600 gap=20\n"
        "    button a h=100 \"A\"\n"
        "    button b h=100 \"B\"\n"
        "  icon coin anchor=top-right pos=24,24 size=96x96\n";

    // A config whose steps lay the screen out (one agent step)
    DataDocument ConfigWithScreen()
    {
        DataDocument config;
        config.SetObject();
        SetConfigJson(config, "agentModel", Json("gpt-6.1-sol"));
        auto step = MakeAgentStep("menu", "Done.", { MakeDocOp(Parse(screen)) }, 1762862400000, "gpt-6.1-sol");
        WriteSteps(config, { step }, {});
        return config;
    }

    UiDoc DocOf(const DataValue& config) { return DocAt(config).doc; }

    Json UpdateSet(const char* key, const Json& value)
    {
        Json set = Json::Object();
        set[key] = value;
        return set;
    }
}

TEST(UiWorkspaceModel, CommitWritesTheStepsAndEmptiesRedo)
{
    auto config = ConfigWithScreen();
    SetConfigJson(config, "uiRedo", StepsToJson({ UiStep(Json::Object()) }));
    Int64 t0 = 1762862500000;
    ASSERT_TRUE(Ws::CommitUser(config, { MakeUpdateOp("coin", UpdateSet("x", 30)) }, "coin: move", t0, false));
    auto json = FromDataValue(config);
    EXPECT_STR_EQ(json.Get("uiRedo")->Dump(), "[]");
    auto steps = StepsOf(config);
    ASSERT_EQ(steps.Count(), 2);
    EXPECT_STR_EQ(steps[1].By(), "user");
    EXPECT_STR_EQ(*steps[1].Prompt(), "coin: move");
    EXPECT_STR_EQ(steps[1].At(), IsoTime((double)t0));
    EXPECT_STR_EQ(json.Get("steps")->At(1)->Get("ops")->Dump(), "[{\"op\":\"update\",\"id\":\"coin\",\"set\":{\"x\":30}}]");

    // The config keeps its order: agentModel, steps, uiRedo
    Vector<String> keys;
    for (auto& m : json.Members())
        keys.Add(m.first);
    EXPECT_EQ(keys, Vector<String>({ "agentModel", "steps", "uiRedo" }));
    EXPECT_FALSE(Ws::CommitUser(config, {}, "nothing", t0));
}

TEST(UiWorkspaceModel, TypingFoldsGesturesDoNot)
{
    auto config = ConfigWithScreen();
    Int64 t = 1762862500000;
    Ws::CommitUser(config, { MakeUpdateOp("title", UpdateSet("h", 1)) }, "title: h", t);
    Ws::CommitUser(config, { MakeUpdateOp("title", UpdateSet("h", 15)) }, "title: h", t + 500);
    Ws::CommitUser(config, { MakeUpdateOp("title", UpdateSet("h", 150)) }, "title: h", t + 900);
    auto steps = StepsOf(config);
    ASSERT_EQ(steps.Count(), 2);
    EXPECT_EQ(StepParts(steps[1]).Count(), 1);
    EXPECT_EQ(*FindNode(DocOf(config), "title")->h, 150.0);

    // The same update as a gesture is its own action
    Ws::CommitUser(config, { MakeUpdateOp("title", UpdateSet("h", 160)) }, "title: h", t + 1200, false);
    EXPECT_EQ(StepParts(StepsOf(config)[1]).Count(), 2);
}

TEST(UiWorkspaceModel, UndoRedoByActionAndRevert)
{
    auto config = ConfigWithScreen();
    Int64 t = 1762862500000;
    Ws::CommitUser(config, { MakeUpdateOp("coin", UpdateSet("visible", false)) }, "coin: Hide", t, false);
    Ws::CommitUser(config, { MakeUpdateOp("card", UpdateSet("visible", false)) }, "card: Hide", t + 100, false);
    ASSERT_TRUE(Ws::UndoStep(config));
    auto doc = DocOf(config);
    EXPECT_TRUE(FindNode(doc, "card")->IsShown());
    EXPECT_FALSE(FindNode(doc, "coin")->IsShown());
    auto redo = RedoOf(config);
    ASSERT_EQ(redo.Count(), 1);
    EXPECT_TRUE(redo[0].PartOf().has_value());

    ASSERT_TRUE(Ws::RedoStep(config));
    EXPECT_FALSE(FindNode(DocOf(config), "card")->IsShown());
    EXPECT_TRUE(RedoOf(config).IsEmpty());
    EXPECT_FALSE(Ws::RedoStep(config));

    // Revert: the later steps go to redo, newest last
    ASSERT_TRUE(Ws::RevertTo(config, 0));
    EXPECT_TRUE(StepsOf(config).IsEmpty());
    auto r = RedoOf(config);
    ASSERT_EQ(r.Count(), 2);
    EXPECT_TRUE(r[0].By() == "user");
    EXPECT_TRUE(r[1].IsAgent());
    EXPECT_FALSE(Ws::RevertTo(config, 0)) << "nothing after it";
}

TEST(UiWorkspaceModel, TimelineKeepsTheDocumentsOfStepsThatDidNotChange)
{
    auto config = ConfigWithScreen();
    Ws::UiTimeline timeline;
    timeline.Update(FromDataValue(config));
    ASSERT_EQ(timeline.Count(), 2);
    EXPECT_TRUE(timeline.DocAfter(0).root.children.empty());
    EXPECT_TRUE(FindNode(timeline.DocAfter(1), "coin"));

    Ws::CommitUser(config, { MakeRemoveOp({ "coin" }) }, "\xE2\x88\x92 coin", 1762862500000);
    timeline.Update(FromDataValue(config));
    ASSERT_EQ(timeline.Count(), 3);
    EXPECT_FALSE(FindNode(timeline.DocAfter(2), "coin"));
    EXPECT_TRUE(FindNode(timeline.DocAfter(1), "coin"));

    // An edit the agent could not apply is reported for its step
    Ws::CommitUser(config, { MakeRemoveOp({ "nothing" }) }, "\xE2\x88\x92 nothing", 1762862501000, false);
    timeline.Update(FromDataValue(config));
    EXPECT_FALSE(timeline.ErrorsOf(2).IsEmpty());
}

TEST(UiWorkspaceModel, PreviewScreens)
{
    UiDoc doc = Parse(screen);
    Json config = Json::Object();
    auto setting = Ws::PreviewSettingOf(config);
    EXPECT_STR_EQ(setting.device, "design");
    auto p = Ws::PreviewOf(doc, setting, nullptr);
    EXPECT_STR_EQ(p.kind, "design");
    EXPECT_EQ(p.frame.w, 1080.0);
    EXPECT_EQ(p.frame.scale, 1.0);

    // The reference asked for without one: the design, marked as a fallback
    config["uiDevice"] = "reference";
    p = Ws::PreviewOf(doc, Ws::PreviewSettingOf(config), nullptr);
    EXPECT_STR_EQ(p.kind, "design");
    EXPECT_TRUE(p.fallback);
    UiScreen reference{ 750, 1334, 326 };
    p = Ws::PreviewOf(doc, Ws::PreviewSettingOf(config), &reference);
    EXPECT_STR_EQ(p.kind, "reference");
    EXPECT_EQ(p.screen.w, 750.0);

    // A device turned; a custom screen read clean
    config["uiDevice"] = "iphone-15";
    config["uiLandscape"] = true;
    p = Ws::PreviewOf(doc, Ws::PreviewSettingOf(config), nullptr);
    EXPECT_STR_EQ(p.name, "iPhone 15");
    EXPECT_GT(p.screen.w, p.screen.h);
    EXPECT_EQ(p.frame, PreviewFrame(1080, 1920, std::nullopt, p.screen, "dpi"));

    Json custom = Json::Object();
    custom["w"] = 20;
    custom["h"] = "900";
    custom["dpi"] = -5;
    config["uiDevice"] = "custom";
    config["uiDeviceCustom"] = custom;
    config["uiLandscape"] = false;
    config["uiScale"] = "fit";
    setting = Ws::PreviewSettingOf(config);
    EXPECT_EQ(setting.custom.w, 64.0);
    EXPECT_EQ(setting.custom.h, 900.0);
    EXPECT_EQ(setting.custom.dpi, UI_BASE_DPI);
    EXPECT_STR_EQ(setting.scale, "fit");
    EXPECT_STR_EQ(Ws::ScreenText(setting.custom), "64\xC3\x97" "900 \xC2\xB7 480 dpi");
}

TEST(UiWorkspaceModel, AddPutsTheBlockIntoTheSelectionOrAfterALeaf)
{
    UiDoc doc = Parse(screen);
    String label, id;
    UiOp op = Ws::AddBlockOp(doc, "card", "button", label, id);
    EXPECT_STR_EQ(id, "button1");
    EXPECT_STR_EQ(label, "+ button1");
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"add\",\"parent\":\"card\",\"nodes\":[{\"id\":\"button1\",\"type\":\"button\",\"w\":360,\"h\":110,\"text\":\"Button\"}]}");

    op = Ws::AddBlockOp(doc, "title", "text", label, id);
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"add\",\"parent\":\"card\",\"after\":\"title\",\"nodes\":[{\"id\":\"text1\",\"type\":\"text\",\"w\":480,\"h\":64,\"text\":\"Text\"}]}");

    // A taken id counts on; nothing selected: the screen
    doc = ApplyOps(doc, { op }).doc;
    op = Ws::AddBlockOp(doc, "", "text", label, id);
    EXPECT_STR_EQ(id, "text2");
    EXPECT_STR_EQ(op.Get("parent")->Str(), "root");
    op = Ws::AddBlockOp(doc, "", "column", label, id);
    EXPECT_STR_EQ(op.Get("nodes")->At(0)->Dump(), "{\"id\":\"column1\",\"type\":\"group\",\"layout\":\"column\",\"anchor\":\"center\",\"w\":600,\"h\":500,\"gap\":24}");
}

TEST(UiWorkspaceModel, DropZonesAndDrops)
{
    UiDoc doc = Parse(screen);
    Ws::UiDropAt at;
    // A container: thirds; a leaf: halves
    ASSERT_TRUE(Ws::ZoneAt(doc, "coin", "card", 0.2f, false, at));
    EXPECT_STR_EQ(at.zone, "before");
    ASSERT_TRUE(Ws::ZoneAt(doc, "coin", "card", 0.5f, false, at));
    EXPECT_STR_EQ(at.zone, "inside");
    ASSERT_TRUE(Ws::ZoneAt(doc, "coin", "title", 0.6f, false, at));
    EXPECT_STR_EQ(at.zone, "after");
    // Below an open container: above its first child; folded: after it
    ASSERT_TRUE(Ws::ZoneAt(doc, "coin", "card", 0.8f, false, at));
    EXPECT_EQ(at, (Ws::UiDropAt{ "title", "before" }));
    ASSERT_TRUE(Ws::ZoneAt(doc, "coin", "card", 0.8f, true, at));
    EXPECT_EQ(at, (Ws::UiDropAt{ "card", "after" }));
    // Never onto itself or inside itself; below the rows: the screen
    EXPECT_FALSE(Ws::ZoneAt(doc, "card", "title", 0.5f, false, at));
    ASSERT_TRUE(Ws::ZoneAt(doc, "coin", "", 0.0f, false, at));
    EXPECT_EQ(at, (Ws::UiDropAt{ "root", "inside" }));

    // Into the card: the coin keeps its place on the screen
    auto rects = SolveLayout(doc);
    Vector<UiOp> ops;
    String label;
    ASSERT_TRUE(Ws::DropOps(doc, rects, "coin", Ws::UiDropAt{ "card", "inside" }, ops, label));
    EXPECT_STR_EQ(label, "coin \xE2\x86\x92 card");
    auto moved = ApplyOps(doc, ops).doc;
    EXPECT_STR_EQ(FindParent(moved, "coin")->id, "card");
    EXPECT_EQ(SolveLayout(moved).Get("coin"), rects.Get("coin"));

    // A reorder among siblings; where it was: nothing
    ASSERT_TRUE(Ws::DropOps(doc, rects, "b", Ws::UiDropAt{ "a", "before" }, ops, label));
    EXPECT_STR_EQ(label, "b: order");
    EXPECT_FALSE(Ws::DropOps(doc, rects, "b", Ws::UiDropAt{ "a", "after" }, ops, label));
}

TEST(UiWorkspaceModel, RenameAndVisibility)
{
    UiDoc doc = Parse(screen);
    UiOp op;
    String label;
    ASSERT_TRUE(Ws::RenameOp(doc, "play", "start game", op, label));
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"update\",\"id\":\"play\",\"set\":{\"id\":\"start_game\"}}");
    EXPECT_STR_EQ(label, "play \xE2\x86\x92 start_game");
    EXPECT_FALSE(Ws::RenameOp(doc, "play", "coin", op, label)) << "taken";
    EXPECT_FALSE(Ws::RenameOp(doc, "play", "play", op, label)) << "the same";
    EXPECT_FALSE(Ws::RenameOp(doc, "play", "  ", op, label)) << "empty";
    ASSERT_TRUE(Ws::RenameOp(doc, "root", "Shop", op, label));
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"screen\",\"name\":\"Shop\"}");
    EXPECT_STR_EQ(label, "Menu \xE2\x86\x92 Shop");

    op = Ws::VisibilityOp(*FindNode(doc, "coin"), label);
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"update\",\"id\":\"coin\",\"set\":{\"visible\":false}}");
    EXPECT_STR_EQ(label, "coin: Hide");
    doc = ApplyOps(doc, { op }).doc;
    op = Ws::VisibilityOp(*FindNode(doc, "coin"), label);
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"update\",\"id\":\"coin\",\"unset\":[\"visible\"]}");
    EXPECT_STR_EQ(label, "coin: Show");
}

TEST(UiWorkspaceModel, InspectorEdits)
{
    UiDoc doc = Parse(screen);
    auto rects = SolveLayout(doc);
    String label;
    const UiNode& title = *FindNode(doc, "title");
    EXPECT_STR_EQ(Ws::SetFieldOp(title, "h", Json(150), label).Dump(), "{\"op\":\"update\",\"id\":\"title\",\"set\":{\"h\":150}}");
    EXPECT_STR_EQ(label, "title: h");
    EXPECT_STR_EQ(Ws::SetFieldOp(title, "h", Json(), label).Dump(), "{\"op\":\"update\",\"id\":\"title\",\"unset\":[\"h\"]}");
    EXPECT_STR_EQ(Ws::SetFieldOp(title, "variant", Json(""), label).Dump(), "{\"op\":\"update\",\"id\":\"title\",\"unset\":[\"variant\"]}");

    // Margins: a side set, all zero unset
    UiDoc stretched = Parse("screen S 400x400\n  panel p anchor=stretch margin=10\n");
    const UiNode& p = *FindNode(stretched, "p");
    EXPECT_STR_EQ(Ws::SetBoxOp(p, "margin", 1, 30.0, label).Dump(), "{\"op\":\"update\",\"id\":\"p\",\"set\":{\"margin\":[10,30,10,10]}}");
    UiDoc zero = Parse("screen S 400x400\n  panel p anchor=stretch margin=0,5,0,0\n");
    EXPECT_STR_EQ(Ws::SetBoxOp(*FindNode(zero, "p"), "margin", 1, std::nullopt, label).Dump(),
                  "{\"op\":\"update\",\"id\":\"p\",\"unset\":[\"margin\"]}");

    // A new anchor keeps the rect; Alt also moves the block to it
    UiOp op;
    ASSERT_TRUE(Ws::AnchorOp(doc, rects, "coin", "center", false, op, label));
    EXPECT_STR_EQ(label, "coin: anchor");
    auto moved = ApplyOps(doc, { op }).doc;
    EXPECT_STR_EQ(*FindNode(moved, "coin")->anchor, "center");
    EXPECT_EQ(SolveLayout(moved).Get("coin"), rects.Get("coin"));
    ASSERT_TRUE(Ws::AnchorOp(doc, rects, "coin", "top-left", true, op, label));
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"update\",\"id\":\"coin\",\"set\":{\"anchor\":\"top-left\"},\"unset\":[\"x\",\"y\"]}");

    EXPECT_STR_EQ(Ws::AnchorWords(ParseAnchor(String("top-stretch"))), "Top, full width");
    EXPECT_STR_EQ(Ws::AnchorWords(ParseAnchor(String("center"))), "Centre");
    EXPECT_STR_EQ(Ws::AnchorWords(ParseAnchor(String("stretch"))), "Fills the parent");
    EXPECT_STR_EQ(Ws::AnchorWords(ParseAnchor(String("bottom-right"))), "Bottom, right");
}

TEST(UiWorkspaceModel, ViewKeysAndTitles)
{
    Json config = Json::Object();
    EXPECT_STR_EQ(Ws::ViewOf(config, true), "kit");
    EXPECT_STR_EQ(Ws::ViewOf(config, false), "lines");
    config["uiView"] = "lines";
    EXPECT_STR_EQ(Ws::ViewOf(config, true), "lines");
    EXPECT_STR_EQ(Ws::TabOf(config), "agent");
    config["uiTab"] = "history";
    EXPECT_STR_EQ(Ws::TabOf(config), "history");
    EXPECT_STR_EQ(Ws::AgentModelOf(config), "gpt-6.1-sol");
    config["agentModel"] = " local:codex ";
    EXPECT_STR_EQ(Ws::AgentModelOf(config), "local:codex");

    UiDoc doc = Parse(screen);
    config["uiSelected"] = "coin";
    EXPECT_STR_EQ(Ws::SelectedOf(config, doc), "coin");
    config["uiSelected"] = "gone";
    EXPECT_STR_EQ(Ws::SelectedOf(config, doc), "");

    UiKit kit;
    UiKitElement a, b, c;
    a.name = "element_3";
    b.name = "btn";
    b.type = "other";
    c.name = "panel";
    c.type = "panel";
    kit.elements = { a, b, c };
    EXPECT_EQ(Ws::KitUndescribed(&kit), 2);

    // Titles: the request; one hand action; a run of them
    auto agent = MakeAgentStep("make a menu", "ok", {}, 1762862400000, "m");
    EXPECT_STR_EQ(Ws::StepTitle(agent), "make a menu");
    auto steps = PushUserAction({}, { MakeRemoveOp({ "a" }) }, "\xE2\x88\x92 a", 1762862400000, false);
    EXPECT_STR_EQ(Ws::StepTitle(steps[0]), "\xE2\x88\x92 a");
    steps = PushUserAction(steps, { MakeRemoveOp({ "b" }) }, "\xE2\x88\x92 b", 1762862401000, false);
    EXPECT_STR_EQ(Ws::StepTitle(steps[0]), "Hand edits \xC2\xB7 2");
    EXPECT_EQ(Ws::LocalTime(steps[0].At()).Length(), 5);
}
