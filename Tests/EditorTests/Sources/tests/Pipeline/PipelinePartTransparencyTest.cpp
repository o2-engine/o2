#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

// Own background settings of the parts of an AI extract node ("transparency" of a region): the effective
// config, the cache keys, the per-part cut, the format and the price

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-part-bg-work-" + (String)(int)Math::Random(0, 1000000);
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

    DataDocument Json(const char* text)
    {
        DataDocument doc;
        doc.LoadFromData(text);
        return doc;
    }

    // An extract node with parts a, b and c; the node asks for a transparent background by two renders
    Ref<PipelineNode> MakeExtract(bool chroma = false)
    {
        auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
        Vector<PipelineExtractRegion> regions;
        for (auto id : { "a", "b", "c" })
        {
            PipelineExtractRegion region;
            region.id = id;
            region.name = String("part ") + id;
            region.x = 0.1f; region.y = 0.2f; region.w = 0.3f; region.h = 0.4f;
            regions.Add(region);
        }
        PipelineRegions::Write(*node, regions);
        PipelineRegions::SyncPorts(*node);
        node->SetConfigBool("transparentBg", true);
        node->SetConfigString("transparentMode", chroma ? "chroma" : "twoPass");
        return node;
    }

    // Puts an own settings object on a part, as a file written by any of the editors has it
    void SetOwn(PipelineNode& node, const String& portId, const char* json)
    {
        for (auto& region : node.config["regions"])
        {
            if (String(region["id"].GetString()) == portId)
                region["transparency"] = static_cast<const DataValue&>(Json(json));
        }
    }

    // The port key as it was computed before parts could have settings of their own
    String LegacyPortSignature(const PipelineNode& node, const String& portId, bool rawRender)
    {
        auto impl = PipelineNodeRegistry::Get(node.nodeType);
        bool chroma = rawRender && node.GetConfigBool("transparentBg", false) && node.GetConfigString("transparentMode", "twoPass") == "chroma";
        Vector<String> exclude = { "crop", "cropEnabled" };
        exclude.Add(impl->PortCacheExcludedKeys());
        if (chroma)
            exclude.Add(PipelineTransparency::ChromaConfigKeys());

        return PipelineGraph::ComputeNodeSignature(node, {}, exclude, -1, impl->PortCacheVariant(node, portId) + (chroma ? "|chroma-raw" : ""));
    }

    // A square of subject colour in the middle of a backdrop
    Ref<Bitmap> SubjectOn(const Color4& backdrop)
    {
        auto bitmap = PipelineImageOps::Blank(40, 40, backdrop);
        for (int y = 12; y < 28; y++)
        {
            for (int x = 12; x < 28; x++)
            {
                UInt8* p = PipelineImageOps::Pixel(*bitmap, x, y);
                p[0] = 200; p[1] = 40; p[2] = 30; p[3] = 255;
            }
        }
        return bitmap;
    }
}

