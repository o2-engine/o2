#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/Sync/AssetsLineClient.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"

using namespace o2;
using namespace Editor;

// End-to-end sync against a real AssetsLine backend: start a throwaway one with
// AssetsLine/backend/scripts/test-instance.sh start and point
// O2_ASSETSLINE_TEST_URL at it (http://localhost:8799). The "web editor" side of
// each test is a signed-in session talking to the same API.

namespace
{
    String ServerUrl()
    {
        const char* url = getenv("O2_ASSETSLINE_TEST_URL");
        return url ? String(url) : String();
    }

    Ref<HttpResponse> SendSync(const Ref<HttpRequest>& request)
    {
        auto coroutine = o2Network.RequestAsync(request);
        coroutine.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 60.0f));
        return coroutine.GetResult();
    }

    // The web editor: a dev-login session (its cookie kept by the network system)
    struct Web
    {
        String url;
        String projectId;
        String email;

        explicit Web(const String& serverUrl): url(serverUrl)
        {
            email = "o2sync-" + (String)(int)Math::Random(0, 100000000) + "@example.com";
            DataDocument body;
            body.SetObject();
            body["email"] = email;
            auto res = Call(HttpMethod::Post, "/api/auth/dev", body);
            projectId = PipelineUtils::ValueToString(res["projects"][0]["id"]);
        }

        DataDocument Call(HttpMethod method, const String& path, const DataDocument* body = nullptr, int* status = nullptr)
        {
            auto request = mmake<HttpRequest>(url + path, method);
            request->useCookies = true;
            request->cachePolicy = HttpCachePolicy::Bypass;
            if (!projectId.IsEmpty())
                request->headers["X-Project-Id"] = projectId;

            if (body)
                request->SetBodyJson(*body);

            auto response = SendSync(request);
            DataDocument json;
            if (response)
            {
                json.LoadFromData(response->body);
                if (status)
                    *status = response->status;
            }

            return json;
        }

        DataDocument Call(HttpMethod method, const String& path, const DataDocument& body)
        {
            return Call(method, path, &body);
        }

        int PutBytes(const String& path, const String& bytes)
        {
            auto request = mmake<HttpRequest>(url + path, HttpMethod::Put);
            request->useCookies = true;
            request->headers["X-Project-Id"] = projectId;
            request->SetBody(bytes, "application/octet-stream");
            auto response = SendSync(request);
            return response ? response->status : 0;
        }

        String GetBytes(const String& path)
        {
            auto request = mmake<HttpRequest>(url + path, HttpMethod::Get);
            request->useCookies = true;
            request->cachePolicy = HttpCachePolicy::Bypass;
            request->headers["X-Project-Id"] = projectId;
            auto response = SendSync(request);
            return response ? response->body : String();
        }

        // What a run on the web leaves behind: the node's result, up to date with the stored graph
        void Generated(const String& pipelineId, int rev, const String& key, const String& nodeId, const String& png)
        {
            EXPECT_EQ(PutBytes("/api/sync/pipelines/" + pipelineId + "/result?key=" + AssetsLineClient::Encode(key) + "&ext=.png", png), 200);
            DataDocument body;
            body.SetObject();
            body["rev"] = rev;
            body["nodeIds"].SetArray();
            body["nodeIds"].AddElement() = nodeId;
            Call(HttpMethod::Post, "/api/sync/pipelines/" + pipelineId + "/fresh", body);
        }

        String Token()
        {
            DataDocument body;
            body.SetObject();
            body["projectId"] = projectId;
            body["name"] = "o2 sync test";
            return PipelineUtils::ValueToString(Call(HttpMethod::Post, "/api/connect/tokens", body)["token"]);
        }

        DataDocument Pipeline(const String& id)
        {
            return Call(HttpMethod::Get, "/api/sync/pipelines/" + id);
        }

        // Saves like the web editor's autosave: against the revision it has
        DataDocument Save(const DataValue& pipeline, int baseRev)
        {
            DataDocument body;
            body.SetObject();
            body["pipeline"] = pipeline;
            body["baseRev"] = baseRev;
            return Call(HttpMethod::Put, "/api/sync/pipelines/" + PipelineUtils::ValueToString(pipeline["id"]), body);
        }

        Vector<String> Names()
        {
            Vector<String> names;
            for (auto& item : Call(HttpMethod::Get, "/api/sync/state")["pipelines"])
                names.Add(PipelineUtils::ValueToString(item["name"]));
            return names;
        }
    };

    // An o2 project linked to the web user's project
    struct Project
    {
        String             root;
        String             assets;
        String             work;
        Ref<AssetsLineSync> sync;

        Project(const String& url, Web& web)
        {
            String relative = "./assetsline-sync-" + (String)(int)Math::Random(0, 100000000);
            o2FileSystem.FolderCreate(relative, true);
            root = o2FileSystem.CanonicalizePath(relative);
            assets = root + "/Assets/";
            work = root + "/Work/";
            o2FileSystem.FolderCreate(assets + "Pipelines", true);
            o2FileSystem.FolderCreate(work, true);

            Use();
            sync = mmake<AssetsLineSync>();
            sync->rebuildAssets = []() {};
            sync->openUrl = [](const String&) {};
            AssetsLineConfig config;
            config.enabled = true;
            sync->SetConfigOverride(config);
            sync->SetAssetsPath(assets);
            sync->ReloadState();
            sync->ConnectWithToken(url, web.Token());
            EXPECT_TRUE(NetPumpUntil([&] { return !sync->IsBusy(); }, 30.0f));
            EXPECT_TRUE(sync->IsConnected()) << sync->GetStatusText();
        }

        ~Project()
        {
            PipelineUtils::SetWorkPathOverride("");
            o2FileSystem.FolderRemove(root, true);
        }

        void Use()
        {
            PipelineUtils::SetWorkPathOverride(work);
        }

        bool Pass()
        {
            Use();
            auto coroutine = sync->SyncPass();
            coroutine.Start(JobThread::Main);
            EXPECT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 120.0f));
            return coroutine.GetResult();
        }

        void Write(const String& file, const String& text)
        {
            PipelineUtils::WriteFileBytes(assets + file, text);
        }

        PipelineGraph Read(const String& file)
        {
            PipelineGraph graph;
            EXPECT_TRUE(graph.LoadFromJsonString(PipelineUtils::ReadFileBytes(assets + file))) << file;
            return graph;
        }

        bool Exists(const String& file)
        {
            return o2FileSystem.IsFileExist(assets + file);
        }
    };

    String Uid()
    {
        UID uid;
        uid.Randomize();
        return (String)uid;
    }

    // A text source feeding a finish node
    String TextPipeline(const String& id, const String& name, const String& text)
    {
        return String(R"({"schemaVersion":1,"id":")") + id + R"(","name":")" + name + R"(","nodes":[
            {"id":")" + id + R"(-src","type":"sourceText","position":{"x":0,"y":0},"config":{"text":")" + text + R"("},
             "inputs":[],"outputs":[{"id":")" + id + R"(-so","name":"out","type":"text"}]},
            {"id":")" + id + R"(-fin","type":"finishText","position":{"x":400,"y":0},"config":{"filename":")" + name + R"("},
             "inputs":[{"id":")" + id + R"(-fi","name":"in","type":"text"}],"outputs":[]}],
            "edges":[{"id":")" + id + R"(-e","fromNodeId":")" + id + R"(-src","fromPortId":")" + id + R"(-so","toNodeId":")" + id +
            R"(-fin","toPortId":")" + id + R"(-fi"}]})";
    }

    String PngBytes(const Color4& color)
    {
        return PipelineValue::Image(PipelineImageOps::Blank(6, 4, color)).GetPngBytes();
    }

    // Colour of the middle pixel of a PNG file
    Color4 MiddlePixel(const String& file)
    {
        auto bitmap = DecodeImageBytes(PipelineUtils::ReadFileBytes(file));
        if (!bitmap)
            return Color4(0, 0, 0, 0);

        const UInt8* p = PipelineImageOps::Pixel(*bitmap, bitmap->GetSize().x/2, bitmap->GetSize().y/2);
        return Color4(p[0], p[1], p[2], p[3]);
    }

    // A text prompt, an image generation and a finish node saving the picture as Generated/<name>.png
    String GenPipeline(const String& id, const String& name)
    {
        return String(R"({"schemaVersion":1,"id":")") + id + R"(","name":")" + name + R"(","nodes":[
            {"id":")" + id + R"(-src","type":"sourceText","position":{"x":0,"y":0},"config":{"text":"a square"},
             "inputs":[],"outputs":[{"id":"so","name":"out","type":"text"}]},
            {"id":")" + id + R"(-gen","type":"nanoBananaGen","position":{"x":400,"y":0},"config":{"prompt":"x"},
             "inputs":[{"id":"gp","name":"prompt","type":"text"}],"outputs":[{"id":"go","name":"out","type":"image"}]},
            {"id":")" + id + R"(-fin","type":"finishImage","position":{"x":800,"y":0},"config":{"filename":")" + name + R"("},
             "inputs":[{"id":"fi","name":"in","type":"image"}],"outputs":[]}],
            "edges":[{"id":"e1","fromNodeId":")" + id + R"(-src","fromPortId":"so","toNodeId":")" + id + R"(-gen","toPortId":"gp"},
                     {"id":"e2","fromNodeId":")" + id + R"(-gen","fromPortId":"go","toNodeId":")" + id + R"(-fin","toPortId":"fi"}]})";
    }

    // The key a local run looks the node's generation up by
    String ContentSig(const PipelineGraph& graph, const String& nodeId, const String& portId = "")
    {
        auto sigs = graph.ComputeSignatures();
        auto node = graph.FindNode(nodeId);
        return PipelineExecutor::ContentSignature(*node, graph.UpstreamSignatures(*node, sigs), graph.ResolveSeeds()[nodeId], portId);
    }

    String ContentFile(const PipelineGraph& graph, const String& nodeId, const String& portId = "")
    {
        return PipelineExecutor::GetContentPath(graph.id, ContentSig(graph, nodeId, portId), "png");
    }
}

