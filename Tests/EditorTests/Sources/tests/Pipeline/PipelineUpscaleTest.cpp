#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUpscale.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/Providers/OpenAiProvider.h"
#include "o2Editor/Pipeline/Providers/OpenRouterProvider.h"

using namespace o2;
using namespace Editor;

// AI upscale: the size, frame and render size rules, the providers' render size and frame, and the run with a stand-in
// for the image model

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-upscale-work-" + (String)(int)Math::Random(0, 1000000);
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

    struct StubGuard
    {
        ~StubGuard() { AiRouter::SetImageStub({}); }
    };

    Ref<PipelineNode> Upscale(const String& mode, float targetW = 0.0f, float targetH = 0.0f, bool locked = true)
    {
        auto node = PipelineNodeRegistry::CreateNode("aiUpscale", Vec2F());
        node->SetConfigString("upscale", mode);
        if (targetW > 0.0f) node->SetConfigNumber("targetW", targetW);
        if (targetH > 0.0f) node->SetConfigNumber("targetH", targetH);
        if (!locked) node->SetConfigBool("lockAspect", false);
        return node;
    }

    Color4 At(const Bitmap& bitmap, int x, int y)
    {
        const UInt8* p = PipelineImageOps::Pixel(bitmap, x, y);
        return Color4(p[0], p[1], p[2], p[3]);
    }

    struct Call
    {
        int            count = 0;
        String         prompt;
        int            seed = -1;
        AiImageOptions options;
        Vec2I          referenceSize;
        Color4         referenceCorner;
    };

    // Runs a source image into an upscale node; the stand-in answers with a solid image of the size
    Ref<Bitmap> RunUpscale(const Ref<Bitmap>& input, const Ref<PipelineNode>& upscale, const Vec2I& answerSize, Call& call)
    {
        PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("upscale.png"), EncodeBitmapPng(*input));
        PipelineGraph graph;
        auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
        source->SetConfigString("uploadId", "upscale.png");
        graph.nodes = { source, upscale };
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = source->id;
        edge->fromPortId = source->outputs[0].id;
        edge->toNodeId = upscale->id;
        edge->toPortId = upscale->inputs[0].id;
        graph.edges.Add(edge);

        AiRouter::SetImageStub([&](const String&, const String& prompt, const Vector<AiImageRef>& refs, int seed, bool,
                                   const AiImageOptions& options)
        {
            call.count++;
            call.prompt = prompt;
            call.seed = seed;
            call.options = options;
            if (auto reference = refs.IsEmpty() ? nullptr : EnsureRgba(DecodeImageBytes(refs[0].data)))
            {
                call.referenceSize = reference->GetSize();
                call.referenceCorner = At(*reference, 0, 0);
            }

            AiBytesResult result;
            result.ok = true;
            result.mimeType = "image/png";
            result.data = EncodeBitmapPng(*PipelineImageOps::Blank(answerSize.x, answerSize.y, Color4(30, 60, 200, 255)));
            return result;
        });

        Ref<Bitmap> output;
        bool done = false;
        String fatal;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == upscale->id && e.value.IsImage()) output = e.value.GetBitmap();
            if (e.type == PipelineExecEvent::Type::NodeState && e.state == "error") fatal = e.error;
            if (e.type == PipelineExecEvent::Type::Done) done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
        };
        executor->Execute("upscale", graph, upscale->id, {}, false);
        EXPECT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 20.0f));
        EXPECT_TRUE(fatal.IsEmpty()) << fatal;
        return output;
    }
}

TEST(PipelineUpscale, TheTargetFollowsTheModeAndTheLock)
{
    EXPECT_EQ(PipelineUpscale::Target(*Upscale("x2"), Vec2I(300, 200)), Vec2I(600, 400));
    EXPECT_EQ(PipelineUpscale::Target(*Upscale("x3"), Vec2I(300, 200)), Vec2I(900, 600));
    EXPECT_EQ(PipelineUpscale::Target(*Upscale("x4"), Vec2I(300, 200)), Vec2I(1200, 800));
    EXPECT_EQ(PipelineUpscale::Target(*Upscale("bogus"), Vec2I(300, 200)), Vec2I(600, 400)) << "x2 for anything else";

    EXPECT_EQ(PipelineUpscale::Target(*Upscale("size", 1000), Vec2I(300, 200)), Vec2I(1000, 667)) << "the height follows the shape";
    EXPECT_EQ(PipelineUpscale::Target(*Upscale("size"), Vec2I(300, 200)), Vec2I(600, 400)) << "twice the input by default";
    EXPECT_EQ(PipelineUpscale::Target(*Upscale("size", 1000, 300, false), Vec2I(300, 200)), Vec2I(1000, 300)) << "unlocked";
    EXPECT_EQ(PipelineUpscale::Target(*Upscale("size", 1000, 0, false), Vec2I(300, 200)), Vec2I(1000, 400));

    EXPECT_EQ(PipelineUpscale::Target(*Upscale("x4"), Vec2I(5000, 1000)), Vec2I(8192, 1638)) << "capped keeping the shape";
}

