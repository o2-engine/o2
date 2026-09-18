#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-perport-work-" + (String)(int)Math::Random(0, 1000000);
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

    // Runs of the test node by part name, so a cache hit is visible as a missing run
    Map<String, int> partRuns;

    // A node shaped like the extract node: its outputs come from its own config, each is run and
    // cached on its own, and the part list is kept out of the shared signature. A part named
    // "fail" refuses, standing in for one provider call going wrong
    class TestPerPortNode : public IPipelineNodeImpl
    {
    public:
        TestPerPortNode()
        {
            mSchema.type = "testPerPort";
            mSchema.label = "Test per port";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.perPortRun = true;
            mSchema.inputs = { PipelinePort("text", "text", PipelinePortType::Text, false) };
            mSchema.outputs = { PipelinePort("out", "out", PipelinePortType::Text, false) };
        }

        const PipelineNodeSchema& GetSchema() const override { return mSchema; }

        Vector<String> PortCacheExcludedKeys() const override { return { "parts" }; }

        String PortCacheVariant(const PipelineNode& node, const String& portId) const override
        {
            return "part:" + PartOfPort(node, portId);
        }

        bool SyncPorts(const Ref<PipelineNode>& node) const override
        {
            auto parts = Parts(*node);
            Vector<PipelinePort> ports;
            for (int i = 0; i < parts.Count(); i++)
            {
                String id = i < node->outputs.Count() ? node->outputs[i].id : PipelineNode::GenerateId();
                ports.Add(PipelinePort(id, parts[i], PipelinePortType::Text, false));
            }
            node->outputs = ports;
            return true;
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            String part = PartOfPort(*node, ctx->outputPortId);
            partRuns[part] = partRuns.ContainsKey(part) ? partRuns[part] + 1 : 1;
            if (part == "fail")
                co_return PipelineRunResult::Fail("part \"fail\" always fails");

            PipelineValue in;
            inputs.TryGetValue("text", in);
            co_return PipelineRunResult::Single(PipelineValue::Text(in.data + "|" + part), ctx->outputPort);
        }

        static Vector<String> Parts(const PipelineNode& node)
        {
            return node.GetConfigString("parts", "out").Split(",");
        }

        static String PartOfPort(const PipelineNode& node, const String& portId)
        {
            auto port = node.outputs.FindOrDefault([&](const PipelinePort& p) { return p.id == portId; });
            return port.id.IsEmpty() ? (node.outputs.IsEmpty() ? String("out") : node.outputs[0].name) : port.name;
        }

    private:
        PipelineNodeSchema mSchema;
    };

    struct PerPortFixture : ::testing::Test
    {
        WorkDirGuard work;

        void SetUp() override
        {
            // Touches the registry first, so registering the test type does not race the built-ins
            PipelineNodeRegistry::Get("sourceText");
            PipelineNodeRegistry::Register(mmake<TestPerPortNode>());
            partRuns.Clear();
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
        Map<String, String> states;

        // Returns the node errors of the run, for a readable assertion message
        String Errors() const
        {
            String text = fatal;
            for (auto& state : states)
            {
                if (state.second.Contains("error"))
                    text += " [" + state.first + ": " + state.second + "]";
            }
            return text;
        }

        // Returns the value the node produced on the port, invalid when it produced none
        PipelineValue PortOutput(const String& nodeId, const String& portId) const
        {
            for (auto& event : events)
            {
                if (event.type == PipelineExecEvent::Type::NodeOutput && event.nodeId == nodeId && event.portId == portId)
                    return event.value;
            }
            return PipelineValue();
        }
    };

    RunResult RunPipeline(const PipelineGraph& graph, const String& target, const String& pipelineId = "perport",
                          const Vector<String>& bypass = {})
    {
        RunResult result;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            result.events.Add(e);
            if (e.type == PipelineExecEvent::Type::NodeState) result.states[e.nodeId] = e.state + (e.error.IsEmpty() ? String() : ": " + e.error);
            if (e.type == PipelineExecEvent::Type::Done) result.done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) result.fatal = e.error;
        };
        executor->Execute(pipelineId, graph, target, bypass, false);
        EXPECT_TRUE(NetPumpUntil([&] { return result.done || !result.fatal.IsEmpty(); }, 20.0f));
        return result;
    }

    Ref<PipelineNode> AddPerPortNode(PipelineGraph& graph, const String& parts)
    {
        auto node = AddNode(graph, "testPerPort");
        node->SetConfigString("parts", parts);
        PipelineNodeRegistry::SyncNodeWithSchema(node);
        return node;
    }
}