TEST(PipelinePartTransparency, EffectiveConfigLaysThePartOverTheNode)
{
    auto node = MakeExtract();
    node->SetConfigString("chromaColor", "#0047bb");
    SetOwn(*node, "b", R"json({"transparentBg":true,"transparentMode":"chroma","chromaColor":"#ff00ff",
                               "chromaTolerance":40,"chromaSoftness":5,"chromaSpill":70})json");
    SetOwn(*node, "c", R"json({"transparentMode":"chroma"})json");

    auto a = PipelineTransparency::Read(*node, "a");
    EXPECT_TRUE(a.transparent);
    EXPECT_EQ(a.mode, "twoPass");
    EXPECT_EQ(a.colorHex, "#0047BB");

    auto b = PipelineTransparency::Read(*node, "b");
    EXPECT_TRUE(b.transparent);
    EXPECT_EQ(b.mode, "chroma");
    EXPECT_EQ(b.colorHex, "#FF00FF");
    EXPECT_FLOAT_EQ(b.chroma.tolerance, 40.0f);
    EXPECT_FLOAT_EQ(b.chroma.softness, 5.0f);
    EXPECT_FLOAT_EQ(b.chroma.spill, 70.0f);

    // A partial object still reads: the keys it lacks come from the node, then from the defaults
    auto c = PipelineTransparency::Read(*node, "c");
    EXPECT_TRUE(c.transparent);
    EXPECT_EQ(c.mode, "chroma");
    EXPECT_EQ(c.colorHex, "#0047BB");
    EXPECT_FLOAT_EQ(c.chroma.tolerance, 30.0f);
    EXPECT_FLOAT_EQ(c.chroma.spill, 60.0f);

    // The node itself and its parts without settings of their own read the node
    EXPECT_EQ(PipelineTransparency::Read(*node).mode, "twoPass");
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node));
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node, "a"));
    EXPECT_TRUE(PipelineTransparency::UsesChromaPostStep(*node, "b"));
    EXPECT_TRUE(PipelineTransparency::UsesChromaPostStep(*node, "c"));
    EXPECT_TRUE(PipelineTransparency::AnyChromaPostStep(*node));

    auto regions = PipelineRegions::Read(*node);
    EXPECT_FALSE(regions[0].ownTransparency);
    EXPECT_TRUE(regions[1].ownTransparency);

    // "This part" copies what the part uses now, all six keys; "All parts" drops the copy
    EXPECT_TRUE(PipelineRegions::SetOwnTransparency(*node, "a", true));
    EXPECT_FALSE(PipelineRegions::SetOwnTransparency(*node, "a", true));
    auto own = PipelineRegions::FindTransparency(*node, "a");
    ASSERT_TRUE(own);
    EXPECT_EQ(own->GetMembersCount(), 6);
    EXPECT_TRUE((bool)(*own)["transparentBg"]);
    EXPECT_EQ(String((*own)["transparentMode"].GetString()), "twoPass");
    EXPECT_EQ(String((*own)["chromaColor"].GetString()), "#0047bb");
    EXPECT_FLOAT_EQ((float)(*own)["chromaTolerance"], 30.0f);
    EXPECT_FLOAT_EQ((float)(*own)["chromaSoftness"], 15.0f);
    EXPECT_FLOAT_EQ((float)(*own)["chromaSpill"], 60.0f);

    node->SetConfigBool("transparentBg", false);
    EXPECT_TRUE(PipelineTransparency::Read(*node, "a").transparent) << "a part with its own settings ignores the node";
    EXPECT_TRUE(PipelineRegions::SetOwnTransparency(*node, "a", false));
    EXPECT_FALSE(PipelineRegions::FindTransparency(*node, "a"));
    EXPECT_FALSE(PipelineTransparency::Read(*node, "a").transparent);

    // Parts found by auto split start without settings of their own
    auto split = PipelineRegions::ParseAutoSplit(R"([{"name":"coin","box_2d":[100,100,300,300]}])");
    ASSERT_EQ(split.Count(), 1);
    EXPECT_FALSE(split[0].ownTransparency);
}

