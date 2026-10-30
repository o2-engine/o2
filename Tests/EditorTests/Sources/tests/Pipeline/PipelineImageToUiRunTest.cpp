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

namespace
{
    // The ids of a trace's steps
    Vector<String> StepIds(const String& traceJson)
    {
        Json trace;
        Vector<String> ids;
        if (!Json::Parse(traceJson, trace) || !trace.Get("steps"))
            return ids;
        for (auto& step : trace.Get("steps")->Items())
            ids.Add(step.Get("id") ? step.Get("id")->Str() : String());
        return ids;
    }

    Vector<String> LayoutNames(const String& pipelineId, const Ref<PipelineNode>& node)
    {
        Vector<String> names;
        auto port = node->outputs.Find([](const PipelinePort& p) { return p.name == "layout"; });
        if (!port)
            return names;
        Json structure;
        if (!Json::Parse(PipelineExecutor::LoadPortPreview(pipelineId, node->id, port->id, PipelinePortType::Text).data, structure))
            return names;
        for (auto& child : structure.Get("root")->Get("children")->Items())
            names.Add(child.Get("name") ? child.Get("name")->Str() : String());
        return names;
    }

    // Drops every cached bundle of the pipeline: the next run of the node has to make its outputs again
    void DropBundles(const String& pipelineId)
    {
        String folder = PipelineExecutor::GetCachePath(pipelineId) + "content/";
        for (auto& file : o2FileSystem.GetFolderInfo(folder).files)
        {
            if (UiLayout::HasSuffix(file.path, "-outputs.json"))
                o2FileSystem.FileDelete(file.path);
        }
    }

    PipelineImageToUiEditRound Round(const String& id, const String& text, bool point)
    {
        PipelineImageToUiEditItem item;
        item.text = text;
        if (point)
            item.at = PipelineImageToUiEditPoint{ "result", 120, 245 };
        return { id, { item } };
    }

    // A node fed by the mockup, run with the stubs: maxTurns 20, the sprites cut by the model, a draft that is never sent
    struct EditsGraph
    {
        PipelineGraph     graph;
        Ref<PipelineNode> source;
        Ref<PipelineNode> ui;

        EditsGraph()
        {
            PipelineNodeRegistry::Get("sourceImage");
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("itu-mockup.png"), ImageToUiTestKit::MockupPng());
            source = AddNode(graph, "sourceImage");
            source->SetConfigString("uploadId", "itu-mockup.png");
            ui = AddNode(graph, "imageToUi");
            ui->SetConfigNumber("maxTurns", 20);
            ui->SetConfigString("screenName", "Test");
            ui->SetConfigString("sprites", "ai");
            PipelineImageToUiEditItem draft;
            draft.text = "not sent";
            PipelineImageToUi::WriteDraft(*ui, { draft });
            PipelineImageToUi::WriteEdits(*ui, {});
            Connect(graph, source, "out", ui, "image");
        }

        Outcome Run(const Vector<PipelineImageToUiEditRound>& edits, const String& assets)
        {
            PipelineImageToUi::WriteEdits(*ui, edits);
            return RunGraph(graph, ui->id, assets);
        }
    };
}

