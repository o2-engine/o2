#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

using namespace o2;
using namespace Editor;

// The extract card's "Settings for: All parts | This part" row, the mark of the parts with their own
// background settings and the progress of the parts. Screenshots go to Work/Pipelines/shots/ when run from build/o2

namespace
{
    void Step(int frames = 1)
    {
        for (int i = 0; i < frames; i++)
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
            o2Render.End();
            AppTestDriver::PumpFrames(1);
        }
    }

    void Shot(const String& name)
    {
        String dir = getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots");
        Ref<Bitmap> shot;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { shot = bitmap; });
        Step();
        ASSERT_TRUE(shot);
        o2FileSystem.FolderCreate(dir, true);
        EXPECT_TRUE(shot->Save(dir + "/" + name + ".png", Bitmap::ImageType::Png));
    }

    Ref<Toggle> FindSegment(const Ref<Widget>& root, const String& caption)
    {
        for (auto& child : root->GetChildWidgets())
        {
            if (auto toggle = DynamicCast<Toggle>(child); toggle && (String)toggle->GetCaption() == caption)
                return toggle;

            if (auto found = FindSegment(child, caption))
                return found;
        }
        return nullptr;
    }

    struct PipelinePartSettingsUiFixture : ::testing::Test
    {
        Ref<PipelineEditor> editor;
        Ref<PipelineAsset>  asset;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }
            PipelineUtils::SetWorkPathOverride("../../Work/Pipelines/uitest-parts/");

            PushEditorScopeOnStack scope;
            editor = mmake<PipelineEditor>();
            *editor->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
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
            editor = nullptr;
            asset = nullptr;
            o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
            PipelineUtils::SetWorkPathOverride("");
            if (UIRoot::IsSingletonInitialzed())
                EditorUIRoot.RemoveAllWidgets();
        }

        // An extract card with the parts coin, gem and chest and their results; the node renders a transparent
        // background in two passes and its parameter list is open. addMore puts other nodes and links beside it
        Ref<PipelineNode> OpenExtract(int parts, const Function<void(PipelineGraph&, const Ref<PipelineNode>&)>& addMore = {})
        {
            PipelineGraph graph;
            graph.id = "part-settings-ui";
            auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
            graph.nodes.Add(node);

            const char* names[] = { "coin", "gem", "chest" };
            Color4 colors[] = { Color4(235, 190, 40, 255), Color4(60, 170, 230, 255), Color4(150, 90, 50, 255) };
            Vector<PipelineExtractRegion> regions;
            for (int i = 0; i < parts; i++)
            {
                PipelineExtractRegion region;
                region.id = "p" + (String)i;
                region.name = names[i];
                region.x = 0.3f*i; region.y = 0.1f; region.w = 0.3f; region.h = 0.5f;
                regions.Add(region);
                PipelineUtils::WriteFileBytes(PipelineExecutor::GetPortPreviewPath(graph.id, node->id, region.id, "png"),
                                              PipelineValue::Image(PipelineImageOps::Blank(48, 48, colors[i])).GetPngBytes());
            }
            PipelineRegions::Write(*node, regions);
            PipelineNodeRegistry::SyncNodeWithSchema(node);
            node->SetConfigBool("transparentBg", true);
            node->SetConfigString("transparentMode", "twoPass");
            node->SetConfigBool("paramsOpen", true);
            node->SetConfigString("selectedRegion", parts > 1 ? "p1" : "p0");
            if (addMore)
                addMore(graph, node);

            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(3);
            auto card = editor->GetNodeWidget(node->id);
            editor->SetView(card->GetCardRect().Center(), 1.0f);
            Step(3);
            return editor->GetGraph()->FindNode(node->id);
        }

        Ref<Widget> Params(const Ref<PipelineNodeWidget>& card)
        {
            return card->FindChildByTypeAndName<Widget>("params list");
        }

        Ref<Widget> Dot(const Ref<PipelineNodeWidget>& card, int cell)
        {
            auto grid = card->FindChildByTypeAndName<Widget>("parts");
            return grid ? grid->GetChildWidgets()[cell]->FindChildByTypeAndName<Widget>("own settings") : nullptr;
        }
    };
}

