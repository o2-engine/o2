#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

// A .pipeline asset is the AssetsLine pipeline document itself. These tests hold
// the editor to that: documents written by the AssetsLine web editor read and
// write back without losing or changing anything, the older o2 format migrates,
// and the numbers both editors derive from a document (seeds) agree.

namespace
{
    // Order-insensitive text of a JSON value; numbers at one decimal, so the float
    // noise of positions the editor keeps as floats does not count as a change
    void Canon(const DataValue& value, String& out)
    {
        if (value.IsObject())
        {
            Vector<String> keys;
            for (auto it = value.BeginMember(); it != value.EndMember(); ++it)
                keys.Add(it->name.GetString());

            keys.Sort([](const String& a, const String& b) { return strcmp(a.Data(), b.Data()) < 0; });
            out += "{";
            for (auto& key : keys)
            {
                out += "\"" + key + "\":";
                Canon(*value.FindMember(key.Data()), out);
                out += ",";
            }
            out += "}";
        }
        else if (value.IsArray())
        {
            out += "[";
            for (auto& item : value)
            {
                Canon(item, out);
                out += ",";
            }
            out += "]";
        }
        else if (value.IsNumber())
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.1f", (double)value);
            out += buf;
        }
        else if (value.IsString())
            out += "\"" + String(value.GetString()) + "\"";
        else if (value.IsBoolean())
            out += (bool)value ? "true" : "false";
        else
            out += "null";
    }

    String Canon(const DataValue& value)
    {
        String out;
        Canon(value, out);
        return out;
    }

    // Pipelines stored by a local AssetsLine (its data/projects/*/pipelines), found by
    // walking up from the working directory, or named by O2_ASSETSLINE_PIPELINES
    Vector<String> AssetsLinePipelineFiles()
    {
        Vector<String> dirs;
        if (const char* env = getenv("O2_ASSETSLINE_PIPELINES"))
            dirs.Add(env);
        else
        {
            String base = o2FileSystem.CanonicalizePath(".");
            for (int i = 0; i < 8 && !base.IsEmpty(); i++)
            {
                String projects = base + "/AssetsLine/data/projects";
                if (o2FileSystem.IsFolderExist(projects))
                {
                    for (auto& project : o2FileSystem.GetFolderInfo(projects).folders)
                        dirs.Add(project.path + "/pipelines");
                    break;
                }

                base = o2FileSystem.GetParentPath(base);
            }
        }

        Vector<String> files;
        for (auto& dir : dirs)
        {
            if (!o2FileSystem.IsFolderExist(dir))
                continue;

            for (auto& file : o2FileSystem.GetFolderInfo(dir).files)
            {
                if (file.path.EndsWith(".json"))
                    files.Add(file.path);
            }
        }

        return files;
    }

    // An AssetsLine document using every member the format has, and some it may grow
    const char* kFullDocument = R"json({
        "schemaVersion": 1,
        "id": "3b0f6c1e-2d5a-4d7e-9c1f-8a2b3c4d5e6f",
        "name": "full",
        "nodes": [
            { "id": "n-text", "type": "sourceText", "position": { "x": 10, "y": 20 }, "config": { "text": "a cat" },
              "inputs": [], "outputs": [ { "id": "t-out", "name": "out", "type": "text", "color": "var(--red)" } ] },
            { "id": "n-gen", "type": "nanoBananaGen", "position": { "x": 400.5, "y": -120 }, "size": { "width": 312, "height": 480 },
              "config": { "prompt": "cute", "seed": "", "model": "gemini-3.1-flash-image", "customInputs": [ { "id": "c-in", "name": "style", "type": "image" } ] },
              "inputs": [ { "id": "g-prompt", "name": "prompt", "type": "text" }, { "id": "c-in", "name": "style", "type": "image", "custom": true } ],
              "outputs": [ { "id": "g-out", "name": "out", "type": "image" } ], "futureNodeField": { "keep": [1, 2] } }
        ],
        "edges": [
            { "id": "e1", "fromNodeId": "n-text", "fromPortId": "t-out", "toNodeId": "n-gen", "toPortId": "g-prompt",
              "points": [ { "x": 200, "y": 30 } ], "futureEdgeField": true }
        ],
        "camera": { "x": 120, "y": -40, "scale": 0.35 },
        "annotations": [ { "id": "a1", "kind": "comment", "position": { "x": 0, "y": 0 }, "color": "#f00",
                           "messages": [ { "id": "m1", "author": { "id": "u", "name": "U" }, "text": "hi", "createdAt": "2026-09-01" } ] } ],
        "icon": "node:n-gen",
        "futureTopField": "kept"
    })json";
}

