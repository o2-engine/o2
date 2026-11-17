#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTextStyleFx.h"

// A Text style wired through a UI kit into an AI UI layout: an edit of the style shows on the layout's card at once, not
// after the kit's run. The AI text style: its name and role apply without a model call, "-> Text style" takes its place
// in its links

namespace
{
    struct PipelineLiveKitStylesFixture : UiWorkspaceFixture
    {
        Ref<PipelineNode> styleNode;

        String WorkDir() const override { return "../../Work/Pipelines/uitest-livestyles/"; }

        static Ref<PipelineEdge> Link(PipelineGraph& graph, const Ref<PipelineNode>& from, const String& fromPort, const Ref<PipelineNode>& to,
                                      const String& toPort)
        {
            auto edge = mmake<PipelineEdge>();
            edge->id = PipelineNode::GenerateId();
            edge->fromNodeId = from->id;
            edge->fromPortId = fromPort;
            edge->toNodeId = to->id;
            edge->toPortId = toPort;
            graph.edges.Add(edge);
            return edge;
        }

        // Text style (or an AI text style reading an uploaded picture) -> UI kit -> AI UI layout (a screen with a title in the
        // kit's "title" style), the kit run once
        void OpenChain(bool aiStyle = false)
        {
            PipelineGraph graph;
            graph.id = "uitest-livestyles";
            auto style = PipelineNodeRegistry::CreateNode(aiStyle ? "textStyleExtract" : "textStyle", Vec2F(-1100, 0));
            if (aiStyle)
            {
                PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("livestyles-src.png"),
                                              PipelineUtils::Base64Decode(KitPng(300, 120, Color4(40, 60, 120, 255), false)));
                auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-1500, 0));
                source->SetConfigString("uploadId", "livestyles-src.png");
                graph.nodes.Add(source);
                Link(graph, source, source->outputs[0].id, style, style->inputs[0].id);
            }
            else
            {
                style->SetConfigString("color", "#FFFFFF");
                style->SetConfigNumber("size", 96);
            }
            auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F(-560, 0));
            kit->SetCustomInputs({ PipelinePort("kit-title", "title", PipelinePortType::TextStyle, true) });
            PipelineNodeRegistry::SyncNodeWithSchema(kit);
            auto layout = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F(0, 0));
            layout->size = Vec2F(520, 0);
            auto doc = UiDoc::ParseUiml(workspaceScreen).doc;
            UiDoc::WriteSteps(layout->config, { UiDoc::MakeAgentStep("lay out a menu", "A menu.", { UiDoc::MakeDocOp(doc) }, 1762862400000,
                                                                     "gpt-6.1-sol") }, {});
            graph.nodes.Add(style);
            graph.nodes.Add(kit);
            graph.nodes.Add(layout);
            Link(graph, style, style->outputs[0].id, kit, "kit-title");
            Link(graph, kit, kit->outputs[0].id, layout,
                 layout->inputs.Find([](const PipelinePort& p) { return p.portType == PipelinePortType::UiKit; })->id);
            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(3);
            styleNode = editor->GetGraph()->FindNode(style->id);
            kitNode = editor->GetGraph()->FindNode(kit->id);
            layoutNode = editor->GetGraph()->FindNode(layout->id);

            editor->RunNode(kitNode->id, false);
            WaitForRuns();
        }

        void WaitForRuns()
        {
            Step(2);
            for (int i = 0; i < 600 && editor->IsRunning(); i++)
            {
                Step();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            Step(30);
            for (int i = 0; i < 600 && editor->IsRunning(); i++)
            {
                Step();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            editor->FinishBackgroundWork();
            Step(3);
        }

        // The name of the style the node outputs now
        String OutputStyleName(const String& nodeId)
        {
            UiDoc::Json json;
            UiDoc::Json::Parse(editor->GetNodeWidget(nodeId)->GetRuntime().output.data, json);
            return UiDoc::JsStringOr(json.Get("name"));
        }

        Ref<PipelineImageView> LayoutPreview()
        {
            auto card = editor->GetNodeWidget(layoutNode->id);
            return card ? card->FindChildByTypeAndName<PipelineImageView>("layout preview") : nullptr;
        }

        Ref<PipelineTextFxPanel> StyleEffects()
        {
            auto card = editor->GetNodeWidget(styleNode->id);
            return card ? card->FindChildByTypeAndName<PipelineTextFxPanel>("style effects") : nullptr;
        }

        // Frames at 60 per second until the layout card shows another picture: the frames, the milliseconds; -1 when it never does
        std::pair<int, double> FramesUntilTheLayoutRedraws(int limit = 240)
        {
            auto preview = LayoutPreview();
            int before = preview->GetImageVersion();
            auto start = std::chrono::steady_clock::now();
            for (int frame = 1; frame <= limit; frame++)
            {
                std::this_thread::sleep_until(start + std::chrono::microseconds(16667*frame));
                Step();
                if (preview->GetImageVersion() != before)
                    return { frame, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() };
            }
            return { -1, 0.0 };
        }
    };
}

