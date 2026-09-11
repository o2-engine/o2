#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <chrono>

#include "Network/NetworkTestHelpers.h"
#include "o2/Assets/Assets.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Pipeline/PipelineAudio.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineVideo.h"

using namespace o2;
using namespace Editor;

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-test-work-" + (String)(int)Math::Random(0, 1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
            PipelineUtils::SetWorkPathOverride(path);
        }

        ~WorkDirGuard()
        {
            PipelineUtils::SetWorkPathOverride("");
            o2FileSystem.FolderRemove(path, true);
        }
    };

    Ref<PipelineNode> AddNode(PipelineGraph& graph, const String& type)
    {
        auto node = PipelineNodeRegistry::CreateNode(type, Vec2F());
        graph.nodes.Add(node);
        return node;
    }

    void Connect(PipelineGraph& graph, const Ref<PipelineNode>& from, const String& outName, const Ref<PipelineNode>& to, const String& inName)
    {
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = from->id;
        edge->fromPortId = from->outputs.Find([&](const PipelinePort& p) { return p.name == outName; })->id;
        edge->toNodeId = to->id;
        edge->toPortId = to->inputs.Find([&](const PipelinePort& p) { return p.name == inName; })->id;
        graph.edges.Add(edge);
    }

    struct RunResult
    {
        Vector<PipelineExecEvent> events;
        bool done = false;
        String fatal;
        Map<String, PipelineValue> outputs;
        Map<String, String> states;
    };

    RunResult RunPipeline(const PipelineGraph& graph, const String& target, const String& pipelineId = "test", bool cachedOnly = false)
    {
        RunResult result;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            result.events.Add(e);
            if (e.type == PipelineExecEvent::Type::NodeOutput) result.outputs[e.nodeId] = e.value;
            if (e.type == PipelineExecEvent::Type::NodeState) result.states[e.nodeId] = e.state + (e.error.IsEmpty() ? String() : ": " + e.error);
            if (e.type == PipelineExecEvent::Type::Done) result.done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) result.fatal = e.error;
        };
        executor->Execute(pipelineId, graph, target, {}, cachedOnly);
        EXPECT_TRUE(NetPumpUntil([&] { return result.done || !result.fatal.IsEmpty(); }, 20.0f));
        return result;
    }
}