class AssetsLineSync_ : public ::testing::Test
{
protected:
    String url;

    void SetUp() override
    {
        url = ServerUrl();
        if (url.IsEmpty())
            GTEST_SKIP() << "set O2_ASSETSLINE_TEST_URL (AssetsLine/backend/scripts/test-instance.sh start)";
    }
};

TEST_F(AssetsLineSync_, ALocalPipelineIsStoredAndItsWebEditsComeBack)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    project.Write("Pipelines/alpha.pipeline", TextPipeline(id, "alpha", "hello"));
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    EXPECT_TRUE(web.Names().Contains("alpha"));
    auto stored = web.Pipeline(id);
    ASSERT_TRUE(stored.FindMember("pipeline"));
    EXPECT_EQ(PipelineUtils::ValueToString(stored["pipeline"]["nodes"][0]["config"]["text"]), "hello");
    EXPECT_EQ(project.sync->GetPipelineState(id)->rev, 1);

    // The web editor edits it
    auto& doc = stored["pipeline"];
    doc["nodes"][0]["config"]["text"] = "edited on the web";
    auto saved = web.Save(doc, 1);
    EXPECT_EQ((int)saved["pipeline"]["rev"], 2);

    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    EXPECT_EQ(project.Read("Pipelines/alpha.pipeline").nodes[0]->GetConfigString("text"), "edited on the web");
    EXPECT_EQ(project.sync->GetPipelineState(id)->rev, 2);
}

