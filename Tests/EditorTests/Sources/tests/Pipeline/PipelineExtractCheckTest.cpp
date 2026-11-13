#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <regex>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/ImageExtractNode.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineExtractCheck.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"

using namespace o2;
using namespace Editor;

// The AI extract's check against the reference (AssetsLine backend/test/extractCheck.test.ts): the sprite laid over the place
// it was cut from, a vision model's verdict, regenerations with the judge's notes in the prompt, the best attempt kept, the
// debug data; the image and vision models are stubs

namespace
{
    using UiLayout::Json;
    using UiLayout::UiImage;

    const char* imageModel = "gemini-3.1-flash-image";
    const char* judgeModel = "openai/gpt-5.5";

    struct WorkDir
    {
        String path;

        WorkDir()
        {
            String relative = "./pipeline-extract-check-work-" + (String)(int)(Math::Random()%1000000);
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

    Color4 Hex(const char* hex)
    {
        Color4 c;
        PipelineUtils::ParseHexColor(hex, c);
        return c;
    }

    // A w x h picture of the colour with a rectangle of another colour on it
    String Picture(int w, int h, const char* background, int rx, int ry, int rw, int rh, const char* color)
    {
        auto bmp = PipelineImageOps::Blank(w, h, Hex(background));
        Color4 c = Hex(color);
        for (int y = std::max(0, ry); y < std::min(h, ry + rh); y++)
        {
            for (int x = std::max(0, rx); x < std::min(w, rx + rw); x++)
            {
                UInt8* p = PipelineImageOps::Pixel(*bmp, x, y);
                p[0] = (UInt8)c.r; p[1] = (UInt8)c.g; p[2] = (UInt8)c.b; p[3] = 255;
            }
        }
        return EncodeBitmapPng(*bmp);
    }

    // The source: a 200x100 blue picture with a 40x40 square at (80, 30)
    String SourcePng() { return Picture(200, 100, "#3366cc", 80, 30, 40, 40, "#d02020"); }

    // What the image model answers: the crop at twice its size on the key colour, the square where the crop has it
    String KeyRender(const char* color, int shift = 0, int width = 80) { return Picture(160, 120, "#00b140", 40 + shift, 20, width, 80, color); }

    void SetChroma(PipelineNode& node)
    {
        node.SetConfigString("model", imageModel);
        node.SetConfigBool("transparentBg", true);
        node.SetConfigString("transparentMode", "chroma");
        node.SetConfigString("chromaColor", "#00b140");
        node.SetConfigNumber("chromaTolerance", 30);
        node.SetConfigNumber("chromaSoftness", 15);
        node.SetConfigNumber("chromaSpill", 60);
    }

    // The part under test (pa) is the red square in the middle; pb never runs
    Ref<PipelineNode> MakeExtract()
    {
        auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
        SetChroma(*node);
        PipelineExtractRegion a, b;
        a.id = "pa";
        a.name = "red square - the red square alone";
        a.x = 0.3f; a.y = 0.2f; a.w = 0.4f; a.h = 0.6f;
        b.id = "pb";
        b.name = "corner";
        b.x = 0; b.y = 0; b.w = 0.2f; b.h = 0.2f;
        PipelineRegions::Write(*node, { a, b });
        PipelineRegions::SyncPorts(*node);
        return node;
    }

    // The image and vision models, counted
    struct Models
    {
        Vector<String> images;                   // Prompts of the image calls
        Vector<Pair<String, int>> judged;        // Prompts of the judge calls and how many pictures each got
        Function<String(int, const String&)> draw;
        Function<String(int)> judge;

        Models()
        {
            AiRouter::SetImageStub([this](const String&, const String& prompt, const Vector<AiImageRef>&, int, bool, const AiImageOptions&)
            {
                images.Add(prompt);
                AiBytesResult r;
                r.ok = true;
                r.mimeType = "image/png";
                r.data = draw(images.Count(), prompt);
                return r;
            });
            UiDoc::SetUiAiTextStub([this](const String& model, const String& prompt, const Vector<AiImageRef>& pictures)
            {
                AiTextResult r;
                EXPECT_EQ(model, judgeModel);
                judged.Add({ prompt, pictures.Count() });
                r.ok = true;
                r.text = judge(judged.Count());
                return r;
            });
        }

        ~Models()
        {
            AiRouter::SetImageStub({});
            UiDoc::SetUiAiTextStub({});
        }
    };

    // A source and an extract of two parts, with a consumer of the part under test: a run computes that part alone
    struct Rig
    {
        PipelineGraph     graph;
        Ref<PipelineNode> source, extract, consumer;
        String            pipelineId = "xcheck";

        Rig()
        {
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("source.png"), SourcePng());
            source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
            source->SetConfigString("uploadId", "source.png");
            extract = MakeExtract();
            consumer = PipelineNodeRegistry::CreateNode("imageCrop", Vec2F());
            graph.nodes = { source, extract, consumer };
            Link(source, source->outputs[0].id, extract, extract->inputs[0].id);
            Link(extract, "pa", consumer, consumer->inputs[0].id);
        }

        void Link(const Ref<PipelineNode>& from, const String& fromPort, const Ref<PipelineNode>& to, const String& toPort)
        {
            auto edge = mmake<PipelineEdge>();
            edge->id = PipelineNode::GenerateId();
            edge->fromNodeId = from->id;
            edge->fromPortId = fromPort;
            edge->toNodeId = to->id;
            edge->toPortId = toPort;
            graph.edges.Add(edge);
        }

        // Runs the part (as a consumer of it does, or as ↻ on it does); the log lines, the fatal error in fatalOut
        Vector<String> RunPart(bool regenerate = false, bool cachedOnly = false, String* fatalOut = nullptr, const String& target = "")
        {
            Vector<String> logs, errors;
            bool done = false;
            String fatal;
            auto executor = mmake<PipelineExecutor>();
            executor->onEvent = [&](const PipelineExecEvent& e)
            {
                if (e.type == PipelineExecEvent::Type::Log) logs.Add(e.message);
                if (e.type == PipelineExecEvent::Type::NodeState && e.state == "error") errors.Add(e.error);
                if (e.type == PipelineExecEvent::Type::Done) done = true;
                if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
            };
            Vector<String> bypass;
            if (regenerate)
                bypass.Add(extract->id + "#pa");
            executor->Execute(pipelineId, graph, target.IsEmpty() ? consumer->id : target, bypass, cachedOnly);
            EXPECT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 60.0f));
            if (fatalOut)
                *fatalOut = fatal;
            else
                EXPECT_TRUE(fatal.IsEmpty() && errors.IsEmpty()) << fatal.Data() << (errors.IsEmpty() ? "" : errors[0].Data());
            return logs;
        }