TEST(PipelineFormat, FullDocumentReadsAndWritesBackUnchanged)
{
    DataDocument original;
    ASSERT_TRUE(original.LoadFromData(kFullDocument));

    PipelineGraph graph;
    ASSERT_TRUE(graph.LoadFromJson(original));
    EXPECT_EQ(graph.id, "3b0f6c1e-2d5a-4d7e-9c1f-8a2b3c4d5e6f");
    EXPECT_EQ(graph.name, "full");
    ASSERT_EQ(graph.nodes.Count(), 2);
    EXPECT_EQ(graph.nodes[0]->outputs[0].color, "var(--red)");
    EXPECT_TRUE(graph.nodes[1]->inputs[1].custom);
    EXPECT_EQ(graph.nodes[1]->size, Vec2F(312, 480));
    ASSERT_EQ(graph.edges.Count(), 1);
    EXPECT_EQ(graph.edges[0]->points.Count(), 1);

    DataDocument written;
    graph.SaveToJson(written);
    EXPECT_EQ(Canon(written), Canon(original));
}

TEST(PipelineFormat, WritingIsByteStable)
{
    PipelineGraph first;
    ASSERT_TRUE(first.LoadFromJsonString(kFullDocument));
    String once = first.ToJsonString();

    PipelineGraph second;
    ASSERT_TRUE(second.LoadFromJsonString(once));
    EXPECT_EQ(second.ToJsonString(), once);
}

TEST(PipelineFormat, WholeNumbersStayIntegers)
{
    PipelineGraph graph;
    ASSERT_TRUE(graph.LoadFromJsonString(kFullDocument));
    String text = graph.ToJsonString();
    EXPECT_TRUE(text.Contains("\"x\": 10,")) << text;
    EXPECT_TRUE(text.Contains("\"x\": 400.5,")) << text;
    EXPECT_FALSE(text.Contains("10.0"));
}

TEST(PipelineFormat, TheCameraSurvivesTheConversionToTheEditorView)
{
    PipelineGraph graph;
    ASSERT_TRUE(graph.LoadFromJsonString(kFullDocument));
    EXPECT_NEAR(graph.cameraScale, 1.0f / 0.35f, 1e-4f);

    DataDocument written;
    graph.SaveToJson(written);
    auto camera = written.FindMember("camera");
    ASSERT_TRUE(camera);
    EXPECT_NEAR((float)camera->GetMember("x"), 120.0f, 0.01f);
    EXPECT_NEAR((float)camera->GetMember("y"), -40.0f, 0.01f);
    EXPECT_NEAR((float)camera->GetMember("scale"), 0.35f, 1e-5f);
}

TEST(PipelineFormat, NotAPipelineIsRefused)
{
    PipelineGraph graph;
    EXPECT_FALSE(graph.LoadFromJsonString("[1, 2]"));
    EXPECT_FALSE(graph.LoadFromJsonString("{\"nodes\": 5}"));
    EXPECT_FALSE(graph.LoadFromJsonString("not json"));
    EXPECT_TRUE(graph.nodes.IsEmpty());
}

