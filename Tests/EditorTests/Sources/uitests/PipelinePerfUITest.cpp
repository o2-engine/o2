#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <thread>
#if defined(__APPLE__)
#include <mach/mach.h>
#endif

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Application/VKCodes.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerLayers.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerStage.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineModelPicker.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"

using namespace o2;
using namespace Editor;

// Timed scenarios of the Pipeline window on real big pipelines (O2_PIPELINE_PERF=1 or a project folder): per-frame and
// per-action times of the main user actions, printed as a table and written as JSON (O2_PIPELINE_PERF_OUT)

namespace
{
    double NowMs()
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    struct PerfStats
    {
        int    count = 0;
        double min = 0, median = 0, p95 = 0, max = 0;
    };

    PerfStats StatsOf(Vector<double> values)
    {
        PerfStats s;
        s.count = values.Count();
        if (values.IsEmpty())
            return s;

        std::sort(values.begin(), values.end());
        auto at = [&](double q) { return values[Math::Clamp((int)Math::Round(q*(values.Count() - 1)), 0, values.Count() - 1)]; };
        s.min = values[0];
        s.median = at(0.5);
        s.p95 = at(0.95);
        s.max = values.Last();
        return s;
    }

    struct PerfScenario
    {
        String         name;
        String         graph;
        Vector<double> frames;  // Every frame of the scenario
        Vector<double> actions; // The frames an input landed in, or the time of a one-shot call
    };

    struct PerfReport
    {
        Vector<PerfScenario> scenarios;

        static PerfReport& Get()
        {
            static PerfReport report;
            return report;
        }

        static String OutPath()
        {
            const char* env = getenv("O2_PIPELINE_PERF_OUT");
            return env ? String(env) : String("../../Work/Pipelines/perf-results.json");
        }

        void Add(const PerfScenario& scenario)
        {
            scenarios.RemoveAll([&](const PerfScenario& s) { return s.name == scenario.name && s.graph == scenario.graph; });
            scenarios.Add(scenario);
            auto f = StatsOf(scenario.frames), a = StatsOf(scenario.actions);
            printf("[perf] %-34s %-9s frames %4d  med %7.2f  p95 %7.2f  max %8.2f | actions %3d  med %7.2f  max %8.2f ms\n",
                   scenario.name.Data(), scenario.graph.Data(), f.count, f.median, f.p95, f.max, a.count, a.median, a.max);
            fflush(stdout);
            Write();
        }

        void Write() const
        {
            auto stats = [](const PerfStats& s)
            {
                char buf[256];
                snprintf(buf, sizeof(buf), "{\"count\": %d, \"min\": %.2f, \"median\": %.2f, \"p95\": %.2f, \"max\": %.2f}",
                         s.count, s.min, s.median, s.p95, s.max);
                return String(buf);
            };

            String json = "{\n  \"build\": \"Debug\",\n  \"scenarios\": [\n";
            for (int i = 0; i < scenarios.Count(); i++)
            {
                auto& s = scenarios[i];
                json += "    {\"name\": \"" + s.name + "\", \"graph\": \"" + s.graph + "\", \"frames\": " + stats(StatsOf(s.frames)) +
                    ", \"actions\": " + stats(StatsOf(s.actions)) + "}" + (i + 1 < scenarios.Count() ? ",\n" : "\n");
            }
            json += "  ]\n}\n";
            PipelineUtils::WriteFileBytes(OutPath(), json);
        }
    };

