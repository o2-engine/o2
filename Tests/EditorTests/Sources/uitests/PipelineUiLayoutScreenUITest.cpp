#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiOwnGraphicsStrip.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsInspector.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsRefCompare.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsStylePicker.h"

// The AI UI layout's screen on the card and in the workspace (AssetsLine RefCompare.tsx, DevicePicker.tsx, StylePicker.tsx,
// the older-units notice, the node's own graphics): the reference compared with the layout (alpha, split, diff) on the stage
// and on the card, a layout in older units brought to its kit's device in one undoable step, the first hand edit of an empty
// layout putting it into the device's pixels, the hierarchy following the selection, the style picker's previews and the own
// graphics' ports on their cells

namespace
{
    // The screen in its device's pixels (iPhone 16's density)
    String OnDevice(const String& uiml)
    {
        return uiml.ReplacedAll("screen Menu 1080x1920", "screen Menu 1080x1920 dpi=460");
    }

    // A 9:16 reference: red on its left half, blue on its right half
    String ReferencePng()
    {
        auto bitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(270, 480));
        UInt8* data = bitmap->GetData();
        for (int y = 0; y < 480; y++)
        {
            for (int x = 0; x < 270; x++)
            {
                UInt8* p = data + (y*270 + x)*4;
                bool left = x < 135;
                p[0] = left ? 220 : 30;
                p[1] = 30;
                p[2] = left ? 30 : 220;
                p[3] = 255;
            }
        }
        return PipelineValue::Image(bitmap).GetPngBytes();
    }

    struct UiLayoutScreenFixture: UiWorkspaceFixture
    {
        Ref<ActionsList> undo;

        String WorkDir() const override { return "../../Work/Pipelines/uitest-uiscreen/"; }

        void SetUp() override
        {
            UiWorkspaceFixture::SetUp();
            undo = mmake<ActionsList>();
            editor->actionsListDelegate = undo;
        }

        void TearDown() override
        {
            editor->actionsListDelegate = nullptr;
            undo = nullptr;
            UiWorkspaceFixture::TearDown();
        }

        // Opens the layout fed by a reference picture (an image source), extra steps after the screen; the workspace stays
        // closed
        void OpenWithReference(const String& uiml, const Vector<UiDoc::UiStep>& extra = {})
        {
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("ref.png"), ReferencePng());
            PipelineGraph graph;
            graph.id = "uitest-uiscreen";
            auto node = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F(0, 0));
            node->size = Vec2F(520, 0);
            auto doc = UiDoc::ParseUiml(uiml).doc;
            Vector<UiDoc::UiStep> steps = { UiDoc::MakeAgentStep("lay out a menu", "A menu.", { UiDoc::MakeDocOp(doc) }, 1762862400000, "gpt-6.1-sol") };
            steps.Add(extra);
            UiDoc::WriteSteps(node->config, steps, {});
            node->SetCustomInputs({ PipelinePort("layout-ref", "reference", PipelinePortType::Image, true) });
            PipelineNodeRegistry::SyncNodeWithSchema(node);
            auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-420, 0));
            source->SetConfigString("uploadId", "ref.png");
            auto edge = mmake<PipelineEdge>();
            edge->id = PipelineNode::GenerateId();
            edge->fromNodeId = source->id;
            edge->fromPortId = source->outputs[0].id;
            edge->toNodeId = node->id;
            edge->toPortId = "layout-ref";
            graph.nodes = { node, source };
            graph.edges.Add(edge);
            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(4);
            layoutNode = editor->GetGraph()->FindNode(node->id);
        }

        Ref<PipelineNodeWidget> Card() const { return editor->GetNodeWidget(layoutNode->id); }

        // A pixel of the frame at a world point of the screen-space UI (the workspace)
        Color4 ScreenPixel(const Ref<Bitmap>& capture, const Vec2F& world)
        {
            Vec2I p = ScreenToCapture(world, capture);
            const UInt8* c = PipelineImageOps::Pixel(*capture, p.x, p.y);
            return Color4(c[0], c[1], c[2], 255);
        }

        // Shows the card at 1:1 in the view
        void ViewCard()
        {
            editor->SetView(Card()->GetCardRect().Center(), 1.0f);
            editor->FinishBackgroundWork();
            Step(4);
        }
    };

    bool Reddish(const Color4& c) { return c.r > 150 && c.b < 90; }
    bool Bluish(const Color4& c) { return c.b > 150 && c.r < 90; }
}