TEST_F(AssetsLineSync_, EditsMadeOnBothSidesAreMerged)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    project.Write("Pipelines/beta.pipeline", TextPipeline(id, "beta", "one"));
    ASSERT_TRUE(project.Pass());

    // The web moves the finish node while o2 changes the text
    auto stored = web.Pipeline(id);
    stored["pipeline"]["nodes"][1]["position"]["x"] = 777;
    web.Save(stored["pipeline"], 1);

    auto local = project.Read("Pipelines/beta.pipeline");
    local.nodes[0]->SetConfigString("text", "two");
    project.Write("Pipelines/beta.pipeline", local.ToJsonString());
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    auto mergedDoc = web.Pipeline(id);
    auto& merged = mergedDoc["pipeline"];
    EXPECT_EQ(PipelineUtils::ValueToString(merged["nodes"][0]["config"]["text"]), "two");
    EXPECT_EQ((int)merged["nodes"][1]["position"]["x"], 777);

    auto after = project.Read("Pipelines/beta.pipeline");
    EXPECT_EQ(after.nodes[0]->GetConfigString("text"), "two");
    EXPECT_EQ(after.nodes[1]->position.x, 777.0f);
}

TEST_F(AssetsLineSync_, NewAndRenamedPipelinesOfTheWebArriveAsAssets)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    DataDocument doc;
    doc.LoadFromData(TextPipeline(id, "from web", "x"));
    DataDocument body;
    body.SetObject();
    body["pipeline"] = static_cast<const DataValue&>(doc);
    int status = 0;
    auto created = web.Call(HttpMethod::Put, "/api/sync/pipelines/" + id, &body, &status);
    ASSERT_EQ(status, 200) << created.SaveAsString();

    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    ASSERT_TRUE(project.Exists("Pipelines/from web.pipeline"));
    PipelineUtils::WriteFileBytes(project.assets + "Pipelines/from web.pipeline.meta", "{\"Value\":{\"mId\":\"keep-me\"}}");

    auto storedDoc = web.Pipeline(id);
    auto& stored = storedDoc["pipeline"];
    stored["name"] = "renamed on web";
    web.Save(stored, 1);
    ASSERT_TRUE(project.Pass());
    EXPECT_FALSE(project.Exists("Pipelines/from web.pipeline"));
    ASSERT_TRUE(project.Exists("Pipelines/renamed on web.pipeline"));
    EXPECT_TRUE(project.Exists("Pipelines/renamed on web.pipeline.meta"));

    // A local rename goes the other way
    o2FileSystem.FileMove(project.assets + "Pipelines/renamed on web.pipeline", project.assets + "Pipelines/renamed in o2.pipeline");
    ASSERT_TRUE(project.Pass());
    EXPECT_TRUE(web.Names().Contains("renamed in o2"));
    EXPECT_FALSE(web.Names().Contains("renamed on web"));
}