    // One editor frame as the editor application has it: the UI root updated and drawn, then the application frame that
    // delivers the input and runs the main-thread jobs
    double Frame()
    {
        double start = NowMs();
        {
            PushEditorScopeOnStack scope;
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(30, 31, 34, 255));
            auto root = EditorUIRoot.GetRootWidget();
            root->Update(1.0f/60.0f);
            root->UpdateChildren(1.0f/60.0f);
            root->UpdateChildrenTransforms();
            root->Draw();
            o2UI.DrawCurrentLayerTopWidgets();
            o2Render.End();
        }
        AppTestDriver::PumpFrames(1);
        return NowMs() - start;
    }

    // Frames of the scenario being measured
    struct Recorder
    {
        PerfScenario scenario;

        Recorder(const String& name, const String& graph) { scenario.name = name; scenario.graph = graph; }

        double Step(bool action = false)
        {
            double ms = Frame();
            scenario.frames.Add(ms);
            if (ms > 50.0 && getenv("O2_PIPELINE_PERF_TRACE"))
                printf("[perf] slow frame: %s #%d %.0f ms\n", scenario.name.Data(), scenario.frames.Count() - 1, ms);
            if (action)
                scenario.actions.Add(ms);
            return ms;
        }

        void Steps(int count) { for (int i = 0; i < count; i++) Step(); }

        void AddAction(double ms) { scenario.actions.Add(ms); }

        // Reports the scenario; a budget, when given, fails a frame or an action over it (Debug numbers, generous)
        void Done(double maxFrameMs = 0.0, double maxActionMs = 0.0)
        {
            PerfReport::Get().Add(scenario);
            if (maxFrameMs > 0.0)
                EXPECT_LT(StatsOf(scenario.frames).max, maxFrameMs) << scenario.name.Data() << ", " << scenario.graph.Data();
            if (maxActionMs > 0.0)
                EXPECT_LT(StatsOf(scenario.actions).max, maxActionMs) << scenario.name.Data() << ", " << scenario.graph.Data();
        }
    };

    // Physical footprint of the process, MB
    double FootprintMb()
    {
#if defined(__APPLE__)
        task_vm_info_data_t info;
        mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
        if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
            return 0.0;
        return info.phys_footprint/(1024.0*1024.0);
#else
        return 0.0;
#endif
    }

    String ProjectRoot()
    {
        const char* env = getenv("O2_PIPELINE_PERF");
        String value = env ? String(env) : String();
        if (!value.IsEmpty() && value != "1" && o2FileSystem.IsFolderExist(value))
            return value.EndsWith("/") ? value : value + "/";

        return "../../";
    }

    const char* theLoop = "the loop";
    const char* wordfall = "wordfall";

    struct PipelinePerfUiFixture : ::testing::Test
    {
        Ref<PipelineEditor> editor;
        Ref<PipelineAsset>  asset;
        Ref<ActionsList>    actions;
        String              graphName;

        static String WorkPath() { return "../../Work/Pipelines/perf/"; }

        static void SetUpTestSuite()
        {
            if (!getenv("O2_PIPELINE_PERF"))
                return;

            // A fresh copy of the results each run: the scenarios re-apply effects and must start from the same cache
            namespace fs = std::filesystem;
            std::error_code ec;
            fs::remove_all(WorkPath().Data(), ec);
            fs::create_directories((WorkPath() + "cache").Data(), ec);
            String source = ProjectRoot() + "Work/Pipelines/";
            fs::copy((source + "uploads").Data(), (WorkPath() + "uploads").Data(), fs::copy_options::recursive, ec);
            for (const char* name : { theLoop, wordfall })
            {
                DataDocument doc;
                if (!doc.LoadFromFile(ProjectRoot() + "Assets/Pipelines/" + name + ".pipeline"))
                    continue;
                String id = doc.FindMember("id") ? (String)*doc.FindMember("id") : String();
                fs::copy((source + "cache/" + id).Data(), (WorkPath() + "cache/" + id).Data(), fs::copy_options::recursive, ec);
            }
        }

        void SetUp() override
        {
            if (!getenv("O2_PIPELINE_PERF"))
                GTEST_SKIP() << "set O2_PIPELINE_PERF=1 (or the project folder) to time the big pipelines";

            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }
            PipelineUtils::SetWorkPathOverride(WorkPath());

            PushEditorScopeOnStack scope;
            editor = mmake<PipelineEditor>();
            *editor->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
            actions = mmake<ActionsList>();
            editor->actionsListDelegate = actions;
            editor->GetExecutor()->assetsPathOverride = WorkPath() + "assets/";

            auto horScroll = o2UI.CreateHorScrollBar();
            *horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 5, 15, 10);
            editor->SetHorScrollbar(horScroll);
            auto verScroll = o2UI.CreateVerScrollBar();
            *verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 5, 15, 10);
            editor->SetVerScrollbar(verScroll);

            auto root = EditorUIRoot.GetRootWidget();
            *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize());
            EditorUIRoot.AddWidget(editor);
            editor->SetEnabledForcible(false);
            editor->SetEnabledForcible(true);
            root->UpdateSelfTransform();
            root->UpdateChildrenTransforms();
            for (int i = 0; i < 12; i++)
            {
                root->Update(0.1f);
                root->UpdateChildren(0.1f);
                root->UpdateChildrenTransforms();
            }
            asset = mmake<PipelineAsset>();
        }

        void TearDown() override
        {
            if (editor)
                editor->StopRun();
            for (int i = 0; i < 200 && editor && editor->IsRunning(); i++)
                Frame();
            o2Input.OnCursorReleased();
            if (UIRoot::IsSingletonInitialzed())
                o2UI.FocusWidget(nullptr);
            // The cards hold their editor as their drag selection group: the graph is cleared, or the editor outlives the test
            if (editor)
                editor->SetAsset(nullptr);
            editor = nullptr;
            asset = nullptr;
            actions = nullptr;
            PipelineUtils::SetWorkPathOverride("");
            if (UIRoot::IsSingletonInitialzed())
                EditorUIRoot.RemoveAllWidgets();
        }

        // Opens a big pipeline the way the window does, timed
        bool Open(const char* name, const Function<void(PipelineGraph&)>& patch = {})
        {
            graphName = name;
            String path = ProjectRoot() + "Assets/Pipelines/" + name + ".pipeline";
            if (!asset->document.LoadFromFile(path))
                return false;

            if (patch)
            {
                PipelineGraph graph;
                graph.LoadFromAsset(*asset);
                patch(graph);
                graph.SaveToAsset(*asset);
            }

            Recorder rec("open", graphName);
            double start = NowMs();
            editor->SetAsset(asset);
            rec.AddAction(NowMs() - start);
            rec.Step(true);
            rec.Steps(10);
            rec.Done(0.0, 5000.0);
            return editor->GetGraph() && !editor->GetGraph()->nodes.IsEmpty();
        }

        Ref<PipelineNodeWidget> Card(const String& id) const { return editor->GetNodeWidget(id); }

        // Every widget under the card, of the type
        template<typename T>
        static Vector<Ref<T>> Descendants(const Ref<Widget>& root)
        {
            Vector<Ref<Actor>> all;
            root->GetAllChildrenActors(all);
            Vector<Ref<T>> res;
            for (auto& actor : all)
            {
                if (auto typed = DynamicCast<T>(actor))
                    res.Add(typed);
            }
            return res;
        }

        // The card of the first node of the type, the one with the most inputs when asked
        Ref<PipelineNodeWidget> FirstCard(const String& type, const Function<bool(const PipelineNode&)>& filter = {}) const
        {
            Ref<PipelineNodeWidget> best;
            for (auto& node : editor->GetGraph()->nodes)
            {
                if (node->nodeType != type || (filter && !filter(*node)))
                    continue;
                if (!best || node->inputs.Count() > best->GetNode()->inputs.Count())
                    best = Card(node->id);
            }
            return best;
        }

        // Brings the card into view at the scale and lets the layout settle
        void Focus(const Ref<PipelineNodeWidget>& card, float scale = 1.0f)
        {
            RectF rect = card->GetCardRect();
            editor->SetView(rect.Center(), scale);
            for (int i = 0; i < 4; i++)
                Frame();
        }

        // Centre of the widget on screen
        Vec2F ScreenCenter(const Ref<Widget>& widget) const
        {
            return editor->LocalToScreenPoint(widget->layout->GetWorldRect().Center());
        }

        Vec2F CanvasToScreen(const Vec2F& point) const { return editor->LocalToScreenPoint(point); }

        void Click(Recorder& rec, const Vec2F& screen)
        {
            // Two presses within 0.3 s of real time are a double click
            std::this_thread::sleep_for(std::chrono::milliseconds(320));
            o2Input.OnCursorMoved(screen, 0, false);
            o2Input.OnCursorPressed(screen);
            o2Input.PreUpdate();
            rec.Step(true);
            o2Input.OnCursorReleased();
            rec.Step(true);
        }

        void Drag(Recorder& rec, const Vec2F& from, const Vec2F& to, int steps)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(320));
            o2Input.OnCursorMoved(from, 0, false);
            o2Input.OnCursorPressed(from);
            rec.Step(true);
            for (int i = 1; i <= steps; i++)
            {
                o2Input.OnCursorMoved(Math::Lerp(from, to, (float)i/steps), 0);
                rec.Step(true);
            }
            o2Input.OnCursorReleased();
            rec.Step(true);
        }

        // Types the text key by key, a frame for the press and one for the release
        void Type(Recorder& rec, const String& text)
        {
            for (int i = 0; i < text.Length(); i++)
            {
                char c = text[i];
                KeyboardKey key = c == ' ' ? VK_SPACE : (KeyboardKey)toupper((unsigned char)c);
                o2Input.OnKeyPressed(key);
                rec.Step(true);
                o2Input.OnKeyReleased(key);
                rec.Step();
            }
        }

        // Focuses the field by a click, types into it, ends the edit with a click on empty canvas and lets the consequences settle
        void TypeInto(const String& name, const Ref<EditBox>& edit, const String& text, bool commitWithEnter = false)
        {
            ASSERT_TRUE(edit) << name.Data();
            // A tall card can hold the field away from its middle
            editor->SetView(edit->layout->GetWorldRect().Center(), editor->GetCamera().GetScale2D().x);
            for (int i = 0; i < 4; i++)
                Frame();

            Recorder rec(name, graphName);
            Click(rec, ScreenCenter(edit));
            ASSERT_TRUE(edit->IsFocused()) << name.Data();
            Type(rec, text);
            if (commitWithEnter)
            {
                o2Input.OnKeyPressed(VK_RETURN);
                rec.Step(true);
                o2Input.OnKeyReleased(VK_RETURN);
                rec.Step(true);
            }
            else
            {
                o2UI.FocusWidget(nullptr);
                rec.Step(true);
            }
            rec.Steps(40);
            rec.Done(250.0);
        }

        void RunAllCachedOnly()
        {
            Recorder rec("run all (cached only, 12 targets)", graphName);
            auto graph = editor->GetGraph();
            auto targets = graph->GetRunTargets();
            for (int t = 0; t < targets.Count() && t < 12; t++)
            {
                auto& target = targets[t];
                double start = NowMs();
                editor->GetExecutor()->Execute(editor->GetPipelineId(), *graph, target, {}, true);
                rec.AddAction(NowMs() - start);
                for (int i = 0; i < 600 && editor->IsRunning(); i++)
                    rec.Step();
                rec.Step();
            }
            rec.Steps(20);
            rec.Done();
        }
    };

    const char* longText = "pixel art treasure chest with golden trim and a soft glow";
}

