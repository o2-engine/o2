#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRun.h"

// Edits after a run of AI image to UI, AssetsLine's backend/test/imageToUi.test.ts "edits after a run": the config rules of
// shared/imageToUi.ts, the dialog's rules, the state trace the debug view reads and the agent's command line of a round

using namespace o2;
using namespace Editor;
using namespace Editor::UiLayout;

namespace
{
    Json Parsed(const String& text)
    {
        Json json;
        EXPECT_TRUE(Json::Parse(text, json)) << text.Data();
        return json;
    }

    PipelineImageToUiEditItem Note(const String& text, const String& on = "", int x = 0, int y = 0)
    {
        PipelineImageToUiEditItem item;
        item.text = text;
        if (!on.IsEmpty())
            item.at = PipelineImageToUiEditPoint{ on, x, y };
        return item;
    }
}

TEST(ImageToUiEdits, RoundsAndDraftsAreCleanedAndARoundHasHalfTheTurns)
{
    auto rounds = PipelineImageToUi::Edits(Parsed(
        "[{\"id\": \"a\", \"items\": [{\"text\": \"  bigger  \", \"at\": {\"on\": \"design\", \"x\": 10.6, \"y\": -3}}, {\"text\": \"\"},"
        " {\"text\": \"no point\", \"at\": {\"x\": \"q\"}}]}, {\"id\": \"b\", \"items\": [{\"text\": \" \"}]}, \"junk\"]"));
    EXPECT_EQ(PipelineImageToUi::EditsJson(rounds).Dump(),
              "[{\"id\":\"a\",\"items\":[{\"text\":\"bigger\",\"at\":{\"on\":\"design\",\"x\":11,\"y\":0}},{\"text\":\"no point\"}]}]");
    EXPECT_EQ(PipelineImageToUi::EditItemsJson(PipelineImageToUi::EditItems(Parsed("[{\"text\": \"x\", \"at\": {\"on\": \"nope\", \"x\": 1, \"y\": 2}}]"))).Dump(),
              "[{\"text\":\"x\",\"at\":{\"on\":\"result\",\"x\":1,\"y\":2}}]");
    EXPECT_EQ(PipelineImageToUi::EditTurns(60), 30);
    EXPECT_EQ(PipelineImageToUi::EditTurns(20), 12);
    EXPECT_EQ(PipelineImageToUi::EditTurns(25), 13) << "halves round up, as Math.round";

    // A missing id is e<n>, ids and texts are cut as JavaScript cuts them, at most 20 notes, numbers as text are numbers
    String many = "[";
    for (int i = 0; i < 25; i++)
        many += String(i ? "," : "") + "{\"text\": \"n" + (String)i + "\"}";
    auto cut = PipelineImageToUi::Edits(Parsed("[{\"items\": " + many + "]}, {\"id\": 7, \"items\": [{\"text\": 5, \"at\": {\"x\": \"12\", \"y\": null}}]}]"));
    ASSERT_EQ(cut.Count(), 2);
    EXPECT_EQ(cut[0].id, "e1");
    EXPECT_EQ(cut[0].items.Count(), PipelineImageToUi::editItemsMax);
    EXPECT_EQ(cut[1].id, "7");
    EXPECT_EQ(cut[1].items[0].text, "5");
    ASSERT_TRUE(cut[1].items[0].at.has_value());
    EXPECT_EQ(cut[1].items[0].at->x, 12);
    EXPECT_EQ(cut[1].items[0].at->y, 0);
    String longText;
    for (int i = 0; i < 2100; i++)
        longText += "\xD0\xB6";
    auto longItems = PipelineImageToUi::EditItems(Parsed("[{\"text\": \"" + longText + "\"}]"));
    ASSERT_EQ(longItems.Count(), 1);
    EXPECT_EQ(longItems[0].text.Length(), 2*PipelineImageToUi::editTextMax) << "2000 characters, not bytes";

    // The rounds sent are in the signature, the draft and the open dialog are not
    auto node = PipelineNodeRegistry::CreateNode("imageToUi", Vec2F());
    ASSERT_TRUE(node);
    EXPECT_TRUE(PipelineGraph::GetUiOnlyConfigKeys().Contains("editsDraft"));
    EXPECT_TRUE(PipelineGraph::GetUiOnlyConfigKeys().Contains("editsView"));
    EXPECT_FALSE(PipelineGraph::GetUiOnlyConfigKeys().Contains("edits"));
    String sig = PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1);
    PipelineImageToUi::WriteDraft(*node, { Note("not sent", "result", 3, 4) });
    node->SetConfigBool("editsView", true);
    EXPECT_EQ(PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1), sig);
    PipelineImageToUi::WriteEdits(*node, { { "r1", { Note("call the red button start", "result", 120, 245) } } });
    String withRound = PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1);
    EXPECT_NE(withRound, sig);
    ASSERT_EQ(PipelineImageToUi::Edits(*node).Count(), 1);
    EXPECT_EQ(PipelineImageToUi::Edits(*node)[0].items[0].at->x, 120);
    EXPECT_EQ(PipelineImageToUi::ReadDraft(*node)[0].text, "not sent");

    // The kept states are keyed by the JSON of a round's notes, as run.ts roundHash
    EXPECT_EQ(ImageToUiRoundHash(PipelineImageToUi::Edits(*node)[0]), "73f5d30a8f2a3ab8");
}

