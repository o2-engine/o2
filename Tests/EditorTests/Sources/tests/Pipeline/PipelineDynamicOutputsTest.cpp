#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
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
            String relative = "./pipeline-dynamic-work-" + (String)(int)(Math::Random()%1000000);
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

    int dynamicRuns = 0;
    String lastWorkDir;

    // Decides its outputs while it runs, like AI image to UI: the preview, the layout and one image per name in "sprites"
    class TestDynamicNode: public IPipelineNodeImpl
    {
    public:
        TestDynamicNode()
        {
            mSchema.type = "testDynamic";
            mSchema.label = "Test dynamic";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.dynamicOutputs = true;
            mSchema.inputs = { PipelinePort("text", "text", PipelinePortType::Text, false) };
            mSchema.outputs = { PipelinePort("preview", "preview", PipelinePortType::Image, false),
                                PipelinePort("layout", "layout", PipelinePortType::Text, false) };
        }

        const PipelineNodeSchema& GetSchema() const override { return mSchema; }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            dynamicRuns++;
            lastWorkDir = ctx->workDir;
            PipelineValue in;
            inputs.TryGetValue("text", in);

            PipelineRunResult result;
            result.outputs["preview"] = PipelineValue::Image(PipelineImageOps::Blank(8, 8, Color4(10, 20, 30, 255)));
            result.order.Add("preview");
            result.outputs["layout"] = PipelineValue::Text("{\"from\": \"" + in.data + "\"}");
            result.order.Add("layout");
            for (auto& name : node->GetConfigString("sprites", "zeta,alpha").Split(","))
            {
                result.outputs[name] = PipelineValue::Image(PipelineImageOps::Blank(4, 4, Color4(200, 0, 0, 255)));
                result.order.Add(name);
            }
            co_return result;
        }

    private:
        PipelineNodeSchema mSchema;
    };

    struct DynamicFixture: ::testing::Test
    {
        WorkDirGuard work;

        void SetUp() override
        {
            PipelineNodeRegistry::Get("sourceText");
            PipelineNodeRegistry::Register(mmake<TestDynamicNode>());
            dynamicRuns = 0;
        }
    };

    struct Result
    {
        bool                       done = false;
        String                     fatal;
        Vector<PipelinePort>       ports;
        Map<String, PipelineValue> outputs;
        Vector<String>             logs;
    };

    Result RunGraph(PipelineGraph& graph, const String& target, const Vector<String>& bypass = {}, bool cachedOnly = false)
    {
        Result result;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodePorts) result.ports = e.ports;
            if (e.type == PipelineExecEvent::Type::NodeOutput && e.portId.IsEmpty()) result.outputs[e.nodeId] = e.value;
            if (e.type == PipelineExecEvent::Type::Log) result.logs.Add(e.message);
            if (e.type == PipelineExecEvent::Type::Done) result.done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) result.fatal = e.error;
        };
        executor->Execute("dyn", graph, target, bypass, cachedOnly);
        EXPECT_TRUE(NetPumpUntil([&] { return result.done || !result.fatal.IsEmpty(); }, 20.0f));
        return result;
    }

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
}