// The stage: the switch lays the reference over the screen see-through and shows the layout on the reference's screen; split
// shows the reference left of a divider the user drags; diff shows the per-pixel difference, worked out on a worker
TEST_F(UiLayoutScreenFixture, StageComparesWithTheReference)
{
    OpenWithReference(OnDevice(workspaceScreen));
    UiWorkspace::Open(editor, layoutNode->id);
    Step(4);
    auto stage = Window()->GetStage();
    stage->FlushRender();
    Step(3);
    ASSERT_TRUE(Ctx()->reference.has_value());
    EXPECT_EQ(Ctx()->reference->w, 1179.0) << "the picture taken to be the device's screen, in its pixels";
    EXPECT_EQ(Ctx()->reference->h, 2096.0);
    EXPECT_FALSE(Ctx()->ref.on);
    EXPECT_EQ(Ctx()->preview.kind, String("kit"));
    int steps = StepsNow().Count();

    stage->ClickHot("cmp:toggle");
    Step(3);
    EXPECT_TRUE(ConfigNow().Get("uiRefOn")->Bool());
    EXPECT_EQ(ConfigNow().Get("uiRefOverlay")->Num(), 0.5);
    EXPECT_EQ(Ctx()->preview.kind, String("reference"));
    EXPECT_TRUE(Ctx()->preview.comparing) << "shown on the reference's screen while compared";
    EXPECT_EQ(Ctx()->preview.frame.h, 1920.0);
    EXPECT_TRUE(stage->GetReferenceLayers().HasReference());
    Shot("compare_alpha");

    // Split: the reference left of the divider, the layout right of it; the divider drags and is written when let go
    stage->ClickHot("cmp:mode:split");
    Step(3);
    EXPECT_EQ(ConfigNow().Get("uiRefMode")->Str(), String("split"));
    stage->FlushRender();
    Step(3);
    RectF page(stage->DesignToWorld(0, 0), stage->DesignToWorld(1080, 1920));
    page = RectF(Math::Min(page.left, page.right), Math::Max(page.top, page.bottom), Math::Max(page.left, page.right), Math::Min(page.top, page.bottom));
    RectF divider = stage->HotRect("cmp:divider");
    ASSERT_GT(divider.Width(), 0.0f);
    EXPECT_NEAR(divider.Center().x, page.left + page.Width()*0.5f, 1.0f);
    auto capture = Capture();
    EXPECT_TRUE(Reddish(ScreenPixel(capture, Vec2F(page.left + page.Width()*0.1f, page.top - page.Height()*0.02f))))
        << "the reference's red half left of the divider";
    Shot("compare_split");

    Drag(divider.Center(), Vec2F(page.left + page.Width()*0.75f, divider.Center().y));
    Step(2);
    EXPECT_NEAR(ConfigNow().Get("uiRefSplit")->Num(), 0.75, 0.01);
    capture = Capture();
    EXPECT_TRUE(Bluish(ScreenPixel(capture, Vec2F(page.left + page.Width()*0.6f, page.top - page.Height()*0.02f))))
        << "now the reference's blue half shows left of the divider too";

    // Diff: the difference picture over the screen
    stage->ClickHot("cmp:mode:diff");
    Step(3);
    stage->GetReferenceLayers().FinishDiff(*Ctx(), Ctx()->view, Ctx()->showIds);
    EXPECT_TRUE(stage->GetReferenceLayers().IsDiffReady());
    EXPECT_EQ(stage->GetReferenceLayers().GetDiffSize(), Vec2I(900, 1600)) << "at most 1600 on its longer side";
    Step(2);
    Shot("compare_diff");

    // Off again: the chosen screen is back; none of it is a step of the layout
    stage->ClickHot("cmp:toggle");
    Step(3);
    EXPECT_FALSE(ConfigNow().Get("uiRefOn")->Bool());
    EXPECT_EQ(Ctx()->preview.kind, String("kit"));
    EXPECT_EQ(StepsNow().Count(), steps);
}