TEST(PipelineFormat, TheOlderO2FormatMigrates)
{
    PipelineGraph graph;
    ASSERT_TRUE(graph.LoadFromJsonString(kFullDocument));

    // What the editor used to store: {"graph": <reflection of PipelineGraph>}
    PipelineAsset legacy;
    legacy.document.SetObject();
    legacy.document.AddMember("graph").Set(graph);
    ASSERT_TRUE(PipelineGraph::IsLegacyDocument(legacy.document));

    PipelineGraph migrated;
    migrated.LoadFromAsset(legacy);
    ASSERT_EQ(migrated.nodes.Count(), 2);
    EXPECT_EQ(migrated.id, graph.id);
    EXPECT_EQ(migrated.nodes[1]->GetConfigString("prompt"), "cute");
    EXPECT_EQ(migrated.edges.Count(), 1);

    PipelineAsset saved;
    migrated.SaveToAsset(saved);
    EXPECT_FALSE(saved.document.FindMember("graph"));
    ASSERT_TRUE(saved.document.FindMember("nodes"));
    EXPECT_FALSE(PipelineGraph::IsLegacyDocument(saved.document));
}

TEST(PipelineFormat, UndoTextRestoresEverything)
{
    PipelineGraph graph;
    ASSERT_TRUE(graph.LoadFromJsonString(kFullDocument));
    PipelineGraph copy = graph;
    EXPECT_EQ(copy.ToJsonString(), graph.ToJsonString());
}

TEST(PipelineFormat, AssetsLineDocumentsOnThisMachineRoundTrip)
{
    auto files = AssetsLinePipelineFiles();
    if (files.IsEmpty())
        GTEST_SKIP() << "no local AssetsLine data found (set O2_ASSETSLINE_PIPELINES)";

    int checked = 0;
    for (auto& file : files)
    {
        DataDocument original;
        ASSERT_TRUE(original.LoadFromFile(file)) << file;
        PipelineGraph graph;
        ASSERT_TRUE(graph.LoadFromJson(original)) << file;

        DataDocument written;
        graph.SaveToJson(written);
        // The camera goes through the editor's view model; it is compared on its own
        for (auto doc : { (DataValue*)&original, (DataValue*)&written })
            if (doc->FindMember("camera")) doc->RemoveMember("camera");

        EXPECT_EQ(Canon(written), Canon(original)) << file;
        checked++;
    }

    EXPECT_GT(checked, 0);
}

TEST(PipelineFormat, SchemaSyncKeepsEveryPortAndLinkOfAssetsLineDocuments)
{
    auto files = AssetsLinePipelineFiles();
    if (files.IsEmpty())
        GTEST_SKIP() << "no local AssetsLine data found (set O2_ASSETSLINE_PIPELINES)";

    for (auto& file : files)
    {
        PipelineGraph graph;
        DataDocument doc;
        ASSERT_TRUE(doc.LoadFromFile(file));
        ASSERT_TRUE(graph.LoadFromJson(doc));

        String before = graph.ToJsonString();
        int edges = graph.edges.Count();
        // What the editor does to a graph it opens
        for (auto& node : graph.nodes)
        {
            if (PipelineNodeRegistry::Get(node->nodeType))
                PipelineNodeRegistry::SyncNodeWithSchema(node);
        }
        graph.RemoveDanglingEdges();

        EXPECT_EQ(graph.edges.Count(), edges) << file;
        for (auto& node : graph.nodes)
        {
            DataDocument original;
            original.LoadFromData(before);
            const DataValue* stored = nullptr;
            for (auto& item : original["nodes"])
                if (String(item["id"].GetString()) == node->id) stored = &item;

            ASSERT_TRUE(stored);
            // Every stored input keeps its id; outputs may only be added
            for (auto& port : (*stored)["inputs"])
                EXPECT_TRUE(node->FindInput(port["id"].GetString())) << file << " " << node->nodeType << " lost input " << port["name"].GetString();

            // An extract with a region list has one output per region, like the web editor
            // makes it on load; an older build stored a leftover "out" beside them
            Vector<String> regionIds;
            bool legacyExtract = false;
            if (node->nodeType == "imageExtract")
            {
                if (auto regions = (*stored)["config"].FindMember("regions"); regions && regions->IsArray())
                    for (auto& region : *regions) regionIds.Add(region["id"].GetString());

                legacyExtract = regionIds.IsEmpty();
            }

            // An extract without a region list keeps the one output "out", as the web editor regenerates it; the
            // unlinked extra outputs such a node picked up (a phantom "out" beside a prompt-named port) are dropped,
            // a linked one never
            if (legacyExtract)
            {
                ASSERT_EQ(node->outputs.Count(), 1) << file;
                EXPECT_EQ(node->outputs[0].name, "out") << file;
            }

            for (auto& port : (*stored)["outputs"])
            {
                String portId = port["id"].GetString();
                if (!regionIds.IsEmpty() && !regionIds.Contains(portId))
                    continue;

                bool linked = false;
                for (auto& edge : original["edges"])
                    linked = linked || (String(edge["fromNodeId"].GetString()) == node->id && String(edge["fromPortId"].GetString()) == portId);

                if (legacyExtract && !linked && portId != (*stored)["outputs"].GetElement(0)["id"].GetString())
                    continue;

                EXPECT_TRUE(node->FindOutput(portId)) << file << " " << node->nodeType << " lost output " << port["name"].GetString();
            }
        }
    }
}