TEST_F(AssetsLineSync_, DeletionsTravelButNeverEraseAnEdit)
{
    Web web(url);
    Project project(url, web);
    String a = Uid(), b = Uid();
    project.Write("Pipelines/a.pipeline", TextPipeline(a, "a", "x"));
    project.Write("Pipelines/b.pipeline", TextPipeline(b, "b", "x"));
    ASSERT_TRUE(project.Pass());

    // Deleted on the web, untouched here: gone here too
    web.Call(HttpMethod::Delete, "/api/sync/pipelines/" + a);
    ASSERT_TRUE(project.Pass());
    EXPECT_FALSE(project.Exists("Pipelines/a.pipeline"));

    // Deleted here: deleted there
    o2FileSystem.FileDelete(project.assets + "Pipelines/b.pipeline");
    ASSERT_TRUE(project.Pass());
    EXPECT_FALSE(web.Names().Contains("b"));

    // Deleted on the web while edited here: stored again
    String c = Uid();
    project.Write("Pipelines/c.pipeline", TextPipeline(c, "c", "x"));
    ASSERT_TRUE(project.Pass());
    web.Call(HttpMethod::Delete, "/api/sync/pipelines/" + c);
    auto local = project.Read("Pipelines/c.pipeline");
    local.nodes[0]->SetConfigString("text", "kept");
    project.Write("Pipelines/c.pipeline", local.ToJsonString());
    ASSERT_TRUE(project.Pass());
    EXPECT_TRUE(web.Names().Contains("c"));
}

