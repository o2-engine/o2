#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Windows/PipelineWindow/PipelineUiAi.h"

// The Text style, UI kit and AI UI layout cards: drawn with their content in both themes, and their AI buttons going through
// the model calls (stubbed: no network) into the node's config as one undo step

namespace
{
    struct PipelineUiCardsFixture : UiWorkspaceFixture
    {
        std::mutex     lock;
        Vector<String> prompts;
        std::function<String(const String&)> answer;
        Ref<ActionsList> actions;

        String WorkDir() const override { return "../../Work/Pipelines/uitest-uicards/"; }

        void SetUp() override
        {
            UiWorkspaceFixture::SetUp();
            UiDoc::SetUiAiTextStub([this](const String& model, const String& prompt, const Vector<AiImageRef>& images)
            {
                std::lock_guard<std::mutex> guard(lock);
                prompts.Add(prompt);
                AiTextResult r;
                r.text = answer ? answer(prompt) : String();
                r.ok = r.text != "!fail";
                if (!r.ok)
                    r.error = "the model is down";
                return r;
            });
        }

        void TearDown() override
        {
            UiDoc::SetUiAiTextStub({});
            UiWorkspaceFixture::TearDown();
        }

        // Steps until the call of the kind is done for the node
        bool WaitIdle(const String& kind, const String& nodeId)
        {
            for (int i = 0; i < 600 && PipelineUiAi::IsBusy(kind, nodeId); i++)
            {
                Step();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            Step(3);
            return !PipelineUiAi::IsBusy(kind, nodeId);
        }

        Ref<PipelineNode> Node(const String& type)
        {
            auto found = editor->GetGraph()->nodes.Find([&](const Ref<PipelineNode>& n) { return n->nodeType == type; });
            return found ? *found : nullptr;
        }

        // Opens the graph in the editor
        void OpenGraph(PipelineGraph& graph)
        {
            graph.id = "uitest-uicards";
            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(4);
        }

        void ShotBoth(const Ref<PipelineNode>& node, const String& name)
        {
            for (auto kind : { PipelineThemeKind::Light, PipelineThemeKind::Dark })
            {
                PipelineTheme::SetKind(kind);
                Step(4);
                auto card = editor->GetNodeWidget(node->id);
                editor->SetView(card->GetCardRect().Center(), 1.0f);
                editor->FinishBackgroundWork();
                Step(4);
                ShotCard(node, name + (kind == PipelineThemeKind::Dark ? "_dark" : "_light"));
            }
            PipelineTheme::FollowEditor();
            Step(3);
        }
    };
}

TEST_F(PipelineUiCardsFixture, LayoutCardShowsTheScreenAndTheAgentAddsAStep)
{
    actions = mmake<ActionsList>();
    editor->actionsListDelegate = actions;
    OpenWorkspace(true);
    UiWorkspace::Close();
    Step(3);
    auto card = editor->GetNodeWidget(layoutNode->id);
    ASSERT_TRUE(card);

    // The screen is drawn on the card, the status counts its blocks and steps
    auto preview = card->FindChildByTypeAndName<PipelineImageView>("layout preview");
    ASSERT_TRUE(preview);
    EXPECT_TRUE(preview->HasImage());
    ASSERT_TRUE(card->FindChildByTypeAndName<Button>("open workspace"));
    ASSERT_TRUE(card->FindChildByTypeAndName<Button>("send"));
    ShotBoth(layoutNode, "uicard_layout");

    // Send: the agent answers with an edit, the step joins the node in one undo step and the card shows the reply
    answer = [](const String&) { return String("Added a shop button.\n```uiml-edit\n@add list\n  button shop h=100 \"SHOP\"\n```"); };
    int before = UiDoc::StepsOf(layoutNode->config).Count();
    PipelineUiAi::SendAgent(editor, layoutNode->id, "add a shop button", "");
    EXPECT_TRUE(PipelineUiAi::IsBusy("agent", layoutNode->id));
    ASSERT_TRUE(WaitIdle("agent", layoutNode->id));
    layoutNode = editor->GetGraph()->FindNode(layoutNode->id);
    EXPECT_EQ(UiDoc::StepsOf(layoutNode->config).Count(), before + 1);
    EXPECT_TRUE(UiDoc::FindNode(UiDoc::DocAt(layoutNode->config).doc, "shop") != nullptr);
    EXPECT_TRUE(PipelineUiAi::LastError("agent", layoutNode->id).IsEmpty());
    ASSERT_FALSE(prompts.IsEmpty());
    EXPECT_TRUE(prompts[0].Contains("add a shop button"));
    auto reply = editor->GetNodeWidget(layoutNode->id)->FindChildByTypeAndName<Label>("reply");
    ASSERT_TRUE(reply);
    EXPECT_EQ((String)reply->GetText(), "Added a shop button.");

    // One undo takes the step back
    ASSERT_TRUE(actions);
    actions->UndoAction();
    Step(3);
    layoutNode = editor->GetGraph()->FindNode(layoutNode->id);
    EXPECT_EQ(UiDoc::StepsOf(layoutNode->config).Count(), before);

    // A failed call is shown on the card, the node keeps its steps
    answer = [](const String&) { return String("!fail"); };
    PipelineUiAi::SendAgent(editor, layoutNode->id, "again", "");
    ASSERT_TRUE(WaitIdle("agent", layoutNode->id));
    EXPECT_EQ(UiDoc::StepsOf(editor->GetGraph()->FindNode(layoutNode->id)->config).Count(), before);
    EXPECT_FALSE(PipelineUiAi::LastError("agent", layoutNode->id).IsEmpty());
    reply = editor->GetNodeWidget(layoutNode->id)->FindChildByTypeAndName<Label>("reply");
    ASSERT_TRUE(reply);
    EXPECT_TRUE(((String)reply->GetText()).Contains("the model is down")) << (String)reply->GetText();
}

TEST_F(PipelineUiCardsFixture, TextStyleCardDrawsTheSampleAndTheAiWritesTheStyle)
{
    PipelineGraph graph;
    auto made = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    made->SetConfigString("font", "Russo One");
    made->SetConfigString("color", "#FFE066");
    graph.nodes.Add(made);
    OpenGraph(graph);
    auto style = Node("textStyle");
    ASSERT_TRUE(style);
    auto card = editor->GetNodeWidget(style->id);
    ASSERT_TRUE(card);
    auto sample = card->FindChildByTypeAndName<PipelineImageView>("style sample");
    ASSERT_TRUE(sample);
    EXPECT_TRUE(sample->HasImage());
    ShotBoth(style, "uicard_textstyle");

    answer = [](const String&) { return String("```json\n{\"font\":\"Lobster\",\"size\":96,\"color\":\"#FF8800\",\"stroke\":{\"color\":\"#331100\",\"width\":6}}\n```"); };
    style->SetConfigString("stylePrompt", "a playful orange title");
    PipelineUiAi::GenerateStyle(editor, style->id);
    ASSERT_TRUE(WaitIdle("style", style->id));
    style = editor->GetGraph()->FindNode(style->id);
    EXPECT_EQ(style->GetConfigString("font"), "Lobster");
    EXPECT_EQ(style->GetConfigNumber("size"), 96.0f);
    EXPECT_EQ(style->GetConfigString("name"), "title") << "the name stays";
    EXPECT_TRUE(PipelineUiAi::LastError("style", style->id).IsEmpty());
    ShotBoth(style, "uicard_textstyle_ai");
}

TEST_F(PipelineUiCardsFixture, KitCardListsItsInputsAndTheAiDescribesThem)
{
    // A picture input on the kit, fed by an image source (a green plate)
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("kit-btn.png"), PipelineUtils::Base64Decode(KitPng(160, 80, Color4(76, 175, 80, 255), false)));
    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-600, 0));
    source->SetConfigString("uploadId", "kit-btn.png");
    auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
    kit->SetCustomInputs({ PipelinePort("kit-btn", "btn", PipelinePortType::Image, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(kit);
    graph.nodes = { source, kit };
    auto edge = mmake<PipelineEdge>();
    edge->id = "kit-edge";
    edge->fromNodeId = source->id;
    edge->fromPortId = source->outputs[0].id;
    edge->toNodeId = kit->id;
    edge->toPortId = "kit-btn";
    graph.edges.Add(edge);
    OpenGraph(graph);
    kitNode = editor->GetGraph()->FindNode(kit->id);

    auto card = editor->GetNodeWidget(kitNode->id);
    ASSERT_TRUE(card);
    EXPECT_TRUE(card->GetBody()->InputsInBody());
    ASSERT_TRUE(card->FindChildByTypeAndName<Button>("kit row"));
    ASSERT_TRUE(card->FindChildByTypeAndName<Widget>("kit source"));
    ShotBoth(kitNode, "uicard_kit");

    answer = [](const String&) { return String("```json\n[{\"name\":\"Btn_Green\",\"type\":\"button\",\"assetType\":\"sliced\",\"variant\":\"primary\","
                                               "\"description\":\"the main green button\",\"background\":\"none\"}]\n```"); };
    PipelineUiAi::AnalyzeKit(editor, kitNode->id, "");
    EXPECT_EQ(PipelineUiAi::KitBusyPort(kitNode->id), "*");
    ASSERT_TRUE(WaitIdle("kit", kitNode->id));
    EXPECT_TRUE(PipelineUiAi::LastError("kit", kitNode->id).IsEmpty()) << PipelineUiAi::LastError("kit", kitNode->id);
    auto items = UiDoc::KitItems(editor->GetGraph()->FindNode(kitNode->id)->config);
    ASSERT_EQ(items.Count(), 1);
    EXPECT_EQ(items[0].first, "kit-btn");
    EXPECT_EQ(items[0].second.type, "button");
    EXPECT_EQ(items[0].second.description, "the main green button");
    ShotBoth(kitNode, "uicard_kit_ai");
}

// Shift-dragging an output of a node with several of its type links all of them: dropped on a kit each gets an input of its
// own, on an input the rest go to the next free ones; one undo step
TEST_F(PipelineUiCardsFixture, ShiftDraggedPartsWireIntoTheKitAtOnce)
{
    actions = mmake<ActionsList>();
    editor->actionsListDelegate = actions;
    PipelineGraph graph;
    auto extract = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F(-600, 0));
    Vector<PipelineExtractRegion> regions;
    for (auto id : { "pa", "pb", "pc" })
    {
        PipelineExtractRegion region;
        region.id = id;
        region.name = String("part ") + id;
        regions.Add(region);
    }
    PipelineRegions::Write(*extract, regions);
    PipelineNodeRegistry::SyncNodeWithSchema(extract);
    auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
    auto finish = PipelineNodeRegistry::CreateNode("composer", Vec2F(0, 900));
    graph.nodes = { extract, kit, finish };
    OpenGraph(graph);

    EXPECT_EQ(editor->ConnectOutputs(extract->id, { "pa", "pb", "pc" }, kit->id, ""), 3);
    Step(2);
    auto liveKit = editor->GetGraph()->FindNode(kit->id);
    ASSERT_EQ(liveKit->inputs.Count(), 3);
    for (int i = 0; i < 3; i++)
    {
        auto edge = editor->GetGraph()->FindEdgeToPort(kit->id, liveKit->inputs[i].id);
        ASSERT_TRUE(edge);
        EXPECT_EQ(edge->fromPortId, String(i == 0 ? "pa" : i == 1 ? "pb" : "pc"));
    }

    actions->UndoAction();
    Step(2);
    EXPECT_EQ(editor->GetGraph()->FindNode(kit->id)->inputs.Count(), 0);
    EXPECT_TRUE(editor->GetGraph()->edges.IsEmpty());
}
