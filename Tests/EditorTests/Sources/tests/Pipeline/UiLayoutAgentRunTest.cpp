#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <mutex>
#include <sys/stat.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgentRun.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

using namespace o2;
using namespace Editor;
namespace L = Editor::UiLayout;
namespace D = Editor::UiDoc;

// The layout agent's run (backend/test uiMerged.test.ts / uiAgentParts.test.ts, stubbed models): an edit is one turn and a
// second one only when the check finds something, an empty layout starts on its device's screen, a reference is marked up in
// terms of the kit and then compared and fixed in turns, graphics the kit lacks are drawn, and the Image-To-UI skill gets the
// draft as its first ui_structure.json

namespace
{
    struct WorkDir
    {
        String path;

        WorkDir()
        {
            String relative = "./ui-agent-run-work-" + (String)(int)(Math::Random()%1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
            PipelineUtils::SetWorkPathOverride(path);
            o2FileSystem.FolderCreate(PipelineUtils::GetUploadsPath(), true);
        }

        ~WorkDir()
        {
            PipelineUtils::SetWorkPathOverride("");
            o2FileSystem.FolderRemove(path, true);
        }
    };

    // The text model: every prompt kept, the answer chosen by the test
    struct TextModel
    {
        std::mutex                                   lock;
        Vector<String>                               prompts;
        Vector<int>                                  images;
        std::function<String(const String&, int)>    answer; // prompt, call index (1-based)

        TextModel()
        {
            D::SetUiAiTextStub([this](const String&, const String& prompt, const Vector<AiImageRef>& refs)
            {
                std::lock_guard<std::mutex> guard(lock);
                prompts.Add(prompt);
                images.Add(refs.Count());
                AiTextResult r;
                r.ok = true;
                r.text = answer ? answer(prompt, prompts.Count()) : String();
                return r;
            });
        }

        ~TextModel() { D::SetUiAiTextStub({}); }
    };

    struct ImageModel
    {
        int calls = 0;

        ImageModel()
        {
            AiRouter::SetImageStub([this](const String&, const String&, const Vector<AiImageRef>&, int, bool, const AiImageOptions&)
            {
                calls++;
                // A red badge on the chroma key: the key colour goes, the badge stays
                L::UiImage img = L::UiImage::Blank(200, 200, 0, 255, 0, 255);
                for (int y = 40; y < 160; y++)
                {
                    for (int x = 40; x < 160; x++)
                    {
                        UInt8* p = img.At(x, y);
                        p[0] = 220; p[1] = 30; p[2] = 40;
                    }
                }
                AiBytesResult r;
                r.ok = true;
                r.data = img.Png();
                r.mimeType = "image/png";
                return r;
            });
        }

        ~ImageModel() { AiRouter::SetImageStub({}); }
    };

    bool Has(const String& text, const String& part) { return text.Contains(part); }

#define EXPECT_STR_EQ(a, b) EXPECT_EQ(std::string(a), std::string(b))