TEST_F(AssetsLineSync_, ACopiedAssetBecomesAPipelineOfItsOwn)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    project.Write("Pipelines/orig.pipeline", TextPipeline(id, "orig", "x"));
    ASSERT_TRUE(project.Pass());
    o2FileSystem.FileCopy(project.assets + "Pipelines/orig.pipeline", project.assets + "Pipelines/orig copy.pipeline");
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    auto copy = project.Read("Pipelines/orig copy.pipeline");
    EXPECT_NE(copy.id, id);
    EXPECT_NE(copy.nodes[0]->id, id + "-src");
    EXPECT_EQ(project.Read("Pipelines/orig.pipeline").id, id);
    EXPECT_TRUE(web.Names().Contains("orig copy"));
}

TEST_F(AssetsLineSync_, UnsavedEditorEditsAreNeverOverwritten)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    project.Write("Pipelines/busy.pipeline", TextPipeline(id, "busy", "x"));
    ASSERT_TRUE(project.Pass());

    auto storedDoc = web.Pipeline(id);
    auto& stored = storedDoc["pipeline"];
    stored["nodes"][0]["config"]["text"] = "web";
    web.Save(stored, 1);

    bool busy = true;
    project.sync->isPipelineFileBusy = [&](const String& file) { return busy && file == "Pipelines/busy.pipeline"; };
    ASSERT_TRUE(project.Pass());
    EXPECT_EQ(project.Read("Pipelines/busy.pipeline").nodes[0]->GetConfigString("text"), "x");

    busy = false;
    ASSERT_TRUE(project.Pass());
    EXPECT_EQ(project.Read("Pipelines/busy.pipeline").nodes[0]->GetConfigString("text"), "web");
}

TEST_F(AssetsLineSync_, ResultsTravelBothWaysAndStayFresh)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    String gen = id + "-gen";
    String doc = String(R"({"schemaVersion":1,"id":")") + id + R"(","name":"results","nodes":[
        {"id":")" + id + R"(-src","type":"sourceText","position":{"x":0,"y":0},"config":{"text":"a cat"},
         "inputs":[],"outputs":[{"id":"so","name":"out","type":"text"}]},
        {"id":")" + gen + R"(","type":"nanoBananaGen","position":{"x":400,"y":0},"config":{"prompt":"x"},
         "inputs":[{"id":"gp","name":"prompt","type":"text"}],"outputs":[{"id":"go","name":"out","type":"image"}]}],
        "edges":[{"id":"e","fromNodeId":")" + id + R"(-src","fromPortId":"so","toNodeId":")" + gen + R"(","toPortId":"gp"}]})";
    project.Write("Pipelines/results.pipeline", doc);
    ASSERT_TRUE(project.Pass());

    // o2 ran the source: its result and freshness reach AssetsLine
    project.Use();
    auto graph = project.Read("Pipelines/results.pipeline");
    auto sigs = graph.ComputeSignatures();
    PipelineUtils::WriteFileBytes(PipelineExecutor::GetPreviewPath(id, id + "-src", "txt"), "a cat");
    PipelineExecutor::MarkRan(id, sigs[id + "-src"]);
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    auto manifest = web.Call(HttpMethod::Get, "/api/sync/pipelines/" + id + "/results");
    ASSERT_TRUE(manifest["results"].FindMember((id + "-src").Data()));
    bool fresh = false;
    for (auto& item : manifest["fresh"]) fresh = fresh || PipelineUtils::ValueToString(item) == id + "-src";
    EXPECT_TRUE(fresh);

    // The web generated the image: o2 gets it, fresh, and cached where a local run looks for it
    String png = PngBytes(Color4(0, 200, 0, 255));
    EXPECT_EQ(web.PutBytes("/api/sync/pipelines/" + id + "/result?key=" + gen + "&ext=.png", png), 200);
    DataDocument fresher;
    fresher.SetObject();
    fresher["rev"] = 1;
    fresher["nodeIds"].SetArray();
    fresher["nodeIds"].AddElement() = gen;
    web.Call(HttpMethod::Post, "/api/sync/pipelines/" + id + "/fresh", fresher);

    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    project.Use();
    EXPECT_EQ(PipelineUtils::ReadFileBytes(PipelineExecutor::GetPreviewPath(id, gen, "png")), png);
    EXPECT_TRUE(PipelineExecutor::ComputeFreshNodes(id, graph).Contains(gen));
    auto upstream = graph.UpstreamSignatures(*graph.FindNode(gen), sigs);
    String contentSig = PipelineExecutor::ContentSignature(*graph.FindNode(gen), upstream, graph.ResolveSeeds()[gen]);
    EXPECT_TRUE(o2FileSystem.IsFileExist(PipelineExecutor::GetContentPath(id, contentSig, "png")));

    // Nothing changed: the next pass moves no files
    auto before = project.sync->GetPipelineState(id)->results;
    ASSERT_TRUE(project.Pass());
    EXPECT_EQ(project.sync->GetPipelineState(id)->results.Count(), before.Count());
}