        String Folder() const { return PipelineExtractCheck::CheckFolder(PipelineExecutor::GetWorkPath(pipelineId, extract->id)); }

        bool HasDebug() const { return o2FileSystem.IsFileExist(Folder() + "pa.json"); }

        Json Debug() const
        {
            Json d;
            EXPECT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(Folder() + "pa.json"), d));
            return d;
        }

        Ref<Bitmap> Result() const
        {
            auto value = PipelineExecutor::LoadPortPreview(pipelineId, extract->id, "pa");
            return value.GetBitmap();
        }

        void CheckOn(int limit = 0)
        {
            extract->SetConfigBool("refCheck", true);
            extract->SetConfigString("refCheckModel", judgeModel);
            if (limit)
                extract->SetConfigNumber("refCheckMax", (float)limit);
        }
    };

    bool AnyLine(const Vector<String>& lines, const char* pattern)
    {
        std::regex re(pattern);
        return lines.Any([&](const String& l) { return std::regex_search((std::string)l, re); });
    }

    String Ok() { return "{\"verdict\":\"ok\",\"likeness\":0.97,\"differences\":[],\"instructions\":\"\"}"; }

    String Bad(double likeness, const String& instructions = "Keep the whole red square, with its right edge.")
    {
        return "{\"verdict\":\"significant\",\"likeness\":" + UiDoc::JsNum(likeness) +
            ",\"differences\":[\"the right edge of the square is cut off\"],\"instructions\":\"" + instructions + "\"}";
    }

    int AttemptsOf(const Json& d) { return d.Get("attempts") ? d.Get("attempts")->Count() : 0; }
    const Json& Attempt(const Json& d, int i) { return *d.Get("attempts")->At(i); }
}