TEST_F(DynamicFixture, OneRunMakesEveryOutputAndTheCacheServesThemTogether)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText");
    source->SetConfigString("text", "mockup");
    auto dynamic = AddNode(graph, "testDynamic");
    Connect(graph, source, "out", dynamic, "text");
    auto concat = AddNode(graph, "textConcat");
    concat->SetCustomInputs({ PipelinePort("a", "a", PipelinePortType::Text, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(concat);
    Connect(graph, dynamic, "layout", concat, "a");
    String previewId = dynamic->outputs[0].id, layoutId = dynamic->outputs[1].id;

    auto first = RunGraph(graph, concat->id);
    ASSERT_TRUE(first.done) << first.fatal;
    EXPECT_EQ(dynamicRuns, 1) << "one run returns every output";
    EXPECT_FALSE(lastWorkDir.IsEmpty());

    ASSERT_EQ(first.ports.Count(), 4);
    EXPECT_EQ(first.ports[0].id, previewId) << "schema outputs keep the instance's port";
    EXPECT_EQ(first.ports[1].id, layoutId);
    EXPECT_EQ(first.ports[2].id, "sp-zeta") << "the others follow in run order";
    EXPECT_EQ(first.ports[3].id, "sp-alpha");
    EXPECT_EQ(first.ports[2].portType, PipelinePortType::Image);

    EXPECT_EQ(first.outputs[concat->id].data, "{\"from\": \"mockup\"}") << "a text consumer of the layout reads the JSON";
    EXPECT_TRUE(first.outputs[dynamic->id].IsImage()) << "the main preview is the first output";

    EXPECT_EQ(PipelineExecutor::LoadPortPreview("dyn", dynamic->id, layoutId, PipelinePortType::Text).data, "{\"from\": \"mockup\"}");
    EXPECT_TRUE(o2FileSystem.IsFileExist(PipelineExecutor::GetPortPreviewPath("dyn", dynamic->id, "sp-alpha", "png")));
    EXPECT_FALSE(o2FileSystem.IsFileExist(PipelineExecutor::GetPortPreviewPath("dyn", dynamic->id, previewId, "png")))
        << "the first output is the node's own preview";

    // The editor gives the live node the ports of the run
    dynamic->outputs = first.ports;
    auto thumb = AddNode(graph, "imageOutline");
    Connect(graph, dynamic, "alpha", thumb, "image");

    auto second = RunGraph(graph, thumb->id);
    ASSERT_TRUE(second.done) << second.fatal;
    EXPECT_EQ(dynamicRuns, 1) << "the bundle cached under the signature serves every output";
    EXPECT_TRUE(second.logs.Any([](const String& l) { return l.Contains("testDynamic: cache hit"); }));
    EXPECT_TRUE(second.outputs[thumb->id].IsImage());

    auto sigs = graph.ComputeSignatures();
    EXPECT_NE(sigs[dynamic->id + "#sp-alpha"], sigs[dynamic->id + "#sp-zeta"]) << "two outputs never share a consumer's cache slot";

    auto regenerated = RunGraph(graph, thumb->id, { dynamic->id });
    ASSERT_TRUE(regenerated.done) << regenerated.fatal;
    EXPECT_EQ(dynamicRuns, 2) << "the regenerate button runs it again";

    dynamic->SetConfigString("sprites", "alpha");
    auto fewer = RunGraph(graph, dynamic->id);
    ASSERT_TRUE(fewer.done) << fewer.fatal;
    EXPECT_EQ(fewer.ports.Count(), 3);
    EXPECT_FALSE(o2FileSystem.IsFileExist(PipelineExecutor::GetPortPreviewPath("dyn", dynamic->id, "sp-zeta", "png")))
        << "the preview of an output the run no longer makes goes";

    dynamic->SetConfigString("sprites", "beta");
    auto cachedOnly = RunGraph(graph, dynamic->id, {}, true);
    EXPECT_EQ(cachedOnly.fatal, "Not cached");
    EXPECT_EQ(dynamicRuns, 3);
}

TEST_F(DynamicFixture, BundleIsAMissWhenAFileIsGone)
{
    Vector<Pair<String, PipelineValue>> values = {
        { "preview", PipelineValue::Image(PipelineImageOps::Blank(2, 2, Color4::White())) },
        { "layout", PipelineValue::Text("{}") },
    };
    PipelineExecutor::SaveBundle("dyn", "sig1", values);

    Vector<Pair<String, PipelineValue>> loaded;
    ASSERT_TRUE(PipelineExecutor::LoadBundle("dyn", "sig1", loaded));
    ASSERT_EQ(loaded.Count(), 2);
    EXPECT_EQ(loaded[0].first, "preview");
    EXPECT_TRUE(loaded[0].second.IsImage());
    EXPECT_EQ(loaded[1].second.data, "{}");

    o2FileSystem.FileDelete(PipelineExecutor::GetContentPath("dyn", PipelineExecutor::BundleKey("sig1", "layout"), "txt"));
    EXPECT_FALSE(PipelineExecutor::LoadBundle("dyn", "sig1", loaded));
}