TEST_F(PipelinePerfUiFixture, CanvasNavigationAndSelection)
{
    ASSERT_TRUE(Open(theLoop));

    {
        Recorder rec("fit view", graphName);
        double start = NowMs();
        editor->FitView();
        rec.AddAction(NowMs() - start);
        rec.Steps(60);
        rec.Done(1000.0);
    }

    auto someCard = FirstCard("composer");
    ASSERT_TRUE(someCard);
    Focus(someCard, 1.5f);

    {
        Recorder rec("pan (right drag)", graphName);
        Vec2F center = (Vec2F)o2Application.GetContentSize()*0.0f;
        o2Input.OnCursorMoved(center, 0, false);
        o2Input.OnAltCursorPressed(center);
        rec.Step(true);
        for (int i = 1; i <= 60; i++)
        {
            o2Input.OnCursorMoved(center + Vec2F(i*8.0f, i*3.0f), 0);
            rec.Step(true);
        }
        o2Input.OnAltCursorReleased();
        rec.Step(true);
        rec.Steps(10);
        rec.Done(100.0);
    }

    {
        Recorder rec("zoom (wheel in and out)", graphName);
        o2Input.OnCursorMoved(Vec2F(), 0, false);
        for (int i = 0; i < 20; i++)
        {
            o2Input.OnMouseWheel(i < 10 ? 120.0f : -120.0f);
            rec.Step(true);
            rec.Step();
        }
        rec.Steps(30);
        rec.Done(150.0);
    }

    {
        Recorder rec("far zoom (wheel out, pan)", graphName);
        o2Input.OnCursorMoved(Vec2F(), 0, false);
        for (int i = 0; i < 30; i++)
        {
            o2Input.OnMouseWheel(-120.0f);
            rec.Step(true);
        }
        rec.Steps(30);
        o2Input.OnAltCursorPressed(Vec2F());
        for (int i = 1; i <= 30; i++)
        {
            o2Input.OnCursorMoved(Vec2F(i*10.0f, 0), 0);
            rec.Step(true);
        }
        o2Input.OnAltCursorReleased();
        rec.Steps(10);
        rec.Done(300.0);
        printf("[perf] far view scale %.2f\n", editor->GetCamera().GetScale2D().x);
    }

    Focus(someCard, 1.5f);
    {
        Recorder rec("select (click header)", graphName);
        RectF rect = someCard->GetCardRect();
        Click(rec, CanvasToScreen(Vec2F(rect.left + 60, rect.top - 12)));
        rec.Steps(10);
        rec.Done(100.0);
        EXPECT_TRUE(someCard->IsSelected());
    }

    editor->SetView(someCard->GetCardRect().Center(), 4.0f);
    for (int i = 0; i < 4; i++) Frame();
    {
        Recorder rec("box select", graphName);
        RectF view = editor->GetVisibleCanvasRect();
        Vec2F from = CanvasToScreen(Vec2F(view.left + view.Width()*0.05f, view.top - view.Height()*0.05f));
        Vec2F to = CanvasToScreen(Vec2F(view.left + view.Width()*0.6f, view.top - view.Height()*0.7f));
        // Starts on empty canvas: a free spot left of every card
        Drag(rec, from, to, 30);
        rec.Steps(10);
        rec.Done(100.0);
        printf("[perf] box selected %d cards\n", editor->GetSelectedNodes().Count());
    }
}