    template<typename T>
    T RunToEnd(Coroutine<T> co)
    {
        co.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return co.IsDone(); }, 60.0f));
        return co.GetResult();
    }

    String KitPng(int w, int h, UInt8 r, UInt8 g, UInt8 b)
    {
        return PipelineUtils::Base64Encode(L::UiImage::Blank(w, h, r, g, b, 255).Png());
    }

    // A kit: a sliced panel, a button and an icon, with a device
    D::UiKit TestKit()
    {
        D::UiKit kit = D::ParseKit("{\"version\":1,\"name\":\"k\",\"device\":{\"w\":1000,\"h\":1600,\"dpi\":300},\"elements\":["
            "{\"name\":\"panel_wood\",\"type\":\"panel\",\"assetType\":\"sliced\",\"width\":120,\"height\":120,\"dpi\":300,\"nineSlice\":{\"left\":20,\"top\":20,\"right\":20,\"bottom\":20},\"png\":\"" + KitPng(120, 120, 140, 90, 40) + "\"},"
            "{\"name\":\"btn_green\",\"type\":\"button\",\"assetType\":\"sliced\",\"width\":200,\"height\":80,\"dpi\":300,\"png\":\"" + KitPng(200, 80, 40, 180, 60) + "\"},"
            "{\"name\":\"icon_star\",\"type\":\"icon\",\"assetType\":\"simple\",\"width\":64,\"height\":64,\"dpi\":300,\"png\":\"" + KitPng(64, 64, 250, 210, 40) + "\"}],"
            "\"textStyles\":[{\"name\":\"title\",\"role\":\"title\",\"font\":\"Roboto\",\"weight\":700,\"size\":60,\"color\":\"#FFFFFF\"}]}");
        return kit;
    }

    L::LayoutAgentInput RunInput(const WorkDir& work, const String& uiml, const String& prompt)
    {
        L::LayoutAgentInput a;
        a.ai = mmake<L::AiCtx>();
        a.ai->exec = mmake<PipelineExecContext>();
        a.ai->textModel = "gpt-6.1-sol";
        a.ai->imageModel = "gpt-image-test";
        a.runId = "srun" + (String)(int)(Math::Random()%100000);
        a.doc = uiml.IsEmpty() ? D::EmptyDoc() : D::ParseUiml(uiml).doc;
        a.prompt = prompt;
        a.workDir = work.path + "node/";
        a.log = mmake<L::RunLog>(L::RunLog::DirOf(a.workDir, a.runId));
        a.log->Open();
        a.nowMs = 1700000000000LL;
        return a;
    }

    Vector<String> ActIds(const D::UiStep& step)
    {
        Vector<String> ids;
        const D::Json* run = step.data.Get("run");
        if (run && run->Get("acts"))
        {
            for (auto& a : run->Get("acts")->Items())
                ids.Add(a.Get("id")->Str());
        }
        return ids;
    }

    const char* const MENU = "@screen Menu 1000x1600 dpi=300\npanel board anchor=center w=800 h=1000 kit=panel_wood\n"
                             "  button play anchor=bottom-center y=60 w=400 h=140 kit=btn_green \"PLAY\"";
}

TEST(UiLayoutAgentRun, AnEditIsOneTurnAndOneStep)
{
    WorkDir work;
    TextModel tm;
    tm.answer = [](const String&, int) { return String("Bigger now.\n```uiml-edit\n@update board w=900\n@add board\n  icon star kit=icon_star w=80 h=80\n```"); };
    auto a = RunInput(work, MENU, "make the board wider");
    a.kit = TestKit();
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    ASSERT_TRUE(r.ok) << r.error.Data();
    EXPECT_EQ(tm.prompts.Count(), 1);
    EXPECT_EQ(tm.images[0], 2); // the screen and the kit sheet
    EXPECT_TRUE(Has(tm.prompts[0], "make the board wider"));
    EXPECT_STR_EQ(r.step.Id(), a.runId);
    EXPECT_STR_EQ(*r.step.Reply(), "Bigger now.");
    ASSERT_EQ(r.step.Ops().Count(), 2);
    EXPECT_STR_EQ(D::OpKind(r.step.Ops()[0]), "update");
    EXPECT_EQ(ActIds(r.step), Vector<String>({ "turn-1" }));
    const D::Json& run = *r.step.data.Get("run");
    EXPECT_EQ(run.Get("turns")->Num(), 1);
    EXPECT_FALSE(run.Has("built"));
    EXPECT_STR_EQ(run.Get("acts")->At(0)->Get("status")->Str(), "ok");
    // The picture the turn looked at is kept in the run's folder
    const D::Json* images = run.Get("acts")->At(0)->Get("images");
    ASSERT_TRUE(images && images->Count() == 1);
    EXPECT_TRUE(o2FileSystem.IsFileExist(a.log->GetDir() + "/" + images->At(0)->Get("file")->Str()));
    D::UiDoc doc = D::ApplyOps(a.doc, r.step.Ops()).doc;
    EXPECT_EQ(*D::FindNode(doc, "board")->w, 900);
}