TEST(PipelineExtractCheck, OffByDefaultLimitOneToThreeJudgeDefault)
{
    auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    PipelineExtractCheck::Config check;
    EXPECT_FALSE(PipelineExtractCheck::Read(*node, check));
    node->SetConfigBool("refCheck", false);
    node->SetConfigNumber("refCheckMax", 3);
    EXPECT_FALSE(PipelineExtractCheck::Read(*node, check));

    node->RemoveConfig("refCheckMax");
    node->SetConfigBool("refCheck", true);
    ASSERT_TRUE(PipelineExtractCheck::Read(*node, check));
    EXPECT_EQ(check.model, PipelineExtractCheck::defaultModel);
    EXPECT_EQ(check.limit, 2);

    node->SetConfigNumber("refCheckMax", 3);
    node->SetConfigString("refCheckModel", " local:codex ");
    ASSERT_TRUE(PipelineExtractCheck::Read(*node, check));
    EXPECT_EQ(check.model, "local:codex");
    EXPECT_EQ(check.limit, 3);

    node->SetConfigNumber("refCheckMax", 9);
    ASSERT_TRUE(PipelineExtractCheck::Read(*node, check));
    EXPECT_EQ(check.limit, 2);
}

TEST(PipelineExtractCheck, NeverMovesAPartsGenerationKeyAndOffStaysOutOfTheSignature)
{
    auto keys = [](const std::function<void(PipelineNode&)>& config)
    {
        auto node = MakeExtract();
        config(*node);
        return Pair<String, String>(PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1),
                                    PipelineExecutor::PortSignature(*node, {}, -1, "pa"));
    };
    auto plain = keys([](PipelineNode&) {});
    auto off = keys([](PipelineNode& n) { n.SetConfigBool("refCheck", false); n.SetConfigNumber("refCheckMax", 3); n.SetConfigString("refCheckModel", "x"); });
    auto on = keys([](PipelineNode& n) { n.SetConfigBool("refCheck", true); });
    auto on3 = keys([](PipelineNode& n) { n.SetConfigBool("refCheck", true); n.SetConfigNumber("refCheckMax", 3); });
    EXPECT_EQ(off.first, plain.first);
    EXPECT_EQ(off.second, plain.second);
    EXPECT_EQ(on.second, plain.second);
    EXPECT_NE(on.first, plain.first);
    EXPECT_NE(on3.first, on.first);
    EXPECT_EQ(on3.second, plain.second);
}

TEST(PipelineExtractCheck, AChromaRenderIsLaidWhereTheModelDrewItAndAShiftedOneRegisteredBack)
{
    auto node = MakeExtract();
    UiImage src;
    ASSERT_TRUE(UiImage::Decode(SourcePng(), src));
    auto region = PipelineRegions::RegionOfPort(*node, "pa");
    auto rr = PipelineExtractCheck::RegionRect(src.w, src.h, region);
    EXPECT_EQ(rr, UiLayout::Box(60, 20, 80, 60));
    auto cr = PipelineExtractCheck::ContextRect(src.w, src.h, rr);
    EXPECT_EQ(cr, UiLayout::Box(40, 0, 120, 100));
    PipelineExtractRegion whole;
    EXPECT_EQ(PipelineExtractCheck::RegionRect(200, 100, whole), UiLayout::Box(0, 0, 200, 100));
    UiImage crop = src.Crop(60, 20, 80, 60), reference = src.Crop(40, 0, 120, 100);

    auto view = PipelineExtractCheck::SpriteViewOf(KeyRender("#d02020"), *node, "pa", 80, 60);
    EXPECT_EQ(view.sprite.w, 80);
    EXPECT_EQ(view.sprite.h, 80);
    EXPECT_EQ(view.from, "tracked");
    EXPECT_EQ(view.nominal, UiLayout::Box(20, 10, 40, 40));
    auto placed = PipelineExtractCheck::PlaceOnCrop(view.sprite, view.nominal, crop);
    EXPECT_NEAR(placed.box.x, 20, 1);
    EXPECT_NEAR(placed.box.y, 10, 1);
    EXPECT_NEAR(placed.box.w, 40, 2);
    auto same = PipelineExtractCheck::OverlayOn(reference, view.sprite, UiLayout::Box(placed.box.x + 20, placed.box.y + 20, placed.box.w, placed.box.h));
    EXPECT_LT(same.pixelDiff, 0.03);

    // The model drew it 12 px (6 source px) to the right: the registration brings it back
    auto moved = PipelineExtractCheck::SpriteViewOf(KeyRender("#d02020", 12), *node, "pa", 80, 60);
    EXPECT_EQ(moved.nominal.x, 26);
    auto back = PipelineExtractCheck::PlaceOnCrop(moved.sprite, moved.nominal, crop);
    EXPECT_NEAR(back.box.x, 20, 1.5);
    EXPECT_TRUE(back.fit);
    EXPECT_NEAR(back.dx, -6, 1.5);

    // Another colour: the pixels under the sprite differ
    auto other = PipelineExtractCheck::SpriteViewOf(KeyRender("#e0c010"), *node, "pa", 80, 60);
    auto diff = PipelineExtractCheck::OverlayOn(reference, other.sprite, UiLayout::Box(40, 30, 40, 40));
    EXPECT_GT(diff.pixelDiff, 0.2);
    EXPECT_GT(diff.pixelChanged, 0.9);
}

