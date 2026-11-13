#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineExtractCheck.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleAi.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleCheck.h"

using namespace o2;
using namespace Editor;

// The AI text style's check against its picture (AssetsLine backend/test/textStyleFx.test.ts, the check part) and the
// executor's second step it runs as: the sample drawn on the picture's own backdrop, the judge's corrections (never the size,
// the name or the role), the closest attempt kept, nothing asked twice, a cached-only run never paying for it

namespace
{
    using UiLayout::Json;

    const char* model = "gemini-3.1-pro-preview";

    struct WorkDir
    {
        String path;

        WorkDir()
        {
            String relative = "./pipeline-style-check-work-" + (String)(int)(Math::Random()%1000000);
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

    // A 540x400 title picture: a dark blue screen with a yellow band where the lettering is
    String TitlePicture()
    {
        auto bmp = PipelineImageOps::Blank(540, 400, Color4(0x20, 0x30, 0x50, 255));
        for (int y = 110; y < 190; y++)
        {
            for (int x = 120; x < 420; x++)
            {
                UInt8* p = PipelineImageOps::Pixel(*bmp, x, y);
                p[0] = 0xFF; p[1] = 0xE4; p[2] = 0x5C; p[3] = 255;
            }
        }
        return EncodeBitmapPng(*bmp);
    }

    String Read(const String& text = "QUEST")
    {
        return "{\"text\":\"" + text + "\",\"name\":\"title_gold\",\"role\":\"title\",\"font\":\"Russo One\",\"weight\":400,\"capHeight\":71,"
            "\"screenWidth\":540,\"color\":\"#FFE45C\",\"fill\":null,\"effects\":[{\"type\":\"stroke\",\"color\":\"#3A1800\",\"width\":1}],"
            "\"uppercase\":true,\"description\":\"a golden title\"}";
    }

    bool IsJudge(const String& prompt) { return prompt.StartsWith("You are checking a text style"); }

    // The vision model: the reading and the judgements, counted
    struct Stub
    {
        Vector<String>      prompts;
        Vector<Vector<int>> pictureWidths;
        Function<String(const String&)> answer;

        Stub()
        {
            UiDoc::SetUiAiTextStub([this](const String& m, const String& prompt, const Vector<AiImageRef>& pictures)
            {
                EXPECT_EQ(m, model);
                prompts.Add(prompt);
                Vector<int> widths;
                for (auto& picture : pictures)
                {
                    UiLayout::UiImage img;
                    UiLayout::UiImage::Decode(picture.data, img);
                    widths.Add(img.w);
                }
                pictureWidths.Add(widths);
                AiTextResult r;
                r.ok = true;
                r.text = answer(prompt);
                return r;
            });
        }

        ~Stub() { UiDoc::SetUiAiTextStub({}); }
    };

    struct Rig
    {
        PipelineGraph     graph;
        Ref<PipelineNode> source, node;
        String            pipelineId = "stylecheck";

        Rig()
        {
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("title.png"), TitlePicture());
            source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
            source->SetConfigString("uploadId", "title.png");
            node = PipelineNodeRegistry::CreateNode("textStyleExtract", Vec2F());
            node->SetConfigString("model", model);
            node->SetConfigString("refCheckModel", model);
            graph.nodes = { source, node };
            auto edge = mmake<PipelineEdge>();
            edge->id = "e";
            edge->fromNodeId = source->id;
            edge->fromPortId = source->outputs[0].id;
            edge->toNodeId = node->id;
            edge->toPortId = node->inputs[0].id;
            graph.edges.Add(edge);
        }

        // Runs the node; the events of the run
        Vector<PipelineExecEvent> Run(bool cachedOnly = false, bool restart = false)
        {
            Vector<PipelineExecEvent> events;
            bool done = false;
            String fatal;
            auto executor = mmake<PipelineExecutor>();
            executor->onEvent = [&](const PipelineExecEvent& e)
            {
                events.Add(e);
                if (e.type == PipelineExecEvent::Type::Done) done = true;
                if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
            };
            executor->Execute(pipelineId, graph, node->id, restart ? Vector<String>{ node->id } : Vector<String>{}, cachedOnly);
            EXPECT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 60.0f));
            EXPECT_TRUE(fatal.IsEmpty()) << fatal.Data();
            for (auto& e : events)
                EXPECT_FALSE(e.type == PipelineExecEvent::Type::NodeState && e.state == "error") << e.error.Data();
            return events;
        }