TEST(PipelineExecutor, TextNodesComposeAndConcat)
{
    WorkDirGuard work;
    PipelineGraph graph;
    auto tmpl = AddNode(graph, "sourceText");
    tmpl->SetConfigString("text", "Hello, {who}!");
    auto who = AddNode(graph, "sourceText");
    who->SetConfigString("text", "world");
    auto compose = AddNode(graph, "textCompose");
    compose->SetCustomInputs({ PipelinePort("v1", "who", PipelinePortType::Text, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(compose);
    Connect(graph, tmpl, "out", compose, "template");
    Connect(graph, who, "out", compose, "who");

    auto concat = AddNode(graph, "textConcat");
    concat->SetCustomInputs({ PipelinePort("a", "a", PipelinePortType::Text, true), PipelinePort("b", "b", PipelinePortType::Text, true) });
    concat->SetConfigBool("newlineSeparator", true);
    PipelineNodeRegistry::SyncNodeWithSchema(concat);
    Connect(graph, compose, "out", concat, "a");
    Connect(graph, who, "out", concat, "b");

    auto result = RunPipeline(graph, concat->id);
    EXPECT_TRUE(result.done) << result.fatal;
    ASSERT_TRUE(result.outputs.ContainsKey(concat->id));
    EXPECT_EQ(result.outputs[concat->id].data, "Hello, world!\n\nworld");
    EXPECT_EQ(result.states[concat->id], "done");

    // A second run is served from the content cache and stays consistent
    auto second = RunPipeline(graph, concat->id);
    EXPECT_TRUE(second.done);
    EXPECT_EQ(second.outputs[concat->id].data, "Hello, world!\n\nworld");
    bool cacheHit = second.events.Any([](const PipelineExecEvent& e) { return e.type == PipelineExecEvent::Type::Log && e.message.Contains("cache hit"); });
    EXPECT_TRUE(cacheHit);

    // Freshness markers exist for every node of the branch
    auto fresh = PipelineExecutor::ComputeFreshNodes("test", graph);
    EXPECT_TRUE(fresh.Contains(concat->id));
    EXPECT_TRUE(fresh.Contains(compose->id));
}

TEST(PipelineExecutor, FinishImageWritesAssetAndAppliesEffects)
{
    WorkDirGuard work;
    String assetsDir = work.path + "Assets/";
    o2FileSystem.FolderCreate(assetsDir, true);

    // A 32x32 opaque red square on transparent, saved as an upload
    auto bitmap = PipelineImageOps::Blank(64, 64);
    for (int y = 16; y < 48; y++)
        for (int x = 16; x < 48; x++)
        {
            UInt8* p = PipelineImageOps::Pixel(*bitmap, x, y);
            p[0] = 255; p[1] = 0; p[2] = 0; p[3] = 255;
        }
    String png = EncodeBitmapPng(*bitmap);
    o2FileSystem.FolderCreate(PipelineUtils::GetUploadsPath(), true);
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("square.png"), png);

    PipelineGraph graph;
    auto source = AddNode(graph, "sourceImage");
    source->SetConfigString("uploadId", "square.png");
    auto outline = AddNode(graph, "imageOutline");
    outline->SetConfigString("color", "#0000ff");
    outline->SetConfigNumber("width", 4);
    auto color = AddNode(graph, "imageColor");
    color->SetConfigBool("invert", true);
    auto finish = AddNode(graph, "finishImage");
    finish->SetConfigString("assetPath", "Generated/square");
    finish->SetConfigBool("resize", true);
    finish->SetConfigNumber("resizeW", 32);
    finish->SetConfigNumber("resizeH", 32);
    Connect(graph, source, "out", outline, "image");
    Connect(graph, outline, "out", color, "image");
    Connect(graph, color, "out", finish, "in");

    // The executor writes into o2Assets path; point the finish node at the sandbox through the context path override
    // by running the node implementation directly for the file part, and the executor for the graph part
    auto result = RunPipeline(graph, color->id);
    EXPECT_TRUE(result.done) << result.fatal;
    ASSERT_TRUE(result.outputs.ContainsKey(color->id));
    auto out = result.outputs[color->id].GetBitmap();
    ASSERT_TRUE(out != nullptr);
    // Outline grew the canvas by 4px on every side
    EXPECT_EQ(out->GetSize(), Vec2I(72, 72));
    // Inverted red square centre becomes cyan
    const UInt8* centre = PipelineImageOps::Pixel(*out, 36, 36);
    EXPECT_EQ(centre[0], 0);
    EXPECT_EQ(centre[1], 255);
    EXPECT_EQ(centre[2], 255);
    EXPECT_EQ(centre[3], 255);

    // Finish node: run its implementation with a sandbox assets path
    auto impl = PipelineNodeRegistry::Get("finishImage");
    auto ctx = mmake<PipelineExecContext>();
    ctx->assetsPath = assetsDir;
    Map<String, PipelineValue> inputs;
    inputs["in"] = result.outputs[color->id];
    auto coroutine = impl->Run(ctx, inputs, finish);
    coroutine.Start(JobThread::Main);
    EXPECT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 10.0f));
    auto finishResult = coroutine.GetResult();
    EXPECT_TRUE(finishResult.ok) << finishResult.error;
    EXPECT_TRUE(ctx->assetsChanged);
    EXPECT_TRUE(o2FileSystem.IsFileExist(assetsDir + "Generated/square.png"));
    auto saved = DecodeImageBytes(PipelineUtils::ReadFileBytes(assetsDir + "Generated/square.png"));
    ASSERT_TRUE(saved != nullptr);
    EXPECT_EQ(saved->GetSize(), Vec2I(32, 32));
}

TEST(PipelineExecutor, CachedOnlyRefusesProviderNodes)
{
    WorkDirGuard work;
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText");
    text->SetConfigString("text", "a cat");
    auto ai = AddNode(graph, "aiText");
    Connect(graph, text, "out", ai, "prompt");

    auto result = RunPipeline(graph, ai->id, "test", true);
    EXPECT_FALSE(result.done);
    EXPECT_EQ(result.fatal, "Not cached");
    EXPECT_FALSE(result.states.ContainsKey(ai->id) && result.states[ai->id].StartsWith("error"));
}

