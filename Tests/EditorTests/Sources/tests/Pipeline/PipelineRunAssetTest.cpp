#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

namespace
{
    String EnvOr(const char* name, const String& def = "")
    {
        const char* value = getenv(name);
        return value && *value ? String(value) : def;
    }

    bool LoadGraphFile(const String& path, PipelineGraph& graph)
    {
        DataDocument document;
        if (!document.LoadFromFile(path))
            return false;

        PipelineAsset asset;
        asset.document = document;
        graph.LoadFromAsset(asset);
        for (auto& node : graph.nodes)
            PipelineNodeRegistry::SyncNodeWithSchema(node);

        return !graph.nodes.IsEmpty();
    }
}

// Headless runner of a .pipeline file: O2_PIPELINE_RUN=<file> runs every run target of the graph
// against the real providers, so an art pipeline can be executed from the command line instead of
// the Pipeline window. O2_PIPELINE_NODE limits the run to one node, O2_PIPELINE_ASSETS redirects
// where finish nodes write, O2_PIPELINE_TIMEOUT sets the seconds budget of a single target
TEST(PipelineRunAsset, RunFile)
{
    String file = EnvOr("O2_PIPELINE_RUN");
    if (file.IsEmpty())
        GTEST_SKIP() << "O2_PIPELINE_RUN is not set";

    PipelineGraph graph;
    ASSERT_TRUE(LoadGraphFile(file, graph)) << "can't read a pipeline graph from " << file;

    String work = EnvOr("O2_PIPELINE_WORK");
    if (!work.IsEmpty())
        PipelineUtils::SetWorkPathOverride(work);

    String key = EnvOr("PIPELINE_LIVE_GEMINI_KEY");
    if (!key.IsEmpty())
    {
        PipelineSettings settings = PipelineSettings::Load();
        settings.geminiApiKey = key;
        settings.Save();
    }

    String assets = EnvOr("O2_PIPELINE_ASSETS");
    if (!assets.IsEmpty() && !assets.EndsWith("/"))
        assets += "/";

    String pipelineId = graph.id.IsEmpty() ? String("runasset") : graph.id;
    float timeout = (float)atof(EnvOr("O2_PIPELINE_TIMEOUT", "900").Data());

    Vector<String> targets;
    String single = EnvOr("O2_PIPELINE_NODE");
    if (!single.IsEmpty())
        targets.Add(single);
    else
        targets = graph.GetRunTargets();

    printf("[pipeline] %s: %d nodes, %d edges, %d targets\n", file.Data(), graph.nodes.Count(), graph.edges.Count(), targets.Count());

    int failed = 0;
    for (auto& target : targets)
    {
        auto node = graph.FindNode(target);
        String label = node ? node->nodeType : String("?");

        bool done = false;
        String fatal;
        Map<String, String> states;
        auto executor = mmake<PipelineExecutor>();
        executor->assetsPathOverride = assets;
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodeState)
            {
                states[e.nodeId + (e.portId.IsEmpty() ? String() : "#" + e.portId)] = e.state + (e.error.IsEmpty() ? String() : ": " + e.error);
                if (e.state == "error")
                    printf("[pipeline] error %s: %s\n", e.nodeId.Data(), e.error.Data());
            }
            else if (e.type == PipelineExecEvent::Type::Log)
                printf("[pipeline] %s\n", e.message.Data());
            else if (e.type == PipelineExecEvent::Type::Retry)
                printf("[pipeline] retry %d/%d status %d: %s\n", e.attempt, e.maxAttempts, e.status, e.message.Data());
            else if (e.type == PipelineExecEvent::Type::Done)
                done = true;
            else if (e.type == PipelineExecEvent::Type::Fatal)
                fatal = e.error;
        };

        printf("[pipeline] --- target %s (%s)\n", target.Data(), label.Data());
        executor->Execute(pipelineId, graph, target, {}, false);
        bool finished = NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, timeout);

        for (auto& kv : states)
        {
            if (kv.second != "done")
                printf("[pipeline] state %s: %s\n", kv.first.Data(), kv.second.Data());
        }

        if (!finished || !fatal.IsEmpty())
        {
            failed++;
            printf("[pipeline] target %s failed: %s\n", target.Data(), finished ? fatal.Data() : "timeout");
        }
    }

    EXPECT_EQ(failed, 0) << failed << " of " << targets.Count() << " targets failed";
}
