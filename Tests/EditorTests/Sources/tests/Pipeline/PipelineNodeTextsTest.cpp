#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

using namespace o2;
using namespace Editor;

// What the pipeline nodes tell the user names o2 alone; the AI UI layout keeps the web's export target untouched

TEST(PipelineNodeTexts, NoLabelDescriptionOrPortNamesAnotherEngine)
{
    static const char* const words[] = { "unity", "cocos", "godot", "unreal", "ugui", "prefab" };
    auto schemas = PipelineNodeRegistry::AllSchemas();
    ASSERT_GT(schemas.Count(), 20);
    for (auto* schema : schemas)
    {
        Vector<String> texts = { schema->label, schema->description };
        for (auto& port : schema->inputs)
            texts.Add(port.name);
        for (auto& port : schema->outputs)
            texts.Add(port.name);
        for (auto& text : texts)
        {
            String lower = text.ToLowerCase();
            for (auto word : words)
                EXPECT_FALSE(lower.Contains(word)) << schema->type.Data() << ": " << text.Data();
        }
    }
}

// The web's download target (exportFor: unity / o2 / json) is a key the layout node never reads: it changes nothing the
// node computes and survives a save and a load as it was
TEST(PipelineNodeTexts, TheLayoutKeepsTheWebsExportTargetAndIgnoresIt)
{
    auto node = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F());
    ASSERT_TRUE(node);
    String sig = PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1);
    for (const char* target : { "unity", "json", "o2" })
    {
        node->SetConfigString("exportFor", target);
        EXPECT_EQ(PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1), sig) << target;
    }
    node->SetConfigString("assetPath", "UI/Menus");
    EXPECT_EQ(PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1), sig) << "where the prototype is saved changes nothing drawn";

    node->SetConfigString("exportFor", "unity");
    PipelineGraph graph;
    graph.id = "texts-test";
    graph.nodes.Add(node);
    PipelineGraph loaded;
    ASSERT_TRUE(loaded.LoadFromJsonString(graph.ToJsonString()));
    ASSERT_EQ(loaded.nodes.Count(), 1);
    EXPECT_EQ(loaded.nodes[0]->GetConfigString("exportFor", ""), String("unity"));
    EXPECT_EQ(loaded.nodes[0]->GetConfigString("assetPath", ""), String("UI/Menus"));
}