// A round of edits continues the finished run: no analysis, no cut, five turns of its own (rename, check, two exact edits of
// the review, finalize); removing it is the cached result of the run without it; a lost cache is rebuilt from the kept files
// with one check and no model call; a round whose model gives out fails the node and the round before stays the result
TEST(ImageToUiRun, ARoundOfEditsContinuesTheRunAndUndoBringsTheResultBack)
{
#if defined(_WIN32)
    GTEST_SKIP() << "the agent runs on POSIX hosts";
#endif
    if (ImageToUiHome().IsEmpty())
        GTEST_SKIP() << "no o2/Tools/ImageToUi: run tools/sync-image-to-ui.sh";
    if (ImageToUiTestKit::PythonWithDependencies().IsEmpty())
        GTEST_SKIP() << "no Python 3 with Pillow and numpy";

    RunWorkDir work;
    String assets = work.path + "assets/";
    o2FileSystem.FolderCreate(assets, true);
    EditsGraph g;
    String workDir = PipelineExecutor::GetWorkPath("itu", g.ui->id);
    ImageToUiTestKit::StubModels models(workDir + "task/");
    auto ok = [](const Outcome& o) { return o.done && o.errors.IsEmpty() && o.fatal.IsEmpty(); };

    Outcome first = g.Run({}, assets);
    ASSERT_TRUE(ok(first)) << first.fatal.Data() << Joined(first.errors).Data() << Joined(first.logs).Data();
    EXPECT_EQ(models.agent.Load(), 4);
    int cuts = models.cuts.Load();
    int baseSteps = StepIds(PipelineUtils::ReadFileBytes(workDir + "trace/trace.json")).Count();

    // A round: the agent continues the finished task folder and the layout is what it made of it
    auto round = Round("r1", "call the red button start", true);
    Outcome edited = g.Run({ round }, assets);
    ASSERT_TRUE(ok(edited)) << edited.fatal.Data() << Joined(edited.errors).Data() << Joined(edited.logs).Data();
    EXPECT_EQ(models.agent.Load(), 9) << "patch, check, two review edits, finalize";
    EXPECT_EQ(models.analysis.Load(), 1);
    EXPECT_EQ(models.cuts.Load(), cuts) << "no sprite is cut again";
    EXPECT_TRUE(edited.logs.Any([](const String& l) { return l.Contains("imageToUi: edit round 1 completed"); })) << Joined(edited.logs).Data();
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("start"));
    Json report;
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(workDir + "task/completion_report.json"), report));
    EXPECT_TRUE(report.Get("complete")->Bool()) << report.Get("blockers")->Dump(1).Data();
    EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "edits/round-1.json"));

    String traceText = PipelineUtils::ReadFileBytes(workDir + "trace/trace.json");
    Json trace;
    ASSERT_TRUE(Json::Parse(traceText, trace));
    auto& steps = trace.Get("steps")->Items();
    ASSERT_GT(steps.Count(), baseSteps + 3);
    EXPECT_EQ(steps[baseSteps - 1].Get("kind")->Str(), "output") << "the run's own steps are kept";
    EXPECT_EQ(steps[baseSteps].Get("id")->Str(), "edit1-check");
    EXPECT_EQ(steps[baseSteps + 1].Get("id")->Str(), "edit1-request");
    EXPECT_EQ(steps[baseSteps + 1].Get("kind")->Str(), "request");
    EXPECT_EQ(steps[baseSteps + 2].Get("id")->Str(), "edit1-turn-1");
    EXPECT_EQ(steps.Last().Get("id")->Str(), "edit1-output");
    EXPECT_TRUE(steps.Last().Get("title")->Str().StartsWith("Edit 1 \xC2\xB7 Done")) << steps.Last().Get("title")->Str().Data();
    EXPECT_EQ(trace.Get("summary")->Get("edits")->Num(), 1);
    for (auto& image : steps[baseSteps + 1].Get("images")->Items())
        EXPECT_TRUE(image.Get("file")->Str().StartsWith("edit1-")) << "a round's pictures are named after it";
    Json kept;
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(workDir + "rounds.json"), kept));
    ASSERT_EQ(kept.Get("rounds")->Count(), 1);
    EXPECT_EQ(kept.Get("rounds")->At(0)->Get("hash")->Str(), ImageToUiRoundHash(round));

    // Removing the round: the cached result of the run without it, no call
    Outcome undone = g.Run({}, assets);
    ASSERT_TRUE(ok(undone)) << Joined(undone.errors).Data();
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("play"));
    EXPECT_EQ(models.agent.Load(), 9);
    EXPECT_TRUE(undone.logs.Any([](const String& l) { return l.Contains("imageToUi: cache hit"); })) << Joined(undone.logs).Data();
    // The debug view reads the trace of the state its edits name: the cached result did not rewrite trace/
    auto idsOf = [&](const Vector<PipelineImageToUiEditRound>& edits)
    {
        return StepIds(PipelineUtils::ReadFileBytes(PipelineImageToUi::StateTracePath(workDir, edits)));
    };
    EXPECT_FALSE(idsOf({}).Any([](const String& id) { return id.StartsWith("edit1-"); }));
    EXPECT_TRUE(idsOf({ round }).Contains("edit1-request"));
    EXPECT_TRUE(idsOf({ Round("nope", "x", false) }).Contains("edit1-request")) << "an unknown state: the live trace";

    // A lost cache: the kept files of that state go back into the task folder and one check makes the reconstruction again
    DropBundles("itu");
    Outcome restored = g.Run({}, assets);
    ASSERT_TRUE(ok(restored)) << Joined(restored.errors).Data() << Joined(restored.logs).Data();
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("play"));
    EXPECT_TRUE(restored.logs.Any([](const String& l) { return l.Contains("continuing from the run without edits"); })) << Joined(restored.logs).Data();
    DropBundles("itu");
    Outcome again = g.Run({ round }, assets);
    ASSERT_TRUE(ok(again)) << Joined(again.errors).Data() << Joined(again.logs).Data();
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("start"));
    EXPECT_EQ(models.agent.Load(), 9);
    EXPECT_EQ(models.analysis.Load(), 1);
    EXPECT_EQ(models.cuts.Load(), cuts);
    auto live = StepIds(PipelineUtils::ReadFileBytes(workDir + "trace/trace.json"));
    EXPECT_EQ(live.Count([](const String& id) { return id == "edit1-request"; }), 1) << "the trace is the one of that state, not a pile-up";
    EXPECT_EQ(live.Last(), "edit1-output");

    // The task folder itself gone (a hundred megabytes of evidence, the first thing to clean up): prepared afresh around the
    // kept files - prepare refuses a non-empty folder without its state - and still no model call
    o2FileSystem.FolderRemove(workDir + "task", true);
    DropBundles("itu");
    Outcome cleaned = g.Run({ round }, assets);
    ASSERT_TRUE(ok(cleaned)) << Joined(cleaned.errors).Data() << Joined(cleaned.logs).Data();
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("start"));
    EXPECT_EQ(models.agent.Load(), 9);
    EXPECT_EQ(models.analysis.Load(), 1);
    EXPECT_EQ(models.cuts.Load(), cuts);
    EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "task/workflow_state.json"));
    EXPECT_EQ(PipelineUtils::ReadFileBytes(workDir + "task/alignment_review.md"), PipelineUtils::ReadFileBytes(workDir + "rounds/1/alignment_review.md"));

    // Only its state lost: the same
    o2FileSystem.FileDelete(workDir + "task/workflow_state.json");
    DropBundles("itu");
    Outcome stateless = g.Run({ round }, assets);
    ASSERT_TRUE(ok(stateless)) << Joined(stateless.errors).Data() << Joined(stateless.logs).Data();
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("start"));
    EXPECT_EQ(models.agent.Load(), 9);

    // A round its model gives out on: the node fails with why, nothing of it is kept, the round before stays the result
    models.failSecondRound.Store(1);
    Outcome failed = g.Run({ round, Round("r2", "make the panel green", false) }, assets);
    ASSERT_EQ(failed.errors.Count(), 1) << Joined(failed.logs).Data();
    EXPECT_TRUE(failed.errors[0].Contains("imageToUi: edit round 2 stopped \xE2\x80\x94 the model call failed")) << failed.errors[0].Data();
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(workDir + "rounds.json"), kept));
    EXPECT_EQ(kept.Get("rounds")->Count(), 1);
    models.failSecondRound.Store(0);
    Outcome back = g.Run({ round }, assets);
    ASSERT_TRUE(ok(back)) << Joined(back.errors).Data();
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("start"));

    // The same round again after the failure, its cache lost: state 1 is restored from the files kept for it, the failed
    // round's changes to the task folder are gone
    DropBundles("itu");
    Outcome rebuilt = g.Run({ round }, assets);
    ASSERT_TRUE(ok(rebuilt)) << Joined(rebuilt.errors).Data() << Joined(rebuilt.logs).Data();
    EXPECT_TRUE(rebuilt.logs.Any([](const String& l) { return l.Contains("continuing from edit round 1"); })) << Joined(rebuilt.logs).Data();
    EXPECT_EQ(PipelineUtils::ReadFileBytes(workDir + "task/ui_structure.json"), PipelineUtils::ReadFileBytes(workDir + "rounds/1/ui_structure.json"));
}