// The card: the compare button in the head row opens the menu (off and the modes, the amount), the reference lies over the
// card's screen, the split's divider drags on the card; the card keeps its height
TEST_F(UiLayoutScreenFixture, CardComparesFromItsMenu)
{
    OpenWithReference(OnDevice(workspaceScreen));
    ViewCard();
    auto card = Card();
    float height = card->GetCardSize().y;
    auto button = card->FindChildByTypeAndName<UiRefButton>("compare");
    ASSERT_TRUE(button);
    EXPECT_TRUE(button->IsEnabled() && button->IsUsable());
    button->ToggleMenu();
    Step(3);
    ASSERT_TRUE(button->IsMenuOpen());
    auto menu = button->GetMenuPanel();
    menu->Pick("split");
    Step(4);
    EXPECT_TRUE(ConfigNow().Get("uiRefOn")->Bool());
    EXPECT_EQ(ConfigNow().Get("uiRefMode")->Str(), String("split"));
    menu->SetAmount(0.3);
    Step(4);
    EXPECT_NEAR(ConfigNow().Get("uiRefSplit")->Num(), 0.3, 1e-9);
    button->ToggleMenu();
    Step(2);
    editor->FinishBackgroundWork();
    Step(4);

    auto preview = card->FindChildByTypeAndName<PipelineImageView>("layout preview");
    auto overlay = card->FindChildByTypeAndName<UiRefOverlayView>("compare overlay");
    ASSERT_TRUE(preview && overlay);
    RectF image = preview->GetImageRect();
    image = RectF(Math::Min(image.left, image.right), Math::Max(image.top, image.bottom), Math::Max(image.left, image.right), Math::Min(image.top, image.bottom));
    float x = image.left + image.Width()*0.3f;
    EXPECT_TRUE(overlay->IsUnderPoint(Vec2F(x, image.Center().y))) << "the divider takes the pointer";
    EXPECT_FALSE(overlay->IsUnderPoint(Vec2F(image.left + image.Width()*0.8f, image.Center().y))) << "the rest is the card's";
    auto capture = Capture();
    EXPECT_TRUE(Reddish(PixelAt(capture, Vec2F(image.left + image.Width()*0.1f, image.top - 6))));
    ShotCard(layoutNode, "uicard_compare_split");

    Drag(editor->LocalToScreenPoint(Vec2F(x, image.Center().y)), editor->LocalToScreenPoint(Vec2F(image.left + image.Width()*0.6f, image.Center().y)));
    Step(2);
    EXPECT_NEAR(ConfigNow().Get("uiRefSplit")->Num(), 0.6, 0.02);
    EXPECT_EQ(card->GetCardSize().y, height) << "the comparison costs the card no height";

    button->ToggleMenu();
    Step(2);
    menu->Pick("off");
    Step(3);
    EXPECT_FALSE(ConfigNow().Get("uiRefOn")->Bool());
}

// A layout in older units: the notice on the card's answer line, the title bar and the root inspector; one hand action with
// one screen op brings it to the kit's device, undo takes it back
TEST_F(UiLayoutScreenFixture, OlderLayoutIsBroughtToTheKitsDevice)
{
    OpenWorkspace(false);
    UiWorkspace::Close();
    Step(2);
    ViewCard();
    auto reply = Card()->FindChildByTypeAndName<Label>("reply");
    auto bring = Card()->FindChildByTypeAndName<Button>("bring to device");
    ASSERT_TRUE(reply && bring);
    EXPECT_TRUE(((String)reply->GetText()).StartsWith("This layout is in older units (1080\xC3\x97" "1920 at 480 dpi)"))
        << (String)reply->GetText();
    EXPECT_TRUE(bring->IsEnabled());
    ShotCard(layoutNode, "uicard_older");

    UiWorkspace::Open(editor, layoutNode->id);
    Step(4);
    EXPECT_GT(Window()->HotRect("bring").Width(), 0.0f) << "the title bar's notice";
    Ctx()->SetUi("uiTab", UiDocJson("inspect"));
    Step(3);
    auto inspector = Window()->GetSide()->GetInspector();
    ASSERT_TRUE(inspector);
    EXPECT_TRUE(inspector->GetField("num:screen:dpi")) << "an older layout's DPI is still edited by hand";
    Shot("older_inspector");

    int steps = StepsNow().Count();
    inspector->ClickHot("bring");
    Step(3);
    ASSERT_EQ(StepsNow().Count(), steps + 1);
    EXPECT_EQ(LastLabel(), String("Units \xE2\x86\x92 iPhone 16"));
    EXPECT_EQ(StepsNow().Last().Ops()[0].Dump(), String("{\"op\":\"screen\",\"width\":1179,\"height\":2096,\"dpi\":460,\"scale\":1.09167,\"fromDpi\":480}"));
    EXPECT_EQ(DocNow().width, 1179.0);
    EXPECT_FALSE(UiDoc::Ws::InOlderUnits(DocNow(), Ctx()->target));
    EXPECT_FALSE(inspector->GetField("num:screen:dpi")) << "the device's DPI, read-only";
    EXPECT_EQ(Window()->HotRect("bring").Width(), 0.0f);
    Shot("brought_inspector");

    Ctx()->Undo();
    Step(3);
    EXPECT_EQ(StepsNow().Count(), steps);
    EXPECT_EQ(DocNow().width, 1080.0);
}

