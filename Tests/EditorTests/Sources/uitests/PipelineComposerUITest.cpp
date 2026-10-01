#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "PipelinePairUiFixture.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerLayers.h"

// The composer card: the layers list left of the work area, each input's port on its layer row, a grip that reorders
// the rows, "+ input" in the list and a card that grows with the list

namespace
{
    struct PipelineComposerUiFixture : PipelinePairUiFixture
    {
        Ref<PipelineNodeWidget> Card(const Ref<PipelineNode>& node) { return editor->GetNodeWidget(node->id); }

        Ref<PipelineComposerLayersPanel> Panel(const Ref<PipelineNode>& node)
        {
            return Card(node)->FindChildByTypeAndName<PipelineComposerLayersPanel>("layers panel");
        }

        // Opens a composer with that many image inputs; the source, when asked, is an image node beside it
        Ref<PipelineNode> OpenComposer(int inputs, Ref<PipelineNode>* source = nullptr)
        {
            PipelineGraph graph;
            graph.id = "uitest-composer";
            auto node = PipelineNodeRegistry::CreateNode("composer", Vec2F(0, 0));
            Vector<PipelinePort> customs;
            for (int i = 0; i < inputs; i++)
                customs.Add(PipelinePort("in" + (String)i, "layer " + (String)i, PipelinePortType::Image, true));
            node->SetCustomInputs(customs);
            PipelineNodeRegistry::SyncNodeWithSchema(node);
            graph.nodes.Add(node);

            Ref<PipelineNode> image;
            if (source)
            {
                image = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-400, 0));
                image->SetConfigString("uploadId", uploadId);
                graph.nodes.Add(image);
            }

            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(3);
            auto live = editor->GetGraph()->FindNode(node->id);
            if (source)
                *source = editor->GetGraph()->FindNode(image->id);
            editor->SetView(Card(live)->GetCardRect().Center(), 1.0f);
            Step(3);
            return live;
        }

        void Click(const Ref<Widget>& widget)
        {
            Vec2F point = editor->LocalToScreenPoint(widget->layout->GetWorldRect().Center());
            Drag(point, point, 1);
        }

        // Every input with a layer row has its port on the card's left edge, centred on the row
        void ExpectPortsOnRows(const Ref<PipelineNode>& node, const char* state)
        {
            auto card = Card(node);
            auto panel = Panel(node);
            ASSERT_TRUE(panel) << state;
            for (auto& port : node->inputs)
            {
                auto row = panel->FindRow(port.id);
                ASSERT_TRUE(row) << state << ": " << port.id.Data();
                Vec2F position = card->GetPortPosition(port.id, true);
                EXPECT_NEAR(position.y, row->layout->GetWorldRect().Center().y, 0.5f) << state << ": " << port.id.Data();
                EXPECT_NEAR(position.x, card->GetCardRect().left, 0.5f) << state;
            }
        }

        Vector<String> LayerOrder(const Ref<PipelineNode>& node)
        {
            Vector<String> ids;
            if (auto order = node->GetConfigValue("layerOrder"))
            {
                for (auto& id : *order)
                    ids.Add(PipelineUtils::ValueToString(id));
            }
            return ids;
        }
    };
}

// The ports follow their rows through every change of the list
TEST_F(PipelineComposerUiFixture, InputPortsSitOnTheirLayerRows)
{
    auto node = OpenComposer(4);
    auto panel = Panel(node);
    ASSERT_TRUE(panel);
    RectF main = Card(node)->FindChildByTypeAndName<Widget>("composer main")->layout->GetWorldRect();
    EXPECT_NEAR(panel->layout->GetWorldRect().left, main.left, 0.5f) << "the list is on the left";
    EXPECT_LT(panel->layout->GetWorldRect().Height(), main.Height() - 1.0f) << "the list keeps its own height";
    EXPECT_FLOAT_EQ(Card(node)->GetPortsHeight(), PipelineNodeWidget::portRow) << "the port rows hold only the output";
    EXPECT_NEAR(main.top, panel->layout->GetWorldRect().top, 0.5f) << "the list starts the body, no toolbar above it";
    ExpectPortsOnRows(node, "initial");
    ShotCard(node, "composer_initial");

    // A row's settings opened, then 9-slice on: the rows below and their ports move down
    Click(panel->FindRow("in2")->FindChildByTypeAndName<Button>("settings"));
    EXPECT_EQ(node->GetConfigString("openLayerSettings", ""), "in2");
    ExpectPortsOnRows(node, "settings open");
    auto nine = Panel(node)->FindChildByTypeAndName<Toggle>("nine slice");
    ASSERT_TRUE(nine);
    nine->SetValue(true);
    nine->onToggleByUser(true);
    Step(3);
    ExpectPortsOnRows(node, "9-slice on");
    ShotCard(node, "composer_open");

    // After a grip drag and after adding an input
    Click(Panel(node)->FindRow("in2")->FindChildByTypeAndName<Button>("settings"));
    auto grip = Panel(node)->FindGrip("in0");
    ASSERT_TRUE(grip);
    RectF top = Panel(node)->FindRow("in3")->layout->GetWorldRect();
    Drag(editor->LocalToScreenPoint(grip->layout->GetWorldRect().Center()), editor->LocalToScreenPoint(Vec2F(top.Center().x, top.top - 2.0f)));
    ExpectPortsOnRows(node, "after a grip drag");

    Click(Panel(node)->GetAddRow());
    EXPECT_EQ(node->inputs.Count(), 5);
    ExpectPortsOnRows(node, "after adding an input");

    // A copy has a row and no port of its own
    Click(Panel(node)->FindRow("in1")->FindChildByTypeAndName<Button>("duplicate"));
    EXPECT_EQ(node->inputs.Count(), 5);
    ExpectPortsOnRows(node, "with a copy");
}