// A node generated again keeps its signature: the new picture must replace the generation a local run reads,
// or a run here that consumes the node brings the old picture back
TEST_F(AssetsLineSync_, AResultGeneratedAgainOnTheWebReplacesTheLocalGeneration)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    String gen = id + "-gen";
    project.Write("Pipelines/regen.pipeline", GenPipeline(id, "regen"));
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    web.Generated(id, 1, gen, gen, PngBytes(Color4(0, 200, 0, 255)));
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    String again = PngBytes(Color4(200, 0, 0, 255));
    web.Generated(id, 1, gen, gen, again);
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    project.Use();
    auto graph = project.Read("Pipelines/regen.pipeline");
    EXPECT_EQ(PipelineUtils::ReadFileBytes(PipelineExecutor::GetPreviewPath(id, gen, "png")), again);
    EXPECT_EQ(PipelineUtils::ReadFileBytes(ContentFile(graph, gen)), again);
    EXPECT_EQ(MiddlePixel(project.assets + "Generated/regen.png"), Color4(200, 0, 0, 255)) << "the finish node rewritten by the pass";

    // A run here that consumes the node takes the new picture and leaves the node's own result alone
    o2FileSystem.FileDelete(project.assets + "Generated/regen.png");
    auto executor = mmake<PipelineExecutor>();
    executor->assetsPathOverride = project.assets;
    executor->Execute(id, graph, id + "-fin");
    ASSERT_TRUE(NetPumpUntil([&] { return !executor->IsRunning(); }, 30.0f));
    EXPECT_EQ(MiddlePixel(project.assets + "Generated/regen.png"), Color4(200, 0, 0, 255));
    EXPECT_EQ(PipelineUtils::ReadFileBytes(PipelineExecutor::GetPreviewPath(id, gen, "png")), again);
}

// The other way: a node generated again here becomes the generation the web editor reuses, and a take of its history
TEST_F(AssetsLineSync_, AResultGeneratedAgainHereReplacesTheStoredGeneration)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    String gen = id + "-gen";
    project.Write("Pipelines/regen.pipeline", GenPipeline(id, "regen"));
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    // What a local run of the node leaves behind
    auto generate = [&](const Color4& color)
    {
        project.Use();
        auto graph = project.Read("Pipelines/regen.pipeline");
        auto value = PipelineValue::Image(PipelineImageOps::Blank(6, 4, color));
        PipelineExecutor::SaveContent(id, ContentSig(graph, gen), value);
        PipelineUtils::WriteFileBytes(PipelineExecutor::GetPreviewPath(id, gen, "png"), value.GetPngBytes());
        PipelineExecutor::MarkRan(id, graph.ComputeSignatures()[gen]);
        return value.GetPngBytes();
    };

    generate(Color4(0, 0, 200, 255));
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    String again = generate(Color4(200, 200, 0, 255));
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    DataDocument ask;
    ask.SetObject();
    ask["nodeIds"].SetArray();
    ask["nodeIds"].AddElement() = gen;
    auto history = web.Call(HttpMethod::Post, "/api/history/list", ask);
    auto entries = history.FindMember(gen.Data());
    ASSERT_TRUE(entries) << history.SaveAsString();
    ASSERT_EQ((*entries)["entries"].GetElementsCount(), 2) << history.SaveAsString();
    EXPECT_EQ(web.GetBytes(PipelineUtils::ValueToString((*entries)["entries"][0]["url"])), again);
}