TEST_F(PipelineLiveKitStylesFixture, AStyleEditShowsOnTheLayoutCardAtOnce)
{
    OpenChain();
    auto preview = LayoutPreview();
    ASSERT_TRUE(preview);
    ASSERT_TRUE(preview->HasImage());
    auto effects = StyleEffects();
    ASSERT_TRUE(effects);

    // An effect added on the Text style card
    effects->AddEffect("stroke");
    auto added = FramesUntilTheLayoutRedraws();
    printf("an effect added: the layout card redrew after %d frame(s), %.0f ms\n", added.first, added.second);
    EXPECT_GE(added.first, 1);
    EXPECT_LE(added.first, 2);

    // The fill's colour typed in through the config, as an undo would
    Step(30);
    styleNode->SetConfigString("color", "#FF3040");
    editor->OnNodeConfigChanged(editor->GetNodeWidget(styleNode->id), "color", true);
    auto colored = FramesUntilTheLayoutRedraws();
    printf("a colour set: the layout card redrew after %d frame(s), %.0f ms\n", colored.first, colored.second);
    EXPECT_GE(colored.first, 1);
    EXPECT_LE(colored.first, 2);
}

TEST_F(PipelineLiveKitStylesFixture, TheAiTextStylesNameAppliesWithoutARunAndTheTextStyleNodeTakesItsPlace)
{
    std::atomic<int> calls{ 0 };
    UiDoc::SetUiAiTextStub([&](const String&, const String&, const Vector<AiImageRef>&)
    {
        calls++;
        AiTextResult r;
        r.ok = true;
        r.text = "{\"text\":\"QUEST\",\"name\":\"gold\",\"role\":\"title\",\"font\":\"Russo One\",\"capHeight\":40,\"color\":\"#FFD040\","
                 "\"effects\":[{\"type\":\"stroke\",\"color\":\"#402000\",\"width\":3}]}";
        return r;
    });
    auto actions = mmake<ActionsList>();
    editor->actionsListDelegate = actions;
    OpenChain(true);
    editor->RunNode(styleNode->id, false);
    WaitForRuns();
    ASSERT_EQ(calls.load(), 1);
    EXPECT_EQ(OutputStyleName(styleNode->id), "gold");
    // The kit took the style the model read
    editor->RunNode(kitNode->id, false);
    WaitForRuns();

    // A name set on the card: the stored answer gets it, no model call; the layout card follows
    int version = LayoutPreview()->GetImageVersion();
    styleNode->SetConfigString("name", "title");
    editor->OnNodeConfigChanged(editor->GetNodeWidget(styleNode->id), "name", true);
    WaitForRuns();
    EXPECT_EQ(calls.load(), 1);
    EXPECT_EQ(OutputStyleName(styleNode->id), "title");
    EXPECT_NE(LayoutPreview()->GetImageVersion(), version);

    // "-> Text style": the new node feeds the kit in the AI node's place, in one undo step
    auto card = editor->GetNodeWidget(styleNode->id);
    auto toStyle = card->FindChildByTypeAndName<Button>("to text style");
    ASSERT_TRUE(toStyle);
    int nodes = editor->GetGraph()->nodes.Count();
    toStyle->onClick();
    Step(3);
    ASSERT_EQ(editor->GetGraph()->nodes.Count(), nodes + 1);
    auto made = editor->GetGraph()->nodes.Last();
    EXPECT_EQ(made->nodeType, "textStyle");
    auto link = editor->GetGraph()->FindEdgeToPort(kitNode->id, "kit-title");
    ASSERT_TRUE(link);
    EXPECT_EQ(link->fromNodeId, made->id);
    EXPECT_TRUE(editor->GetGraph()->GetOutgoingEdges(styleNode->id).IsEmpty());

    // Its edits reach the layout card at once
    styleNode = made;
    Step(30);
    auto effects = StyleEffects();
    ASSERT_TRUE(effects);
    effects->AddEffect("shadow");
    auto shadowed = FramesUntilTheLayoutRedraws();
    EXPECT_GE(shadowed.first, 1);
    EXPECT_LE(shadowed.first, 2);

    actions->UndoAction();
    Step(3);
    EXPECT_EQ(editor->GetGraph()->nodes.Count(), nodes);
    link = editor->GetGraph()->FindEdgeToPort(kitNode->id, "kit-title");
    ASSERT_TRUE(link);
    EXPECT_NE(link->fromNodeId, made->id);
    UiDoc::SetUiAiTextStub({});
}