// Existing results stay valid: a part without settings of its own hashes byte for byte as before, whatever
// its neighbours carry; a part with them is keyed by its own values instead of the node's
TEST(PipelinePartTransparency, PortSignaturesChangeOnlyForPartsWithOwnSettings)
{
    for (bool chroma : { false, true })
    {
        auto node = MakeExtract(chroma);
        node->SetConfigString("chromaColor", "#00b140");

        String rawA = PipelineExecutor::PortSignature(*node, {}, -1, "a");
        String downA = PipelineGraph::ComputePortSignature(*node, {}, -1, "a", false);
        String rawB = PipelineExecutor::PortSignature(*node, {}, -1, "b");
        EXPECT_EQ(rawA, LegacyPortSignature(*node, "a", true)) << chroma;
        EXPECT_EQ(downA, LegacyPortSignature(*node, "a", false)) << chroma;

        SetOwn(*node, "b", R"json({"transparentBg":true,"transparentMode":"chroma","chromaColor":"#ff00ff",
                                   "chromaTolerance":40,"chromaSoftness":5,"chromaSpill":70})json");
        EXPECT_EQ(PipelineExecutor::PortSignature(*node, {}, -1, "a"), rawA) << chroma;
        EXPECT_EQ(PipelineGraph::ComputePortSignature(*node, {}, -1, "a", false), downA) << chroma;
        EXPECT_EQ(PipelineExecutor::ContentSignature(*node, {}, -1, "a"), rawA) << chroma;

        String ownB = PipelineExecutor::PortSignature(*node, {}, -1, "b");
        String ownDownB = PipelineGraph::ComputePortSignature(*node, {}, -1, "b", false);
        EXPECT_NE(ownB, rawB) << chroma;

        // The node's background keys no longer reach the part
        node->SetConfigBool("transparentBg", !chroma);
        node->SetConfigString("transparentMode", "twoPass");
        node->SetConfigString("chromaColor", "#0047bb");
        node->SetConfigNumber("chromaTolerance", 77);
        EXPECT_EQ(PipelineExecutor::PortSignature(*node, {}, -1, "b"), ownB) << chroma;
        EXPECT_EQ(PipelineGraph::ComputePortSignature(*node, {}, -1, "b", false), ownDownB) << chroma;

        // Its cut settings re-cut the raw render; what goes downstream follows them
        SetOwn(*node, "b", R"json({"transparentBg":true,"transparentMode":"chroma","chromaColor":"#ff00ff",
                                   "chromaTolerance":55,"chromaSoftness":9,"chromaSpill":20})json");
        EXPECT_EQ(PipelineExecutor::PortSignature(*node, {}, -1, "b"), ownB) << chroma;
        EXPECT_NE(PipelineGraph::ComputePortSignature(*node, {}, -1, "b", false), ownDownB) << chroma;

        // The key colour is what the provider renders against, and the mode changes the generation
        SetOwn(*node, "b", R"json({"transparentBg":true,"transparentMode":"chroma","chromaColor":"#00b140",
                                   "chromaTolerance":55,"chromaSoftness":9,"chromaSpill":20})json");
        EXPECT_NE(PipelineExecutor::PortSignature(*node, {}, -1, "b"), ownB) << chroma;
        String green = PipelineExecutor::PortSignature(*node, {}, -1, "b");
        SetOwn(*node, "b", R"json({"transparentBg":true,"transparentMode":"twoPass","chromaColor":"#00b140",
                                   "chromaTolerance":55,"chromaSoftness":9,"chromaSpill":20})json");
        EXPECT_NE(PipelineExecutor::PortSignature(*node, {}, -1, "b"), green) << chroma;

        // Dropping the own settings brings the part back to the key the node gives it
        PipelineRegions::SetOwnTransparency(*node, "b", false);
        EXPECT_EQ(PipelineExecutor::PortSignature(*node, {}, -1, "b"), LegacyPortSignature(*node, "b", true)) << chroma;
    }
}