namespace
{
    DataDocument Json(const char* text)
    {
        DataDocument doc;
        doc.LoadFromData(text);
        return doc;
    }

    int SeedOf(const char* configJson, const char* nodeId = "node-1")
    {
        PipelineGraph graph;
        String doc = String("{\"schemaVersion\":1,\"id\":\"p\",\"name\":\"p\",\"edges\":[],\"nodes\":[{\"id\":\"") + nodeId +
            "\",\"type\":\"nanoBananaGen\",\"position\":{\"x\":0,\"y\":0},\"config\":" + configJson +
            ",\"inputs\":[],\"outputs\":[{\"id\":\"o\",\"name\":\"out\",\"type\":\"image\"}]}]}";
        EXPECT_TRUE(graph.LoadFromJsonString(doc));
        return graph.ResolveSeeds()[nodeId];
    }
}

TEST(PipelineFormat, SeedsAreTheOnesAssetsLineUses)
{
    // Reference values from AssetsLine's hashSeed() and Number() (backend/src/pipeline/signature.ts)
    EXPECT_EQ(SeedOf("{}", "f5255400-a963-4d54-9b72-01fd4567357e"), 246063642);
    EXPECT_EQ(SeedOf("{}", "node-1"), 1422144387);
    EXPECT_EQ(SeedOf("{\"seed\":\"abc\"}", "abc"), 440920331);
    EXPECT_EQ(SeedOf("{\"seed\":\"\"}"), 0);
    EXPECT_EQ(SeedOf("{\"seed\":\" 12 \"}"), 12);
    EXPECT_EQ(SeedOf("{\"seed\":\"0x1F\"}"), 31);
    EXPECT_EQ(SeedOf("{\"seed\":\"1e3\"}"), 1000);
    EXPECT_EQ(SeedOf("{\"seed\":null}"), 0);
    EXPECT_EQ(SeedOf("{\"seed\":true}"), 1);
    EXPECT_EQ(SeedOf("{\"seed\":7.9}"), 7);
    EXPECT_EQ(SeedOf("{\"seed\":-3.2}"), -4);
    EXPECT_EQ(SeedOf("{\"seed\":\"Infinity\"}"), 1422144387);
}