TEST_F(PipelinePerfUiFixture, DragAndResizeCards)
{
    ASSERT_TRUE(Open(theLoop));
    auto card = FirstCard("imageEdit");
    ASSERT_TRUE(card);
    Focus(card, 1.5f);

    {
        Recorder rec("drag node", graphName);
        RectF rect = card->GetCardRect();
        Vec2F from = CanvasToScreen(Vec2F(rect.left + 80, rect.top - 12));
        Drag(rec, from, from + Vec2F(200, -80), 40);
        rec.Steps(10);
        rec.Done(100.0);
    }

    {
        // A group: the card and the twenty nearest to it
        Vector<String> ids;
        Vec2F c = card->GetNode()->position;
        auto nodes = editor->GetGraph()->nodes;
        nodes.Sort([&](const Ref<PipelineNode>& a, const Ref<PipelineNode>& b) { return (a->position - c).Length() < (b->position - c).Length(); });
        for (int i = 0; i < nodes.Count() && i < 21; i++)
            ids.Add(nodes[i]->id);
        editor->SelectNodes(ids);
        Frame();

        Recorder rec("drag group (21 nodes)", graphName);
        RectF rect = card->GetCardRect();
        Vec2F from = CanvasToScreen(Vec2F(rect.left + 80, rect.top - 12));
        Drag(rec, from, from + Vec2F(-150, 60), 40);
        rec.Steps(10);
        rec.Done(100.0);
        editor->SelectNodes({});
    }

    {
        Recorder rec("resize card (corner)", graphName);
        RectF rect = card->GetCardRect();
        Vec2F from = CanvasToScreen(Vec2F(rect.right - 1, rect.bottom + 1));
        Drag(rec, from, from + Vec2F(120, -90), 40);
        rec.Steps(10);
        rec.Done(100.0);
    }
}