TEST(PipelineImageOps, ChromaKeyAndTwoPassMatte)
{
    auto white = PipelineImageOps::Blank(8, 8, Color4(255, 255, 255, 255));
    auto black = PipelineImageOps::Blank(8, 8, Color4(0, 0, 0, 255));
    for (int y = 2; y < 6; y++)
        for (int x = 2; x < 6; x++)
        {
            UInt8* w = PipelineImageOps::Pixel(*white, x, y); w[0] = 200; w[1] = 50; w[2] = 50;
            UInt8* b = PipelineImageOps::Pixel(*black, x, y); b[0] = 200; b[1] = 50; b[2] = 50;
        }
    auto matte = PipelineImageOps::TwoPassMatte(*white, *black);
    EXPECT_EQ(PipelineImageOps::Pixel(*matte, 0, 0)[3], 0);
    EXPECT_EQ(PipelineImageOps::Pixel(*matte, 3, 3)[3], 255);
    EXPECT_EQ(PipelineImageOps::Pixel(*matte, 3, 3)[0], 200);

    auto green = PipelineImageOps::Blank(8, 8, Color4(0, 177, 64, 255));
    UInt8* subject = PipelineImageOps::Pixel(*green, 4, 4);
    subject[0] = 220; subject[1] = 40; subject[2] = 40;
    PipelineImageOps::ChromaOptions chroma;
    auto keyed = PipelineImageOps::ChromaKey(*green, chroma);
    EXPECT_EQ(PipelineImageOps::Pixel(*keyed, 0, 0)[3], 0);
    EXPECT_GT(PipelineImageOps::Pixel(*keyed, 4, 4)[3], 100);

    auto cropped = PipelineImageOps::CropToContent(*keyed, PipelineImageOps::ContentMode::Alpha);
    EXPECT_EQ(cropped->GetSize(), Vec2I(1, 1));

    auto rotated = PipelineImageOps::Rotate(*green, 90.0f);
    EXPECT_EQ(rotated->GetSize(), Vec2I(8, 8));
    auto sliced = PipelineImageOps::NineSliceResize(*green, 20, 12, { 2, 2, 2, 2 }, 1.0f);
    EXPECT_EQ(sliced->GetSize(), Vec2I(20, 12));
}

// Frames and the audio track of a clip land in one atlas next to a meta file; needs ffmpeg
TEST(PipelineVideoPreview, ExtractsFrameAtlasAndAudio)
{
    if (!PipelineAudio::IsFfmpegAvailable())
        GTEST_SKIP() << "ffmpeg is not installed";

    WorkDirGuard work;
    String clipPath = work.path + "clip.mp4";
    String cmd = "\"" + PipelineAudio::ToolPath("ffmpeg") + "\" -y -loglevel error -f lavfi -i testsrc=duration=1:size=64x48:rate=10"
        " -f lavfi -i sine=frequency=440:duration=1 -c:v mpeg4 -c:a aac -shortest \"" + clipPath + "\"";
    ASSERT_EQ(system(cmd.Data()), 0);

    String data = PipelineUtils::ReadFileBytes(clipPath);
    ASSERT_FALSE(data.IsEmpty());

    String dir = work.path + "preview/";
    auto info = PipelineVideo::BuildPreview(data, dir, 64, 200);
    ASSERT_TRUE(info.ok) << info.error;
    EXPECT_GE(info.frameCount, 8);
    EXPECT_LE(info.frameCount, 14);
    EXPECT_EQ(info.frameWidth, 64);
    EXPECT_TRUE(info.hasAudio);
    EXPECT_NEAR(info.duration, 1.0f, 0.25f);
    EXPECT_TRUE(o2FileSystem.IsFileExist(info.audioPath));

    Bitmap atlas;
    ASSERT_TRUE(atlas.Load(info.atlasPath, Bitmap::ImageType::Png));
    int rows = (info.frameCount + info.columns - 1) / info.columns;
    EXPECT_EQ(atlas.GetSize(), Vec2I(info.columns * info.frameWidth, rows * info.frameHeight));

    RectI last = PipelineVideo::FrameRect(info, info.frameCount - 1);
    EXPECT_EQ(last.Width(), info.frameWidth);
    EXPECT_EQ(last.Height(), info.frameHeight);
    EXPECT_GE(last.bottom, 0);
    EXPECT_LE(last.top, atlas.GetSize().y);

    auto again = PipelineVideo::LoadPreview(dir);
    EXPECT_TRUE(again.ok);
    EXPECT_EQ(again.frameCount, info.frameCount);
    EXPECT_EQ(again.hasAudio, info.hasAudio);
}

