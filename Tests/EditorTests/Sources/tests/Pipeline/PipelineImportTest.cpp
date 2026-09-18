#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineImport.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineZip.h"

using namespace o2;
using namespace Editor;

namespace
{
    struct ImportWorkDir
    {
        String path;

        ImportWorkDir()
        {
            String relative = "./pipeline-import-work-" + (String)(int)Math::Random(0, 1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
            PipelineUtils::SetWorkPathOverride(path);
        }

        ~ImportWorkDir()
        {
            PipelineUtils::SetWorkPathOverride("");
            o2FileSystem.FolderRemove(path, true);
        }
    };

    String PngBytes()
    {
        return PipelineValue::Image(PipelineImageOps::Blank(4, 3, Color4(255, 0, 0, 255))).GetPngBytes();
    }

    String PipelineJson()
    {
        return String(R"({"schemaVersion":1,"id":"p1","name":"My pipe: v2","nodes":[)"
            R"({"id":"n1","type":"sourceText","position":{"x":10,"y":20},"config":{"text":"hello"},"inputs":[],"outputs":[{"id":"o1","name":"out","type":"text"}]},)"
            R"({"id":"n2","type":"nanoBananaGen","position":{"x":300,"y":20},"size":{"width":320,"height":400},)"
            R"("config":{"model":"gemini-3.1-flash-image","customInputs":[{"id":"c1","name":"ref","type":"image"}]},)"
            R"("inputs":[{"id":"i1","name":"prompt","type":"text"},{"id":"c1","name":"ref","type":"image","custom":true}],"outputs":[{"id":"o2","name":"out","type":"image"}]}],)"
            R"("edges":[{"id":"e1","fromNodeId":"n1","fromPortId":"o1","toNodeId":"n2","toPortId":"i1","points":[{"x":150,"y":20}]}]})");
    }

    String BundleJson()
    {
        String dataUrl = "data:image/png;base64," + PipelineUtils::Base64Encode(PngBytes());
        return String(R"({"format":"assetsline-bundle","version":1,"pipeline":)") + PipelineJson() +
            R"(,"results":{"n1":{"mediaType":"text","text":"hello"},"n2":{"mediaType":"image","mime":"image/png","dataUrl":")" + dataUrl + R"("}}})";
    }

    String BundleZip(bool withManifest)
    {
        Vector<PipelineZip::Entry> entries = {
            { "pipeline.json", PipelineJson() },
            { "results/n1.txt", "hello" },
            { "results/n2.png", PngBytes() },
            { "results/ghost.png", PngBytes() }
        };
        if (withManifest)
        {
            entries.Add({ "manifest.json", R"({"format":"assetsline-bundle","version":2,"pipeline":"pipeline.json",)"
                R"("results":{"n1":{"mediaType":"text","file":"results/n1.txt"},"n2":{"mediaType":"image","mime":"image/png","file":"results/n2.png"}}})" });
        }
        return PipelineZip::Write(entries, withManifest);
    }