TEST_F(PipelinePerfUiFixture, TypingIntoCardFields)
{
    ASSERT_TRUE(Open(theLoop));

    auto editCard = FirstCard("imageEdit");
    ASSERT_TRUE(editCard);
    Focus(editCard);
    auto prompts = Descendants<EditBox>(editCard).FindAll([](const Ref<EditBox>& e) { return e->IsMultiLine(); });
    ASSERT_FALSE(prompts.IsEmpty());
    TypeInto("type prompt (imageEdit)", prompts[0], longText);

    auto textCard = FirstCard("sourceText", [&](const PipelineNode& n) { return !editor->GetGraph()->GetOutgoingEdges(n.id).IsEmpty(); });
    ASSERT_TRUE(textCard);
    Focus(textCard);
    TypeInto("type text source", textCard->FindChildByType<EditBox>(), longText);

    auto finishCard = FirstCard("finishImage");
    ASSERT_TRUE(finishCard);
    Focus(finishCard);
    auto finishEdits = Descendants<EditBox>(finishCard);
    ASSERT_GE(finishEdits.Count(), 2);
    TypeInto("type finish folder", finishEdits[0], "sprites chests", true);
    Focus(finishCard);
    TypeInto("type finish name", finishEdits[1], "golden chest big", true);

    auto composer = FirstCard("composer");
    ASSERT_TRUE(composer);
    Focus(composer);
    auto name = composer->FindChildByTypeAndName<EditBox>("layer name");
    TypeInto("type composer layer name", name, "background layer with hills", true);

}

