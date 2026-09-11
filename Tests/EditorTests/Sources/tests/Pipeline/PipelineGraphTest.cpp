#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

namespace
{
    Ref<PipelineNode> AddNode(PipelineGraph& graph, const String& type, const Vec2F& pos)
    {
        auto node = PipelineNodeRegistry::CreateNode(type, pos);
        graph.nodes.Add(node);
        return node;
    }

    Ref<PipelineEdge> Connect(PipelineGraph& graph, const Ref<PipelineNode>& from, const String& outName,
                              const Ref<PipelineNode>& to, const String& inName)
    {
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = from->id;
        edge->fromPortId = from->outputs.Find([&](const PipelinePort& p) { return p.name == outName; })->id;
        edge->toNodeId = to->id;
        edge->toPortId = to->inputs.Find([&](const PipelinePort& p) { return p.name == inName; })->id;
        graph.edges.Add(edge);
        return edge;
    }
}

TEST(PipelineGraph, RegistryHasAllAssetsLineNodes)
{
    const char* types[] = {
        "finishImage", "finishText", "finishVideo", "finishAudio", "sourceText", "sourceImage", "sourceAudio",
        "textCompose", "textConcat", "aiText", "textEdit", "promptGen", "nanoBananaGen", "videoGen", "sfxGen",
        "ttsSpeech", "musicGen", "imageEdit", "imageExtract", "imageOutline", "imageShadow", "imageGradient",
        "imageColor", "removeBackground", "drawImage", "audioProcess", "composer"
    };
    for (auto type : types)
        EXPECT_TRUE(PipelineNodeRegistry::Get(type) != nullptr) << type;
}