// One run cuts every part with its own settings: a part following the node's green key, a part with its own
// two-pass settings passing through, a part keyed on magenta. The raw renders are seeded into the content
// cache and the run is cached-only, so no provider is ever asked
TEST(PipelinePartTransparency, EachPartIsCutWithItsOwnSettings)
{
    WorkDirGuard work;
    auto source = PipelineImageOps::Blank(64, 64, Color4(128, 128, 128, 255));
    o2FileSystem.FolderCreate(PipelineUtils::GetUploadsPath(), true);
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("sheet.png"), EncodeBitmapPng(*source));

    PipelineGraph graph;
    auto image = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    image->SetConfigString("uploadId", "sheet.png");
    auto extract = MakeExtract(true);
    extract->SetConfigString("chromaColor", "#00ff00");
    SetOwn(*extract, "b", R"json({"transparentBg":true,"transparentMode":"twoPass","chromaColor":"#00ff00",
                                  "chromaTolerance":30,"chromaSoftness":15,"chromaSpill":60})json");
    SetOwn(*extract, "c", R"json({"transparentBg":true,"transparentMode":"chroma","chromaColor":"#ff00ff",
                                  "chromaTolerance":30,"chromaSoftness":15,"chromaSpill":60})json");
    graph.nodes = { image, extract };
    auto edge = mmake<PipelineEdge>();
    edge->id = "e";
    edge->fromNodeId = image->id;
    edge->fromPortId = image->outputs[0].id;
    edge->toNodeId = extract->id;
    edge->toPortId = extract->inputs[0].id;
    graph.edges.Add(edge);

    auto upstream = graph.UpstreamSignatures(*extract, graph.ComputeSignatures());
    int seed = graph.ResolveSeeds()[extract->id];
    auto green = SubjectOn(Color4(0, 255, 0, 255));
    auto magenta = SubjectOn(Color4(255, 0, 255, 255));
    PipelineExecutor::SaveContent("parts", PipelineExecutor::PortSignature(*extract, upstream, seed, "a"), PipelineValue::Image(green));
    PipelineExecutor::SaveContent("parts", PipelineExecutor::PortSignature(*extract, upstream, seed, "b"), PipelineValue::Image(green));
    PipelineExecutor::SaveContent("parts", PipelineExecutor::PortSignature(*extract, upstream, seed, "c"), PipelineValue::Image(magenta));

    auto run = [&]()
    {
        Map<String, PipelineValue> parts;
        bool done = false;
        String fatal;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == extract->id && !e.portId.IsEmpty()) parts[e.portId] = e.value;
            if (e.type == PipelineExecEvent::Type::Done) done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
        };
        executor->Execute("parts", graph, extract->id, {}, true);
        EXPECT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 20.0f));
        EXPECT_TRUE(fatal.IsEmpty()) << fatal;
        return parts;
    };

    auto parts = run();
    ASSERT_TRUE(parts.ContainsKey("a") && parts.ContainsKey("b") && parts.ContainsKey("c"));

    // Keyed and trimmed to the subject: only the square is left
    auto a = parts["a"].GetBitmap();
    ASSERT_TRUE(a);
    EXPECT_EQ(a->GetSize(), Vec2I(16, 16));
    EXPECT_EQ((int)PipelineImageOps::Pixel(*a, 8, 8)[3], 255);

    // Two passes are not cut afterwards: the stored render passes through as it is
    auto b = parts["b"].GetBitmap();
    ASSERT_TRUE(b);
    EXPECT_EQ(b->GetSize(), Vec2I(40, 40));
    EXPECT_EQ((int)PipelineImageOps::Pixel(*b, 1, 1)[1], 255);
    EXPECT_EQ((int)PipelineImageOps::Pixel(*b, 1, 1)[3], 255);

    // Cut on its own key colour, which the node's green would have left in place
    auto c = parts["c"].GetBitmap();
    ASSERT_TRUE(c);
    EXPECT_EQ(c->GetSize(), Vec2I(16, 16));

    // A part's cut settings re-cut its cached render: a tolerance that swallows the subject leaves the raw render
    SetOwn(*extract, "c", R"json({"transparentBg":true,"transparentMode":"chroma","chromaColor":"#ff00ff",
                                  "chromaTolerance":100,"chromaSoftness":0,"chromaSpill":60})json");
    auto again = run();
    ASSERT_TRUE(again.ContainsKey("c"));
    EXPECT_NE(again["c"].GetBitmap()->GetSize(), Vec2I(16, 16));
    EXPECT_EQ(again["a"].GetBitmap()->GetSize(), Vec2I(16, 16));
}