TEST(UiLayoutAgentRun, TheCheckOfAnEditGivesTheAgentASecondTurn)
{
    WorkDir work;
    TextModel tm;
    tm.answer = [](const String&, int call)
    {
        if (call == 1)
            return String("Added.\n```uiml-edit\n@add board\n  text star w=80 h=80 \"\xE2\x98\x85\"\n```");
        return String("The star is an icon now.\n```uiml-edit\n@replace star\n  icon star kit=icon_star w=80 h=80\n```\n@done");
    };
    auto a = RunInput(work, MENU, "add a star");
    a.kit = TestKit();
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    ASSERT_TRUE(r.ok) << r.error.Data();
    ASSERT_EQ(tm.prompts.Count(), 2);
    EXPECT_TRUE(Has(tm.prompts[1], "Blocks that draw an icon with characters: star")) << tm.prompts[1].Data();
    EXPECT_TRUE(Has(tm.prompts[1], "Fix only what this check found"));
    EXPECT_EQ(ActIds(r.step), Vector<String>({ "turn-1", "turn-2" }));
    EXPECT_STR_EQ(*r.step.Reply(), "The star is an icon now.");
    EXPECT_EQ(r.step.Ops().Count(), 2);

    // Without the check: one turn
    TextModel again;
    again.answer = tm.answer;
    auto off = RunInput(work, MENU, "add a star");
    off.kit = TestKit();
    off.review = false;
    RunToEnd(L::RunLayoutAgent(off));
    EXPECT_EQ(again.prompts.Count(), 1);
}

TEST(UiLayoutAgentRun, AnEmptyLayoutStartsOnTheKitsDevice)
{
    WorkDir work;
    TextModel tm;
    tm.answer = [](const String&, int) { return String("A menu.\n```uiml-edit\n@add root\n  panel board anchor=center w=600 h=800 kit=panel_wood\n```"); };
    auto a = RunInput(work, "", "a main menu");
    a.kit = TestKit();
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    ASSERT_TRUE(r.ok) << r.error.Data();
    ASSERT_EQ(r.step.Ops().Count(), 2);
    EXPECT_STR_EQ(D::OpKind(r.step.Ops()[0]), "screen");
    EXPECT_EQ(r.step.Ops()[0].Get("width")->Num(), 1000);
    EXPECT_EQ(r.step.Ops()[0].Get("dpi")->Num(), 300);
    // A described screen with a kit connected is laid out from the kit alone: no @asset in the rules
    EXPECT_FALSE(Has(tm.prompts[0], "@asset <name>"));

    // Nothing made: the screen alone is no step's edit
    TextModel quiet;
    quiet.answer = [](const String&, int) { return String("I need more detail."); };
    L::LayoutAgentResult none = RunToEnd(L::RunLayoutAgent(RunInput(work, "", "hm")));
    ASSERT_TRUE(none.ok);
    EXPECT_EQ(none.step.Ops().Count(), 0);
    EXPECT_STR_EQ(*none.step.Reply(), "I need more detail.");
}