        Json Style() const
        {
            Json style;
            EXPECT_TRUE(Json::Parse(PipelineExecutor::LoadPreview(pipelineId, *node).data, style));
            return style;
        }

        String Folder() const { return PipelineExtractCheck::CheckFolder(PipelineExecutor::GetWorkPath(pipelineId, node->id)); }

        Json Debug() const
        {
            Json d;
            EXPECT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(Folder() + "style.json"), d));
            return d;
        }
    };

    String EffectsOf(const Json& style)
    {
        Vector<String> out;
        for (auto& e : style.Get("effects")->Items())
            out.Add(e.Get("type")->Str() + ":" + UiDoc::JsNum(e.Get("width") ? e.Get("width")->Num() : e.Get("y")->Num()));
        return UiDoc::Join(out, ",");
    }

    bool Logged(const Vector<PipelineExecEvent>& events, const char* text)
    {
        return events.Any([&](const PipelineExecEvent& e) { return e.type == PipelineExecEvent::Type::Log && e.message.Contains(text); });
    }
}

TEST(PipelineStyleCheck, ASignificantDifferenceCorrectsTheStyleAndComparesAgainNothingAskedTwice)
{
    WorkDir work;
    Stub stub;
    int judged = 0;
    stub.answer = [&](const String& prompt)
    {
        if (!IsJudge(prompt))
            return Read();
        judged++;
        return judged == 1 ?
            String("{\"verdict\":\"significant\",\"likeness\":0.55,\"differences\":[\"the outline is much thinner than in the original\",\"no shadow under the letters\"],"
                   "\"style\":{\"size\":999,\"name\":\"hacked\",\"effects\":[{\"type\":\"stroke\",\"color\":\"#3A1800\",\"width\":12},"
                   "{\"type\":\"shadow\",\"color\":\"#000000\",\"x\":0,\"y\":8,\"blur\":6,\"opacity\":0.5}]}}") :
            String("{\"verdict\":\"ok\",\"likeness\":0.93,\"differences\":[]}");
    };
    Rig rig;
    rig.node->SetConfigString("hint", "the title");
    rig.node->SetConfigBool("refCheck", true);

    auto events = rig.Run();
    ASSERT_EQ(stub.prompts.Count(), 3) << "the reading and two judgements";
    EXPECT_EQ(stub.pictureWidths[1].Count(), 2) << "the judge gets the picture and the sample";
    EXPECT_EQ(stub.pictureWidths[1][0], 540);
    EXPECT_TRUE(stub.prompts[1].Contains("The lettering that was read: \"the title\"") && stub.prompts[1].Contains("the words \"QUEST\""));
    EXPECT_TRUE(stub.prompts[1].Contains("\"effects\":[{\"type\":\"stroke\",\"color\":\"#3A1800\",\"width\":2,\"opacity\":1}]"))
        << "the style it drew, at the style's scale";
    EXPECT_TRUE(stub.prompts[2].Contains("\"width\":12")) << "the second judgement is of the corrected style";
    EXPECT_TRUE(stub.prompts[2].Contains("- attempt 1: Russo One 400 \xE2\x80\x94 significant, likeness 0.55: the outline is much thinner"))
        << "the judge is told the earlier attempts";

    Json style = rig.Style();
    EXPECT_EQ(EffectsOf(style), "stroke:12,shadow:8");
    EXPECT_EQ(style.Get("name")->Str(), "title_gold");
    EXPECT_EQ(style.Get("role")->Str(), "title");
    EXPECT_EQ(style.Get("size")->Num(), 200) << "the size is not the judge's to change";
    EXPECT_EQ(style.Get("extracted")->Get("text")->Str(), "QUEST");
    EXPECT_EQ(style.Get("stroke")->Dump(), "{\"color\":\"#3A1800\",\"width\":12}");
    EXPECT_TRUE(events.Any([&](const PipelineExecEvent& e) {
        return e.type == PipelineExecEvent::Type::NodeState && e.nodeId == rig.node->id && e.state == "running" && e.phase == "check";
    })) << "the editor is told the node is checking";

    Json d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "ok");
    EXPECT_EQ(d.Get("kept")->Num(), 2);
    EXPECT_EQ(d.Get("limit")->Num(), 2);
    EXPECT_EQ(d.Get("text")->Str(), "QUEST");
    EXPECT_EQ(d.Get("calls")->Get("judge")->Num(), 2);
    ASSERT_EQ(d.Get("attempts")->Count(), 2);
    auto& a1 = *d.Get("attempts")->At(0);
    auto& a2 = *d.Get("attempts")->At(1);
    EXPECT_EQ(a1.Get("verdict")->Str(), "significant");
    EXPECT_EQ(a1.Get("likeness")->Num(), 0.55);
    EXPECT_EQ(a1.Get("differences")->Count(), 2);
    EXPECT_EQ(a2.Get("verdict")->Str(), "ok");
    EXPECT_EQ(a2.Get("likeness")->Num(), 0.93);
    EXPECT_EQ(UiDoc::JsonToStrings(a1.Get("changes")), Vector<String>({ "\xE2\x88\x92 stroke 2px #3A1800", "+ stroke 12px #3A1800",
                                                                        "+ shadow 0,8 blur 6 #000000 50%" }));
    for (auto file : { d.Get("reference")->Str(), a1.Get("render")->Str(), a2.Get("render")->Str() })
    {
        auto picture = DecodeImageBytes(PipelineUtils::ReadFileBytes(rig.Folder() + file));
        ASSERT_TRUE(picture) << file.Data();
        EXPECT_GT(picture->GetSize().x, 100);
    }

    // Again: everything is stored
    rig.Run();
    EXPECT_EQ(stub.prompts.Count(), 3);
    // The check switched off: the style as it was read, with no new call
    rig.node->SetConfigBool("refCheck", false);
    rig.Run();
    EXPECT_EQ(stub.prompts.Count(), 3);
    EXPECT_EQ(rig.Style().Get("effects")->Dump(), "[{\"type\":\"stroke\",\"color\":\"#3A1800\",\"width\":2,\"opacity\":1}]");
    // A higher limit is another check, but the same judgements are remembered
    rig.node->SetConfigBool("refCheck", true);
    rig.node->SetConfigNumber("refCheckMax", 3);
    rig.Run();
    EXPECT_EQ(stub.prompts.Count(), 3) << "replayed from the memo";
    EXPECT_EQ(EffectsOf(rig.Style()), "stroke:12,shadow:8");
}