// The grip moves a row: the landing line follows the pointer and the release writes the order; landing in place does nothing
TEST_F(PipelineComposerUiFixture, TheGripReordersTheLayers)
{
    auto node = OpenComposer(4);
    auto panel = Panel(node);
    // Shown front first: in3, in2, in1, in0
    auto grip = panel->FindGrip("in0");
    ASSERT_TRUE(grip);
    Vec2F from = editor->LocalToScreenPoint(grip->layout->GetWorldRect().Center());
    RectF second = panel->FindRow("in2")->layout->GetWorldRect();
    Vec2F to = editor->LocalToScreenPoint(Vec2F(second.Center().x, second.top - 1.0f));

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    o2Input.OnCursorMoved(from, 0, false);
    o2Input.OnCursorPressed(from);
    Step();
    for (int i = 1; i <= 8; i++)
    {
        o2Input.OnCursorMoved(Math::Lerp(from, to, i/8.0f), 0);
        Step();
    }
    EXPECT_EQ(panel->GetDropIndex(), 1) << "the line stands above the second row";
    EXPECT_LT(panel->FindRow("in0")->GetTransparency(), 1.0f) << "the dragged row is dimmed";
    ShotCard(node, "composer_grip_drag");
    o2Input.OnCursorReleased();
    Step(2);
    EXPECT_EQ(LayerOrder(node), (Vector<String>{ "in1", "in2", "in0", "in3" }));

    // Dropped where it already is: nothing changes
    auto before = LayerOrder(node);
    grip = Panel(node)->FindGrip("in2");
    Vec2F at = editor->LocalToScreenPoint(grip->layout->GetWorldRect().Center());
    Drag(at, at + Vec2F(0.0f, -3.0f), 3);
    EXPECT_EQ(LayerOrder(node), before);
}

// "+ input" adds an input on a click and creates and connects one when a link is dropped on it
TEST_F(PipelineComposerUiFixture, AddInputRowTakesAClickAndALink)
{
    Ref<PipelineNode> source;
    auto node = OpenComposer(1, &source);
    Click(Panel(node)->GetAddRow());
    ASSERT_EQ(node->inputs.Count(), 2);
    auto added = Panel(node)->FindRow(node->inputs[1].id);
    ASSERT_TRUE(added);
    EXPECT_GT(added->layout->GetWorldRect().top, Panel(node)->FindRow("in0")->layout->GetWorldRect().top) << "the new layer is the front one";

    editor->SetView((Card(node)->GetCardRect().Center() + editor->GetNodeWidget(source->id)->GetCardRect().Center())*0.5f, 1.5f);
    Step(3);
    Vec2F out = editor->GetNodeWidget(source->id)->GetPortPosition(source->outputs[0].id, false);
    Vec2F add = Panel(node)->GetAddRow()->layout->GetWorldRect().Center();
    Drag(editor->LocalToScreenPoint(out), editor->LocalToScreenPoint(add), 12);
    ASSERT_EQ(node->inputs.Count(), 3) << "a link dropped on + input makes an input";
    String newPort = node->inputs[2].id;
    EXPECT_TRUE(editor->GetGraph()->edges.Any([&](const Ref<PipelineEdge>& e)
    {
        return e->fromNodeId == source->id && e->toNodeId == node->id && e->toPortId == newPort;
    })) << "and connects it";
    ExpectPortsOnRows(node, "after the link");
}

