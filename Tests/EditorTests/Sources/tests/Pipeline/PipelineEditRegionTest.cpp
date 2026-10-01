#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineEditRegion.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"

using namespace o2;
using namespace Editor;

// Stubbed provider runs of the AI image nodes: the image edit's region (validation, context box, feathered paste) and
// the extract's single reference

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-region-work-" + (String)(int)Math::Random(0, 1000000);
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

    bool ReadRegion(const String& json, PipelineImageOps::CropRect& region)
    {
        DataDocument doc;
        doc.LoadFromData(json);
        return PipelineEditRegion::Read(&doc, region);
    }

    Color4 At(const Bitmap& bitmap, int x, int y)
    {
        const UInt8* p = PipelineImageOps::Pixel(bitmap, x, y);
        return Color4(p[0], p[1], p[2], p[3]);
    }

    PipelineEditRegion::Boxes SampleBoxes()
    {
        PipelineImageOps::CropRect region;
        region.x = 0.25f; region.y = 0.25f; region.w = 0.5f; region.h = 0.5f;
        return PipelineEditRegion::PixelBoxes(Vec2I(200, 100), region);
    }
}

TEST(PipelineEditRegion, ARegionIsValidatedAndClamped)
{
    PipelineImageOps::CropRect region;
    EXPECT_FALSE(PipelineEditRegion::Read(nullptr, region));
    EXPECT_FALSE(ReadRegion("null", region));
    EXPECT_FALSE(ReadRegion("{\"x\": 0.1, \"y\": 0.1, \"w\": 0.5}", region)) << "all four numbers";
    EXPECT_FALSE(ReadRegion("{\"x\": \"a\", \"y\": 0.1, \"w\": 0.5, \"h\": 0.5}", region));
    EXPECT_FALSE(ReadRegion("{\"x\": 0.1, \"y\": 0.1, \"w\": 0.01, \"h\": 0.5}", region)) << "narrower than 0.02";
    EXPECT_FALSE(ReadRegion("{\"x\": 0.99, \"y\": 0.1, \"w\": 0.5, \"h\": 0.5}", region)) << "clamped to what is left";

    ASSERT_TRUE(ReadRegion("{\"x\": -0.2, \"y\": 0.5, \"w\": 0.4, \"h\": 0.9}", region));
    EXPECT_FLOAT_EQ(region.x, 0.0f);
    EXPECT_FLOAT_EQ(region.w, 0.4f);
    EXPECT_FLOAT_EQ(region.y, 0.5f);
    EXPECT_FLOAT_EQ(region.h, 0.5f);

    auto edit = PipelineNodeRegistry::CreateNode("imageEdit", Vec2F());
    EXPECT_FALSE(PipelineEditRegion::Of(*edit, region));
    auto& value = edit->config["editRegion"];
    value.SetObject();
    value["x"] = 0.25f; value["y"] = 0.25f; value["w"] = 0.5f; value["h"] = 0.5f;
    EXPECT_TRUE(PipelineEditRegion::Of(*edit, region));

    auto gen = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
    gen->config["editRegion"] = edit->config["editRegion"];
    EXPECT_FALSE(PipelineEditRegion::Of(*gen, region)) << "only the image edit node has one";
}

TEST(PipelineEditRegion, TheContextBoxGrowsTheRegionByItsMargin)
{
    auto b = SampleBoxes();
    EXPECT_EQ(b.region.left, 50);
    EXPECT_EQ(b.region.top, 25);
    EXPECT_EQ(b.region.right, 150);
    EXPECT_EQ(b.region.bottom, 75);
    // A quarter of the longer side, 25 px, clamped to the image
    EXPECT_EQ(b.box.left, 25);
    EXPECT_EQ(b.box.top, 0);
    EXPECT_EQ(b.box.right, 175);
    EXPECT_EQ(b.box.bottom, 100);

    PipelineImageOps::CropRect small;
    small.x = 0.5f; small.y = 0.5f; small.w = 0.05f; small.h = 0.05f;
    auto s = PipelineEditRegion::PixelBoxes(Vec2I(400, 400), small);
    EXPECT_EQ(s.box.left, s.region.left - 16) << "never less than 16 px";
}