    // A v3 bundle: a source upload and the parts of a per-port node
    String BundleZipV3()
    {
        String pipeline = String(R"({"schemaVersion":1,"id":"p3","name":"Parts","nodes":[)"
            R"({"id":"n0","type":"sourceImage","position":{"x":0,"y":0},"config":{"uploadId":"up1.png"},"inputs":[],"outputs":[{"id":"o0","name":"out","type":"image"}]},)"
            R"({"id":"n3","type":"imageExtract","position":{"x":300,"y":0},)"
            R"("config":{"regions":[{"id":"pa","name":"coin","x":0,"y":0,"w":0.5,"h":1},{"id":"pb","name":"chest","x":0.5,"y":0,"w":0.5,"h":1}]},)"
            R"("inputs":[{"id":"i3","name":"image","type":"image"}],)"
            R"("outputs":[{"id":"pa","name":"coin","type":"image"},{"id":"pb","name":"chest","type":"image"}]}],)"
            R"("edges":[{"id":"e3","fromNodeId":"n0","fromPortId":"o0","toNodeId":"n3","toPortId":"i3","points":[]}]})");

        Vector<PipelineZip::Entry> entries = {
            { "pipeline.json", pipeline },
            { "results/n3.pa.png", PngBytes() },
            { "results/n3.pb.png", PngBytes() },
            { "uploads/up1.png", PngBytes() },
            { "manifest.json", R"({"format":"assetsline-bundle","version":3,"pipeline":"pipeline.json","results":{)"
                R"("n3#pa":{"mediaType":"image","mime":"image/png","file":"results/n3.pa.png","nodeId":"n3","portId":"pa"},)"
                R"("n3#pb":{"mediaType":"image","mime":"image/png","file":"results/n3.pb.png","nodeId":"n3","portId":"pb"}},)"
                R"("uploads":{"up1.png":{"file":"uploads/up1.png","name":"coin.png"}}})" }
        };
        return PipelineZip::Write(entries, true);
    }
}

TEST(PipelineImport, ParsesAssetsLineBundleWithResults)
{
    auto bundle = PipelineImport::Parse(BundleJson());
    ASSERT_TRUE(bundle.ok) << bundle.error;
    EXPECT_EQ(bundle.name, String("My pipe: v2"));
    ASSERT_EQ(bundle.graph.nodes.Count(), 2);
    ASSERT_EQ(bundle.graph.edges.Count(), 1);

    auto source = bundle.graph.FindNode("n1");
    ASSERT_TRUE(source);
    EXPECT_EQ(source->nodeType, String("sourceText"));
    EXPECT_EQ(source->position, Vec2F(10, 20));
    EXPECT_EQ(source->GetConfigString("text"), String("hello"));
    ASSERT_EQ(source->outputs.Count(), 1);
    EXPECT_EQ(source->outputs[0].id, String("o1"));

    auto gen = bundle.graph.FindNode("n2");
    ASSERT_TRUE(gen);
    EXPECT_EQ(gen->size, Vec2F(320, 400));
    ASSERT_EQ(gen->inputs.Count(), 2);
    EXPECT_TRUE(gen->inputs[1].custom);
    EXPECT_EQ(gen->inputs[1].portType, PipelinePortType::Image);
    EXPECT_EQ(gen->GetCustomInputs().Count(), 1);

    EXPECT_EQ(bundle.graph.edges[0]->fromPortId, String("o1"));
    EXPECT_EQ(bundle.graph.edges[0]->toPortId, String("i1"));
    EXPECT_EQ(bundle.graph.edges[0]->points.Count(), 1);

    ASSERT_EQ(bundle.results.Count(), 2);
    EXPECT_EQ(bundle.results["n1"].data, String("hello"));
    ASSERT_TRUE(bundle.results["n2"].IsImage());
    auto image = bundle.results["n2"].GetBitmap();
    ASSERT_TRUE(image);
    EXPECT_EQ(image->GetSize(), Vec2I(4, 3));
}

TEST(PipelineImport, ParsesZipBundlesWithAndWithoutManifest)
{
    for (bool withManifest : { true, false })
    {
        String bytes = BundleZip(withManifest);
        auto bundle = PipelineImport::ParseBytes(bytes);
        ASSERT_TRUE(bundle.ok) << bundle.error;
        EXPECT_EQ(bundle.name, String("My pipe: v2"));
        ASSERT_EQ(bundle.graph.nodes.Count(), 2);
        ASSERT_EQ(bundle.graph.edges.Count(), 1);

        // Results of nodes the pipeline does not have are dropped
        ASSERT_EQ(bundle.results.Count(), 2) << withManifest;
        EXPECT_EQ(bundle.results["n1"].data, String("hello"));
        ASSERT_TRUE(bundle.results["n2"].IsImage());
        EXPECT_EQ(bundle.results["n2"].mimeType, String("image/png"));
        auto image = bundle.results["n2"].GetBitmap();
        ASSERT_TRUE(image);
        EXPECT_EQ(image->GetSize(), Vec2I(4, 3));
    }

    EXPECT_FALSE(PipelineImport::ParseBytes(PipelineZip::Write({ { "readme.txt", "no pipeline here" } }, false)).ok);
}

TEST(PipelineImport, EveryImportGetsItsOwnCacheId)
{
    auto first = PipelineImport::ParseBytes(BundleJson());
    auto second = PipelineImport::ParseBytes(BundleZip(true));
    ASSERT_TRUE(first.ok && second.ok);
    EXPECT_FALSE(first.graph.id.IsEmpty());
    EXPECT_FALSE(second.graph.id.IsEmpty());
    EXPECT_NE(first.graph.id, second.graph.id);
}

TEST(PipelineImport, PlainPipelineJsonAndBrokenInput)
{
    auto plain = PipelineImport::Parse(R"({"schemaVersion":1,"id":"p","name":"Plain","nodes":[{"id":"a","type":"sourceText","position":{"x":0,"y":0},"config":{},"inputs":[],"outputs":[]}],"edges":[]})");
    ASSERT_TRUE(plain.ok) << plain.error;
    EXPECT_EQ(plain.graph.nodes.Count(), 1);
    EXPECT_TRUE(plain.results.IsEmpty());

    EXPECT_FALSE(PipelineImport::Parse("not json").ok);
    EXPECT_FALSE(PipelineImport::Parse(R"({"name":"x"})").ok);
    EXPECT_EQ(PipelineImport::SafeAssetName("My pipe: v2 / final"), String("My pipe_ v2 _ final"));
}

// Stored results are found the way the editor loads previews, and their nodes count as up to date
TEST(PipelineImport, StoresResultsAsPreviewsAndFreshness)
{
    ImportWorkDir work;
    auto bundle = PipelineImport::Parse(BundleJson());
    ASSERT_TRUE(bundle.ok);

    EXPECT_EQ(PipelineImport::StoreResults("pipe1", bundle), 2);

    String path;
    auto image = PipelineExecutor::LoadPreview("pipe1", *bundle.graph.FindNode("n2"), &path);
    EXPECT_TRUE(image.IsImage());
    EXPECT_TRUE(path.EndsWith(".png"));
    auto text = PipelineExecutor::LoadPreview("pipe1", *bundle.graph.FindNode("n1"), &path);
    EXPECT_EQ(text.data, String("hello"));

    auto fresh = PipelineExecutor::ComputeFreshNodes("pipe1", bundle.graph);
    EXPECT_TRUE(fresh.Contains("n2"));
}


// A v3 bundle brings the parts of a per-port node and the files its sources read
TEST(PipelineImport, ImportsPartsAndSourceUploadsOfAV3Bundle)
{
    ImportWorkDir work;
    auto bundle = PipelineImport::ParseBytes(BundleZipV3());
    ASSERT_TRUE(bundle.ok) << bundle.error;

    ASSERT_TRUE(bundle.portResults.ContainsKey("n3"));
    EXPECT_EQ(bundle.portResults["n3"].size(), 2u);
    EXPECT_TRUE(bundle.portResults["n3"]["pa"].IsImage());
    EXPECT_TRUE(bundle.results.IsEmpty());
    ASSERT_TRUE(bundle.uploads.ContainsKey("up1.png"));

    EXPECT_EQ(PipelineImport::StoreResults("pipe3", bundle), 2);
    EXPECT_TRUE(o2FileSystem.IsFileExist(PipelineUtils::GetUploadPath("up1.png")));

    // Each part comes back as its own preview and is a cache hit for the next run
    auto part = PipelineExecutor::LoadPortPreview("pipe3", "n3", "pb");
    ASSERT_TRUE(part.IsImage());
    EXPECT_EQ(part.GetBitmap()->GetSize(), Vec2I(4, 3));

    auto node = bundle.graph.FindNode("n3");
    ASSERT_TRUE(node);
    auto upstream = bundle.graph.UpstreamSignatures(*node, bundle.graph.ComputeSignatures());
    int seed = -1;
    bundle.graph.ResolveSeeds().TryGetValue("n3", seed);
    String sig = PipelineExecutor::PortSignature(*node, upstream, seed, "pa");
    EXPECT_TRUE(o2FileSystem.IsFileExist(PipelineExecutor::GetContentPath("pipe3", sig, "png")));
}