// A hand-sized card grows when the list outgrows it and comes back to its height when the list shrinks
TEST_F(PipelineComposerUiFixture, TheCardGrowsWithTheList)
{
    auto node = OpenComposer(2);
    auto card = Card(node);
    float handSet = card->GetAutoHeight() + 40.0f;
    node->size = Vec2F(720.0f, handSet);
    card->UpdateFromNode();
    Step(3);
    EXPECT_NEAR(card->GetCardRect().Height(), handSet, 0.5f);

    for (int i = 0; i < 14; i++)
        editor->AddCustomInput(Card(node));
    Step(3);
    EXPECT_GT(Card(node)->GetCardRect().Height(), handSet + 50.0f) << "the list never scrolls: the card grows";
    EXPECT_NEAR(Card(node)->GetCardRect().Height(), Card(node)->GetAutoHeight(), 0.5f);
    EXPECT_NEAR(Panel(node)->layout->GetWorldRect().Height(), Panel(node)->GetContentHeight(), 0.5f);
    ExpectPortsOnRows(node, "grown");
    ShotCard(node, "composer_grown");

    while (node->inputs.Count() > 2)
        editor->RemoveCustomInput(Card(node), node->inputs.Last().id);
    Step(3);
    EXPECT_NEAR(Card(node)->GetCardRect().Height(), handSet, 0.5f) << "back to the hand-set height";
}

// The parameters sit under the work area in captioned groups, right-aligned, wrapping in a narrow card; the flips show
// while a layer is selected
TEST_F(PipelineComposerUiFixture, ParametersSitUnderTheWorkArea)
{
    auto node = OpenComposer(2);
    auto card = Card(node);
    auto controls = card->FindChildByTypeAndName<Widget>("composer controls");
    auto stage = card->FindChildByType<PipelineComposerStage>();
    ASSERT_TRUE(controls && stage);
    RectF controlsRect = controls->layout->GetWorldRect(), stageRect = stage->layout->GetWorldRect();
    RectF main = card->FindChildByTypeAndName<Widget>("composer main")->layout->GetWorldRect();
    EXPECT_NEAR(controlsRect.top, stageRect.bottom - 6.0f, 0.5f) << "under the work area";
    EXPECT_NEAR(controlsRect.bottom, main.bottom, 0.5f);
    EXPECT_NEAR(controlsRect.right, main.right, 0.5f);

    for (auto caption : { "Canvas", "Zoom", "Background" })
    {
        auto group = controls->FindChildByTypeAndName<Widget>(caption);
        ASSERT_TRUE(group && group->IsEnabledInHierarchy()) << caption;
    }
    auto flip = controls->FindChildByTypeAndName<Widget>("Flip");
    ASSERT_TRUE(flip);
    EXPECT_FALSE(flip->IsEnabled()) << "no layer selected";
    EXPECT_NEAR(controlsRect.Height(), 2*22.0f + 4.0f, 0.5f) << "two lines in a 720 card";
    float right = controls->FindChildByTypeAndName<Widget>("Zoom")->layout->GetWorldRect().right;
    EXPECT_NEAR(right, controlsRect.right, 1.0f) << "right-aligned";

    Click(Panel(node)->FindRow("in0")->FindChildByTypeAndName<Button>("settings"));
    flip = Card(node)->FindChildByTypeAndName<Widget>("Flip");
    EXPECT_TRUE(flip && flip->IsEnabled()) << "a selected layer brings the flips";
    ShotCard(node, "composer_controls");

    node->size = Vec2F(1300.0f, 0.0f);
    Card(node)->UpdateFromNode();
    editor->SetView(Card(node)->GetCardRect().Center(), 1.0f);
    Step(3);
    controls = Card(node)->FindChildByTypeAndName<Widget>("composer controls");
    EXPECT_NEAR(controls->layout->GetWorldRect().Height(), 22.0f, 0.5f) << "one line in a wide card";
    ExpectPortsOnRows(node, "wide");
}

// Focusing a layer's name field, by a click or the keyboard, selects the layer and leaves the focus in the field
TEST_F(PipelineComposerUiFixture, FocusingANameSelectsTheLayer)
{
    auto node = OpenComposer(3);
    auto stage = Card(node)->FindChildByType<PipelineComposerStage>();
    ASSERT_TRUE(stage);
    EXPECT_TRUE(stage->GetSelectedLayer().IsEmpty());

    auto name = Panel(node)->FindRow("in1")->FindChildByTypeAndName<EditBox>("layer name");
    ASSERT_TRUE(name);
    Click(name);
    EXPECT_EQ(stage->GetSelectedLayer(), "in1");
    EXPECT_GT(Panel(node)->FindRow("in1")->FindLayer("select")->transparency, 0.0f) << "the row is highlighted";
    EXPECT_EQ(o2UI.GetFocusedWidget(), Ref<Widget>(name)) << "the field keeps the focus";

    auto other = Panel(node)->FindRow("in2")->FindChildByTypeAndName<EditBox>("layer name");
    ASSERT_TRUE(other);
    o2UI.FocusWidget(other);
    Step(2);
    EXPECT_EQ(stage->GetSelectedLayer(), "in2");
    EXPECT_FLOAT_EQ(Panel(node)->FindRow("in1")->FindLayer("select")->transparency, 0.0f);
    EXPECT_EQ(o2UI.GetFocusedWidget(), Ref<Widget>(other));
}