TEST(UiLayoutAgentRun, GraphicsTheAgentAsksForAreDrawnIntoTheStep)
{
    WorkDir work;
    TextModel tm;
    ImageModel im;
    tm.answer = [](const String&, int) {
        return String("A badge.\n```uiml-edit\n@asset star_badge type=icon asset=simple size=120x120 \"a red star badge\"\n"
                      "@add root\n  icon badge kit=star_badge anchor=center w=120 h=120\n```");
    };
    auto a = RunInput(work, "", "a badge in the middle");
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    ASSERT_TRUE(r.ok) << r.error.Data();
    EXPECT_EQ(im.calls, 1);
    const D::Json* assets = r.step.data.Get("assets");
    ASSERT_TRUE(assets && assets->Count() == 1) << r.step.data.Dump().Data();
    EXPECT_STR_EQ(assets->At(0)->Get("name")->Str(), "star_badge");
    EXPECT_STR_EQ(assets->At(0)->Get("from")->Str(), "drawn");
    EXPECT_TRUE(o2FileSystem.IsFileExist(PipelineUtils::GetUploadPath(assets->At(0)->Get("uploadId")->Str())));
    EXPECT_EQ(r.step.data.Get("run")->Get("made")->Num(), 1);

    // The same request in the same run is drawn once: the memo keeps what was drawn
    Map<String, String> memo;
    auto b = RunInput(work, "", "a badge in the middle");
    b.runId = "srepeat";
    b.ai->memo.get = [&](const String& key, const String&, String& data) { return memo.TryGetValue(key, data); };
    b.ai->memo.put = [&](const String& key, const String&, const String& data) { memo[key] = data; };
    RunToEnd(L::RunLayoutAgent(b));
    b.log = mmake<L::RunLog>(L::RunLog::DirOf(b.workDir, b.runId));
    RunToEnd(L::RunLayoutAgent(b));
    EXPECT_EQ(im.calls, 2);
}

TEST(UiLayoutAgentRun, AGraphicThatPointsIntoTheReferenceIsCutOutOfIt)
{
    WorkDir work;
    TextModel tm;
    ImageModel im;
    tm.answer = [](const String&, int) {
        return String("Cut it.\n```uiml-edit\n@asset gem type=icon asset=simple size=120x120 box=100,100,200,200 purpose=\"reward\" \"the gem\"\n"
                      "@add board\n  icon gem kit=gem w=120 h=120\n```");
    };
    auto a = RunInput(work, MENU, "take the gem from the picture");
    L::LayoutAgentRef ref;
    ref.png = L::UiImage::Blank(1000, 1600, 32, 48, 64, 255).Png();
    ref.w = 1000;
    ref.h = 1600;
    ref.dpi = 300;
    a.refs.Add(ref);
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    ASSERT_TRUE(r.ok) << r.error.Data();
    EXPECT_EQ(im.calls, 1);
    const D::Json* assets = r.step.data.Get("assets");
    ASSERT_TRUE(assets && assets->Count() == 1) << r.step.data.Dump().Data();
    EXPECT_STR_EQ(assets->At(0)->Get("from")->Str(), "cut");
    EXPECT_STR_EQ(assets->At(0)->Get("purpose")->Str(), "reward");
    // Cut at the reference's density: never denser than the layout
    EXPECT_LE(assets->At(0)->Get("dpi")->Num(), 300);
}