TEST_F(PipelinePerfUiFixture, ComposerWithManyLayers)
{
    ASSERT_TRUE(Open(wordfall));
    auto composer = FirstCard("composer");
    ASSERT_TRUE(composer);
    printf("[perf] composer with %d inputs\n", composer->GetNode()->inputs.Count());
    Focus(composer, 1.0f);

    auto name = composer->FindChildByTypeAndName<EditBox>("layer name");
    TypeInto("type composer layer name", name, "background layer with hills", true);

    Focus(composer, 1.0f);
    auto stage = composer->FindChildByType<PipelineComposerStage>();
    ASSERT_TRUE(stage);
    {
        Recorder rec("composer: move layer", graphName);
        RectF r = stage->layout->GetWorldRect();
        Vec2F from = CanvasToScreen(r.Center());
        Drag(rec, from, from + Vec2F(60, 30), 30);
        rec.Steps(20);
        rec.Done(100.0);
    }

    {
        Recorder rec("composer: select layer rows", graphName);
        auto panel = composer->FindChildByTypeAndName<PipelineComposerLayersPanel>("layers panel");
        ASSERT_TRUE(panel);
        auto layers = stage->GetLayers();
        for (int i = 0; i < layers.Count() && i < 5; i++)
        {
            if (auto row = panel->FindRow(layers[i].id))
            {
                // The thumbnail: the row's select button lies under it
                RectF rr = row->layout->GetWorldRect();
                Click(rec, CanvasToScreen(Vec2F(rr.left + 51, rr.Center().y)));
            }
        }
        rec.Steps(10);
        rec.Done(100.0);
    }
}

TEST_F(PipelinePerfUiFixture, EffectNodesLiveReapply)
{
    // A gradient and a colour adjust fed by the outline's result join the outline and shadow nodes of the graph
    String gradientId, colorId;
    ASSERT_TRUE(Open(wordfall, [&](PipelineGraph& graph)
    {
        auto from = graph.nodes.Find([](const Ref<PipelineNode>& n) { return n->nodeType == "imageOutline"; });
        if (!from)
            return;
        int i = 0;
        for (auto type : { "imageGradient", "imageColor" })
        {
            auto node = PipelineNodeRegistry::CreateNode(type, (*from)->position + Vec2F(700.0f, 760.0f*i));
            graph.nodes.Add(node);
            auto edge = mmake<PipelineEdge>();
            edge->id = PipelineNode::GenerateId();
            edge->fromNodeId = (*from)->id;
            edge->fromPortId = (*from)->outputs[0].id;
            edge->toNodeId = node->id;
            edge->toPortId = node->inputs[0].id;
            graph.edges.Add(edge);
            (i == 0 ? gradientId : colorId) = node->id;
            i++;
        }
    }));

    auto sliderDrag = [&](const String& scenario, const Ref<PipelineNodeWidget>& card, int sliderIndex)
    {
        ASSERT_TRUE(card) << scenario.Data();
        Focus(card);
        auto sliders = Descendants<PipelineSlider>(card);
        ASSERT_GT(sliders.Count(), sliderIndex) << scenario.Data();
        auto track = sliders[sliderIndex]->FindChildByType<HorizontalProgress>();
        ASSERT_TRUE(track);
        RectF r = track->layout->GetWorldRect();
        Recorder rec(scenario, graphName);
        Drag(rec, CanvasToScreen(Vec2F(r.left + r.Width()*0.2f, r.Center().y)), CanvasToScreen(Vec2F(r.left + r.Width()*0.7f, r.Center().y)), 40);
        // The re-apply lands after the drag; its frames count
        rec.Steps(90);
        rec.Done(150.0);
    };

    sliderDrag("slider: outline width", FirstCard("imageOutline"), 0);
    sliderDrag("slider: shadow blur", FirstCard("imageShadow"), 3);
    sliderDrag("slider: gradient amount", Card(gradientId), 3);
    sliderDrag("slider: colour hue", Card(colorId), 3);

    {
        auto card = FirstCard("imageOutline");
        Focus(card);
        auto inside = card->FindChildByTypeAndName<Widget>("position options");
        ASSERT_TRUE(inside);
        auto toggles = inside->GetChildWidgets();
        ASSERT_GE(toggles.Count(), 3);
        Recorder rec("segmented: outline position", graphName);
        Click(rec, ScreenCenter(toggles[2]));
        rec.Steps(90);
        rec.Done(150.0);
    }

    {
        auto card = FirstCard("imageShadow");
        Focus(card);
        auto field = card->FindChildByType<PipelineColorField>();
        ASSERT_TRUE(field);
        Recorder rec("colour drag: shadow colour", graphName);
        // What the colour picker sends while its wheel is dragged
        for (int i = 0; i < 40; i++)
        {
            field->onChanged(Color4(i*6, 40, 200 - i*4, 255), false);
            rec.Step(true);
        }
        field->onChanged(Color4(240, 40, 40, 255), true);
        rec.Step(true);
        rec.Steps(90);
        rec.Done(150.0);
    }
}