TEST(ImageToUiEdits, TheDialogPutsPointsAppliesAndUndoesRounds)
{
    // A click is a point in the picture's own pixels; outside the picture nothing
    int x = -1, y = -1;
    RectF drawn(100, 400, 300, 200); // left, top, right, bottom: 200 wide, 200 tall, y up
    EXPECT_TRUE(PipelineImageToUi::PictureClick(drawn, Vec2I(240, 320), Vec2F(200, 300), x, y));
    EXPECT_EQ(x, 120);
    EXPECT_EQ(y, 160);
    EXPECT_TRUE(PipelineImageToUi::PictureClick(drawn, Vec2I(240, 320), Vec2F(100, 400), x, y));
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
    EXPECT_FALSE(PipelineImageToUi::PictureClick(drawn, Vec2I(240, 320), Vec2F(300, 300), x, y)) << "the right edge is outside";
    EXPECT_FALSE(PipelineImageToUi::PictureClick(drawn, Vec2I(240, 320), Vec2F(50, 300), x, y));
    EXPECT_FALSE(PipelineImageToUi::PictureClick(drawn, Vec2I(0, 0), Vec2F(200, 300), x, y));

    // Pins: the draft's numbered by their place in the list, the looked-at round's grey
    Vector<PipelineImageToUiEditItem> draft = { Note("bigger", "design", 10, 20), Note("everything"), Note("", "result", 5, 6) };
    Vector<PipelineImageToUiEditRound> rounds = { { "r1", { Note("a"), Note("b", "result", 1, 2) } } };
    auto pins = PipelineImageToUi::EditPins(draft, rounds, -1);
    ASSERT_EQ(pins.Count(), 2);
    EXPECT_EQ(pins[0].n, 1);
    EXPECT_EQ(pins[0].on, "design");
    EXPECT_EQ(pins[1].n, 3);
    EXPECT_FALSE(pins[1].past);
    pins = PipelineImageToUi::EditPins(draft, rounds, 0);
    ASSERT_EQ(pins.Count(), 3);
    EXPECT_TRUE(pins[2].past);
    EXPECT_EQ(pins[2].n, 2);
    EXPECT_EQ(PipelineImageToUi::EditPlace(draft[0]), "Source \xC2\xB7 10, 20");
    EXPECT_EQ(PipelineImageToUi::EditPlace(draft[1]), "whole screen");
    EXPECT_EQ(PipelineImageToUi::EditPlace(draft[2]), "Result \xC2\xB7 5, 6");

    // Apply needs a result, no run going on and a note with text
    EXPECT_TRUE(PipelineImageToUi::CanApplyEdits(false, true, draft));
    EXPECT_FALSE(PipelineImageToUi::CanApplyEdits(true, true, draft));
    EXPECT_FALSE(PipelineImageToUi::CanApplyEdits(false, false, draft));
    EXPECT_FALSE(PipelineImageToUi::CanApplyEdits(false, true, { Note("  ", "result", 1, 1) }));

    // Apply appends the round of the notes with text, empties the draft and closes the dialog; undo drops the last round
    auto node = PipelineNodeRegistry::CreateNode("imageToUi", Vec2F());
    ASSERT_TRUE(node);
    PipelineImageToUi::WriteDraft(*node, draft);
    node->SetConfigBool("editsView", true);
    EXPECT_EQ(PipelineImageToUi::NewEditId(1761600000000), "emh9n5n9c") << "Date.now().toString(36)";
    ASSERT_TRUE(PipelineImageToUi::ApplyEditRound(*node, PipelineImageToUi::ReadDraft(*node), "e1"));
    auto applied = PipelineImageToUi::Edits(*node);
    ASSERT_EQ(applied.Count(), 1);
    EXPECT_EQ(applied[0].id, "e1");
    EXPECT_EQ(applied[0].items.Count(), 2) << "the empty note is not sent";
    EXPECT_TRUE(PipelineImageToUi::ReadDraft(*node).IsEmpty());
    EXPECT_FALSE(node->GetConfigBool("editsView", true));
    ASSERT_TRUE(PipelineImageToUi::ApplyEditRound(*node, { Note("second") }, "e2"));
    EXPECT_FALSE(PipelineImageToUi::ApplyEditRound(*node, { Note("") }, "e3"));
    EXPECT_EQ(PipelineImageToUi::Edits(*node).Count(), 2);
    ASSERT_TRUE(PipelineImageToUi::UndoEditRound(*node));
    ASSERT_EQ(PipelineImageToUi::Edits(*node).Count(), 1);
    EXPECT_EQ(PipelineImageToUi::Edits(*node)[0].id, "e1");
    ASSERT_TRUE(PipelineImageToUi::UndoEditRound(*node));
    EXPECT_FALSE(PipelineImageToUi::UndoEditRound(*node));
    EXPECT_EQ(PipelineImageToUi::ConfigJson(*node, "edits").Dump(), "[]") << "the rounds are kept as an empty list, as the web stores them";
}