TEST(PipelineUpscale, FramesAndRenderSizes)
{
    EXPECT_EQ(PipelineUpscale::ClosestAspect(300, 200).label, "3:2");
    EXPECT_EQ(PipelineUpscale::ClosestAspect(90, 200).label, "9:16");
    EXPECT_EQ(PipelineUpscale::ClosestAspect(512, 512).label, "1:1");
    EXPECT_EQ(PipelineUpscale::ClosestAspect(2100, 900).label, "21:9");
    EXPECT_EQ(PipelineUpscale::ClosestAspect(1000, 2000).label, "9:16");

    EXPECT_EQ(PipelineUpscale::RenderSizeFor(1100), "1K");
    EXPECT_EQ(PipelineUpscale::RenderSizeFor(1101), "2K");
    EXPECT_EQ(PipelineUpscale::RenderSizeFor(2200), "2K");
    EXPECT_EQ(PipelineUpscale::RenderSizeFor(2201), "4K");

    for (auto model : { "gemini-3.1-flash-image", "gemini-3-pro-image", "models/gemini-3-pro-image-preview",
                        "google/gemini-3.1-flash-image-preview", " Gemini-3-Pro-Image " })
        EXPECT_TRUE(PipelineUpscale::RendersLargeSizes(model)) << model;
    for (auto model : { "gemini-2.5-flash-image", "gpt-image-1", "openai/gpt-5-image", "imagen-4.0-generate-001" })
        EXPECT_FALSE(PipelineUpscale::RendersLargeSizes(model)) << model;

    String plain = PipelineUpscale::Prompt("", false);
    EXPECT_TRUE(plain.StartsWith("Upscale this image"));
    EXPECT_TRUE(plain.EndsWith("Output only the resulting image."));
    EXPECT_FALSE(plain.Contains("gray backdrop"));
    String full = PipelineUpscale::Prompt("crisp outlines", true);
    EXPECT_TRUE(full.Contains("The flat gray backdrop is not part of the subject"));
    EXPECT_TRUE(full.Contains("Details to add and how: crisp outlines"));
}

TEST(PipelineUpscale, ProvidersAskForTheRenderSizeAndTheFrame)
{
    AiImageOptions options;
    options.renderSize = "2K";
    options.aspectRatio = "3:2";
    Vector<AiImageRef> refs = { { "image/png", EncodeBitmapPng(*PipelineImageOps::Blank(4, 4, Color4::White())) } };

    DataDocument gemini;
    GeminiProvider::BuildImageBody(gemini, "gemini-3.1-flash-image", "go", refs, 42, options);
    auto config = gemini.FindMember("generationConfig");
    ASSERT_TRUE(config);
    EXPECT_EQ((int)*config->FindMember("seed"), 42);
    auto imageConfig = config->FindMember("imageConfig");
    ASSERT_TRUE(imageConfig);
    EXPECT_EQ(String(imageConfig->FindMember("imageSize")->GetString()), "2K");
    EXPECT_EQ(String(imageConfig->FindMember("aspectRatio")->GetString()), "3:2");

    DataDocument older;
    GeminiProvider::BuildImageBody(older, "gemini-2.5-flash-image", "go", refs, 42, options);
    imageConfig = older.FindMember("generationConfig")->FindMember("imageConfig");
    ASSERT_TRUE(imageConfig);
    EXPECT_FALSE(imageConfig->FindMember("imageSize")) << "only Gemini 3 image models render a size";
    EXPECT_TRUE(imageConfig->FindMember("aspectRatio"));

    DataDocument plain;
    GeminiProvider::BuildImageBody(plain, "gemini-3.1-flash-image", "go", refs, 42);
    EXPECT_FALSE(plain.FindMember("generationConfig")->FindMember("imageConfig"));

    DataDocument router;
    options.renderSize = "4K";
    options.aspectRatio = "16:9";
    OpenRouterProvider::BuildRequestBody(router, "google/gemini-3.1-flash-image-preview", "go", refs, true, 7, options);
    auto routerConfig = router.FindMember("image_config");
    ASSERT_TRUE(routerConfig);
    EXPECT_EQ(String(routerConfig->FindMember("aspect_ratio")->GetString()), "16:9");
    EXPECT_EQ(String(routerConfig->FindMember("image_size")->GetString()), "4K");

    DataDocument routerOther;
    OpenRouterProvider::BuildRequestBody(routerOther, "openai/gpt-5-image", "go", refs, true, 7, options);
    ASSERT_TRUE(routerOther.FindMember("image_config"));
    EXPECT_FALSE(routerOther.FindMember("image_config")->FindMember("image_size"));

    EXPECT_EQ(OpenAiProvider::SizeFor("gpt-image-1", "3:2"), "1536x1024");
    EXPECT_EQ(OpenAiProvider::SizeFor("gpt-image-1", "2:3"), "1024x1536");
    EXPECT_EQ(OpenAiProvider::SizeFor("gpt-image-1", "1:1"), "1024x1024");
    EXPECT_EQ(OpenAiProvider::SizeFor("gpt-image-1", "5:4"), "1536x1024");
    EXPECT_EQ(OpenAiProvider::SizeFor("gpt-image-1", "4:5"), "1024x1536");
    EXPECT_EQ(OpenAiProvider::SizeFor("gpt-image-1", "4:3"), "1536x1024");
    EXPECT_EQ(OpenAiProvider::SizeFor("gpt-image-1", "21:9"), "1536x1024");
    EXPECT_EQ(OpenAiProvider::SizeFor("dall-e-3", "16:9"), "1024x1024");
    DataDocument openai;
    OpenAiProvider::BuildGenerationsBody(openai, "gpt-image-1", "go", false, OpenAiProvider::SizeFor("gpt-image-1", "9:16"));
    EXPECT_EQ(String(openai.FindMember("size")->GetString()), "1024x1536");
}