TEST_F(PipelinePerfUiFixture, PaintPartsMenusAndParams)
{
    ASSERT_TRUE(Open(theLoop));

    auto editCard = FirstCard("imageEdit", [&](const PipelineNode& n) { return editor->GetRuntime(n.id) && editor->GetRuntime(n.id)->output.IsValid(); });
    if (!editCard)
        editCard = FirstCard("imageEdit");
    ASSERT_TRUE(editCard);
    Focus(editCard);
    if (auto paint = editCard->FindChildByType<PipelinePaintEditor>())
    {
        Recorder rec("paint: draw strokes", graphName);
        RectF r = paint->layout->GetWorldRect();
        for (int s = 0; s < 3; s++)
        {
            Vec2F a = CanvasToScreen(Vec2F(r.left + r.Width()*0.2f, r.bottom + r.Height()*(0.3f + 0.15f*s)));
            Vec2F b = CanvasToScreen(Vec2F(r.left + r.Width()*0.8f, r.bottom + r.Height()*(0.35f + 0.15f*s)));
            Drag(rec, a, b, 30);
        }
        rec.Steps(20);
        rec.Done(150.0);
    }

    auto gen = FirstCard("nanoBananaGen");
    ASSERT_TRUE(gen);
    Focus(gen);
    {
        auto head = gen->FindChildByTypeAndName<Button>("params");
        ASSERT_TRUE(head);
        Recorder rec("params fold toggle", graphName);
        Click(rec, ScreenCenter(head));
        rec.Steps(5);
        head = gen->FindChildByTypeAndName<Button>("params");
        Click(rec, ScreenCenter(head));
        rec.Steps(10);
        rec.Done(200.0);
    }

    // The model field lives in the parameter list
    gen->SetParamsOpen(true);
    Focus(gen);
    {
        auto field = gen->FindChildByTypeAndName<Button>("model");
        ASSERT_TRUE(field);
        Recorder rec("model menu: open and filter", graphName);
        Click(rec, ScreenCenter(field));
        rec.Steps(5);
        auto picker = editor->GetModelPicker();
        ASSERT_TRUE(picker);
        if (auto search = picker->GetSearch())
        {
            search->Focus();
            rec.Step();
            Type(rec, "gemini flash");
        }
        rec.Steps(5);
        picker->Close();
        rec.Steps(10);
        rec.Done(150.0);
    }
}

TEST_F(PipelinePerfUiFixture, ExtractParts)
{
    // The big graphs keep one part per extract node: one of them gets eight
    String extractId;
    ASSERT_TRUE(Open(wordfall, [&](PipelineGraph& graph)
    {
        auto node = graph.nodes.Find([](const Ref<PipelineNode>& n) { return n->nodeType == "imageExtract"; });
        if (!node)
            return;
        Vector<PipelineExtractRegion> regions;
        for (int i = 0; i < 8; i++)
        {
            PipelineExtractRegion region;
            region.id = i == 0 && !(*node)->outputs.IsEmpty() ? (*node)->outputs[0].id : PipelineNode::GenerateId();
            region.name = "part " + (String)i;
            region.x = 0.1f*(i % 4); region.y = 0.4f*(i / 4); region.w = 0.1f; region.h = 0.3f;
            regions.Add(region);
        }
        PipelineRegions::Write(**node, regions);
        PipelineNodeRegistry::SyncNodeWithSchema(*node);
        extractId = (*node)->id;
    }));

    auto extract = Card(extractId);
    ASSERT_TRUE(extract);
    Focus(extract);
    {
        Recorder rec("extract: select parts", graphName);
        Vector<Ref<Widget>> cells;
        for (auto& w : Descendants<Button>(extract))
        {
            if (w->GetName() == "select part")
                cells.Add(w);
        }
        ASSERT_GE(cells.Count(), 6);
        for (int i = 0; i < 6; i++)
            Click(rec, ScreenCenter(cells[i]));
        rec.Steps(10);
        rec.Done(100.0);
    }

    Focus(extract);
    auto prompt = extract->FindChildByTypeAndName<EditBox>("part prompt");
    TypeInto("type extract part name", prompt, "golden key item");
}