TEST(PipelineFormat, OnlyALiteralFalseStopsSeedInheritance)
{
    auto graphWith = [](const char* inherit)
    {
        String doc = String(R"({"schemaVersion":1,"id":"p","name":"p","nodes":[
            {"id":"up","type":"nanoBananaGen","position":{"x":0,"y":0},"config":{"seed":5},"inputs":[],
             "outputs":[{"id":"uo","name":"out","type":"image"}]},
            {"id":"down","type":"imageEdit","position":{"x":0,"y":0},"config":{"seed":9,"inheritSeed":)") + inherit + R"(},
             "inputs":[{"id":"di","name":"image","type":"image"}],"outputs":[{"id":"do","name":"out","type":"image"}]}],
            "edges":[{"id":"e","fromNodeId":"up","fromPortId":"uo","toNodeId":"down","toPortId":"di"}]})";
        PipelineGraph graph;
        EXPECT_TRUE(graph.LoadFromJsonString(doc));
        return graph.ResolveSeeds()["down"];
    };

    EXPECT_EQ(graphWith("true"), 5);
    EXPECT_EQ(graphWith("\"false\""), 5);
    EXPECT_EQ(graphWith("false"), 9);
}

TEST(PipelineFormat, FinishNodesOfTheWebEditorWriteUnderTheirDownloadName)
{
    PipelineNode node;
    node.nodeType = "finishImage";
    EXPECT_EQ(PipelineNodeRegistry::GetFinishAssetPath(node), "Generated/output");
    node.SetConfigString("filename", "hero sprite.png");
    EXPECT_EQ(PipelineNodeRegistry::GetFinishAssetPath(node), "Generated/hero sprite");
    node.SetConfigString("assetPath", "Sprites/hero");
    EXPECT_EQ(PipelineNodeRegistry::GetFinishAssetPath(node), "Sprites/hero");
}

TEST(PipelineFormat, CopiedPortIdsFollowIntoComposerLayersAndExtractRegions)
{
    PipelineNode composer;
    ASSERT_TRUE(composer.LoadFromJson(Json(R"({"id":"c","type":"composer","position":{"x":0,"y":0},
        "config":{"layers":{"in1":{"x":1},"dup1":{"x":2}},"dupLayers":[{"id":"dup1","srcPortId":"in1"}],
                  "layerOrder":["dup1","in1"],"selectedLayer":"in1","customInputs":[{"id":"in1","name":"","type":"image"}]},
        "inputs":[{"id":"in1","name":"","type":"image","custom":true}],"outputs":[{"id":"o","name":"out","type":"image"}]})")));
    Map<String, String> ports;
    ports["in1"] = "NEW";
    composer.RemapConfigPortIds(ports);

    auto& cfg = composer.config;
    EXPECT_TRUE(cfg["layers"].FindMember("NEW"));
    EXPECT_FALSE(cfg["layers"].FindMember("in1"));
    String dupId = cfg["dupLayers"][0]["id"].GetString();
    EXPECT_NE(dupId, "dup1");
    EXPECT_TRUE(cfg["layers"].FindMember(dupId.Data()));
    EXPECT_EQ(String(cfg["dupLayers"][0]["srcPortId"].GetString()), "NEW");
    EXPECT_EQ(String(cfg["layerOrder"][0].GetString()), dupId);
    EXPECT_EQ(String(cfg["selectedLayer"].GetString()), "NEW");
    EXPECT_EQ(String(cfg["customInputs"][0]["id"].GetString()), "NEW");

    PipelineNode extract;
    ASSERT_TRUE(extract.LoadFromJson(Json(R"({"id":"x","type":"imageExtract","position":{"x":0,"y":0},
        "config":{"regions":[{"id":"p1","name":"a","x":0,"y":0,"w":1,"h":1}],"selectedRegion":"p1"},
        "inputs":[],"outputs":[{"id":"p1","name":"a","type":"image"}]})")));
    Map<String, String> parts;
    parts["p1"] = "P";
    extract.RemapConfigPortIds(parts);
    EXPECT_EQ(String(extract.config["regions"][0]["id"].GetString()), "P");
    EXPECT_EQ(String(extract.config["selectedRegion"].GetString()), "P");
}