// A node run before rounds existed (no rounds.json, a finished task folder made from the same design) is adopted as the run
// without edits: its first round runs the agent only
TEST(ImageToUiRun, ARunFromBeforeTheRoundsIsAdopted)
{
#if defined(_WIN32)
    GTEST_SKIP() << "the agent runs on POSIX hosts";
#endif
    if (ImageToUiHome().IsEmpty())
        GTEST_SKIP() << "no o2/Tools/ImageToUi: run tools/sync-image-to-ui.sh";
    if (ImageToUiTestKit::PythonWithDependencies().IsEmpty())
        GTEST_SKIP() << "no Python 3 with Pillow and numpy";

    RunWorkDir work;
    String assets = work.path + "assets/";
    o2FileSystem.FolderCreate(assets, true);
    EditsGraph g;
    String workDir = PipelineExecutor::GetWorkPath("itu", g.ui->id);
    ImageToUiTestKit::StubModels models(workDir + "task/");

    Outcome first = g.Run({}, assets);
    ASSERT_TRUE(first.done && first.errors.IsEmpty()) << Joined(first.errors).Data() << Joined(first.logs).Data();
    o2FileSystem.FileDelete(workDir + "rounds.json");
    o2FileSystem.FolderRemove(workDir + "rounds", true);
    DropBundles("itu");

    Outcome edited = g.Run({ Round("r1", "call the red button start", true) }, assets);
    ASSERT_TRUE(edited.done && edited.errors.IsEmpty()) << Joined(edited.errors).Data() << Joined(edited.logs).Data();
    EXPECT_EQ(models.analysis.Load(), 1) << "no new analysis";
    EXPECT_EQ(models.agent.Load(), 9);
    EXPECT_TRUE(LayoutNames("itu", g.ui).Contains("start"));
    Json kept;
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(workDir + "rounds.json"), kept));
    EXPECT_EQ(kept.Get("rounds")->Count(), 1);
    EXPECT_TRUE(kept.Get("note")->Str().StartsWith("About the sliced assets")) << "the sprites note is made again from the folder";
    EXPECT_TRUE(kept.Get("note")->Str().Contains("The backdrop is a flat colour #")) << kept.Get("note")->Str().Data();
    EXPECT_TRUE(o2FileSystem.IsFileExist(workDir + "rounds/0/ui_structure.json"));

    // Another design is not adopted: the run starts over
    o2FileSystem.FileDelete(workDir + "rounds.json");
    o2FileSystem.FolderRemove(workDir + "rounds", true);
    DropBundles("itu");
    UiImage other;
    ASSERT_TRUE(UiImage::Decode(ImageToUiTestKit::MockupPng(), other));
    other.At(1, 1)[0] = 10;
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("itu-mockup-2.png"), other.Png());
    g.source->SetConfigString("uploadId", "itu-mockup-2.png");
    Outcome fresh = g.Run({}, assets);
    ASSERT_TRUE(fresh.done && fresh.errors.IsEmpty()) << Joined(fresh.errors).Data() << Joined(fresh.logs).Data();
    EXPECT_EQ(models.analysis.Load(), 2);
}
