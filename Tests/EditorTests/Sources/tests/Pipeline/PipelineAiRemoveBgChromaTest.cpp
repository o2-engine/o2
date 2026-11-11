#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"

using namespace o2;
using namespace Editor;

// AI remove background in chroma mode is one of the chroma nodes, as the web's CHROMA_NODE_TYPES has it: its run returns
// the raw key-colour render, the executor caches that and cuts it, and the cut sliders re-cut the cached render

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-removebg-work-" + (String)(int)(Math::Random()%1000000);
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

    Color4 At(const Bitmap& bitmap, int x, int y)
    {
        const UInt8* p = PipelineImageOps::Pixel(bitmap, x, y);
        return Color4(p[0], p[1], p[2], p[3]);
    }

    // What the image model answers: the subject (a red square) on the default key colour
    Ref<Bitmap> KeyedRender()
    {
        auto bitmap = PipelineImageOps::Blank(64, 64, Color4(0, 177, 64, 255));
        for (int y = 16; y < 48; y++)
        {
            for (int x = 16; x < 48; x++)
            {
                UInt8* p = PipelineImageOps::Pixel(*bitmap, x, y);
                p[0] = 220; p[1] = 30; p[2] = 30; p[3] = 255;
            }
        }
        return bitmap;
    }
}

TEST(PipelineAiRemoveBgChroma, TheNodeIsAChromaNodeLikeTheWebs)
{
    auto node = PipelineNodeRegistry::CreateNode("aiRemoveBg", Vec2F());
    EXPECT_TRUE(PipelineTransparency::UsesChromaPostStep(*node)) << "a new node: chroma with a model that draws no alpha";
    EXPECT_TRUE(PipelineTransparency::AnyChromaPostStep(*node));

    node->SetConfigString("transparentMode", "twoPass");
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node));

    node->SetConfigString("transparentMode", "chroma");
    node->SetConfigString("model", "gpt-image-1");
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node)) << "the model renders the alpha itself";

    node->SetConfigString("model", "gemini-3.1-flash-image");
    node->SetConfigBool("transparentBg", false);
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node)) << "as the web: the flag off, nothing to cut";
}

TEST(PipelineAiRemoveBgChroma, TheCutSettingsStayOutOfTheStoredRendersKey)
{
    auto node = PipelineNodeRegistry::CreateNode("aiRemoveBg", Vec2F());
    String chroma = PipelineExecutor::ContentSignature(*node, {}, -1);
    node->SetConfigNumber("chromaTolerance", 55.0f);
    node->SetConfigNumber("chromaSpill", 10.0f);
    EXPECT_EQ(PipelineExecutor::ContentSignature(*node, {}, -1), chroma) << "moving a slider re-cuts the same render";

    node->SetConfigString("transparentMode", "twoPass");
    String twoPass = PipelineExecutor::ContentSignature(*node, {}, -1);
    EXPECT_NE(twoPass, chroma);
    node->SetConfigNumber("chromaTolerance", 30.0f);
    EXPECT_NE(PipelineExecutor::ContentSignature(*node, {}, -1), twoPass) << "a two-pass node keys every setting";
}

TEST(PipelineAiRemoveBgChroma, TheExecutorCachesTheRawRenderAndCutsItOnce)
{
    WorkDirGuard work;
    StubGuard stubGuard;

    auto input = PipelineImageOps::Blank(64, 64, Color4(90, 120, 200, 255));
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("removebg.png"), EncodeBitmapPng(*input));

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "removebg.png");
    auto removeBg = PipelineNodeRegistry::CreateNode("aiRemoveBg", Vec2F(300, 0));
    graph.nodes = { source, removeBg };

    auto edge = mmake<PipelineEdge>();
    edge->id = PipelineNode::GenerateId();
    edge->fromNodeId = source->id;
    edge->fromPortId = source->outputs[0].id;
    edge->toNodeId = removeBg->id;
    edge->toPortId = removeBg->inputs[0].id;
    graph.edges.Add(edge);

    int calls = 0;
    String sentPrompt;
    AiRouter::SetImageStub([&](const String&, const String& prompt, const Vector<AiImageRef>&, int, bool, const AiImageOptions&)
    {
        calls++;
        sentPrompt = prompt;
        AiBytesResult result;
        result.ok = true;
        result.mimeType = "image/png";
        result.data = EncodeBitmapPng(*KeyedRender());
        return result;
    });

    auto run = [&](bool cachedOnly, Ref<Bitmap>& output)
    {
        bool done = false;
        String fatal;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == removeBg->id && e.value.IsImage()) output = e.value.GetBitmap();
            if (e.type == PipelineExecEvent::Type::NodeState && e.state == "error") fatal = e.error;
            if (e.type == PipelineExecEvent::Type::Done) done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
        };
        executor->Execute("removebg", graph, removeBg->id, {}, cachedOnly);
        ASSERT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 20.0f));
        ASSERT_TRUE(fatal.IsEmpty()) << fatal;
    };

    Ref<Bitmap> output;
    run(false, output);
    EXPECT_EQ(calls, 1) << "one generation";
    EXPECT_TRUE(sentPrompt.Contains("#00B140")) << "the model is asked for the key colour";
    ASSERT_TRUE(output);
    EXPECT_EQ(At(*output, 2, 2).a, 0) << "the key colour is cut";
    EXPECT_EQ(At(*output, 32, 32), Color4(220, 30, 30, 255)) << "the subject stays as it was";

    // The stored generation is the raw render: the cut is the executor's alone, made once on the way out
    auto upstream = graph.UpstreamSignatures(*removeBg, graph.ComputeSignatures());
    String sig = PipelineExecutor::ContentSignature(*removeBg, upstream, -1);
    String stored = PipelineUtils::ReadFileBytes(PipelineExecutor::GetContentPath("removebg", sig, "png"));
    ASSERT_FALSE(stored.IsEmpty()) << "the render is cached under the chroma key";
    auto raw = DecodeImageBytes(stored);
    ASSERT_TRUE(raw);
    EXPECT_EQ(At(*raw, 2, 2), Color4(0, 177, 64, 255));
    auto once = PipelineTransparency::ApplyChromaPostStep(*removeBg, *raw);
    EXPECT_EQ(once->GetSize(), output->GetSize());
    EXPECT_EQ(At(*once, 20, 20), At(*output, 20, 20));
    EXPECT_EQ(At(*once, 16, 16), At(*output, 16, 16));

    // A slider moved re-cuts the cached render: no new generation, a run that may only reuse
    removeBg->SetConfigNumber("chromaTolerance", 60.0f);
    Ref<Bitmap> recut;
    run(true, recut);
    EXPECT_EQ(calls, 1) << "no generation for a cut setting";
    ASSERT_TRUE(recut);
    EXPECT_EQ(At(*recut, 2, 2).a, 0);
    EXPECT_EQ(At(*recut, 32, 32).a, 255);
}