// The first hand edit of an empty layout at the base density puts it into the device's pixels first (the reference's area on
// the device when there is one)
TEST_F(UiLayoutScreenFixture, FirstHandEditOfAnEmptyLayoutSetsTheScreen)
{
    OpenWithReference("screen Menu 1080x1920\n");
    UiWorkspace::Open(editor, layoutNode->id);
    Step(4);
    EXPECT_FALSE(UiDoc::Ws::InOlderUnits(DocNow(), Ctx()->target)) << "an empty layout is no concern";
    int steps = StepsNow().Count();
    String label, id;
    auto op = UiDoc::Ws::AddBlockOp(Ctx()->Doc(), "", "button", label, id);
    Ctx()->Gesture({ op }, label);
    Step(3);
    ASSERT_EQ(StepsNow().Count(), steps + 1);
    auto ops = StepsNow().Last().Ops();
    ASSERT_EQ(ops.Count(), 2);
    EXPECT_EQ(ops[0].Dump(), String("{\"op\":\"screen\",\"width\":1179,\"height\":2096,\"dpi\":460}"));
    EXPECT_EQ(DocNow().dpi.value_or(0), 460.0);
    EXPECT_TRUE(UiDoc::FindNode(DocNow(), id));

    // In its device's pixels now: the next edit goes alone (an action of its own in the same run of hand edits)
    auto op2 = UiDoc::Ws::AddBlockOp(Ctx()->Doc(), "", "text", label, id);
    Ctx()->Gesture({ op2 }, label);
    Step(3);
    auto parts = UiDoc::StepParts(StepsNow().Last());
    ASSERT_EQ(parts.Count(), 2);
    EXPECT_EQ(parts[0].n, 2.0);
    EXPECT_EQ(parts[1].n, 1.0);
}

// The hierarchy follows a selection made elsewhere: the blocks folded around it open and the list scrolls to its row
TEST_F(UiLayoutScreenFixture, HierarchyScrollsToTheSelection)
{
    String uiml = "screen Menu 1080x1920 dpi=460\n  column list anchor=top-left pos=40,80 size=300x1800 gap=4\n";
    for (int i = 0; i < 40; i++)
        uiml += "    button b" + (String)i + " h=40 \"B\"\n";
    OpenWorkspace(false, uiml);
    auto tree = Window()->GetHierarchy();
    tree->ToggleFold("list");
    Step(2);
    EXPECT_TRUE(tree->IsFolded("list"));
    EXPECT_EQ(tree->GetScroll(), 0.0f);

    Ctx()->Select("b39");
    Step(3);
    EXPECT_FALSE(tree->IsFolded("list")) << "opened around the selection";
    EXPECT_GT(tree->GetScroll(), 0.0f);
    RectF row = tree->RowRect("b39");
    RectF panel = tree->layout->GetWorldRect();
    EXPECT_GT(row.Height(), 0.0f);
    EXPECT_GE(row.bottom, panel.bottom - 1.0f) << "its row is in view";

    // A row in view stays where it is
    float scroll = tree->GetScroll();
    Ctx()->Select("b38");
    Step(3);
    EXPECT_EQ(tree->GetScroll(), scroll);
}