TEST(PipelineStyleCheck, TheCheckStopsAtItsLimitAndKeepsTheClosestAttempt)
{
    WorkDir work;
    Stub stub;
    int judged = 0;
    double likeness[] = { 0.4, 0.7, 0.6 };
    stub.answer = [&](const String& prompt)
    {
        if (!IsJudge(prompt))
            return Read("LIMIT");
        judged++;
        return String("{\"verdict\":\"significant\",\"likeness\":") + UiDoc::JsNum(judged <= 3 ? likeness[judged - 1] : 0.5) +
            ",\"differences\":[\"still off\"],\"style\":{\"effects\":[{\"type\":\"stroke\",\"color\":\"#3A1800\",\"width\":" +
            (String)(4 + judged*2) + "}]}}";
    };
    Rig rig;
    rig.node->SetConfigString("hint", "limit");
    rig.node->SetConfigBool("refCheck", true);
    rig.node->SetConfigNumber("refCheckMax", 2);
    rig.Run();
    EXPECT_EQ(stub.prompts.Count(), 4) << "the reading, the first judgement and two corrections";
    Json d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "limit");
    EXPECT_EQ(d.Get("kept")->Num(), 2);
    EXPECT_EQ(d.Get("attempts")->Count(), 3);
    EXPECT_EQ(rig.Style().Get("effects")->Dump(), "[{\"type\":\"stroke\",\"color\":\"#3A1800\",\"width\":6,\"opacity\":1}]")
        << "attempt 2 - the first correction - was the closest";
}

TEST(PipelineStyleCheck, TheSampleIsDrawnOnThePicturesOwnBorderColour)
{
    EXPECT_EQ(UiDoc::StyleBackdropOf(TitlePicture()), "#203050");

    UiDoc::TextStyle style = UiDoc::NormalizeTextStyle(Json::Object(), "title");
    style.size = 120;
    auto sample = UiDoc::RenderStyleSample(style, "A long title that needs a wide screen to stay on one line", "#203050");
    EXPECT_LE(sample.w, 1024);
    EXPECT_GT(sample.w, 500);
    const UInt8* corner = sample.At(1, 1);
    EXPECT_EQ((int)corner[0], 0x20);
    EXPECT_EQ((int)corner[1], 0x30);
    EXPECT_EQ((int)corner[2], 0x50);
}