TEST_F(PipelinePartSettingsUiFixture, ThisPartGivesTheSelectedPartItsOwnSettings)
{
    auto node = OpenExtract(3);
    auto card = editor->GetNodeWidget(node->id);
    ASSERT_TRUE(card);

    // A part following the node: "All parts" is on, no part is marked
    auto all = card->FindChildByTypeAndName<Toggle>("all parts");
    auto part = card->FindChildByTypeAndName<Toggle>("this part");
    ASSERT_TRUE(all && part);
    EXPECT_TRUE(all->GetValue());
    EXPECT_FALSE(part->GetValue());
    for (int i = 0; i < 3; i++)
    {
        ASSERT_TRUE(Dot(card, i));
        EXPECT_FALSE(Dot(card, i)->IsEnabled()) << i;
    }
    Shot("pipeline_part_settings_all");

    // "This part" copies what the part uses now onto it and marks its cell
    part->onToggleByUser(true);
    Step(3);
    auto own = PipelineRegions::FindTransparency(*node, "p1");
    ASSERT_TRUE(own);
    EXPECT_EQ(own->GetMembersCount(), 6);
    EXPECT_EQ(String((*own)["transparentMode"].GetString()), "twoPass");
    EXPECT_FALSE(PipelineRegions::FindTransparency(*node, "p0"));
    EXPECT_TRUE(Dot(card, 1)->IsEnabled());
    EXPECT_FALSE(Dot(card, 0)->IsEnabled());
    EXPECT_TRUE(card->FindChildByTypeAndName<Toggle>("this part")->GetValue());
    EXPECT_FALSE(card->FindChildByTypeAndName<Toggle>("all parts")->GetValue());

    // The rows below now edit the part: chroma key for it alone
    auto chroma = FindSegment(card, "Chroma key x1");
    ASSERT_TRUE(chroma);
    chroma->onToggleByUser(true);
    Step(3);
    card = editor->GetNodeWidget(node->id);
    EXPECT_EQ(String((*PipelineRegions::FindTransparency(*node, "p1"))["transparentMode"].GetString()), "chroma");
    EXPECT_EQ(node->GetConfigString("transparentMode", ""), "twoPass");
    auto tolerance = Params(card)->FindChildByType<PipelineSlider>();
    ASSERT_TRUE(tolerance);
    tolerance->onChanged(45.0f, true);
    Step(2);
    EXPECT_FLOAT_EQ((float)(*PipelineRegions::FindTransparency(*node, "p1"))["chromaTolerance"], 45.0f);
    EXPECT_FALSE(node->HasConfig("chromaTolerance"));
    Shot("pipeline_part_settings_own");

    // Another part shows the node's settings, the marked one its own again
    auto grid = card->FindChildByTypeAndName<Widget>("parts");
    ASSERT_TRUE(grid);
    grid->GetChildWidgets()[0]->FindChildByTypeAndName<Button>("select part")->onClick();
    Step(3);
    EXPECT_TRUE(card->FindChildByTypeAndName<Toggle>("all parts")->GetValue());
    EXPECT_FALSE(Params(card)->FindChildByType<PipelineColorField>()) << "the node renders two passes: no key colour row";
    EXPECT_TRUE(Dot(card, 1)->IsEnabled());
    Shot("pipeline_part_settings_other_part");

    grid->GetChildWidgets()[1]->FindChildByTypeAndName<Button>("select part")->onClick();
    Step(3);
    EXPECT_TRUE(card->FindChildByTypeAndName<Toggle>("this part")->GetValue());
    EXPECT_TRUE(Params(card)->FindChildByType<PipelineColorField>());
    EXPECT_FLOAT_EQ(Params(card)->FindChildByType<PipelineSlider>()->GetValue(), 45.0f);

    // A copy keeps the settings on its own part
    editor->SelectNodes({ node->id });
    editor->CopySelection();
    editor->Paste(Vec2F(), true);
    Step(3);
    ASSERT_EQ(editor->GetGraph()->nodes.Count(), 2);
    auto copy = editor->GetGraph()->nodes[1];
    ASSERT_EQ(copy->outputs.Count(), 3);
    EXPECT_NE(copy->outputs[1].id, "p1");
    EXPECT_TRUE(PipelineRegions::FindTransparency(*copy, copy->outputs[1].id));
    EXPECT_FALSE(PipelineRegions::FindTransparency(*copy, copy->outputs[0].id));

    // "All parts" returns the part to the node and takes the mark off
    card = editor->GetNodeWidget(node->id);
    card->FindChildByTypeAndName<Toggle>("all parts")->onToggleByUser(true);
    Step(3);
    EXPECT_FALSE(PipelineRegions::FindTransparency(*node, "p1"));
    EXPECT_FALSE(Dot(card, 1)->IsEnabled());
    EXPECT_FALSE(Params(card)->FindChildByType<PipelineColorField>());
}