TEST(PipelineExtractCheck, ATrimmedResultKeepsItsFrame)
{
    auto node = MakeExtract();
    node->SetConfigString("transparentMode", "twoPass");
    String cut = Picture(40, 20, "#d02020", 0, 0, 0, 0, "#d02020");
    // Drawn in a 160x120 frame (twice an 80x60 crop) at (100, 80)
    String tagged = PipelinePngMeta::WriteText(PipelinePngMeta::WithDpi(cut, std::nullopt, 2.0), { { PipelineExtract::frameKey, "160,120,100,80" } });
    PipelineExtract::Frame frame;
    ASSERT_TRUE(PipelineExtract::ReadPngFrame(tagged, frame));
    EXPECT_EQ(frame.w, 160);
    EXPECT_EQ(frame.h, 120);
    EXPECT_EQ(frame.left, 100);
    EXPECT_EQ(frame.top, 80);
    auto view = PipelineExtractCheck::SpriteViewOf(tagged, *node, "pa", 80, 60);
    EXPECT_EQ(view.nominal, UiLayout::Box(50, 40, 20, 10));
    EXPECT_EQ(view.from, "tracked");

    // Without the note (an older result): its size by the scale, centred
    auto old = PipelineExtractCheck::SpriteViewOf(PipelinePngMeta::WithDpi(cut, std::nullopt, 2.0), *node, "pa", 80, 60);
    EXPECT_EQ(old.nominal, UiLayout::Box(30, 25, 20, 10));
    EXPECT_EQ(old.from, "centred");
}

TEST(PipelineExtractCheck, TheJudgesAnswerPromptClarificationsAndTheKeptAttempt)
{
    using namespace PipelineExtractCheck;
    JudgeAnswer a;
    ASSERT_TRUE(ParseJudge("{\"verdict\":\"ok\",\"likeness\":0.97,\"differences\":[],\"instructions\":\"\"}", a));
    EXPECT_EQ(a.verdict, "ok");
    EXPECT_EQ(a.likeness, 0.97);
    EXPECT_TRUE(a.differences.IsEmpty());
    EXPECT_EQ(a.instructions, "");

    ASSERT_TRUE(ParseJudge("Here it is:\n```json\n{\"verdict\": \"Significant\", \"likeness\": 35, \"differences\": [\"the rim is gone\", \"\", \"greener\"], \"instructions\": \" Keep  the gold rim. \"}\n```", a));
    EXPECT_EQ(a.verdict, "significant");
    EXPECT_EQ(a.likeness, 0.35);
    EXPECT_EQ(a.differences, Vector<String>({ "the rim is gone", "greener" }));
    EXPECT_EQ(a.instructions, "Keep the gold rim.");
    ASSERT_TRUE(ParseJudge("{\"likeness\": 0.75}", a));
    EXPECT_EQ(a.verdict, "minor");
    ASSERT_TRUE(ParseJudge("{\"verdict\": \"minor\"}", a));
    EXPECT_EQ(a.likeness, 0.8);
    String why;
    EXPECT_FALSE(ParseJudge("I cannot compare these images.", a, &why));
    EXPECT_EQ(why, "The judge did not answer with a verdict");
    EXPECT_FALSE(ParseJudge("{\"differences\": []}", a));

    std::string prompt = JudgePrompt("btn_green - the green button, without its PLAY label");
    EXPECT_NE(prompt.find("\"btn_green - the green button, without its PLAY label\""), std::string::npos);
    EXPECT_TRUE(std::regex_search(prompt, std::regex("REFERENCE[\\s\\S]*OVERLAY[\\s\\S]*SPRITE")));
    EXPECT_NE(prompt.find("\"verdict\": \"ok\" | \"minor\" | \"significant\""), std::string::npos);
    EXPECT_NE(prompt.find("NOT differences: anything the description asks to leave out"), std::string::npos);

    Judged first, second;
    first.verdict = "significant"; first.differences = { "the left edge is cut off" }; first.instructions = "Keep the whole button.";
    second.verdict = "significant"; second.differences = { "a green fringe" }; second.instructions = "Remove the backdrop colour from the edges.";
    std::string text = ClarificationsOf({ first, second });
    EXPECT_NE(text.find("rejected"), std::string::npos);
    EXPECT_LT(text.find("Remove the backdrop colour"), text.find("Keep the whole button."));
    EXPECT_NE(text.find("a green fringe"), std::string::npos);
    EXPECT_EQ(text.find("left edge is cut off"), std::string::npos);
    Judged ok;
    ok.verdict = "ok";
    EXPECT_EQ(ClarificationsOf({ ok }), "");

    auto judgedAs = [](int n, const char* verdict, std::optional<double> likeness)
    {
        Judged j;
        j.n = n;
        j.verdict = verdict;
        j.likeness = likeness;
        return j;
    };
    EXPECT_EQ(BestAttempt({ judgedAs(1, "significant", 0.5), judgedAs(2, "significant", 0.7), judgedAs(3, "significant", 0.4) }), 2);
    EXPECT_EQ(BestAttempt({ judgedAs(1, "significant", 0.9), judgedAs(2, "minor", 0.8) }), 2);
    EXPECT_EQ(BestAttempt({ judgedAs(1, "significant", 0.5), judgedAs(2, "", std::nullopt) }), 1);
    EXPECT_EQ(BestAttempt({ judgedAs(1, "", std::nullopt) }), 1);
}

