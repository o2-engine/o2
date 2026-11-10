#pragma once

#include "PipelinePairUiFixture.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspace.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWindow.h"

// Fixture of the AI UI layout workspace's UI tests: an editor with a uiLayout node holding a screen (one agent step), an
// optional UI kit wired into it, the workspace open over the editor; clicks and drags in the window, shots of it

namespace
{
    using UiDocJson = Editor::UiDoc::Json;

    // The screen the tests edit
    const char* const workspaceScreen =
        "screen Menu 1080x1920\n"
        "  background bg anchor=stretch\n"
        "  panel card anchor=center size=760x900\n"
        "    text title anchor=top-stretch pos=0,40 h=120 style=title \"FOREST QUEST\"\n"
        "    button play anchor=bottom-center pos=0,60 size=400x120 \"PLAY\"\n"
        "  column list anchor=top-left pos=40,80 size=300x600 gap=20\n"
        "    button a h=100 \"A\"\n"
        "    button b h=100 \"B\"\n"
        "    button c h=100 \"C\"\n"
        "  icon coin anchor=top-right pos=24,24 size=96x96\n";

    // A picture of a kit graphic: a rounded plate of the colour (a darker rim) or a disc
    String KitPng(int w, int h, const Color4& color, bool disc)
    {
        auto bitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(w, h));
        UInt8* data = bitmap->GetData();
        float r = disc ? Math::Min(w, h)*0.5f : Math::Min(w, h)*0.25f;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                float cx = Math::Clamp((float)x + 0.5f, r, w - r), cy = Math::Clamp((float)y + 0.5f, r, h - r);
                float d = Vec2F(x + 0.5f - cx, y + 0.5f - cy).Length();
                float a = Math::Clamp01(r - d + 0.5f);
                bool rim = d > r - 3.0f || x < 3 || y < 3 || x >= w - 3 || y >= h - 3;
                UInt8* p = data + ((h - 1 - y)*w + x)*4;
                p[0] = (UInt8)(color.r*(rim ? 0.7f : 1.0f));
                p[1] = (UInt8)(color.g*(rim ? 0.7f : 1.0f));
                p[2] = (UInt8)(color.b*(rim ? 0.7f : 1.0f));
                p[3] = (UInt8)(a*255);
            }
        }
        return PipelineUtils::Base64Encode(PipelineValue::Image(bitmap).GetPngBytes());
    }

    // A small kit: a green button, a dark panel, a coin icon, a backdrop and two text styles
    String TestKitJson()
    {
        UiDoc::UiKit kit;
        kit.name = "test_kit";
        auto add = [&](const String& name, const String& type, const String& assetType, int w, int h, const Color4& color, bool disc)
        {
            UiDoc::UiKitElement e;
            e.name = name;
            e.type = type;
            e.assetType = assetType;
            e.width = w;
            e.height = h;
            e.description = name;
            e.png = KitPng(w, h, color, disc);
            if (assetType == "sliced")
                e.nineSlice = UiDoc::UiNine{ 16, 16, 16, 16 };
            kit.elements.Add(e);
        };
        add("btn_green", "button", "sliced", 96, 48, Color4(76, 175, 80, 255), false);
        add("panel_dark", "panel", "sliced", 96, 96, Color4(55, 71, 79, 255), false);
        add("icon_coin", "icon", "simple", 64, 64, Color4(255, 193, 7, 255), true);
        add("bg_sky", "background", "simple", 64, 96, Color4(100, 160, 220, 255), false);
        add("icon_heart", "icon", "simple", 64, 64, Color4(229, 57, 53, 255), true);
        UiDoc::TextStyle title;
        title.name = "title";
        title.role = "title";
        title.size = 72;
        title.color = "#FFFFFF";
        kit.textStyles.Add(title);
        return UiDoc::KitToJson(kit).Dump();
    }

    struct UiWorkspaceFixture : PipelinePairUiFixture
    {
        Ref<PipelineNode> layoutNode;
        Ref<PipelineNode> kitNode;

        String WorkDir() const override { return "../../Work/Pipelines/uitest-uiworkspace/"; }

        void TearDown() override
        {
            UiWorkspace::Reset();
            PipelineTheme::FollowEditor();
            PipelinePairUiFixture::TearDown();
        }

        // Opens a graph with the layout node (its screen one agent step) and, when asked, a kit node feeding it; then the
        // workspace for the layout node
        void OpenWorkspace(bool withKit, const String& uiml = workspaceScreen)
        {
            PipelineGraph graph;
            graph.id = "uitest-uiworkspace";
            auto node = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F(0, 0));
            node->size = Vec2F(520, 0);
            auto doc = UiDoc::ParseUiml(uiml).doc;
            auto step = UiDoc::MakeAgentStep("lay out a menu", "A menu with a card, a list and a coin.", { UiDoc::MakeDocOp(doc) },
                                      1762862400000, "gpt-6.1-sol");
            UiDoc::WriteSteps(node->config, { step }, {});
            graph.nodes.Add(node);

            if (withKit)
            {
                auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F(-560, 0));
                graph.nodes.Add(kit);
                auto edge = mmake<PipelineEdge>();
                edge->id = PipelineNode::GenerateId();
                edge->fromNodeId = kit->id;
                edge->fromPortId = kit->outputs[0].id;
                edge->toNodeId = node->id;
                edge->toPortId = node->inputs.Find([](const PipelinePort& p) { return p.portType == PipelinePortType::UiKit; })->id;
                graph.edges.Add(edge);
            }

            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(3);
            layoutNode = editor->GetGraph()->FindNode(node->id);
            if (withKit)
            {
                kitNode = editor->GetGraph()->nodes.Find([](const Ref<PipelineNode>& n) { return n->nodeType == "uiKit"; }) ?
                    *editor->GetGraph()->nodes.Find([](const Ref<PipelineNode>& n) { return n->nodeType == "uiKit"; }) : nullptr;
                auto card = editor->GetNodeWidget(kitNode->id);
                card->GetRuntime().output = PipelineValue::TextOf(PipelinePortType::UiKit, TestKitJson());
                card->OnOutputChanged();
            }
            UiWorkspace::Open(editor, layoutNode->id);
            Step(4);
            Window()->GetStage()->FlushRender();
            Step(2);
        }

        Ref<UiWsWindow> Window() const { return UiWorkspace::GetWindow(); }
        Ref<UiWsContext> Ctx() const { return Window()->GetContext(); }

        // The layout now, read from the node's config
        UiDoc::UiDoc DocNow() const { return UiDoc::DocAt(layoutNode->config).doc; }

        // The node's config as JSON
        UiDocJson ConfigNow() const { return UiDoc::FromDataValue(layoutNode->config); }

        // The steps of the node now
        Vector<UiDoc::UiStep> StepsNow() const { return UiDoc::StepsOf(layoutNode->config); }

        // The label of the last action of the last step
        String LastLabel() const
        {
            auto steps = StepsNow();
            if (steps.IsEmpty())
                return "";
            auto parts = UiDoc::StepParts(steps.Last());
            return parts.IsEmpty() ? String() : parts.Last().label;
        }

        // A click at a world point
        void Click(const Vec2F& point)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(350));
            o2Input.OnCursorMoved(point, 0, false);
            Step();
            o2Input.OnCursorPressed(point);
            Step();
            o2Input.OnCursorReleased();
            Step(2);
        }

        // A drag that holds the keys down meanwhile
        void DragWith(const Vec2F& from, const Vec2F& to, const Vector<int>& keys, int steps = 10)
        {
            for (int key : keys)
                o2Input.OnKeyPressed(key);
            Drag(from, to, steps);
            for (int key : keys)
                o2Input.OnKeyReleased(key);
            Step();
        }

        // Saves the frame (the whole editor with the workspace over it) under the name
        void Shot(const String& name)
        {
            auto capture = Capture();
            ASSERT_TRUE(capture);
            String dir = (getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots")) + "/uiworkspace";
            o2FileSystem.FolderCreate(dir, true);
            capture->Save(dir + "/" + name + ".png", Bitmap::ImageType::Png);
        }
    };
}
