#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

namespace
{
    Ref<PipelineNode> MakeExtractNode()
    {
        return PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    }

    PipelineExtractRegion MakeRegion(const String& id, const String& name, float x, float y, float w, float h)
    {
        PipelineExtractRegion region;
        region.id = id;
        region.name = name;
        region.x = x; region.y = y; region.w = w; region.h = h;
        return region;
    }
}

TEST(PipelineRegions, NodeWithoutRegionsKeepsItsPromptAndRoiAsOnePart)
{
    auto node = MakeExtractNode();
    ASSERT_EQ(node->outputs.Count(), 1);
    node->SetConfigString("prompt", "gold coin");
    node->RemoveConfig("roi");
    auto& roi = node->config["roi"];
    roi.SetObject();
    roi["x"] = 0.25f;
    roi["y"] = 0.5f;
    roi["w"] = 0.25f;
    roi["h"] = 0.25f;

    auto regions = PipelineRegions::Read(*node);
    ASSERT_EQ(regions.Count(), 1);
    // The legacy part keeps the existing port id, so the links of an imported pipeline survive
    EXPECT_EQ(regions[0].id, node->outputs[0].id);
    EXPECT_EQ(regions[0].name, "gold coin");
    EXPECT_NEAR(regions[0].x, 0.25f, 0.0001f);
    EXPECT_NEAR(regions[0].h, 0.25f, 0.0001f);
}

TEST(PipelineRegions, WrittenRegionsComeBackAndNameThePortsUniquely)
{
    auto node = MakeExtractNode();
    Vector<PipelineExtractRegion> regions = {
        MakeRegion("a", "coin", 0.0f, 0.0f, 0.5f, 0.5f),
        MakeRegion("b", "coin", 0.5f, 0.0f, 0.5f, 0.5f),
        MakeRegion("c", "", 0.0f, 0.5f, 0.5f, 0.5f)
    };
    PipelineRegions::Write(*node, regions);

    auto read = PipelineRegions::Read(*node);
    ASSERT_EQ(read.Count(), 3);
    EXPECT_EQ(read[1].id, "b");
    EXPECT_NEAR(read[1].x, 0.5f, 0.0001f);

    auto names = PipelineRegions::PortNames(read);
    ASSERT_EQ(names.Count(), 3);
    EXPECT_EQ(names[0], "coin");
    EXPECT_EQ(names[1], "coin 2");
    EXPECT_EQ(names[2], "part 3");
}

TEST(PipelineRegions, SyncPortsKeepsPortIdsWhenAPartIsRenamed)
{
    auto node = MakeExtractNode();
    PipelineRegions::Write(*node, { MakeRegion("a", "coin", 0, 0, 0.5f, 1), MakeRegion("b", "chest", 0.5f, 0, 0.5f, 1) });
    PipelineRegions::SyncPorts(*node);

    ASSERT_EQ(node->outputs.Count(), 2);
    EXPECT_EQ(node->outputs[0].id, "a");
    EXPECT_EQ(node->outputs[1].name, "chest");

    auto regions = PipelineRegions::Read(*node);
    regions[1].name = "treasure chest";
    PipelineRegions::Write(*node, regions);
    PipelineRegions::SyncPorts(*node);

    ASSERT_EQ(node->outputs.Count(), 2);
    EXPECT_EQ(node->outputs[1].id, "b");
    EXPECT_EQ(node->outputs[1].name, "treasure chest");
}

TEST(PipelineRegions, RegionOfPortFallsBackToTheFirstPart)
{
    auto node = MakeExtractNode();
    PipelineRegions::Write(*node, { MakeRegion("a", "coin", 0, 0, 0.5f, 1), MakeRegion("b", "chest", 0.5f, 0, 0.5f, 1) });
    PipelineRegions::SyncPorts(*node);

    EXPECT_EQ(PipelineRegions::RegionOfPort(*node, "b").name, "chest");
    EXPECT_EQ(PipelineRegions::RegionOfPort(*node, "gone").name, "coin");
}