TEST(PipelineStyleCheck, TheJudgesFixNeverTouchesTheSizeNameOrRoleAndChangesAreListed)
{
    Json raw;
    ASSERT_TRUE(Json::Parse(Read(), raw));
    UiDoc::TextStyle style = UiDoc::NormalizeTextStyle(raw, "title_gold");
    style.size = 200;
    Json fix;
    ASSERT_TRUE(Json::Parse("{\"size\":999,\"name\":\"x\",\"role\":\"body\",\"font\":\"Lilita One\",\"letterSpacing\":2,"
                            "\"fill\":{\"angle\":90,\"stops\":[{\"color\":\"#FFFFFF\",\"at\":0},{\"color\":\"#FF8800\",\"at\":1}]}}", fix));
    auto next = UiDoc::ApplyJudgeFix(style, &fix);
    EXPECT_EQ(next.size, 200);
    EXPECT_EQ(next.name, style.name);
    EXPECT_EQ(next.role, style.role);
    EXPECT_EQ(next.font, "Lilita One");
    auto changes = UiDoc::StyleChanges(style, next);
    EXPECT_TRUE(changes.Contains("font: Russo One 400 \xE2\x86\x92 Lilita One 400"));
    EXPECT_TRUE(changes.Contains("fill: #FFE45C \xE2\x86\x92 gradient #FFFFFF@0 #FF8800@1 90\xC2\xB0"));
    EXPECT_TRUE(changes.Contains("letterSpacing: 0 \xE2\x86\x92 2"));
    EXPECT_TRUE(UiDoc::StyleChanges(style, UiDoc::ApplyJudgeFix(style, nullptr)).IsEmpty());

    auto node = PipelineNodeRegistry::CreateNode("textStyleExtract", Vec2F());
    EXPECT_EQ(UiDoc::StyleCheckKey(*node), "");
    node->SetConfigBool("refCheck", true);
    node->SetConfigNumber("refCheckMax", 3);
    node->SetConfigString("refCheckModel", "judge");
    EXPECT_EQ(UiDoc::StyleCheckKey(*node), "style-check:v2:judge:3");
}

TEST(PipelineStyleCheck, ACachedOnlyRunLeavesTheCheckOwedAndRestartAsksAgain)
{
    WorkDir work;
    Stub stub;
    int reads = 0, judgements = 0;
    stub.answer = [&](const String& prompt)
    {
        if (!IsJudge(prompt))
        {
            reads++;
            return Read();
        }
        judgements++;
        return String("{\"verdict\":\"ok\",\"likeness\":0.95,\"differences\":[]}");
    };
    Rig rig;
    rig.Run();
    EXPECT_EQ(reads, 1);
    auto sigs = rig.graph.ComputeSignatures();
    EXPECT_TRUE(PipelineExecutor::RanExists(rig.pipelineId, sigs[rig.node->id]));

    // Switched on afterwards: a cached-only run applies the stored reading as it is and pays for nothing; the node is not up to
    // date until a real run checks it
    rig.node->SetConfigBool("refCheck", true);
    auto events = rig.Run(true);
    EXPECT_EQ(judgements, 0);
    EXPECT_TRUE(Logged(events, "check not run yet"));
    sigs = rig.graph.ComputeSignatures();
    EXPECT_FALSE(PipelineExecutor::RanExists(rig.pipelineId, sigs[rig.node->id]));

    events = rig.Run();
    EXPECT_EQ(judgements, 1);
    EXPECT_EQ(reads, 1);
    EXPECT_TRUE(PipelineExecutor::RanExists(rig.pipelineId, sigs[rig.node->id]));

    // A run with nothing new reads the check back; a restart (a fresh run) drops both stored steps and asks again: the reading
    // and the judgement, not replayed from what was answered before
    events = rig.Run();
    EXPECT_TRUE(Logged(events, "check cache hit"));
    EXPECT_EQ(judgements, 1);
    events = rig.Run(false, true);
    EXPECT_EQ(reads, 2);
    EXPECT_EQ(judgements, 2);
    EXPECT_FALSE(Logged(events, "check cache hit"));
    EXPECT_TRUE(Logged(events, "textStyleExtract: check ok"));
}