TEST(PipelineEditRegion, WeightsAreOneInsideAndFallLinearlyToTheBoxBorder)
{
    auto b = SampleBoxes();
    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(50, 25, b), 1.0f);
    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(149, 74, b), 1.0f);
    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(100, 50, b), 1.0f);

    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(25, 50, b), 0.0f) << "left border of the box";
    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(174, 50, b), 0.0f) << "right border of the box";
    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(100, 0, b), 0.0f) << "top border of the box";

    EXPECT_NEAR(PipelineEditRegion::Weight(40, 50, b), 1.0f - 10.0f/25.0f, 1e-5f);
    EXPECT_NEAR(PipelineEditRegion::Weight(154, 50, b), 1.0f - 5.0f/25.0f, 1e-5f);
    EXPECT_NEAR(PipelineEditRegion::Weight(100, 15, b), 1.0f - 10.0f/25.0f, 1e-5f);
    EXPECT_NEAR(PipelineEditRegion::Weight(40, 15, b), 0.6f*0.6f, 1e-5f) << "the corners multiply";
}

TEST(PipelineEditRegion, ARegionOnTheImageEdgeHasNoMarginThere)
{
    PipelineImageOps::CropRect corner;
    corner.x = 0.0f; corner.y = 0.0f; corner.w = 0.3f; corner.h = 0.3f;
    auto b = PipelineEditRegion::PixelBoxes(Vec2I(100, 100), corner);
    EXPECT_EQ(b.box.left, 0);
    EXPECT_EQ(b.box.top, 0);
    EXPECT_EQ(b.box.right, 46);
    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(0, 0, b), 1.0f) << "the edge pixel is inside the region";
    EXPECT_FLOAT_EQ(PipelineEditRegion::Weight(45, 10, b), 0.0f);

    auto input = PipelineImageOps::Blank(100, 100, Color4(200, 30, 30, 255));
    auto edited = PipelineImageOps::Blank(20, 20, Color4(30, 30, 200, 255));
    auto out = PipelineEditRegion::PasteRegionEdit(*input, *edited, b);
    EXPECT_EQ(At(*out, 0, 0), Color4(30, 30, 200, 255));
    EXPECT_EQ(At(*out, 29, 29), Color4(30, 30, 200, 255));
}

TEST(PipelineEditRegion, ThePasteBlendsEveryChannelAndLeavesTheRestAlone)
{
    auto b = SampleBoxes();
    auto input = PipelineImageOps::Blank(200, 100, Color4(200, 30, 30, 255));
    PipelineImageOps::Pixel(*input, 5, 5)[1] = 77;
    auto edited = PipelineImageOps::Blank(64, 64, Color4(30, 30, 200, 0));

    auto out = PipelineEditRegion::PasteRegionEdit(*input, *edited, b);
    ASSERT_EQ(out->GetSize(), Vec2I(200, 100));
    EXPECT_EQ(At(*out, 100, 50), Color4(30, 30, 200, 0)) << "inside: the edit, alpha included";
    EXPECT_EQ(At(*out, 5, 5), At(*input, 5, 5)) << "outside the box: the input byte for byte";
    EXPECT_EQ(At(*out, 199, 99), At(*input, 199, 99));
    EXPECT_EQ(At(*out, 25, 50), At(*input, 25, 50)) << "weight 0 on the border";

    // 60 % of the edit ten pixels into the left margin
    Color4 mixed = At(*out, 40, 50);
    EXPECT_NEAR(mixed.r, 200*0.4f + 30*0.6f, 1.0f);
    EXPECT_NEAR(mixed.b, 30*0.4f + 200*0.6f, 1.0f);
    EXPECT_NEAR(mixed.a, 255*0.4f, 1.0f);

    // An edit without alpha counts as opaque
    auto clear = PipelineImageOps::Blank(200, 100, Color4(0, 0, 0, 0));
    auto rgb = mmake<Bitmap>(PixelFormat::R8G8B8, Vec2I(8, 8));
    memset(rgb->GetData(), 120, 8*8*3);
    auto opaque = PipelineEditRegion::PasteRegionEdit(*clear, *rgb, b);
    EXPECT_EQ(At(*opaque, 100, 50), Color4(120, 120, 120, 255));
}