TEST_F(PerPortFixture, EveryOutputIsRunCachedAndPreviewedOnItsOwn)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText");
    source->SetConfigString("text", "src");
    auto parts = AddPerPortNode(graph, "coin,chest");
    Connect(graph, source, "out", parts, "text");

    auto result = RunPipeline(graph, parts->id);
    ASSERT_TRUE(result.done) << result.Errors();
    EXPECT_EQ(result.states[parts->id], "done");
    EXPECT_EQ(partRuns["coin"], 1);
    EXPECT_EQ(partRuns["chest"], 1);
    EXPECT_EQ(result.PortOutput(parts->id, parts->outputs[0].id).data, "src|coin");
    EXPECT_EQ(result.PortOutput(parts->id, parts->outputs[1].id).data, "src|chest");

    // Each part keeps its own preview, so a reopened pipeline shows every part again
    EXPECT_EQ(PipelineExecutor::LoadPortPreview("perport", parts->id, parts->outputs[0].id, PipelinePortType::Text).data, "src|coin");
    EXPECT_EQ(PipelineExecutor::LoadPortPreview("perport", parts->id, parts->outputs[1].id, PipelinePortType::Text).data, "src|chest");

    auto second = RunPipeline(graph, parts->id);
    ASSERT_TRUE(second.done) << second.Errors();
    EXPECT_EQ(partRuns["coin"], 1);
    EXPECT_EQ(partRuns["chest"], 1);
}

TEST_F(PerPortFixture, RenamingOnePartLeavesTheOthersCached)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText");
    source->SetConfigString("text", "src");
    auto parts = AddPerPortNode(graph, "coin,chest");
    Connect(graph, source, "out", parts, "text");

    ASSERT_TRUE(RunPipeline(graph, parts->id).done);
    EXPECT_EQ(partRuns["coin"], 1);

    parts->SetConfigString("parts", "coin,treasure");
    PipelineNodeRegistry::SyncNodeWithSchema(parts);

    auto result = RunPipeline(graph, parts->id);
    ASSERT_TRUE(result.done) << result.Errors();
    EXPECT_EQ(partRuns["coin"], 1);
    EXPECT_EQ(partRuns["treasure"], 1);
    EXPECT_EQ(result.PortOutput(parts->id, parts->outputs[0].id).data, "src|coin");
}

TEST_F(PerPortFixture, OnlyTheOutputsTheBranchConsumesAreProduced)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText");
    source->SetConfigString("text", "src");
    auto parts = AddPerPortNode(graph, "coin,chest");
    auto downstream = AddPerPortNode(graph, "sprite");
    Connect(graph, source, "out", parts, "text");
    Connect(graph, parts, "chest", downstream, "text");

    auto result = RunPipeline(graph, downstream->id);
    ASSERT_TRUE(result.done) << result.Errors();
    EXPECT_EQ(partRuns["chest"], 1);
    EXPECT_FALSE(partRuns.ContainsKey("coin"));
    EXPECT_EQ(result.PortOutput(downstream->id, downstream->outputs[0].id).data, "src|chest|sprite");
}

TEST_F(PerPortFixture, OnePartFailingLeavesTheOthersInPlace)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText");
    source->SetConfigString("text", "src");
    auto parts = AddPerPortNode(graph, "coin,fail");
    Connect(graph, source, "out", parts, "text");

    auto result = RunPipeline(graph, parts->id);
    ASSERT_TRUE(result.done) << result.Errors();
    EXPECT_EQ(result.states[parts->id], "done");
    EXPECT_EQ(result.PortOutput(parts->id, parts->outputs[0].id).data, "src|coin");
    EXPECT_FALSE(result.PortOutput(parts->id, parts->outputs[1].id).IsValid());
}

// "<node>#<port>" in the bypass list regenerates that part and leaves the others in the cache
TEST_F(PerPortFixture, OnePartIsRegeneratedAlone)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText");
    source->SetConfigString("text", "src");
    auto parts = AddPerPortNode(graph, "coin,chest");
    Connect(graph, source, "out", parts, "text");

    ASSERT_TRUE(RunPipeline(graph, parts->id).done);
    EXPECT_EQ(partRuns["coin"], 1);
    EXPECT_EQ(partRuns["chest"], 1);

    auto result = RunPipeline(graph, parts->id, "perport", { parts->id + "#" + parts->outputs[1].id });
    ASSERT_TRUE(result.done) << result.Errors();
    EXPECT_EQ(partRuns["coin"], 1);
    EXPECT_EQ(partRuns["chest"], 2);
}