TEST(PipelineExtractCheck, OffOneGenerationAndNoDebugData)
{
    WorkDir work;
    Models models;
    models.draw = [](int, const String&) { return KeyRender("#d02020"); };
    models.judge = [](int) { return Ok(); };
    Rig rig;
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);
    EXPECT_EQ(models.judged.Count(), 0);
    EXPECT_FALSE(rig.HasDebug());
}

TEST(PipelineExtractCheck, OkOneJudgeCallTheStoredGenerationReusedAndARerunPaysNothing)
{
    WorkDir work;
    Models models;
    models.draw = [](int, const String&) { return KeyRender("#d02020"); };
    models.judge = [](int) { return Ok(); };
    Rig rig;
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);

    rig.CheckOn();
    auto logs = rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1) << "the generation came from the cache";
    ASSERT_EQ(models.judged.Count(), 1);
    EXPECT_EQ(models.judged[0].second, 3) << "reference, overlay, sprite";
    EXPECT_TRUE(models.judged[0].first.Contains("\"red square - the red square alone\""));
    EXPECT_TRUE(AnyLine(logs, "check ok"));

    Json d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "ok");
    EXPECT_EQ(d.Get("kept")->Num(), 1);
    EXPECT_EQ(AttemptsOf(d), 1);
    EXPECT_EQ(d.Get("limit")->Num(), 2);
    EXPECT_EQ(d.Get("judge")->Str(), judgeModel);
    EXPECT_EQ(d.Get("calls")->Get("judge")->Num(), 1);
    EXPECT_EQ(d.Get("calls")->Get("draw")->Num(), 0);
    auto& a = Attempt(d, 0);
    EXPECT_EQ(a.Get("verdict")->Str(), "ok");
    EXPECT_EQ(a.Get("likeness")->Num(), 0.97);
    EXPECT_EQ(a.Get("clarifications")->Str(), "");
    EXPECT_EQ(a.Get("model")->Str(), imageModel);
    // Laid where it was cut from: the square is at (80, 30) 40x40 of the source
    auto placement = a.Get("placement");
    EXPECT_NEAR(placement->Get("x")->Num(), 80, 1);
    EXPECT_NEAR(placement->Get("y")->Num(), 30, 1);
    EXPECT_NEAR(placement->Get("w")->Num(), 40, 2);
    EXPECT_LT(a.Get("pixelDiff")->Num(), 0.03);
    for (auto file : { d.Get("reference")->Str(), a.Get("sprite")->Str(), a.Get("overlay")->Str() })
        EXPECT_TRUE(DecodeImageBytes(PipelineUtils::ReadFileBytes(rig.Folder() + file))) << file.Data();
    // The reference is the region with its context (120x100), enlarged for the judge; the overlay has its framing
    EXPECT_EQ(d.Get("referenceSize")->Get("w")->Num(), 384);
    EXPECT_EQ(d.Get("referenceSize")->Get("h")->Num(), 320);
    auto inRef = d.Get("regionInReference");
    EXPECT_EQ(UiLayout::Box(inRef->Get("x")->Num(), inRef->Get("y")->Num(), inRef->Get("w")->Num(), inRef->Get("h")->Num()),
              UiLayout::Box(64, 64, 256, 192));
    auto overlay = DecodeImageBytes(PipelineUtils::ReadFileBytes(rig.Folder() + a.Get("overlay")->Str()));
    ASSERT_TRUE(overlay);
    EXPECT_EQ(overlay->GetSize(), Vec2I(384, 320));

    // Nothing changed: the check is replayed from the cache
    auto again = rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);
    EXPECT_EQ(models.judged.Count(), 1);
    EXPECT_TRUE(AnyLine(again, "check cache hit"));
    // Off again: the generation as it was, still nothing asked
    rig.extract->SetConfigBool("refCheck", false);
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);
    EXPECT_EQ(models.judged.Count(), 1);
}