TEST(PipelineRegions, ParsesGeminiBoxesPadsThemAndSkipsWholeImageParts)
{
    String answer =
        "```json\n"
        "[{\"name\": \"gold coin\", \"box_2d\": [100, 200, 300, 500]},"
        " {\"name\": \"whole screen\", \"box_2d\": [0, 0, 1000, 1000]},"
        " {\"name\": \"\", \"box_2d\": [10, 10, 20, 20]},"
        " {\"name\": \"health bar\", \"box_2d\": [700, 100, 760, 400]}]\n"
        "```";

    auto regions = PipelineRegions::ParseAutoSplit(answer);
    ASSERT_EQ(regions.Count(), 2);
    EXPECT_EQ(regions[0].name, "gold coin");
    EXPECT_EQ(regions[1].name, "health bar");
    EXPECT_FALSE(regions[0].id.IsEmpty());
    EXPECT_NE(regions[0].id, regions[1].id);

    // box_2d is [ymin, xmin, ymax, xmax] on a 0..1000 grid, grown by a share of the part's own size
    EXPECT_GT(regions[0].x, 0.18f);
    EXPECT_LT(regions[0].x, 0.2f);
    EXPECT_GT(regions[0].y, 0.08f);
    EXPECT_LT(regions[0].y, 0.1f);
    EXPECT_GT(regions[0].w, 0.3f);
    EXPECT_LT(regions[0].w, 0.34f);
}

TEST(PipelineRegions, RefusesAnAnswerThatIsNotARegionList)
{
    EXPECT_TRUE(PipelineRegions::ParseAutoSplit("I could not find any parts").IsEmpty());
    EXPECT_TRUE(PipelineRegions::ParseAutoSplit("[]").IsEmpty());
}

// A long non-ASCII part name used to be cut in the middle of a letter, and the card built from it threw
TEST(PipelineRegions, LongNonAsciiNamesAreCutOnCharacterBoundaries)
{
    auto node = MakeExtractNode();
    String name = "UI подложка под кол-во очков, с пустым прогресс-баром. Без текста";
    node->SetConfigString("prompt", name);

    auto regions = PipelineRegions::Read(*node);
    ASSERT_EQ(regions.Count(), 1);
    EXPECT_EQ(regions[0].name, name);

    auto names = PipelineRegions::PortNames(regions);
    ASSERT_EQ(names.Count(), 1);
    EXPECT_LE(names[0].Length(), 40);
    EXPECT_TRUE(name.StartsWith(names[0]));
    EXPECT_NO_THROW({ WString wide = names[0]; (void)wide; });

    PipelineRegions::SyncPorts(*node);
    ASSERT_EQ(node->outputs.Count(), 1);
    EXPECT_NO_THROW({ WString wide = node->outputs[0].name; (void)wide; });

    // The same cut in the auto-split answer
    auto parsed = PipelineRegions::ParseAutoSplit(
        R"([{"name": "очень длинное название элемента интерфейса для проверки обрезки", "box_2d": [100, 100, 300, 300]}])");
    ASSERT_EQ(parsed.Count(), 1);
    EXPECT_LE(parsed[0].name.Length(), 60);
    EXPECT_NO_THROW({ WString wide = parsed[0].name; (void)wide; });
}

// A node from before regions existed has the one output AssetsLine gives it, "out": a port named after the
// prompt was not found there by that name and got a phantom "out" added beside it
TEST(PipelineRegions, NodeWithoutRegionsKeepsOneOutputNamedOut)
{
    auto node = MakeExtractNode();
    node->SetConfigString("prompt", "gold coin");
    String schemaPort = node->outputs[0].id;
    PipelineRegions::SyncPorts(*node);
    ASSERT_EQ(node->outputs.Count(), 1);
    EXPECT_EQ(node->outputs[0].id, schemaPort);
    EXPECT_EQ(node->outputs[0].name, "out");

    // An older o2 named it after the prompt: renamed, the id and so the links stay
    node->outputs = { PipelinePort("keep", "gold coin", PipelinePortType::Image, false) };
    PipelineRegions::SyncPorts(*node);
    ASSERT_EQ(node->outputs.Count(), 1);
    EXPECT_EQ(node->outputs[0].id, "keep");
    EXPECT_EQ(node->outputs[0].name, "out");

    // The phantom the web editor added beside it has no links and goes, also when the editor syncs the schema
    node->outputs = { PipelinePort("keep", "gold coin", PipelinePortType::Image, false), PipelinePort("phantom", "out", PipelinePortType::Image, false) };
    PipelineNodeRegistry::SyncNodeWithSchema(node);
    ASSERT_EQ(node->outputs.Count(), 1);
    EXPECT_EQ(node->outputs[0].id, "keep");
    EXPECT_EQ(node->outputs[0].name, "out");

    auto legacy = PipelineRegions::Read(*node);
    ASSERT_EQ(legacy.Count(), 1);
    EXPECT_EQ(legacy[0].id, "keep");
    EXPECT_EQ(legacy[0].name, "gold coin");

    // A node without any output gets one
    node->outputs.Clear();
    PipelineRegions::SyncPorts(*node);
    ASSERT_EQ(node->outputs.Count(), 1);
    EXPECT_FALSE(node->outputs[0].id.IsEmpty());
    EXPECT_EQ(node->outputs[0].name, "out");

    // A second part writes the region list: from then on the ports are named after the parts
    node->outputs = { PipelinePort("keep", "out", PipelinePortType::Image, false) };
    auto regions = PipelineRegions::Read(*node);
    regions.Add(MakeRegion("b", "chest", 0.5f, 0, 0.5f, 1));
    PipelineRegions::Write(*node, regions);
    PipelineNodeRegistry::SyncNodeWithSchema(node);
    ASSERT_EQ(node->outputs.Count(), 2);
    EXPECT_EQ(node->outputs[0].id, "keep");
    EXPECT_EQ(node->outputs[0].name, "gold coin");
    EXPECT_EQ(node->outputs[1].id, "b");
    EXPECT_EQ(node->outputs[1].name, "chest");
}