// One part of an extract node generated again on the web replaces that part's generation here, and only it
TEST_F(AssetsLineSync_, APartGeneratedAgainOnTheWebReplacesThatPartHere)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    String ex = id + "-ex";
    project.Write("Pipelines/parts.pipeline", String(R"({"schemaVersion":1,"id":")") + id + R"(","name":"parts","nodes":[
        {"id":")" + ex + R"(","type":"imageExtract","position":{"x":0,"y":0},
         "config":{"regions":[{"id":"pa","name":"a","x":0,"y":0,"w":0.5,"h":1},{"id":"pb","name":"b","x":0.5,"y":0,"w":0.5,"h":1}]},
         "inputs":[{"id":"xi","name":"image","type":"image"}],
         "outputs":[{"id":"pa","name":"a","type":"image"},{"id":"pb","name":"b","type":"image"}]}],"edges":[]})");
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    String first = PngBytes(Color4(10, 10, 10, 255));
    web.Generated(id, 1, ex + "#pa", ex, first);
    web.Generated(id, 1, ex + "#pb", ex, first);
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    String again = PngBytes(Color4(0, 120, 240, 255));
    web.Generated(id, 1, ex + "#pb", ex, again);
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();

    project.Use();
    auto graph = project.Read("Pipelines/parts.pipeline");
    EXPECT_EQ(PipelineUtils::ReadFileBytes(ContentFile(graph, ex, "pb")), again);
    EXPECT_EQ(PipelineUtils::ReadFileBytes(ContentFile(graph, ex, "pa")), first);
}

TEST_F(AssetsLineSync_, WhatReachesAFinishNodeBecomesAProjectAsset)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    project.Write("Pipelines/fin.pipeline", TextPipeline(id, "fin", "the words"));
    ASSERT_TRUE(project.Pass());

    // The web ran the source; the finish node was never run anywhere
    EXPECT_EQ(web.PutBytes("/api/sync/pipelines/" + id + "/result?key=" + id + "-src&ext=.txt", "the words"), 200);
    DataDocument fresher;
    fresher.SetObject();
    fresher["rev"] = 1;
    fresher["nodeIds"].SetArray();
    fresher["nodeIds"].AddElement() = id + "-src";
    web.Call(HttpMethod::Post, "/api/sync/pipelines/" + id + "/fresh", fresher);

    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    ASSERT_TRUE(project.Exists("Generated/fin.txt"));
    EXPECT_EQ(PipelineUtils::ReadFileBytes(project.assets + "Generated/fin.txt"), "the words");

    // The local finish run's result goes back as the finish node's result
    ASSERT_TRUE(project.Pass());
    auto manifest = web.Call(HttpMethod::Get, "/api/sync/pipelines/" + id + "/results");
    EXPECT_TRUE(manifest["results"].FindMember((id + "-fin").Data()));
}