TEST(PipelineGraph, SerializationRoundTripKeepsConfigAndEdges)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText", Vec2F(10, 20));
    source->SetConfigString("text", "hello {name}");
    auto compose = AddNode(graph, "textCompose", Vec2F(300, 20));
    compose->SetCustomInputs({ PipelinePort("cid", "name", PipelinePortType::Text, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(compose);
    Connect(graph, source, "out", compose, "template");
    graph.edges[0]->points.Add(Vec2F(150, 40));

    // The asset document travels through text the same way the .pipeline file does
    PipelineAsset asset;
    graph.SaveToAsset(asset);
    String text = asset.document.SaveAsString();

    PipelineAsset loaded;
    ASSERT_TRUE(loaded.document.LoadFromData(text));
    PipelineGraph restored;
    restored.LoadFromAsset(loaded);

    ASSERT_EQ(restored.nodes.Count(), 2);
    ASSERT_EQ(restored.edges.Count(), 1);
    EXPECT_EQ(restored.nodes[0]->GetConfigString("text"), "hello {name}");
    EXPECT_EQ(restored.nodes[1]->GetCustomInputs().Count(), 1);
    EXPECT_EQ(restored.nodes[1]->GetCustomInputs()[0].name, "name");
    EXPECT_EQ(restored.nodes[1]->inputs.Count(), 2);
    EXPECT_EQ(restored.edges[0]->points.Count(), 1);
    EXPECT_EQ(restored.edges[0]->fromNodeId, source->id);

    // Saving a smaller graph into the same asset replaces the old one instead of merging into it
    PipelineGraph smaller;
    AddNode(smaller, "sourceText", Vec2F());
    smaller.SaveToAsset(asset);
    PipelineGraph replaced;
    replaced.LoadFromAsset(asset);
    EXPECT_EQ(replaced.nodes.Count(), 1);
    EXPECT_EQ(replaced.edges.Count(), 0);
}

TEST(PipelineGraph, ValidateReportsTypeMismatchAndCycles)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    auto gen = AddNode(graph, "nanoBananaGen", Vec2F());
    auto edge = Connect(graph, text, "out", gen, "reference"); // text -> image port
    auto errors = graph.Validate();
    EXPECT_FALSE(errors.IsEmpty());
    graph.RemoveEdge(edge->id);
    EXPECT_TRUE(graph.Validate().IsEmpty());

    auto edit = AddNode(graph, "textEdit", Vec2F());
    auto concat = AddNode(graph, "textConcat", Vec2F());
    concat->SetCustomInputs({ PipelinePort("a", "a", PipelinePortType::Text, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(concat);
    Connect(graph, edit, "out", concat, "a");
    EXPECT_TRUE(graph.WouldMakeCycle(concat->id, edit->id));
    Connect(graph, concat, "out", edit, "text");
    EXPECT_FALSE(graph.Validate().IsEmpty());
}

TEST(PipelineGraph, SignaturesFollowConfigAndUpstream)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    text->SetConfigString("text", "a");
    auto edit = AddNode(graph, "textEdit", Vec2F());
    Connect(graph, text, "out", edit, "text");

    auto sigs1 = graph.ComputeSignatures();
    text->SetConfigString("text", "b");
    auto sigs2 = graph.ComputeSignatures();
    EXPECT_NE(sigs1[text->id], sigs2[text->id]);
    EXPECT_NE(sigs1[edit->id], sigs2[edit->id]);

    // UI-only keys never change a signature
    edit->SetConfigBool("drawOver", true);
    auto sigs3 = graph.ComputeSignatures();
    EXPECT_EQ(sigs2[edit->id], sigs3[edit->id]);
}

TEST(PipelineGraph, SeedInheritsAlongImageChain)
{
    PipelineGraph graph;
    auto gen = AddNode(graph, "nanoBananaGen", Vec2F());
    gen->SetConfigNumber("seed", 42);
    auto edit = AddNode(graph, "imageEdit", Vec2F());
    Connect(graph, gen, "out", edit, "image");

    auto seeds = graph.ResolveSeeds();
    EXPECT_EQ(seeds[gen->id], 42);
    EXPECT_EQ(seeds[edit->id], 42);

    edit->SetConfigBool("inheritSeed", false);
    edit->SetConfigNumber("seed", 7);
    seeds = graph.ResolveSeeds();
    EXPECT_EQ(seeds[edit->id], 7);
}

TEST(PipelineUtils, Base64AndHashing)
{
    String bytes(std::string("\x00\x01\x02\xff hello", 9));
    String encoded = PipelineUtils::Base64Encode(bytes);
    EXPECT_EQ(PipelineUtils::Base64Decode(encoded), bytes);
    EXPECT_EQ(PipelineUtils::Base64Encode("Man"), "TWFu");

    // Known SHA-256 of "abc"
    String digest = PipelineUtils::Sha256("abc");
    EXPECT_EQ((unsigned char)digest[0], 0xba);
    EXPECT_EQ((unsigned char)digest[31], 0xad);

    EXPECT_EQ(PipelineUtils::ClampPromptChars("one. two. three four five", 12), "one. two.");
    Color4 color;
    EXPECT_TRUE(PipelineUtils::ParseHexColor("#00b140", color));
    EXPECT_EQ(color.g, 177);
    EXPECT_EQ(PipelineUtils::ColorName(color), "green");
}

TEST(PipelineGraph, CacheIdTravelsWithTheGraph)
{
    PipelineGraph graph;
    graph.id = "cache-key";
    AddNode(graph, "sourceText", Vec2F());

    PipelineAsset asset;
    graph.SaveToAsset(asset);
    PipelineGraph restored;
    restored.LoadFromAsset(asset);
    EXPECT_EQ(restored.id, String("cache-key"));

    PipelineGraph copy = restored;
    EXPECT_EQ(copy.id, String("cache-key"));

    // An asset written before the id existed loads with an empty one
    PipelineAsset legacy;
    legacy.document["graph"]["nodes"].SetArray();
    legacy.document["graph"]["edges"].SetArray();
    PipelineGraph old;
    old.id = "stale";
    old.LoadFromAsset(legacy);
    EXPECT_TRUE(old.id.IsEmpty());
}

namespace
{
    // Every node whose result something else consumes, plus the targets themselves
    Vector<String> CoveredByRun(const PipelineGraph& graph)
    {
        Vector<String> covered;
        Vector<String> stack = graph.GetRunTargets();
        while (!stack.IsEmpty())
        {
            String id = stack.PopBack();
            if (covered.Contains(id))
                continue;

            covered.Add(id);
            for (auto& edge : graph.GetIncomingEdges(id))
                stack.Add(edge->fromNodeId);
        }
        return covered;
    }
}

// Run all targets every branch end: a graph with one finish node still has chains hanging off other ends
TEST(PipelineGraph, RunTargetsAreEveryBranchEnd)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText", Vec2F());
    auto edit = AddNode(graph, "textEdit", Vec2F(300, 0));
    auto finish = AddNode(graph, "finishText", Vec2F(600, 0));
    auto sideA = AddNode(graph, "textEdit", Vec2F(300, 200));
    auto sideB = AddNode(graph, "textEdit", Vec2F(600, 200));
    auto lonelySource = AddNode(graph, "sourceImage", Vec2F(0, 400));
    Connect(graph, source, "out", edit, "text");
    Connect(graph, edit, "out", finish, "in");
    Connect(graph, source, "out", sideA, "text");
    Connect(graph, sideA, "out", sideB, "text");

    auto targets = graph.GetRunTargets();
    ASSERT_EQ(targets.Count(), 2);
    EXPECT_EQ(targets[0], finish->id) << "finish nodes run first";
    EXPECT_EQ(targets[1], sideB->id);

    // A source nobody consumes has nothing to compute; everything else is reached
    auto covered = CoveredByRun(graph);
    EXPECT_EQ(covered.Count(), graph.nodes.Count() - 1);
    EXPECT_FALSE(covered.Contains(lonelySource->id));
    for (auto& node : graph.nodes)
        EXPECT_TRUE(covered.Contains(node->id) || node->id == lonelySource->id) << node->nodeType;
}

// Checks a real pipeline file (O2_PIPELINE_BIG_ASSET=<.pipeline>): a whole-graph run must reach all of it
TEST(PipelineGraph, RunTargetsCoverAWholeRealPipeline)
{
    const char* path = getenv("O2_PIPELINE_BIG_ASSET");
    if (!path)
        GTEST_SKIP() << "set O2_PIPELINE_BIG_ASSET to a .pipeline file";

    PipelineAsset asset;
    ASSERT_TRUE(asset.document.LoadFromFile(path)) << path;
    PipelineGraph graph;
    graph.LoadFromAsset(asset);
    ASSERT_FALSE(graph.nodes.IsEmpty());

    auto covered = CoveredByRun(graph);
    Vector<String> missed;
    for (auto& node : graph.nodes)
    {
        auto schema = PipelineNodeRegistry::GetSchema(node->nodeType);
        bool unusedSource = schema && schema->category == PipelineNodeCategory::Source && graph.GetOutgoingEdges(node->id).IsEmpty();
        if (!covered.Contains(node->id) && !unusedSource)
            missed.Add(node->nodeType);
    }
    printf("[run] %d nodes, %d targets, %d covered, %d missed\n", graph.nodes.Count(), graph.GetRunTargets().Count(), covered.Count(), missed.Count());
    EXPECT_TRUE(missed.IsEmpty()) << missed.Count() << " nodes never run, first: " << (missed.IsEmpty() ? String() : missed[0]);
}