// A node with a region sends the crops around it, asks once without any transparency and returns the whole image
TEST(PipelineEditRegion, TheNodeSendsTheCropsAndReturnsTheWholeImage)
{
    WorkDirGuard work;
    StubGuard stubGuard;

    auto input = PipelineImageOps::Blank(200, 100, Color4(200, 30, 30, 255));
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("region.png"), EncodeBitmapPng(*input));

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "region.png");
    auto edit = PipelineNodeRegistry::CreateNode("imageEdit", Vec2F(300, 0));
    edit->SetConfigString("prompt", "make it blue");
    edit->SetConfigBool("transparentBg", true);
    auto overlay = PipelineImageOps::Blank(200, 100);
    PipelineImageOps::Pixel(*overlay, 100, 50)[3] = 255;
    edit->SetConfigString("drawing", PipelineUtils::BytesToDataUrl(EncodeBitmapPng(*overlay), "image/png"));
    graph.nodes = { source, edit };

    auto edge = mmake<PipelineEdge>();
    edge->id = PipelineNode::GenerateId();
    edge->fromNodeId = source->id;
    edge->fromPortId = source->outputs[0].id;
    edge->toNodeId = edit->id;
    edge->toPortId = edit->inputs[0].id;
    graph.edges.Add(edge);

    EXPECT_EQ(PipelineTransparency::ImageGenerations(*edit), 2) << "two-pass without a frame";
    String signatureWithout = graph.ComputeSignatures()[edit->id];
    auto& value = edit->config["editRegion"];
    value.SetObject();
    value["x"] = 0.25f; value["y"] = 0.25f; value["w"] = 0.5f; value["h"] = 0.5f;
    EXPECT_NE(graph.ComputeSignatures()[edit->id], signatureWithout) << "the frame re-runs the node";
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*edit), 1) << "one generation with a frame";
    EXPECT_TRUE(edit->GetConfigBool("transparentBg", false)) << "the stored setting stays";
    edit->SetConfigString("transparentMode", "chroma");
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*edit)) << "no key colour cut with a frame";

    Vector<Vec2I> referenceSizes;
    String sentPrompt;
    int calls = 0;
    bool transparentAsked = false;
    AiRouter::SetImageStub([&](const String&, const String& prompt, const Vector<AiImageRef>& references, int, bool transparent,
                               const AiImageOptions&)
    {
        calls++;
        sentPrompt = prompt;
        transparentAsked = transparent;
        for (auto& ref : references)
        {
            auto bitmap = DecodeImageBytes(ref.data);
            referenceSizes.Add(bitmap ? bitmap->GetSize() : Vec2I());
        }
        AiBytesResult result;
        result.ok = true;
        result.mimeType = "image/png";
        result.data = EncodeBitmapPng(*PipelineImageOps::Blank(64, 64, Color4(30, 30, 200, 255)));
        return result;
    });

    Ref<Bitmap> output;
    bool done = false;
    String fatal;
    auto executor = mmake<PipelineExecutor>();
    executor->onEvent = [&](const PipelineExecEvent& e)
    {
        if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == edit->id && e.value.IsImage()) output = e.value.GetBitmap();
        if (e.type == PipelineExecEvent::Type::NodeState && e.state == "error") fatal = e.error;
        if (e.type == PipelineExecEvent::Type::Done) done = true;
        if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
    };
    executor->Execute("region", graph, edit->id, {}, false);
    ASSERT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 20.0f));
    ASSERT_TRUE(fatal.IsEmpty()) << fatal;

    EXPECT_EQ(calls, 1);
    EXPECT_FALSE(transparentAsked);
    ASSERT_EQ(referenceSizes.Count(), 2) << "the input and the drawing over it";
    EXPECT_EQ(referenceSizes[0], Vec2I(150, 100)) << "cropped to the context box";
    EXPECT_EQ(referenceSizes[1], Vec2I(150, 100));
    EXPECT_TRUE(sentPrompt.EndsWith(PipelineEditRegion::promptSuffix));
    EXPECT_TRUE(sentPrompt.Contains("make it blue"));
    EXPECT_FALSE(sentPrompt.Contains("white (#FFFFFF)")) << "no transparency pass";

    ASSERT_TRUE(output);
    EXPECT_EQ(output->GetSize(), Vec2I(200, 100));
    EXPECT_EQ(At(*output, 100, 50), Color4(30, 30, 200, 255));
    EXPECT_EQ(At(*output, 10, 50), Color4(200, 30, 30, 255));
}