TEST_F(PipelinePerfUiFixture, EditsUndoSaveAndRun)
{
    ASSERT_TRUE(Open(theLoop));

    {
        Recorder rec("add and delete nodes", graphName);
        for (int i = 0; i < 4; i++)
        {
            double start = NowMs();
            auto node = editor->AddNodeAtViewCenter(i % 2 ? "imageOutline" : "sourceText");
            rec.AddAction(NowMs() - start);
            rec.Step();
            start = NowMs();
            editor->SelectNodes({ node->id });
            editor->DeleteSelection();
            rec.AddAction(NowMs() - start);
            rec.Step();
        }
        rec.Steps(10);
        rec.Done(0.0, 500.0);
    }

    {
        // Two fresh cards side by side, linked by a drag from the output port, the link clicked and deleted
        auto text = editor->AddNodeAtViewCenter("sourceText");
        auto edit = editor->AddNodeAtViewCenter("textEdit");
        edit->position = text->position + Vec2F(600, 0);
        editor->RebuildAll();
        editor->SetView(editor->GetNodeWidget(text->id)->GetCardRect().Center() + Vec2F(300, 0), 1.5f);
        for (int i = 0; i < 4; i++) Frame();

        auto textCard = Card(text->id), editCard = Card(edit->id);
        Vec2F from = textCard->GetPortPosition(text->outputs[0].id, false);
        Vec2F to = editCard->GetPortPosition(edit->inputs[0].id, true);
        Recorder rec("add and delete a link", graphName);
        Drag(rec, CanvasToScreen(from), CanvasToScreen(to), 20);
        rec.Steps(5);
        EXPECT_EQ(editor->GetGraph()->GetIncomingEdges(edit->id).Count(), 1);
        Click(rec, CanvasToScreen((from + to)*0.5f));
        o2Input.OnKeyPressed(VK_DELETE);
        rec.Step(true);
        o2Input.OnKeyReleased(VK_DELETE);
        rec.Step(true);
        rec.Steps(10);
        rec.Done(200.0);
    }

    {
        Recorder rec("undo and redo", graphName);
        for (int i = 0; i < 4; i++)
        {
            double start = NowMs();
            actions->UndoAction();
            rec.AddAction(NowMs() - start);
            rec.Step();
        }
        for (int i = 0; i < 4; i++)
        {
            double start = NowMs();
            actions->RedoAction();
            rec.AddAction(NowMs() - start);
            rec.Step();
        }
        rec.Steps(10);
        rec.Done(0.0, 500.0);
    }

    {
        // What Cmd+S does with the pipeline: the graph into the asset document, the document to its file
        Recorder rec("save (graph to document to file)", graphName);
        for (int i = 0; i < 3; i++)
        {
            double start = NowMs();
            editor->GetGraph()->SaveToAsset(*asset);
            asset->document.SaveToFile(WorkPath() + "saved.pipeline");
            rec.AddAction(NowMs() - start);
            rec.Step();
        }
        rec.Steps(5);
        rec.Done(0.0, 300.0);
    }

    RunAllCachedOnly();
}

TEST_F(PipelinePerfUiFixture, RebuildsKeepMemoryFlat)
{
    ASSERT_TRUE(Open(theLoop));
    editor->FitView();
    for (int i = 0; i < 30; i++) Frame();
    double base = FootprintMb();
    printf("[perf] footprint after open %.0f MB\n", base);
    Recorder rec("rebuild all cards", graphName);
    double afterFirst = 0.0;
    for (int i = 0; i < 4; i++)
    {
        double start = NowMs();
        editor->RebuildAll();
        rec.AddAction(NowMs() - start);
        rec.Steps(5);
        printf("[perf] footprint after rebuild %d: %.0f MB\n", i + 1, FootprintMb());
        if (i == 0)
            afterFirst = FootprintMb();
    }
    rec.Done();

    // Another pipeline opened and this one again: what the one left showed goes with it
    {
        Recorder sw("switch pipeline and back", graphName);
        auto other = mmake<PipelineAsset>();
        ASSERT_TRUE(other->document.LoadFromFile(ProjectRoot() + "Assets/Pipelines/" + wordfall + ".pipeline"));
        double start = NowMs();
        editor->SetAsset(other);
        sw.AddAction(NowMs() - start);
        sw.Steps(30);
        start = NowMs();
        editor->SetAsset(asset);
        sw.AddAction(NowMs() - start);
        sw.Steps(30);
        sw.Done();
        printf("[perf] footprint after switching to %s and back: %.0f MB\n", wordfall, FootprintMb());
    }

    // A rebuild frees the cards it replaces: their textures, buffers and sounds
    EXPECT_LT(FootprintMb() - afterFirst, 200.0);
}