TEST(UiLayoutAgentRun, AReferenceIsMarkedUpInTermsOfTheKitThenComparedInTurns)
{
    WorkDir work;
    TextModel tm;
    tm.answer = [](const String& prompt, int)
    {
        if (Has(prompt, "## The project's UI kit"))
            return String("```json\n{\"background\": {\"kind\": \"color\", \"color\": \"#203040\"}, \"looks\": {}, \"elements\": ["
                          "{\"id\": \"board\", \"parent\": \"\", \"kind\": \"panel\", \"kit\": \"panel_wood\", \"box\": [60, 120, 540, 780]},"
                          "{\"id\": \"title\", \"parent\": \"board\", \"kind\": \"text\", \"text\": \"VICTORY\", \"color\": \"#FFFFFF\", \"weight\": \"bold\", \"box\": [180, 156, 420, 204]},"
                          "{\"id\": \"ok\", \"parent\": \"board\", \"kind\": \"button\", \"kit\": \"btn_green\", \"box\": [180, 660, 420, 744]},"
                          "{\"id\": \"dim\", \"parent\": \"\", \"kind\": \"panel\", \"color\": \"#000000\", \"opacity\": 0.4, \"box\": [0, 840, 600, 960]}]}\n```");
        if (Has(prompt, "Describe the styles its lettering needs"))
            return String("{\"styles\": [{\"name\": \"header\", \"role\": \"title\", \"font\": \"Roboto\", \"weight\": 700, \"size\": 70, \"color\": \"#FFFFFF\"}], \"labels\": {\"1\": \"header\"}}");
        return String("It matches.\n@done");
    };
    auto a = RunInput(work, "", "");
    a.prompt = "Build this screen from the reference picture.";
    a.kit = TestKit();
    a.skill = false;
    L::LayoutAgentRef ref;
    ref.name = "reference";
    // Small enough for the model to see it as it is (no analysis frame scaling): the boxes are its pixels
    ref.png = L::UiImage::Blank(600, 960, 32, 48, 64, 255).Png();
    ref.w = 600;
    ref.h = 960;
    ref.dpi = 300;
    a.refs.Add(ref);
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    ASSERT_TRUE(r.ok) << r.error.Data();

    EXPECT_EQ(ActIds(r.step), Vector<String>({ "analysis", "styles", "draft", "turn-1", "check" }));
    ASSERT_EQ(r.step.Ops().Count(), 1);
    EXPECT_STR_EQ(D::OpKind(r.step.Ops()[0]), "doc");
    D::UiDoc doc = D::ApplyOps(D::EmptyDoc(), r.step.Ops()).doc;
    EXPECT_EQ(doc.width, 600);
    EXPECT_EQ(*doc.dpi, 300);
    ASSERT_TRUE(D::FindNode(doc, "board"));
    EXPECT_STR_EQ(*D::FindNode(doc, "board")->kit, "panel_wood");
    EXPECT_STR_EQ(*D::FindNode(doc, "ok")->kit, "btn_green");
    EXPECT_STR_EQ(*D::FindNode(doc, "title")->style, "header");
    // A flat shape: no graphic, its colour
    auto dim = D::FindNode(doc, "dim");
    ASSERT_TRUE(dim && dim->kit) << D::DocToUiml(doc).Data();
    EXPECT_STR_EQ(*dim->kit, "none");
    const D::Json& run = *r.step.data.Get("run");
    EXPECT_TRUE(run.Get("built")->Bool());
    EXPECT_EQ(run.Get("reused")->Num(), 2);
    EXPECT_TRUE(run.Has("score"));
    // The turn saw the screen and the reference under one grid, and the work so far
    String turn = tm.prompts.Last();
    EXPECT_TRUE(Has(turn, "The reference was read into")) << turn.Data();
    const D::Json* styles = r.step.data.Get("styles");
    ASSERT_TRUE(styles && styles->Count() == 1);
    EXPECT_STR_EQ(styles->At(0)->Get("name")->Str(), "header");
}

