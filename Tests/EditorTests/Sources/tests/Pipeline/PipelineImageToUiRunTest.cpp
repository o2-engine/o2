#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <set>
#if !defined(_WIN32)
#include <sys/stat.h>
#endif

#include "ImageToUiTestKit.h"
#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunSteps.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutWidgets.h"

using namespace o2;
using namespace Editor;
using namespace Editor::UiLayout;

namespace
{
    struct RunWorkDir
    {
        String path;

        RunWorkDir()
        {
            String relative = "./image-to-ui-run-" + (String)(int)(Math::Random()%1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = AbsolutePath(relative) + "/";
            PipelineUtils::SetWorkPathOverride(path);
        }

        ~RunWorkDir()
        {
            PipelineUtils::SetWorkPathOverride("");
            if (!getenv("O2_IMAGE_TO_UI_KEEP"))
                o2FileSystem.FolderRemove(path, true);
        }
    };

    struct Outcome
    {
        bool                       done = false;
        String                     fatal;
        Vector<String>             errors;
        Vector<PipelinePort>       ports;
        Map<String, PipelineValue> outputs;
        Vector<String>             logs;
    };

    Outcome RunGraph(PipelineGraph& graph, const String& target, const String& assets, const Vector<String>& bypass = {})
    {
        Outcome out;
        auto executor = mmake<PipelineExecutor>();
        executor->assetsPathOverride = assets;
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodePorts) out.ports = e.ports;
            if (e.type == PipelineExecEvent::Type::NodeOutput && e.portId.IsEmpty()) out.outputs[e.nodeId] = e.value;
            if (e.type == PipelineExecEvent::Type::NodeState && e.state == "error") out.errors.Add(e.nodeId + ": " + e.error);
            if (e.type == PipelineExecEvent::Type::Log) out.logs.Add(e.message);
            if (e.type == PipelineExecEvent::Type::Done) out.done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) out.fatal = e.error;
        };
        executor->Execute("itu", graph, target, bypass, false);
        EXPECT_TRUE(NetPumpUntil([&] { return out.done || !out.fatal.IsEmpty(); }, 900.0f)) << "the run did not end";
        return out;
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

    String Joined(const Vector<String>& lines)
    {
        String out;
        for (auto& l : lines)
            out += l + "\n";
        return out;
    }
}