// AI extract part has no drawing: one left in an older pipeline is not sent, and the prompt says nothing of drawn marks
TEST(PipelineExtractRun, ADrawingLeftInTheConfigIsNotSent)
{
    WorkDirGuard work;
    StubGuard stubGuard;

    auto input = PipelineImageOps::Blank(200, 100, Color4(200, 30, 30, 255));
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("extract.png"), EncodeBitmapPng(*input));

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "extract.png");
    auto extract = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F(300, 0));
    PipelineExtractRegion region;
    region.id = "coin";
    region.name = "gold coin";
    region.x = 0.25f; region.y = 0.25f; region.w = 0.5f; region.h = 0.5f;
    PipelineRegions::Write(*extract, { region });
    PipelineNodeRegistry::SyncNodeWithSchema(extract);
    auto overlay = PipelineImageOps::Blank(200, 100);
    PipelineImageOps::Pixel(*overlay, 100, 50)[3] = 255;
    extract->SetConfigString("drawing", PipelineUtils::BytesToDataUrl(EncodeBitmapPng(*overlay), "image/png"));
    graph.nodes = { source, extract };

    auto edge = mmake<PipelineEdge>();
    edge->id = PipelineNode::GenerateId();
    edge->fromNodeId = source->id;
    edge->fromPortId = source->outputs[0].id;
    edge->toNodeId = extract->id;
    edge->toPortId = extract->inputs[0].id;
    graph.edges.Add(edge);

    int calls = 0;
    int references = 0;
    Vec2I referenceSize;
    String sentPrompt;
    AiRouter::SetImageStub([&](const String&, const String& prompt, const Vector<AiImageRef>& refs, int, bool, const AiImageOptions&)
    {
        calls++;
        references = refs.Count();
        sentPrompt = prompt;
        if (auto bitmap = refs.IsEmpty() ? nullptr : DecodeImageBytes(refs[0].data))
            referenceSize = bitmap->GetSize();

        AiBytesResult result;
        result.ok = true;
        result.mimeType = "image/png";
        result.data = EncodeBitmapPng(*PipelineImageOps::Blank(64, 64, Color4(30, 30, 200, 255)));
        return result;
    });

    bool done = false;
    String fatal;
    auto executor = mmake<PipelineExecutor>();
    executor->onEvent = [&](const PipelineExecEvent& e)
    {
        if (e.type == PipelineExecEvent::Type::NodeState && e.state == "error") fatal = e.error;
        if (e.type == PipelineExecEvent::Type::Done) done = true;
        if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
    };
    executor->Execute("extract", graph, extract->id, {}, false);
    ASSERT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 20.0f));
    ASSERT_TRUE(fatal.IsEmpty()) << fatal;

    EXPECT_EQ(calls, 1);
    EXPECT_EQ(references, 1) << "the cropped source alone";
    EXPECT_EQ(referenceSize, Vec2I(100, 50)) << "cropped to the part's box";
    EXPECT_TRUE(sentPrompt.Contains("gold coin"));
    EXPECT_FALSE(sentPrompt.Contains("SECOND image"));
    EXPECT_FALSE(sentPrompt.Contains("hand-drawn"));
    EXPECT_FALSE(extract->GetConfigString("drawing", "").IsEmpty()) << "the old drawing stays in the file";
}