#if !defined(_WIN32)
TEST(UiLayoutAgentRun, TheSkillLaysTheDraftOutFromItsFirstStructure)
{
    WorkDir work;
    TextModel tm;
    tm.answer = [](const String& prompt, int)
    {
        if (Has(prompt, "## The project's UI kit"))
            return String("{\"background\": {\"kind\": \"color\", \"color\": \"#203040\"}, \"looks\": {}, \"elements\": ["
                          "{\"id\": \"board\", \"parent\": \"\", \"kind\": \"panel\", \"kit\": \"panel_wood\", \"box\": [100, 200, 900, 1300]},"
                          "{\"id\": \"ok\", \"parent\": \"board\", \"kind\": \"button\", \"kit\": \"btn_green\", \"box\": [300, 1100, 700, 1240]}]}");
        return String("{}");
    };

    // A stand-in for the skill's Python: takes the draft (--seed) as its structure, writes a trace with a check and a turn,
    // and ends with a checked layout
    String fake = work.path + "fake-python.sh";
    PipelineUtils::WriteFileBytes(fake,
        "#!/bin/sh\n"
        "while [ $# -gt 0 ]; do case \"$1\" in --output) TASK=\"$2\"; shift;; --seed) SEED=\"$2\"; shift;; --trace-dir) TRACE=\"$2\"; shift;; esac; shift; done\n"
        "mkdir -p \"$TASK\" \"$TRACE\"\n"
        "cp \"$SEED\" \"$TASK/ui_structure.json\"\n"
        "echo '{\"check_count\": 2, \"target_count\": 3}' > \"$TASK/workflow_state.json\"\n"
        "echo '{\"steps\": [{\"id\": \"check\", \"title\": \"The first draft: passed\", \"kind\": \"check\", \"status\": \"ok\", \"text\": \"validation passed\"},"
        " {\"id\": \"turn-1\", \"title\": \"Turn 1: patch\", \"kind\": \"agent\", \"data\": {\"action\": {\"action\": \"patch\"}}, \"text\": \"moved ok\"}]}' > \"$TRACE/trace.json\"\n"
        "echo '@@bridge {\"type\": \"result\", \"ok\": true, \"summary\": {\"complete\": true, \"turns\": 7, \"check_count\": 2, \"structure_revision\": 3, \"target_count\": 3, \"blockers\": []}}'\n");
    chmod(fake.Data(), 0755);

    auto a = RunInput(work, "", "Build this screen from the reference picture.");
    a.kit = TestKit();
    a.python = fake;
    L::LayoutAgentRef ref;
    ref.png = L::UiImage::Blank(1000, 1600, 32, 48, 64, 255).Png();
    ref.w = 1000;
    ref.h = 1600;
    ref.dpi = 300;
    a.refs.Add(ref);
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    if (!r.ok && r.error.Contains("Python"))
        GTEST_SKIP() << "the Image-To-UI bundle is not found here: " << r.error.Data();
    ASSERT_TRUE(r.ok) << r.error.Data();

    // The kit went into the skill's sprite folder at the design's density, the draft became its first structure
    EXPECT_TRUE(o2FileSystem.IsFileExist(a.workDir + "sprites/panel_wood.png"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(a.workDir + "sprites/panel_wood.png.meta"));
    String seed = PipelineUtils::ReadFileBytes(a.workDir + "seed-ui_structure.json");
    EXPECT_TRUE(Has(seed, "First draft: a markup of the design in terms of the UI kit.")) << seed.Data();
    // The skill's steps are acts of the run, and the result is its structure turned back into the layout
    Vector<String> ids = ActIds(r.step);
    EXPECT_TRUE(ids.Contains("skill-check")) << r.step.data.Dump().Data();
    EXPECT_TRUE(ids.Contains("skill-turn-1"));
    EXPECT_FALSE(ids.Contains("turn-1")); // laid out by the skill: no plain turns
    const D::Json& run = *r.step.data.Get("run");
    ASSERT_TRUE(run.Has("skill"));
    EXPECT_TRUE(run.Get("skill")->Get("complete")->Bool());
    EXPECT_EQ(run.Get("skill")->Get("checks")->Num(), 2);
    EXPECT_EQ(run.Get("turns")->Num(), 7);
    EXPECT_TRUE(Has(*r.step.Reply(), "Rebuilt from the reference with the UI kit: 2 block(s) drawn by kit graphics"));
    D::UiDoc doc = D::ApplyOps(D::EmptyDoc(), r.step.Ops()).doc;
    EXPECT_GE(D::CountNodes(doc), 2);
}
#endif

TEST(UiLayoutAgentRun, AStopKeepsWhatWasMade)
{
    WorkDir work;
    TextModel tm;
    bool stop = false;
    tm.answer = [&](const String&, int)
    {
        stop = true;
        return String("Half done.\n```uiml-edit\n@update board w=900\n```");
    };
    auto a = RunInput(work, MENU, "change things");
    a.ai->exec->isCancelled = [&]() { return stop; };
    L::LayoutAgentResult r = RunToEnd(L::RunLayoutAgent(a));
    // Stopped right after the answer, before anything was applied: nothing was made, the run fails as cancelled
    EXPECT_FALSE(r.ok);
    EXPECT_STR_EQ(r.error, "Cancelled");
}