// The format keeps a part's own settings and whatever else a region carries, through the file and through
// the edits of the part list; copies with new port ids keep them too
TEST(PipelinePartTransparency, OwnSettingsSurviveTheFileEditsAndCopies)
{
    const char* doc = R"json({"schemaVersion":1,"id":"p","name":"p","edges":[],"nodes":[{"id":"x","type":"imageExtract",
        "position":{"x":0,"y":0},"config":{"regions":[
            {"id":"a","name":"coin","x":0.1,"y":0.2,"w":0.2,"h":0.2,"transparency":{"transparentBg":true,"transparentMode":"chroma",
             "chromaColor":"#00b140","chromaTolerance":30,"chromaSoftness":15,"chromaSpill":60},"futureKey":{"k":[1,2]}},
            {"id":"b","name":"gem","x":0.5,"y":0.5,"w":0.2,"h":0.2}],"selectedRegion":"a"},
        "inputs":[{"id":"i","name":"image","type":"image"}],
        "outputs":[{"id":"a","name":"coin","type":"image"},{"id":"b","name":"gem","type":"image"}]}]})json";

    PipelineGraph graph;
    ASSERT_TRUE(graph.LoadFromJsonString(doc));
    String saved = graph.ToJsonString();
    PipelineGraph reloaded;
    ASSERT_TRUE(reloaded.LoadFromJsonString(saved));
    EXPECT_EQ(reloaded.ToJsonString(), saved);

    auto node = reloaded.nodes[0];
    ASSERT_TRUE(PipelineRegions::FindTransparency(*node, "a"));
    EXPECT_FALSE(PipelineRegions::FindTransparency(*node, "b"));

    // Renaming a part and moving a box rewrite the list: the other members go along
    auto regions = PipelineRegions::Read(*node);
    regions[0].name = "gold coin";
    regions[1].x = 0.6f;
    PipelineRegions::Write(*node, regions);
    auto own = PipelineRegions::FindTransparency(*node, "a");
    ASSERT_TRUE(own);
    EXPECT_EQ(String((*own)["transparentMode"].GetString()), "chroma");
    auto future = node->config["regions"][0].FindMember("futureKey");
    ASSERT_TRUE(future);
    EXPECT_EQ((int)(*future)["k"][1], 2);
    EXPECT_EQ(String(node->config["regions"][0]["name"].GetString()), "gold coin");

    // A paste gives the ports new ids and remaps the regions: the settings stay with their part
    PipelineGraph clip;
    ASSERT_TRUE(clip.LoadFromJsonString(reloaded.ToJsonString()));
    Map<String, String> idMap;
    idMap["a"] = "A2";
    idMap["b"] = "B2";
    auto copy = clip.nodes[0];
    for (auto& port : copy->outputs)
        port.id = idMap[port.id];
    copy->RemapConfigPortIds(idMap);
    EXPECT_TRUE(PipelineRegions::FindTransparency(*copy, "A2"));
    EXPECT_FALSE(PipelineRegions::FindTransparency(*copy, "B2"));
    EXPECT_TRUE(PipelineTransparency::UsesChromaPostStep(*copy, "A2"));
    EXPECT_TRUE(copy->config["regions"][0].FindMember("futureKey"));
}

// The price on the play button of AssetsLine counts generations: per part two for a transparent two-pass
// render, one otherwise
TEST(PipelinePartTransparency, GenerationsFollowEachPartsSettings)
{
    auto node = MakeExtract();
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 6);

    SetOwn(*node, "b", R"json({"transparentBg":true,"transparentMode":"chroma"})json");
    SetOwn(*node, "c", R"json({"transparentBg":false,"transparentMode":"twoPass"})json");
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 4);

    node->SetConfigBool("transparentBg", false);
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 3);

    auto gen = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*gen), 1);
    gen->SetConfigBool("transparentBg", true);
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*gen), 2);

    auto removeBg = PipelineNodeRegistry::CreateNode("aiRemoveBg", Vec2F());
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*removeBg), 1);
    removeBg->SetConfigString("transparentMode", "twoPass");
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*removeBg), 2);
}