// One call at the size that covers the target, in the input's frame, with the node's seed; the answer is resampled to
// the exact target
TEST(PipelineUpscale, TheRunAsksOnceAndGivesTheExactSize)
{
    WorkDirGuard work;
    StubGuard stubGuard;
    auto upscale = Upscale("x4");
    upscale->SetConfigBool("inheritSeed", false);
    upscale->SetConfigNumber("seed", 1234);
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*upscale), 1);

    Call call;
    auto output = RunUpscale(PipelineImageOps::Blank(300, 200, Color4(200, 30, 30, 255)), upscale, Vec2I(1536, 1024), call);
    EXPECT_EQ(call.count, 1);
    EXPECT_EQ(call.options.renderSize, "2K");
    EXPECT_EQ(call.options.aspectRatio, "3:2");
    EXPECT_EQ(call.seed, 1234);
    EXPECT_EQ(call.referenceSize, Vec2I(300, 200)) << "a 3:2 input needs no padding";
    EXPECT_FALSE(call.prompt.Contains("gray backdrop"));
    ASSERT_TRUE(output);
    EXPECT_EQ(output->GetSize(), Vec2I(1200, 800));
    EXPECT_EQ(At(*output, 600, 400), Color4(30, 60, 200, 255));
}

// A narrow transparent sprite is padded to 9:16 on gray, and keeps its own shape: the model's colours inside, nothing outside
TEST(PipelineUpscale, ATransparentSpriteKeepsItsShape)
{
    WorkDirGuard work;
    StubGuard stubGuard;
    auto input = PipelineImageOps::Blank(90, 200, Color4(0, 0, 0, 0));
    for (int y = 50; y < 150; y++)
    {
        for (int x = 20; x < 70; x++)
        {
            UInt8* p = PipelineImageOps::Pixel(*input, x, y);
            p[0] = 220; p[1] = 40; p[2] = 40; p[3] = 255;
        }
    }

    Call call;
    auto output = RunUpscale(input, Upscale("x2"), Vec2I(900, 1600), call);
    EXPECT_EQ(call.count, 1);
    EXPECT_EQ(call.options.aspectRatio, "9:16");
    EXPECT_EQ(call.referenceSize, Vec2I(113, 200)) << "padded to the frame";
    EXPECT_EQ(call.referenceCorner, Color4(128, 128, 128, 255)) << "flattened on gray";
    EXPECT_TRUE(call.prompt.Contains("The flat gray backdrop is not part of the subject"));
    ASSERT_TRUE(output);
    EXPECT_EQ(output->GetSize(), Vec2I(180, 400));
    EXPECT_EQ(At(*output, 10, 10).a, 0) << "outside the sprite";
    EXPECT_EQ(At(*output, 90, 200), Color4(30, 60, 200, 255)) << "inside: the model's colours, opaque";
}