// The whole node through the executor, AssetsLine's "a run through the executor": the analysis and the cuts stubbed, the
// agent's turns scripted to drive XuToWei/Image-To-UI's own scripts (real Python) to complete: true; the outputs it reports,
// the layout byte for byte, the skill's inputs, the trace, the boxes the prototype is built from, one cache for every output,
// and the regeneration asking the models again
TEST(ImageToUiRun, TheSkillCompletesThroughTheExecutor)
{
#if defined(_WIN32)
    GTEST_SKIP() << "the agent runs on POSIX hosts";
#endif
    if (ImageToUiHome().IsEmpty())
        GTEST_SKIP() << "no o2/Tools/ImageToUi: run tools/sync-image-to-ui.sh";
    String python = ImageToUiTestKit::PythonWithDependencies();
    if (python.IsEmpty())
        GTEST_SKIP() << "no Python 3 with Pillow and numpy";

    RunWorkDir work;
    String assets = work.path + "assets/";
    o2FileSystem.FolderCreate(assets, true);
    PipelineNodeRegistry::Get("sourceImage");
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("itu-mockup.png"), ImageToUiTestKit::MockupPng());

    PipelineGraph graph;
    auto source = AddNode(graph, "sourceImage");
    source->SetConfigString("uploadId", "itu-mockup.png");
    auto ui = AddNode(graph, "imageToUi");
    ui->SetConfigNumber("maxTurns", 20);
    ui->SetConfigString("screenName", "Test");
    ui->SetConfigString("sprites", "ai");
    Connect(graph, source, "out", ui, "image");

    String workDir = PipelineExecutor::GetWorkPath("itu", ui->id);
    ImageToUiTestKit::StubModels models(workDir + "task/");

    Outcome first = RunGraph(graph, ui->id, assets);
    ASSERT_TRUE(first.done) << first.fatal.Data() << "\n" << Joined(first.errors).Data() << Joined(first.logs).Data();
    ASSERT_TRUE(first.errors.IsEmpty()) << Joined(first.errors).Data() << Joined(first.logs).Data();

    // The ports: preview, layout, then one per sprite the layout names, by file stem
    ASSERT_GE(first.ports.Count(), 2);
    EXPECT_EQ(first.ports[0].name, "preview");
    EXPECT_EQ(first.ports[1].name, "layout");
    EXPECT_EQ(first.ports[1].portType, PipelinePortType::Text);
    std::set<std::string> names;
    for (int i = 2; i < first.ports.Count(); i++)
    {
        names.insert(first.ports[i].name.Data());
        EXPECT_EQ(first.ports[i].id, "sp-" + first.ports[i].name);
        EXPECT_EQ(first.ports[i].portType, PipelinePortType::Image);
    }
    EXPECT_EQ(names, std::set<std::string>({ "button_red", "icon_tile", "panel_blue" }));
    EXPECT_EQ(models.agent.Load(), 4) << "write, check, review, finalize";
    EXPECT_EQ(models.analysis.Load(), 1);
    EXPECT_GE(models.cuts.Load(), 3) << "a cut per look";
    EXPECT_EQ(models.other.Load(), 0) << "a flat backdrop asks for no plate";
    int firstCuts = models.cuts.Load();

    // The skill completed, and the layout is its ui_structure.json as it left it
    Json report;
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(workDir + "task/completion_report.json"), report));
    EXPECT_TRUE(report.Get("complete")->Bool()) << report.Get("blockers")->Dump(1).Data();
    PipelineValue layout = PipelineExecutor::LoadPortPreview("itu", ui->id, first.ports[1].id, PipelinePortType::Text);
    EXPECT_EQ(layout.data, PipelineUtils::ReadFileBytes(workDir + "task/ui_structure.json"));
    Json structure;
    ASSERT_TRUE(Json::Parse(layout.data, structure));
    EXPECT_EQ(structure.Get("canvas")->Get("width")->Num(), ImageToUiTestKit::W);
    EXPECT_EQ(structure.Get("canvas")->Get("name")->Str(), "Test");
    EXPECT_TRUE(first.outputs[ui->id].IsImage()) << "the preview is the skill's reconstruction";
    EXPECT_EQ(first.outputs[ui->id].GetPngBytes().Length() > 0, true);

    // The skill's inputs: the design, a flat sprite folder with nine-slice metas, the fonts where fontFamily resolves
    EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "input/design.png"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "input/Roboto-Bold.ttf")) << "a bare fontFamily resolves next to the design";
    EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "fonts/Roboto-Bold.ttf"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "fonts/LICENSE-Roboto.txt"));
    for (auto file : { "panel_blue.png", "icon_tile.png", "button_red.png" })
        EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "sprites/" + file)) << file;
    String meta = PipelineUtils::ReadFileBytes(workDir + "sprites/panel_blue.png.meta");
    EXPECT_TRUE(meta.StartsWith("fileFormatVersion: 2\nTextureImporter:\n  spriteBorder: {x: ")) << meta.Data();

    // The debug trace: the node's steps, then the skill's with its evidence, the host's output last
    Json trace;
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(workDir + "trace/trace.json"), trace));
    std::set<std::string> kinds;
    const Json* check = nullptr;
    for (auto& step : trace.Get("steps")->Items())
    {
        kinds.insert(step.Get("kind")->Str().Data());
        if (!check && step.Get("kind")->Str() == "check")
            check = &step;
        for (auto& image : step.Get("images")->Items())
            EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "trace/" + image.Get("file")->Str())) << image.Get("file")->Str().Data();
    }
    for (auto kind : { "input", "analysis", "sprite", "measure", "prepare", "write", "check", "finalize", "output" })
        EXPECT_TRUE(kinds.count(kind)) << kind;
    ASSERT_TRUE(check);
    Vector<String> titles;
    for (auto& image : check->Get("images")->Items())
        titles.Add(image.Get("title")->Str());
    EXPECT_EQ(titles, Vector<String>({ "comparison.png", "all_elements.png", "risk_review.png", "reconstruction.png" }));
    EXPECT_TRUE(check->Get("data")->Get("action")->IsObject()) << "an agent turn keeps its action";
    EXPECT_TRUE(trace.Get("summary")->Get("complete")->Bool());
    EXPECT_EQ(trace.Get("summary")->Get("sprites")->Num(), 3);
    EXPECT_EQ(trace.Get("steps")->Items().Last().Get("kind")->Str(), "output");
    EXPECT_EQ(trace.Get("steps")->Items().Last().Get("status")->Str(), "ok");
    EXPECT_EQ(trace.Get("design")->Get("file")->Str(), "design.png");
    auto run = PipelineImageToUi::ReadRunSummary(PipelineUtils::ReadFileBytes(workDir + "trace/trace.json"));
    ASSERT_TRUE(run.has_value());
    EXPECT_EQ(PipelineImageToUi::RunLine(*run), "completed \xC2\xB7 1 checks");

    // The prototype is built from the boxes the skill drew: the render trace's bboxes
    Json render;
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(workDir + "task/render_trace.json"), render));
    Vector<UiResolvedNode> boxes;
    ASSERT_TRUE(ResolveStructure(structure, boxes));
    int compared = 0;
    for (auto& element : render.Get("elements")->Items())
    {
        String path = element.Get("path")->Str();
        const Json* bbox = element.Get("bbox");
        for (auto& b : boxes)
        {
            if (b.path != path)
                continue;
            EXPECT_EQ(b.abs, Box(bbox->At(0)->Num(), bbox->At(1)->Num(), bbox->At(2)->Num(), bbox->At(3)->Num())) << path.Data();
            compared++;
        }
    }
    EXPECT_EQ(compared, boxes.Count());

    // The export: sprites and fonts of the prototype in the assets (not the editor's own: no .proto)
    for (auto file : { "panel_blue.png", "icon_tile.png", "button_red.png", "white.png" })
        EXPECT_TRUE(o2FileSystem.IsFileExist(assets + "Generated/UI/Test_Resources/Sprites/" + file)) << file;
    EXPECT_TRUE(o2FileSystem.IsFileExist(assets + "Generated/UI/Test_Resources/Fonts/Roboto-Bold.ttf"));
    EXPECT_FALSE(o2FileSystem.IsFileExist(assets + "Generated/UI/Test.proto"));
    // A nine-sliced sprite stretches on its own too: its borders are the image asset's default slice
    {
        UiLayout::Json panelMeta, tileMeta;
        ASSERT_TRUE(UiLayout::Json::Parse(PipelineUtils::ReadFileBytes(assets + "Generated/UI/Test_Resources/Sprites/panel_blue.png.meta"), panelMeta));
        ASSERT_TRUE(UiLayout::Json::Parse(PipelineUtils::ReadFileBytes(assets + "Generated/UI/Test_Resources/Sprites/icon_tile.png.meta"), tileMeta));
        const UiLayout::Json* value = panelMeta.Get("Value");
        ASSERT_NE(value, nullptr);
        EXPECT_NE(value->Get("mId"), nullptr);
        ASSERT_NE(value->Get("sliceBorder"), nullptr);
        EXPECT_GT(value->Get("sliceBorder")->Get("left")->Num(), 0.0);
        EXPECT_EQ(value->Get("defaultMode")->Str(), String("Sliced"));
        EXPECT_EQ(tileMeta.Get("Value")->Get("sliceBorder"), nullptr);
    }

    // Second run: every output from the one cache entry, no model call
    Outcome again = RunGraph(graph, ui->id, assets);
    ASSERT_TRUE(again.done) << again.fatal.Data();
    EXPECT_EQ(models.agent.Load(), 4);
    EXPECT_EQ(models.cuts.Load(), firstCuts);
    EXPECT_TRUE(again.logs.Any([](const String& l) { return l.Contains("imageToUi: cache hit"); })) << Joined(again.logs).Data();

    // Regenerate: the models are asked again and the task folder starts fresh
    Outcome fresh = RunGraph(graph, ui->id, assets, { ui->id });
    ASSERT_TRUE(fresh.done) << fresh.fatal.Data() << Joined(fresh.errors).Data();
    ASSERT_TRUE(fresh.errors.IsEmpty()) << Joined(fresh.errors).Data() << Joined(fresh.logs).Data();
    EXPECT_EQ(models.agent.Load(), 8);
    EXPECT_EQ(models.analysis.Load(), 2);
}