TEST(PipelineExtractCheck, SignificantRegeneratedWithTheNotesAndTheAcceptedAttemptKept)
{
    WorkDir work;
    Models models;
    // The first render lost a third of the square; the regeneration has all of it
    models.draw = [](int i, const String&) { return i == 1 ? KeyRender("#d02020", 0, 52) : KeyRender("#d02020"); };
    models.judge = [](int i) { return i == 1 ? Bad(0.4) : Ok(); };
    Rig rig;
    rig.CheckOn();
    rig.RunPart();
    ASSERT_EQ(models.images.Count(), 2);
    EXPECT_EQ(models.judged.Count(), 2);
    EXPECT_FALSE(models.images[0].Contains("rejected"));
    EXPECT_TRUE(models.images[1].Contains("the red square alone"));
    EXPECT_TRUE(std::regex_search((std::string)models.images[1], std::regex(
        "rejected[\\s\\S]*Keep the whole red square, with its right edge\\.[\\s\\S]*the right edge of the square is cut off")));

    Json d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "ok");
    EXPECT_EQ(d.Get("kept")->Num(), 2);
    ASSERT_EQ(AttemptsOf(d), 2);
    EXPECT_EQ(d.Get("calls")->Get("judge")->Num(), 2);
    EXPECT_EQ(d.Get("calls")->Get("draw")->Num(), 1);
    EXPECT_EQ(Attempt(d, 0).Get("verdict")->Str(), "significant");
    EXPECT_EQ(Attempt(d, 1).Get("verdict")->Str(), "ok");
    EXPECT_EQ(Attempt(d, 0).Get("instructions")->Str(), "Keep the whole red square, with its right edge.");
    EXPECT_EQ(Attempt(d, 0).Get("clarifications")->Str(), "");
    EXPECT_TRUE(Attempt(d, 1).Get("clarifications")->Str().Contains("Keep the whole red square"));
    EXPECT_EQ(Attempt(d, 0).Get("spriteSize")->Get("w")->Num(), 52);
    EXPECT_EQ(Attempt(d, 1).Get("spriteSize")->Get("w")->Num(), 80);
    // The part's result is the second attempt, cut and trimmed
    ASSERT_TRUE(rig.Result());
    EXPECT_EQ(rig.Result()->GetSize().x, 80);

    // A re-run with nothing changed asks no one
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 2);
    EXPECT_EQ(models.judged.Count(), 2);
    EXPECT_EQ(rig.Result()->GetSize().x, 80);
}