// With one part the node's settings are the part's: no scope row and no mark
TEST_F(PipelinePartSettingsUiFixture, OnePartHasNoScopeRow)
{
    auto node = OpenExtract(1);
    auto card = editor->GetNodeWidget(node->id);
    ASSERT_TRUE(card);
    EXPECT_FALSE(card->FindChildByTypeAndName<Toggle>("all parts"));
    EXPECT_FALSE(card->FindChildByTypeAndName<Widget>("settings scope"));
    EXPECT_TRUE(FindSegment(card, "White / black x2"));
    EXPECT_FALSE(card->FindChildByTypeAndName<Widget>("own settings"));
}

// The executor's part events reach the cells in place: a spinner over the running part, a red frame round the
// failed one, both gone once the node stops; a single part spins while the node runs
TEST_F(PipelinePartSettingsUiFixture, PartsShowTheirProgressAndFailures)
{
    auto node = OpenExtract(3);
    auto card = editor->GetNodeWidget(node->id);
    ASSERT_TRUE(card);
    auto grid = card->FindChildByTypeAndName<Widget>("parts");
    ASSERT_TRUE(grid);
    auto cells = grid->GetChildWidgets();
    auto spinner = [&](int i) { return cells[i]->FindChildByTypeAndName<Widget>("part spinner"); };
    auto errorFrame = [&](int i) { return cells[i]->GetLayer("error"); };
    for (int i = 0; i < 3; i++)
    {
        ASSERT_TRUE(spinner(i) && errorFrame(i));
        EXPECT_FALSE(spinner(i)->IsEnabled()) << i;
        EXPECT_FALSE(errorFrame(i)->enabled) << i;
    }

    auto send = [&](const String& portId, const String& state, const String& error = "")
    {
        PipelineExecEvent event;
        event.type = PipelineExecEvent::Type::NodeState;
        event.nodeId = node->id;
        event.portId = portId;
        event.state = state;
        event.error = error;
        editor->GetExecutor()->onEvent(event);
    };

    send("", "running");
    send("p0", "running");
    send("p0", "done");
    send("p1", "running");
    send("p2", "running");
    send("p2", "error", "the model refused");
    Step(10);

    EXPECT_EQ(card->FindChildByTypeAndName<Widget>("parts"), grid) << "part events must not rebuild the card";
    EXPECT_FALSE(spinner(0)->IsEnabled());
    EXPECT_TRUE(spinner(1)->IsEnabled());
    EXPECT_FALSE(spinner(2)->IsEnabled());
    EXPECT_FALSE(errorFrame(1)->enabled);
    EXPECT_TRUE(errorFrame(2)->enabled);
    EXPECT_EQ(card->GetRuntime().state, "running");
    Shot("pipeline_part_progress");

    // The node stops: the part states go with it
    send("", "done");
    Step(2);
    EXPECT_TRUE(card->GetRuntime().portStates.IsEmpty());
    EXPECT_FALSE(spinner(1)->IsEnabled());
    EXPECT_FALSE(errorFrame(2)->enabled);

    // One part is computed as the node: it spins while the node runs
    auto single = OpenExtract(1);
    auto singleCard = editor->GetNodeWidget(single->id);
    ASSERT_TRUE(singleCard);
    auto singleSpinner = singleCard->FindChildByTypeAndName<Widget>("part spinner");
    ASSERT_TRUE(singleSpinner);
    PipelineExecEvent running;
    running.type = PipelineExecEvent::Type::NodeState;
    running.nodeId = single->id;
    running.state = "running";
    editor->GetExecutor()->onEvent(running);
    Step(2);
    EXPECT_TRUE(singleSpinner->IsEnabled());
    running.state = "done";
    editor->GetExecutor()->onEvent(running);
    Step(2);
    EXPECT_FALSE(singleSpinner->IsEnabled());
}