TEST_F(AssetsLineSync_, SourceFilesTravelBothWays)
{
    Web web(url);
    Project project(url, web);
    String id = Uid();
    String png = PngBytes(Color4(10, 20, 30, 255));

    // An o2 asset as a source: it becomes an upload named by its content
    PipelineUtils::WriteFileBytes(project.assets + "Art/hero.png", png);
    project.Write("Pipelines/src.pipeline", String(R"({"schemaVersion":1,"id":")") + id + R"(","name":"src","nodes":[
        {"id":"s1","type":"sourceImage","position":{"x":0,"y":0},"config":{"assetPath":"Art/hero.png"},
         "inputs":[],"outputs":[{"id":"o1","name":"out","type":"image"}]}],"edges":[]})");
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    String uploadId = project.Read("Pipelines/src.pipeline").nodes[0]->GetConfigString("uploadId");
    ASSERT_TRUE(uploadId.StartsWith("o2-")) << uploadId;
    ASSERT_TRUE(project.Pass());
    auto manifest = web.Call(HttpMethod::Get, "/api/sync/pipelines/" + id + "/results");
    EXPECT_TRUE((bool)manifest["uploads"][uploadId.Data()]);

    // An upload of the web: o2 fetches it for the source node
    String webUpload = Uid() + ".png";
    String webPng = PngBytes(Color4(200, 0, 0, 255));
    EXPECT_EQ(web.PutBytes("/api/sync/uploads/" + webUpload, webPng), 200);
    auto storedDoc = web.Pipeline(id);
    auto& stored = storedDoc["pipeline"];
    stored["nodes"][0]["config"].RemoveMember("assetPath");
    stored["nodes"][0]["config"]["uploadId"] = webUpload;
    web.Save(stored, (int)stored["rev"]);
    ASSERT_TRUE(project.Pass()) << project.sync->GetStatusText();
    project.Use();
    EXPECT_EQ(PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(webUpload)), webPng);
}

TEST_F(AssetsLineSync_, ConnectingByCodeWaitsForTheApproval)
{
    Web web(url);
    String relative = "./assetsline-connect-" + (String)(int)Math::Random(0, 100000000);
    o2FileSystem.FolderCreate(relative, true);
    String root = o2FileSystem.CanonicalizePath(relative);
    PipelineUtils::SetWorkPathOverride(root + "/");
    auto sync = mmake<AssetsLineSync>();
    String opened;
    sync->openUrl = [&](const String& page) { opened = page; };
    sync->SetConfigOverride(AssetsLineConfig());
    sync->ReloadState();
    sync->StartConnect(url);
    ASSERT_TRUE(NetPumpUntil([&] { return !sync->GetConnectCode().IsEmpty(); }, 20.0f));
    EXPECT_TRUE(opened.Contains("#/connect?code="));
    EXPECT_EQ(sync->GetStatus(), AssetsLineStatus::Connecting);

    DataDocument approve;
    approve.SetObject();
    approve["code"] = sync->GetConnectCode();
    approve["projectId"] = web.projectId;
    web.Call(HttpMethod::Post, "/api/connect/approve", approve);

    ASSERT_TRUE(NetPumpUntil([&] { return !sync->IsBusy(); }, 30.0f));
    EXPECT_TRUE(sync->IsConnected()) << sync->GetStatusText();
    EXPECT_EQ(sync->GetConfig().projectId, web.projectId);
    EXPECT_EQ(sync->GetUserEmail(), web.email);
    EXPECT_TRUE(sync->GetConfig().enabled);

    PipelineUtils::SetWorkPathOverride("");
    o2FileSystem.FolderRemove(root, true);
}

TEST_F(AssetsLineSync_, ARevokedTokenSaysSo)
{
    Web web(url);
    Project project(url, web);
    ASSERT_TRUE(project.Pass());
    for (auto& token : web.Call(HttpMethod::Get, "/api/connect/tokens"))
        web.Call(HttpMethod::Delete, "/api/connect/tokens/" + PipelineUtils::ValueToString(token["id"]));

    EXPECT_FALSE(project.Pass());
    EXPECT_EQ(project.sync->GetStatus(), AssetsLineStatus::Error);
    EXPECT_TRUE(project.sync->GetStatusText().Contains("token")) << project.sync->GetStatusText();
}