TEST(PipelineExtractCheck, TheLimitIsRespectedAndTheBestAttemptKept)
{
    WorkDir work;
    Models models;
    // Three renders that differ in width so the result says which one was kept
    int widths[] = { 60, 70, 50 };
    double likeness[] = { 0.5, 0.7, 0.4 };
    models.draw = [&](int i, const String&) { return KeyRender("#d02020", 0, i <= 3 ? widths[i - 1] : 40); };
    models.judge = [&](int i) { return Bad(i <= 3 ? likeness[i - 1] : 0.1, "Fix " + (String)i + "."); };
    Rig rig;
    rig.CheckOn(2);
    rig.RunPart();
    ASSERT_EQ(models.images.Count(), 3) << "the generation and two regenerations";
    EXPECT_EQ(models.judged.Count(), 3);
    Json d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "limit");
    EXPECT_EQ(d.Get("kept")->Num(), 2);
    EXPECT_EQ(d.Get("limit")->Num(), 2);
    ASSERT_EQ(AttemptsOf(d), 3);
    for (int i = 0; i < 3; i++)
        EXPECT_EQ(Attempt(d, i).Get("likeness")->Num(), likeness[i]);
    EXPECT_EQ(rig.Result()->GetSize().x, 70) << "the second attempt is the result";
    // The third attempt's prompt carries both asks, the latest first
    EXPECT_TRUE(models.images[2].Contains("- Fix 2.\n- Fix 1."));

    // A limit of 1 replays what was already asked: nothing is paid again, and of those two the second is kept
    rig.extract->SetConfigNumber("refCheckMax", 1);
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 3);
    EXPECT_EQ(models.judged.Count(), 3);
    Json d1 = rig.Debug();
    EXPECT_EQ(d1.Get("stop")->Str(), "limit");
    EXPECT_EQ(d1.Get("kept")->Num(), 2);
    ASSERT_EQ(AttemptsOf(d1), 2);
    EXPECT_EQ(d1.Get("calls")->Get("judge")->Num(), 0);
    EXPECT_EQ(d1.Get("calls")->Get("draw")->Num(), 0);
    EXPECT_EQ(Attempt(d1, 0).Get("replayed")->Dump(), "{\"judge\":true}");
    EXPECT_EQ(Attempt(d1, 1).Get("replayed")->Dump(), "{\"draw\":true,\"judge\":true}");
    // Back to 2: the stored check, and its details again
    rig.extract->SetConfigNumber("refCheckMax", 2);
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 3);
    EXPECT_EQ(models.judged.Count(), 3);
    EXPECT_EQ(AttemptsOf(rig.Debug()), 3);
}

TEST(PipelineExtractCheck, RestartingThePartDrawsItAgainAndChecksTheNewSprite)
{
    WorkDir work;
    Models models;
    models.draw = [](int i, const String&) { return KeyRender(i == 1 ? "#d02020" : i == 2 ? "#c01818" : "#b01010"); };
    models.judge = [](int) { return Ok(); };
    Rig rig;
    rig.CheckOn();
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);
    EXPECT_EQ(models.judged.Count(), 1);
    rig.RunPart(true);
    EXPECT_EQ(models.images.Count(), 2);
    EXPECT_EQ(models.judged.Count(), 2);
    auto result = rig.Result();
    ASSERT_TRUE(result);
    const UInt8* p = PipelineImageOps::Pixel(*result, 10, 10);
    EXPECT_EQ((int)p[0], 0xc0);
    EXPECT_EQ((int)p[1], 0x18);
    EXPECT_EQ((int)p[2], 0x18);
}

TEST(PipelineExtractCheck, AnUnreadableJudgeKeepsTheSpriteStoresNothingAndIsAskedAgain)
{
    WorkDir work;
    Models models;
    bool broken = true;
    models.draw = [](int, const String&) { return KeyRender("#d02020"); };
    models.judge = [&](int) { return broken ? String("Sorry, I cannot compare images.") : Ok(); };
    Rig rig;
    rig.CheckOn();
    auto logs = rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);
    EXPECT_EQ(models.judged.Count(), 1);
    EXPECT_TRUE(AnyLine(logs, "check failed"));
    Json d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "failed");
    EXPECT_EQ(d.Get("kept")->Num(), 1);
    EXPECT_TRUE(Attempt(d, 0).Get("error")->Str().Contains("did not answer with a verdict"));
    ASSERT_TRUE(rig.Result());
    EXPECT_EQ(rig.Result()->GetSize().x, 80) << "the part has its sprite";

    broken = false;
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);
    EXPECT_EQ(models.judged.Count(), 2) << "only the judge is asked again";
    d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "ok");
    EXPECT_EQ(Attempt(d, 0).Get("verdict")->Str(), "ok");
}

TEST(PipelineExtractCheck, AnOpaquePartIsNotCompared)
{
    WorkDir work;
    Models models;
    models.draw = [](int, const String&) { return Picture(160, 120, "#ffffff", 0, 0, 0, 0, "#ffffff"); };
    models.judge = [](int) { return Ok(); };
    Rig rig;
    rig.CheckOn();
    rig.extract->SetConfigBool("transparentBg", false);
    rig.RunPart();
    EXPECT_EQ(models.images.Count(), 1);
    EXPECT_EQ(models.judged.Count(), 0);
    Json d = rig.Debug();
    EXPECT_EQ(d.Get("stop")->Str(), "skipped");
    EXPECT_EQ(AttemptsOf(d), 0);
}