// A link leaving a part's port inside the card is drawn over the cards: next to the port, still inside the card,
// the line shows in the link colour instead of the card behind it
TEST_F(PipelinePartSettingsUiFixture, PartLinksAreDrawnOverTheCard)
{
    String targetId;
    auto node = OpenExtract(2, [&](PipelineGraph& graph, const Ref<PipelineNode>& extract)
    {
        auto target = PipelineNodeRegistry::CreateNode("imageColor", Vec2F(420, 100));
        graph.nodes.Add(target);
        targetId = target->id;
        auto edge = mmake<PipelineEdge>();
        edge->id = "part-link";
        edge->fromNodeId = extract->id;
        edge->fromPortId = "p1";
        edge->toNodeId = target->id;
        edge->toPortId = target->inputs[0].id;
        graph.edges.Add(edge);
    });
    auto card = editor->GetNodeWidget(node->id);
    ASSERT_TRUE(card && editor->GetNodeWidget(targetId));
    Vec2F bodyOffset;
    ASSERT_TRUE(card->IsBodyPort("p1", bodyOffset));

    // Between the port marker and the card edge, zoomed in so that sliver is a few pixels wide
    Vec2F port = card->GetPortPosition("p1", false);
    RectF cardRect = card->GetCardRect();
    ASSERT_GT(cardRect.right - port.x, PipelineNodeWidget::portRadius + 1.0f);
    Vec2F probe(port.x + (PipelineNodeWidget::portRadius + cardRect.right - port.x)*0.5f, port.y);
    editor->SetView(probe, 0.25f);
    Step(4);
    Shot("pipeline_part_link_over_card");

    Ref<Bitmap> capture;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { capture = bitmap; });
    Step();
    ASSERT_TRUE(capture);
    Vec2I resolution = o2Render.GetResolution();
    float scale = (float)capture->GetSize().x/Math::Max(1, resolution.x);
    Vec2F screen = editor->LocalToScreenPoint(probe);
    Vec2I px((int)((screen.x + resolution.x*0.5f)*scale), (int)((resolution.y*0.5f - screen.y)*scale));

    // The image link colour is orange; the card back is a light grey
    int linkRows = 0;
    for (int dy = -12; dy <= 12; dy++)
    {
        const UInt8* p = PipelineImageOps::Pixel(*capture, px.x, px.y + dy);
        if (p[0] > 220 && p[1] > 110 && p[1] < 190 && p[2] < 80)
            linkRows++;
    }
    EXPECT_GT(linkRows, 2) << "the link is hidden under the card at " << px.x << "," << px.y;
}

// A Composer layer linked to a part shows that part's own result; a part not produced yet shows nothing - the extract
// node's result is whichever part came last, never a stand-in for another one
TEST_F(PipelinePartSettingsUiFixture, LinkedPartsResolveToTheirOwnResultsOnly)
{
    String composerId;
    auto node = OpenExtract(3, [&](PipelineGraph& graph, const Ref<PipelineNode>& extract)
    {
        o2FileSystem.FileDelete(PipelineExecutor::GetPortPreviewPath(graph.id, extract->id, "p1", "png"));
        auto composer = PipelineNodeRegistry::CreateNode("composer", Vec2F(420, 0));
        composer->SetCustomInputs({ PipelinePort("l0", "coin", PipelinePortType::Image, true),
                                    PipelinePort("l1", "gem", PipelinePortType::Image, true),
                                    PipelinePort("l2", "chest", PipelinePortType::Image, true) });
        PipelineNodeRegistry::SyncNodeWithSchema(composer);
        graph.nodes.Add(composer);
        composerId = composer->id;
        for (int i = 0; i < 3; i++)
        {
            auto edge = mmake<PipelineEdge>();
            edge->id = "layer" + (String)i;
            edge->fromNodeId = extract->id;
            edge->fromPortId = "p" + (String)i;
            edge->toNodeId = composer->id;
            edge->toPortId = "l" + (String)i;
            graph.edges.Add(edge);
        }
    });
    auto composer = editor->GetGraph()->FindNode(composerId);
    ASSERT_TRUE(composer);
    auto runtime = editor->GetRuntime(node->id);
    ASSERT_TRUE(runtime);
    ASSERT_TRUE(runtime->output.IsImage()) << "the node shows some part as its result";
    EXPECT_FALSE(runtime->portOutputs.ContainsKey("p1"));

    auto colorOf = [&](const String& portId)
    {
        auto value = editor->GetInputValueById(composer, portId);
        if (!value.IsImage())
            return Color4(0, 0, 0, 0);

        const UInt8* p = PipelineImageOps::Pixel(*value.GetBitmap(), 4, 4);
        return Color4(p[0], p[1], p[2], p[3]);
    };
    EXPECT_EQ(colorOf("l0"), Color4(235, 190, 40, 255));
    EXPECT_FALSE(editor->GetInputValueById(composer, "l1").IsValid()) << "a missing part borrowed the node's result";
    EXPECT_EQ(colorOf("l2"), Color4(150, 90, 50, 255));
}