// An agent that cannot finish: the result says not ok and the node fails with the agent's reason; a Python without the
// dependencies fails with what to install
TEST(ImageToUiRun, AnAgentWithoutACheckedLayoutFailsTheNode)
{
#if defined(_WIN32)
    GTEST_SKIP() << "the agent runs on POSIX hosts";
#endif
    if (ImageToUiHome().IsEmpty())
        GTEST_SKIP() << "no o2/Tools/ImageToUi: run tools/sync-image-to-ui.sh";
    String python = ImageToUiTestKit::PythonWithDependencies();
    if (python.IsEmpty())
        GTEST_SKIP() << "no Python 3 with Pillow and numpy";

    RunWorkDir work;
    PipelineNodeRegistry::Get("sourceImage");
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("itu-mockup.png"), ImageToUiTestKit::MockupPng());
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceImage");
    source->SetConfigString("uploadId", "itu-mockup.png");
    auto ui = AddNode(graph, "imageToUi");
    ui->SetConfigNumber("maxTurns", 20);
    ui->SetConfigString("sprites", "ai");
    Connect(graph, source, "out", ui, "image");

    // The model stops at once
    SetAiReplay([](const AiReplayCall& call, String& data)
    {
        if (call.tag == "analysis")
            data = ImageToUiTestKit::Analysis();
        else if (call.tag.StartsWith("cut "))
            return ImageToUiTestKit::CutAnswer(call, data);
        else
        {
            Json stop = ImageToUiTestKit::ActionOf("stop");
            stop["reason"] = "nothing to do";
            data = ImageToUiTestKit::Act(stop);
        }
        return true;
    });
    Outcome stopped = RunGraph(graph, ui->id, work.path + "assets/");
    SetAiReplay({});
    ASSERT_EQ(stopped.errors.Count(), 1) << Joined(stopped.logs).Data();
    EXPECT_TRUE(stopped.errors[0].Contains("imageToUi: the agent ended without a checked layout")) << stopped.errors[0].Data();

    // A configured Python without Pillow and numpy: exit 3, its detail is the error
    String fake = work.path + "fake-python.sh";
    PipelineUtils::WriteFileBytes(fake, "#!/bin/sh\necho '@@bridge {\"type\": \"result\", \"ok\": false, \"error\": \"missing_dependencies\", "
                                        "\"detail\": \"Python 3.9 lacks numpy: install Pillow and numpy\"}'\nexit 3\n");
#if !defined(_WIN32)
    chmod(fake.Data(), 0755);
#endif
    PipelineSettings settings = PipelineSettings::Load();
    settings.imageToUiPython = fake;
    settings.Save();
    ImageToUiTestKit::StubModels models(PipelineExecutor::GetWorkPath("itu", ui->id) + "task/");
    Outcome missing = RunGraph(graph, ui->id, work.path + "assets/", { ui->id });
    settings.imageToUiPython = "";
    settings.Save();
    ASSERT_EQ(missing.errors.Count(), 1) << Joined(missing.logs).Data();
    EXPECT_TRUE(missing.errors[0].Contains("imageToUi: Python 3.9 lacks numpy: install Pillow and numpy")) << missing.errors[0].Data();
    EXPECT_EQ(models.agent.Load(), 0);
}