// The result of a node without regions leaves through its "out" port and reaches the node linked to it. The part's
// render is seeded into the content cache and the run is cached-only, so no provider is asked
TEST(PipelineRegions, NodeWithoutRegionsHandsItsResultThroughOut)
{
    String relative = "./pipeline-legacy-extract-" + (String)(int)Math::Random(0, 1000000);
    o2FileSystem.FolderCreate(relative, true);
    String work = o2FileSystem.CanonicalizePath(relative) + "/";
    PipelineUtils::SetWorkPathOverride(work);
    o2FileSystem.FolderCreate(PipelineUtils::GetUploadsPath(), true);
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("sheet.png"),
                                  PipelineValue::Image(PipelineImageOps::Blank(32, 32, Color4(90, 90, 90, 255))).GetPngBytes());

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "sheet.png");
    auto extract = MakeExtractNode();
    extract->SetConfigString("prompt", "gold coin");
    extract->outputs = { PipelinePort("keep", "gold coin", PipelinePortType::Image, false), PipelinePort("phantom", "out", PipelinePortType::Image, false) };
    auto invert = PipelineNodeRegistry::CreateNode("imageColor", Vec2F());
    invert->SetConfigBool("invert", true);
    graph.nodes = { source, extract, invert };
    PipelineNodeRegistry::SyncNodeWithSchema(extract);

    auto link = [&](const Ref<PipelineNode>& from, const String& fromPort, const Ref<PipelineNode>& to)
    {
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = from->id;
        edge->fromPortId = fromPort;
        edge->toNodeId = to->id;
        edge->toPortId = to->inputs[0].id;
        graph.edges.Add(edge);
    };
    link(source, source->outputs[0].id, extract);
    link(extract, "keep", invert);

    auto upstream = graph.UpstreamSignatures(*extract, graph.ComputeSignatures());
    int seed = graph.ResolveSeeds()[extract->id];
    PipelineExecutor::SaveContent("legacy", PipelineExecutor::PortSignature(*extract, upstream, seed, "keep"),
                                  PipelineValue::Image(PipelineImageOps::Blank(8, 8, Color4(255, 200, 0, 255))));

    PipelineValue part, inverted;
    bool done = false;
    String fatal;
    auto executor = mmake<PipelineExecutor>();
    executor->onEvent = [&](const PipelineExecEvent& e)
    {
        if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == extract->id && e.portId == "keep") part = e.value;
        if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == invert->id) inverted = e.value;
        if (e.type == PipelineExecEvent::Type::Done) done = true;
        if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
    };
    executor->Execute("legacy", graph, invert->id, {}, true);
    EXPECT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 20.0f));
    EXPECT_TRUE(fatal.IsEmpty()) << fatal;

    EXPECT_EQ(extract->outputs[0].name, "out");
    EXPECT_TRUE(part.IsImage());
    ASSERT_TRUE(inverted.IsImage());
    const UInt8* pixel = PipelineImageOps::Pixel(*inverted.GetBitmap(), 4, 4);
    EXPECT_EQ((int)pixel[0], 0);
    EXPECT_EQ((int)pixel[1], 55);
    EXPECT_EQ((int)pixel[2], 255);

    PipelineUtils::SetWorkPathOverride("");
    o2FileSystem.FolderRemove(work, true);
}