TEST(PipelineExtractCheck, ATwoPassPartNotesItsPlaceInTheFrameOnlyWhileTheCheckIsOn)
{
    Models models;
    // The plate drawn at (50, 30) 60x40 of a 160x120 frame: white, then black for the second pass
    models.draw = [](int, const String& prompt) { return Picture(160, 120, prompt.Contains("Reproduce it EXACTLY") ? "#000000" : "#ffffff", 50, 30, 60, 40, "#d02020"); };
    models.judge = [](int) { return Ok(); };
    auto run = [&](bool check)
    {
        auto node = MakeExtract();
        node->SetConfigString("transparentMode", "twoPass");
        if (check)
            node->SetConfigBool("refCheck", true);
        auto source = DecodeImageBytes(SourcePng());
        auto ctx = mmake<PipelineExecContext>();
        ctx->seed = 7;
        ctx->outputPortId = "pa";
        ctx->outputPort = "red square";
        auto coroutine = PipelineExtract::GeneratePart(ctx, node, "pa", PipelineExtract::RoiOf(*source, *node, "pa"), "the red plate", "");
        coroutine.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 10.0f));
        auto result = coroutine.GetResult();
        EXPECT_TRUE(result.ok) << result.error.Data();
        return result.outputs.empty() ? String() : result.outputs.begin()->second.data;
    };

    String off = run(false);
    EXPECT_EQ(models.images.Count(), 2) << "white, then black";
    auto offBmp = DecodeImageBytes(off);
    ASSERT_TRUE(offBmp);
    EXPECT_EQ(offBmp->GetSize(), Vec2I(60, 40));
    const UInt8* p = PipelineImageOps::Pixel(*offBmp, 5, 5);
    EXPECT_EQ((int)p[0], 0xd0);
    EXPECT_EQ((int)p[3], 255);
    EXPECT_EQ(PipelinePngMeta::ReadScale(off).value_or(0), 2.0);
    PipelineExtract::Frame frame;
    EXPECT_FALSE(PipelineExtract::ReadPngFrame(off, frame)) << "with the check off the bytes are what they always were";

    String with = run(true);
    ASSERT_TRUE(PipelineExtract::ReadPngFrame(with, frame));
    EXPECT_EQ(frame.w, 160);
    EXPECT_EQ(frame.h, 120);
    EXPECT_EQ(frame.left, 50);
    EXPECT_EQ(frame.top, 30);
    UiImage a, b;
    ASSERT_TRUE(UiImage::Decode(off, a) && UiImage::Decode(with, b));
    EXPECT_TRUE(a.data == b.data) << "the same pixels";
}

TEST(PipelineExtractCheck, ACachedOnlyRunNeverAsksTheJudge)
{
    WorkDir work;
    Models models;
    models.draw = [](int, const String&) { return KeyRender("#d02020"); };
    models.judge = [](int) { return Ok(); };
    Rig rig;
    rig.RunPart();

    // The check switched on after the part was made: a cached-only run does not ask the judge and does not stop either - the
    // part's stored generation is applied as it is, the check is left for a real run
    rig.CheckOn();
    auto logs = rig.RunPart(false, true);
    EXPECT_EQ(models.judged.Count(), 0);
    EXPECT_TRUE(AnyLine(logs, "check not run yet"));
    EXPECT_EQ(models.images.Count(), 1) << "nothing was drawn again";

    // A part that was never made (the corner, run with the whole node) is still not made by such a run
    String fatal;
    rig.RunPart(false, true, &fatal, rig.extract->id);
    EXPECT_EQ(fatal, "Not cached");
    EXPECT_EQ(models.judged.Count(), 0);
    EXPECT_EQ(models.images.Count(), 1);

    rig.RunPart();
    EXPECT_EQ(models.judged.Count(), 1) << "the real run pays for the check it was owed";
}

// A chroma render the key colour cut away entirely: the judge is shown that empty sprite, not the green render
TEST(PipelineExtractCheck, AnEmptyChromaCutIsShownAsItIs)
{
    auto node = MakeExtract();
    SetChroma(*node);
    String allGreen = Picture(160, 120, "#00b140", 0, 0, 0, 0, "#00b140");
    auto view = PipelineExtractCheck::SpriteViewOf(allGreen, *node, "pa", 80, 60);
    ASSERT_FALSE(view.sprite.IsEmpty());
    bool clear = true;
    for (size_t i = 3; i < view.sprite.data.size() && clear; i += 4)
        clear = view.sprite.data[i] == 0;
    EXPECT_TRUE(clear) << "no green pixel reaches the judge";
}