// The inspector's text style row: a preview with the style's name; the list shows "auto" first and every kit style, each
// drawn; a pick writes the block's style, auto unsets it
TEST_F(UiLayoutScreenFixture, StylePickerPicksFromPreviews)
{
    OpenWorkspace(true, OnDevice(workspaceScreen));
    Ctx()->Select("play");
    Ctx()->SetUi("uiTab", UiDocJson("inspect"));
    Step(4);
    auto inspector = Window()->GetSide()->GetInspector();
    ASSERT_TRUE(inspector);
    RectF button = inspector->HotRect("stylepick");
    ASSERT_GT(button.Width(), 0.0f);
    EXPECT_GT(UiWsStylePicker::CachedPreviews(), 0) << "the button draws its preview";
    inspector->ClickHot("stylepick");
    Step(3);
    auto list = inspector->FindChildByType<UiWsStyleListPanel>();
    ASSERT_TRUE(list && list->IsEnabled());
    EXPECT_EQ(list->RowIds(), Vector<String>({ "auto", "title" }));
    Shot("style_picker");
    list->ClickHot("style:title");
    Step(3);
    EXPECT_EQ(*UiDoc::FindNode(DocNow(), "play")->style, String("title"));

    inspector->ClickHot("stylepick");
    Step(3);
    list->ClickHot("style:");
    Step(3);
    EXPECT_FALSE(UiDoc::FindNode(DocNow(), "play")->style.has_value());
}

// The node's own graphics under the card's screen: a cell each with its picture and name, the graphic's output port on the
// cell's top-right corner; an undo of the step that made them takes the strip away
TEST_F(UiLayoutScreenFixture, OwnGraphicsSitInCellsWithTheirPorts)
{
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("plate.png"),
                                  PipelineValue::Image(PipelineImageOps::Blank(120, 60, Color4(200, 120, 40, 255))).GetPngBytes());
    UiDoc::Json raw;
    UiDoc::Json::Parse(R"json({"id":"s2","by":"agent","at":"2026-11-12T10:00:00.000Z","prompt":"draw a plate","reply":"Drew it.","ops":[],"assets":[
        {"name":"plate","uploadId":"plate.png","type":"panel","assetType":"simple","width":120,"height":60,"dpi":460,"from":"drawn"},
        {"name":"badge","uploadId":"plate.png","type":"badge","assetType":"simple","width":120,"height":60,"dpi":460,"from":"cut"}]})json", raw);
    OpenWithReference(OnDevice(workspaceScreen), { UiDoc::UiStep(raw) });
    ViewCard();
    auto card = Card();
    auto strip = card->FindChildByTypeAndName<UiOwnGraphicsStrip>("own graphics");
    ASSERT_TRUE(strip);
    ASSERT_EQ(strip->GetCells().Count(), 2);
    EXPECT_TRUE(strip->HasPicture(0));
    for (int i = 0; i < 2; i++)
    {
        const auto& cell = strip->GetCells()[i];
        Vec2F corner;
        ASSERT_TRUE(strip->GetCellCorner(i, corner));
        Vec2F port = card->GetPortPosition(cell.portId, false);
        EXPECT_NEAR(port.x, corner.x, 1.5f) << cell.name.Data();
        EXPECT_NEAR(port.y, corner.y, 1.5f) << cell.name.Data();
    }
    Vec2F image = card->GetPortPosition(layoutNode->outputs[0].id, false);
    EXPECT_NEAR(image.x, card->GetCardRect().right, 1.0f) << "the schema outputs keep their port rows";
    ShotCard(layoutNode, "uicard_own_graphics");

    // Undo of the step: no graphics, no strip, the card shorter
    float height = card->GetCardSize().y;
    UiWorkspace::Open(editor, layoutNode->id);
    Step(3);
    Ctx()->Undo();
    Step(4);
    UiWorkspace::Close();
    Step(4);
    card = Card();
    strip = card->FindChildByTypeAndName<UiOwnGraphicsStrip>("own graphics");
    ASSERT_TRUE(strip);
    EXPECT_EQ(strip->GetCells().Count(), 0);
    EXPECT_LT(card->GetCardSize().y, height);
}