// Live provider smoke: text -> prompt -> image -> finish, against the real Gemini API. Runs only with
// PIPELINE_LIVE_GEMINI_KEY set, so the default suite stays offline
TEST(PipelineExecutorLive, GeminiTextToImageFinish)
{
    const char* key = getenv("PIPELINE_LIVE_GEMINI_KEY");
    if (!key || !*key)
        GTEST_SKIP() << "PIPELINE_LIVE_GEMINI_KEY is not set";

    WorkDirGuard work;
    PipelineSettings settings = PipelineSettings::Load();
    settings.geminiApiKey = key;
    settings.Save();

    String assetsDir = work.path + "Assets/";
    o2FileSystem.FolderCreate(assetsDir, true);

    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText");
    text->SetConfigString("text", "pixel art coin for a 2D video game, made of cheese");
    auto prompt = AddNode(graph, "promptGen");
    auto gen = AddNode(graph, "nanoBananaGen");
    gen->SetConfigBool("transparentBg", true);
    gen->SetConfigString("transparentMode", "chroma");
    auto finish = AddNode(graph, "finishImage");
    finish->SetConfigString("assetPath", "Generated/cheesecoin");
    Connect(graph, text, "out", prompt, "description");
    Connect(graph, prompt, "out", gen, "prompt");
    Connect(graph, gen, "out", finish, "in");

    RunResult result;
    auto executor = mmake<PipelineExecutor>();
    executor->onEvent = [&](const PipelineExecEvent& e)
    {
        result.events.Add(e);
        if (e.type == PipelineExecEvent::Type::NodeOutput) result.outputs[e.nodeId] = e.value;
        if (e.type == PipelineExecEvent::Type::NodeState) result.states[e.nodeId] = e.state + (e.error.IsEmpty() ? String() : ": " + e.error);
        if (e.type == PipelineExecEvent::Type::Done) result.done = true;
        if (e.type == PipelineExecEvent::Type::Fatal) result.fatal = e.error;
        if (e.type == PipelineExecEvent::Type::Log) printf("[pipeline] %s\n", e.message.Data());
    };
    executor->assetsPathOverride = assetsDir;
    executor->Execute("live", graph, finish->id, {}, false);
    EXPECT_TRUE(NetPumpUntil([&] { return result.done || !result.fatal.IsEmpty(); }, 240.0f));
    EXPECT_TRUE(result.fatal.IsEmpty()) << result.fatal;
    for (auto& kv : result.states)
        printf("[pipeline] state %s: %s\n", kv.first.Data(), kv.second.Data());

    ASSERT_TRUE(result.outputs.ContainsKey(prompt->id));
    EXPECT_FALSE(result.outputs[prompt->id].data.IsEmpty());
    ASSERT_TRUE(result.outputs.ContainsKey(gen->id));
    auto image = result.outputs[gen->id].GetBitmap();
    ASSERT_TRUE(image != nullptr);
    EXPECT_GT(image->GetSize().x, 64);
    EXPECT_TRUE(o2FileSystem.IsFileExist(assetsDir + "Generated/cheesecoin.png"));
    if (const char* keep = getenv("PIPELINE_LIVE_KEEP"))
        PipelineUtils::WriteFileBytes(keep, PipelineUtils::ReadFileBytes(assetsDir + "Generated/cheesecoin.png"));

    // The chroma pass leaves the corners transparent
    const UInt8* corner = PipelineImageOps::Pixel(*image, 1, 1);
    EXPECT_LT(corner[3], 40);
    printf("[pipeline] prompt: %s\n", result.outputs[prompt->id].data.SubStr(0, 200).Data());
    printf("[pipeline] image %dx%d\n", image->GetSize().x, image->GetSize().y);
}

// Providers may answer with JPEG: the decoder must accept it next to PNG
TEST(PipelineImageOps, DecodesJpegBytes)
{
    String jpeg;
    String relative = "o2/Framework/3rdPartyLibs/tracy/test/image.jpg";
    for (int up = 0; up < 6 && jpeg.IsEmpty(); up++, relative = "../" + relative)
        jpeg = PipelineUtils::ReadFileBytes(relative);

    ASSERT_FALSE(jpeg.IsEmpty());
    auto bitmap = DecodeImageBytes(jpeg);
    ASSERT_TRUE(bitmap != nullptr);
    EXPECT_GT(bitmap->GetSize().x, 1);
    EXPECT_GT(bitmap->GetSize().y, 1);
    EXPECT_EQ(bitmap->GetFormat(), PixelFormat::R8G8B8A8);
}