// The debug view and the card read the trace of the state the edits name (cache.ts imageToUiStateTrace): the kept one when
// rounds.json's rounds start with exactly those ids, else the live one
TEST(ImageToUiEdits, TheTraceOfTheCurrentStateIsTheKeptOne)
{
    String work = AbsolutePath(".") + "/image-to-ui-state-" + (String)(int)(Math::Random()%1000000) + "/";
    o2FileSystem.FolderCreate(work + "trace", true);
    o2FileSystem.FolderCreate(work + "rounds/0", true);
    o2FileSystem.FolderCreate(work + "rounds/1", true);
    PipelineUtils::WriteFileBytes(work + "trace/trace.json", "{}");
    PipelineUtils::WriteFileBytes(work + "rounds/0/trace.json", "{}");
    PipelineUtils::WriteFileBytes(work + "rounds/1/trace.json", "{}");
    auto rounds = [](const Vector<String>& ids)
    {
        Vector<PipelineImageToUiEditRound> out;
        for (auto& id : ids)
            out.Add({ id, { Note("x") } });
        return out;
    };
    String live = work + "trace/trace.json";

    EXPECT_EQ(PipelineImageToUi::StateTracePath(work, rounds({})), live) << "no rounds.json: the live trace";
    PipelineUtils::WriteFileBytes(work + "rounds.json", "{\"base\": \"k\", \"rounds\": [{\"id\": \"r1\"}, {\"id\": \"r2\"}]}");
    EXPECT_EQ(PipelineImageToUi::StateTracePath(work, rounds({})), work + "rounds/0/trace.json");
    EXPECT_EQ(PipelineImageToUi::StateTracePath(work, rounds({ "r1" })), work + "rounds/1/trace.json");
    EXPECT_EQ(PipelineImageToUi::StateTracePath(work, rounds({ "r1", "r2" })), live) << "state 2 is not kept";
    EXPECT_EQ(PipelineImageToUi::StateTracePath(work, rounds({ "r2" })), live) << "the ids must be a prefix";
    EXPECT_EQ(PipelineImageToUi::StateTracePath(work, rounds({ "r1", "r2", "r3" })), live);
    EXPECT_EQ(PipelineImageToUi::StateTracePath(work, rounds({ "a/b" })), live) << "an id that is not a safe name";
    EXPECT_EQ(PipelineImageToUi::StateTracePath(work.SubStr(0, work.Length() - 1), rounds({ "r1" })), work + "rounds/1/trace.json");
    o2FileSystem.FolderRemove(work, true);
}

TEST(ImageToUiEdits, ARoundRunsTheAgentWithItsRequest)
{
    AgentRun run;
    run.design = "/w/input/design.png";
    run.assets = "/w/sprites";
    run.task = "/w/task";
    run.scratch = "/w/agent-view";
    run.traceDir = "/w/trace";
    run.maxTurns = 12;
    run.name = "Test";
    run.revise = "/w/edits/round-1.json";
    auto args = AgentArguments("/home", run);
    ASSERT_GE(args.Count(), 2);
    EXPECT_EQ(args[args.Count() - 2], "--revise");
    EXPECT_EQ(args.Last(), "/w/edits/round-1.json");
    EXPECT_EQ(args[args.Count() - 4], "--trace-dir");
    int turns = args.IndexOf("--max-turns");
    ASSERT_GE(turns, 0);
    EXPECT_EQ(args[turns + 1], "12");

    run.revise = "";
    EXPECT_FALSE(AgentArguments("/home", run).Contains("--revise")) << "a run without edits";
}