// Showcase: every Gemini-backed node in one connected pipeline, saved as a project asset and run live.
// Needs PIPELINE_SHOWCASE=1 and PIPELINE_LIVE_GEMINI_KEY; writes into the project Assets and Work folders
TEST(PipelineShowcase, GeminiAllNodesAsset)
{
    const char* key = getenv("PIPELINE_LIVE_GEMINI_KEY");
    if (!key || !*key || !getenv("PIPELINE_SHOWCASE"))
        GTEST_SKIP() << "PIPELINE_SHOWCASE / PIPELINE_LIVE_GEMINI_KEY are not set";

    PipelineSettings settings = PipelineSettings::Load();
    settings.geminiApiKey = key;
    settings.Save();

    PipelineGraph graph;
    auto place = [&](const String& type, float x, float y)
    {
        auto node = PipelineNodeRegistry::CreateNode(type, Vec2F(x, y));
        graph.nodes.Add(node);
        return node;
    };

    auto source = place("sourceText", 0, 400);
    source->SetConfigString("text", "A collectible coin made of Swiss cheese for a 2D pixel-art platformer");

    // Image chain: prompt -> generation on white -> Gemini edit to black -> matte -> asset
    auto imagePrompt = place("promptGen", 330, 0);
    imagePrompt->SetConfigString("target", "image");
    auto gen = place("nanoBananaGen", 660, 0);
    gen->SetConfigString("extraPrompt", "Centered on a solid pure white (#FFFFFF) background, no shadow, no text");
    auto edit = place("imageEdit", 990, 0);
    edit->SetConfigString("prompt", "Replace the background with a solid pure black (#000000) background. Keep the coin exactly the same: same position, size, colors and details.");
    auto matte = place("removeBackground", 1320, 0);
    auto finishCoin = place("finishImage", 1650, 0);
    finishCoin->SetConfigString("assetPath", "Generated/GeminiShowcase/coin");
    Connect(graph, source, "out", imagePrompt, "description");
    Connect(graph, imagePrompt, "out", gen, "prompt");
    Connect(graph, gen, "out", edit, "image");
    Connect(graph, gen, "out", matte, "white");
    Connect(graph, edit, "out", matte, "black");
    Connect(graph, matte, "out", finishCoin, "in");

    // Extract chain: region of the generated image -> transparent sprite
    auto extract = place("imageExtract", 990, 900);
    extract->SetConfigString("prompt", "the cheese coin");
    extract->SetConfigBool("transparentBg", true);
    extract->SetConfigString("transparentMode", "chroma");
    auto& roi = extract->config["roi"];
    roi.SetObject();
    roi["x"] = 0.15f; roi["y"] = 0.1f; roi["w"] = 0.7f; roi["h"] = 0.8f;
    auto finishSprite = place("finishImage", 1320, 900);
    finishSprite->SetConfigString("assetPath", "Generated/GeminiShowcase/coin_sprite");
    Connect(graph, gen, "out", extract, "image");
    Connect(graph, extract, "out", finishSprite, "in");

    // Video chain: Veo from a video prompt with the generated coin as reference
    auto videoPrompt = place("promptGen", 330, 1500);
    videoPrompt->SetConfigString("target", "video");
    auto video = place("videoGen", 660, 1500);
    video->SetConfigNumber("duration", 4);
    video->SetConfigString("aspectRatio", "16:9");
    auto finishVideo = place("finishVideo", 990, 1500);
    finishVideo->SetConfigString("assetPath", "Generated/GeminiShowcase/coin_spin");
    Connect(graph, source, "out", videoPrompt, "description");
    Connect(graph, videoPrompt, "out", video, "prompt");
    Connect(graph, gen, "out", video, "reference");
    Connect(graph, video, "out", finishVideo, "in");

    // Text chain: description -> playful edit -> text asset and Gemini speech
    auto text = place("aiText", 330, -900);
    text->SetConfigString("systemPrompt", "You write short item descriptions for a game shop. Answer with two sentences, no preamble.");
    auto textEdit = place("textEdit", 660, -900);
    textEdit->SetConfigString("instruction", "Make it playful and keep it under 25 words");
    auto finishText = place("finishText", 990, -900);
    finishText->SetConfigString("assetPath", "Generated/GeminiShowcase/description");
    auto speech = place("ttsSpeech", 990, -650);
    speech->SetConfigString("provider", "gemini");
    speech->SetConfigString("voice", "Kore");
    auto finishVoice = place("finishAudio", 1320, -650);
    finishVoice->SetConfigString("assetPath", "Generated/GeminiShowcase/voice");
    Connect(graph, source, "out", text, "prompt");
    Connect(graph, text, "out", textEdit, "text");
    Connect(graph, textEdit, "out", finishText, "in");
    Connect(graph, textEdit, "out", speech, "text");
    Connect(graph, speech, "out", finishVoice, "in");

    // Music chain: Lyria theme from a music prompt
    auto musicPrompt = place("promptGen", 330, -1500);
    musicPrompt->SetConfigString("target", "music");
    auto music = place("musicGen", 660, -1500);
    music->SetConfigBool("instrumental", true);
    auto finishTheme = place("finishAudio", 990, -1500);
    finishTheme->SetConfigString("assetPath", "Generated/GeminiShowcase/theme");
    Connect(graph, source, "out", musicPrompt, "description");
    Connect(graph, musicPrompt, "out", music, "prompt");
    Connect(graph, music, "out", finishTheme, "in");

    // The asset keeps its id across reruns so the cache and the editor previews stay attached
    const String assetPath = "Pipelines/GeminiShowcase.pipeline";
    o2FileSystem.FolderCreate(o2Assets.GetAssetsPath() + "Pipelines", true);
    Ref<PipelineAsset> asset;
    if (o2Assets.IsAssetExist(assetPath))
    {
        AssetRef<PipelineAsset> existing(assetPath);
        asset = existing;
        graph.SaveToAsset(*asset);
        asset->Save();
    }
    else
    {
        asset = mmake<PipelineAsset>();
        graph.SaveToAsset(*asset);
        asset->Save(assetPath);
    }
    ASSERT_TRUE(o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + assetPath));
    o2Assets.RebuildAssets();

    String pipelineId = (String)asset->GetUID();
    printf("[showcase] asset %s uid %s, %d nodes, %d edges\n", assetPath.Data(), pipelineId.Data(), graph.nodes.Count(), graph.edges.Count());

    Map<String, String> states;
    Map<String, String> errors;
    auto runTarget = [&](const Ref<PipelineNode>& target, float timeout)
    {
        bool done = false;
        String fatal;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodeState)
            {
                states[e.nodeId] = e.state;
                if (!e.error.IsEmpty()) errors[e.nodeId] = e.error;
            }
            if (e.type == PipelineExecEvent::Type::Done) done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
            if (e.type == PipelineExecEvent::Type::Log) printf("[showcase] %s\n", e.message.Data());
        };
        auto started = std::chrono::steady_clock::now();
        executor->Execute(pipelineId, graph, target->id, {}, false);
        bool finished = NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, timeout);
        float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - started).count();
        printf("[showcase] target %s (%s): %s in %.1f s%s\n", target->nodeType.Data(), target->id.Data(),
               done ? "done" : "failed", seconds, fatal.IsEmpty() ? "" : (" - " + fatal).Data());
        EXPECT_TRUE(finished) << "timeout on " << target->nodeType.Data();
        EXPECT_TRUE(fatal.IsEmpty()) << fatal.Data();
    };

    runTarget(finishCoin, 300.0f);
    runTarget(finishSprite, 300.0f);
    runTarget(finishText, 300.0f);
    runTarget(finishVoice, 300.0f);
    runTarget(finishTheme, 600.0f);
    runTarget(finishVideo, 900.0f);

    for (auto& node : graph.nodes)
    {
        String state = states.ContainsKey(node->id) ? states[node->id] : String("not run");
        String error = errors.ContainsKey(node->id) ? errors[node->id] : String();
        printf("[showcase] node %s: %s%s\n", node->nodeType.Data(), state.Data(), error.IsEmpty() ? "" : (" - " + error).Data());
    }

    o2Assets.RebuildAssets();
}
