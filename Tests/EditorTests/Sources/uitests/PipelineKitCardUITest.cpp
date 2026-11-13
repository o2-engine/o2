#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/PipelineKitCard.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTextStyleFx.h"

// The UI kit card as a grid of cells (AssetsLine UiKitBody.tsx, KitPopup.tsx, KitDevicePicker.tsx) and the Text style card's
// fill and effects (TextStyleBody.tsx, TextStyleFx.tsx): the inputs on the cells' corners, the element's settings in a
// popup beside its cell, the numbers that apply on Enter only, the device menu, the gradient stops and the effect rows.
// Shots of both themes go to $O2_PIPELINE_SHOTS/pairs

namespace
{
    struct PipelineKitCardFixture : UiWorkspaceFixture
    {
        Ref<ActionsList> actions;

        String WorkDir() const override { return "../../Work/Pipelines/uitest-kitcard/"; }

        Ref<PipelineNode> Node(const String& id) { return editor->GetGraph()->FindNode(id); }

        void OpenGraph(PipelineGraph& graph)
        {
            graph.id = "uitest-kitcard";
            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(4);
        }

        // A kit fed by a green button picture and a text style; the kit made as a run makes it
        void OpenKit(bool built = true)
        {
            actions = mmake<ActionsList>();
            editor->actionsListDelegate = actions;
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("kit-btn.png"),
                                          PipelineUtils::Base64Decode(KitPng(160, 80, Color4(76, 175, 80, 255), false)));
            PipelineGraph graph;
            auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-600, 0));
            source->SetConfigString("uploadId", "kit-btn.png");
            auto style = PipelineNodeRegistry::CreateNode("textStyle", Vec2F(-600, 400));
            style->SetConfigString("font", "Lilita One");
            style->SetConfigString("color", "#FFE066");
            auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
            kit->SetCustomInputs({ PipelinePort("kit-btn", "btn", PipelinePortType::Image, true),
                                   PipelinePort("kit-title", "title", PipelinePortType::TextStyle, true) });
            PipelineNodeRegistry::SyncNodeWithSchema(kit);
            graph.nodes = { source, style, kit };
            auto link = [&](const Ref<PipelineNode>& from, const String& port)
            {
                auto edge = mmake<PipelineEdge>();
                edge->id = PipelineNode::GenerateId();
                edge->fromNodeId = from->id;
                edge->fromPortId = from->outputs[0].id;
                edge->toNodeId = kit->id;
                edge->toPortId = port;
                graph.edges.Add(edge);
            };
            link(source, "kit-btn");
            link(style, "kit-title");
            OpenGraph(graph);
            kitNode = Node(kit->id);
            styleId = style->id;

            if (built)
            {
                Map<String, PipelineValue> byId;
                byId["kit-btn"] = editor->GetInputValueById(kitNode, "kit-btn");
                byId["kit-title"] = editor->GetInputValueById(kitNode, "kit-title");
                auto made = PipelineUiNodes::BuildKit(*kitNode, byId, {});
                auto card = editor->GetNodeWidget(kitNode->id);
                card->GetRuntime().output = PipelineValue::TextOf(PipelinePortType::UiKit, UiDoc::KitToJson(made).Dump());
                card->OnOutputChanged();
            }
            View();
        }

        void View()
        {
            auto card = editor->GetNodeWidget(kitNode->id);
            editor->SetView(card->GetCardRect().Center() + Vec2F(180, 0), 1.0f);
            editor->FinishBackgroundWork();
            Step(4);
        }

        Ref<PipelineUiKitBody> Body()
        {
            auto card = editor->GetNodeWidget(kitNode->id);
            return card ? DynamicCast<PipelineUiKitBody>(card->GetBody()) : nullptr;
        }

        UiDocJson Item(const String& port)
        {
            return Body()->ItemOf(port);
        }

        Vec2F Screen(const Vec2F& canvas) { return editor->LocalToScreenPoint(canvas); }

        void Key(int key)
        {
            o2Input.OnKeyPressed(key);
            Step();
            o2Input.OnKeyReleased(key);
            Step(3);
        }

        // Types digits into a field, then Enter, Escape or nothing
        void Type(const Ref<EditBox>& field, const String& digits, int finish)
        {
            ASSERT_TRUE(field);
            field->Focus();
            Step(2);
            for (char c : digits)
            {
                o2Input.OnKeyPressed((KeyboardKey)c);
                Step();
                o2Input.OnKeyReleased((KeyboardKey)c);
                Step();
            }
            if (finish)
                Key(finish);
            Step(3);
        }

        // Saves the part of the frame round a canvas rectangle
        void ShotArea(const RectF& area, const String& name)
        {
            auto capture = Capture();
            ASSERT_TRUE(capture);
            Vec2I lt = ScreenToCapture(Screen(Vec2F(area.left - 6, area.top + 6)), capture);
            Vec2I rb = ScreenToCapture(Screen(Vec2F(area.right + 6, area.bottom - 6)), capture);
            lt = Vec2I(Math::Max(0, lt.x), Math::Max(0, lt.y));
            rb = Vec2I(Math::Min(capture->GetSize().x, rb.x), Math::Min(capture->GetSize().y, rb.y));
            auto crop = PipelineImageOps::CropPixels(*capture, lt.x, lt.y, rb.x - lt.x, rb.y - lt.y);
            String dir = (getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots")) + "/pairs";
            o2FileSystem.FolderCreate(dir, true);
            ASSERT_TRUE(crop);
            EXPECT_TRUE(crop->Save(dir + "/" + name + ".png", Bitmap::ImageType::Png));
        }

        // The card with its popup, if one is open, in both themes
        void ShotBoth(const String& name)
        {
            for (auto kind : { PipelineThemeKind::Light, PipelineThemeKind::Dark })
            {
                PipelineTheme::SetKind(kind);
                Step(4);
                View();
                RectF area = editor->GetNodeWidget(kitNode->id)->GetCardRect();
                if (auto popup = Body()->GetPopup())
                {
                    RectF pop = popup->layout->GetWorldRect();
                    area = RectF(Math::Min(area.left, pop.left), Math::Max(area.top, pop.top), Math::Max(area.right, pop.right),
                                 Math::Min(area.bottom, pop.bottom));
                }
                ShotArea(area, name + (kind == PipelineThemeKind::Dark ? "_dark" : "_light"));
            }
            PipelineTheme::FollowEditor();
            Step(3);
        }

        String styleId;
    };
}

// The inputs sit on their cells' top-left corners; a link curves to the card's edge level with its port and runs on to it
TEST_F(PipelineKitCardFixture, CellsCarryTheirInputsOnTheirCorners)
{
    OpenKit();
    auto card = editor->GetNodeWidget(kitNode->id);
    auto body = Body();
    ASSERT_TRUE(body && body->GetGrid());
    EXPECT_EQ(card->GetCardWidth(), 460.0f);
    auto& cells = body->GetGrid()->GetCells();
    ASSERT_EQ(cells.Count(), 2);
    EXPECT_TRUE(cells[0].element.has_value()) << "the kit's own picture of the input";
    EXPECT_TRUE(cells[1].style && cells[1].picture) << "\"Aa\" in the style's look";
    EXPECT_EQ(cells[1].name, "title");

    for (auto id : { "kit-btn", "kit-title" })
    {
        RectF cell = body->GetGrid()->GetCellRect(id);
        Vec2F port = card->GetPortPosition(id, true);
        EXPECT_NEAR(port.x, cell.left, 0.6f) << id;
        EXPECT_NEAR(port.y, cell.top, 0.6f) << id;
        Vec2F entry;
        ASSERT_TRUE(card->GetInputEntry(id, card->GetCardRect(), entry));
        EXPECT_EQ(entry.x, card->GetCardRect().left);
        EXPECT_NEAR(entry.y, port.y, 0.01f);
    }

    // The grid takes a link dropped anywhere on it as a new input; the corners are the ports'
    RectF add = body->GetGrid()->GetCellRect("");
    EXPECT_TRUE(card->IsAddInputAt(add.Center()));
    EXPECT_TRUE(card->FindPortAt(card->GetPortPosition("kit-btn", true)));
    ShotBoth("uicard_kit_grid");
}

// A click on a cell opens its element's settings beside the grid, level with the cell; Escape, the close button, a click
// outside and a second click on the cell close them; another cell moves them
TEST_F(PipelineKitCardFixture, CellOpensItsSettingsBesideIt)
{
    OpenKit();
    auto body = Body();
    RectF cell = body->GetGrid()->GetCellRect("kit-btn");
    Click(Screen(cell.Center()));
    Step(3);
    auto popup = Body()->GetPopup();
    ASSERT_TRUE(popup);
    EXPECT_EQ(popup->portId, "kit-btn");
    RectF pop = popup->layout->GetWorldRect();
    RectF grid = body->GetGrid()->layout->GetWorldRect();
    EXPECT_NEAR(pop.left, grid.right + PipelineKitPopup::gapToCell, 1.0f) << "right of the whole grid";
    EXPECT_GE(pop.top, cell.top + 11.0f) << "level with its cell, or raised to fit the view";
    EXPECT_EQ(pop.Width(), PipelineKitPopup::width);
    ASSERT_TRUE(popup->GetGuides());
    EXPECT_TRUE(popup->GetGuides()->HasImage());
    EXPECT_TRUE(popup->GetField("purpose") && popup->GetField("sizeW"));
    ShotBoth("uicard_kit_popup");

    // It zooms with the canvas and stays beside the grid
    editor->SetView(editor->GetNodeWidget(kitNode->id)->GetCardRect().Center(), 0.7f);
    Step(4);
    grid = Body()->GetGrid()->layout->GetWorldRect();
    pop = Body()->GetPopup()->layout->GetWorldRect();
    EXPECT_NEAR(pop.left, grid.right + PipelineKitPopup::gapToCell, 1.0f);
    View();

    Key(VK_ESCAPE);
    EXPECT_FALSE(Body()->GetPopup()) << "Escape closes it";

    cell = Body()->GetGrid()->GetCellRect("kit-btn");
    Click(Screen(cell.Center()));
    ASSERT_TRUE(Body()->GetPopup());
    Click(Screen(cell.Center()));
    EXPECT_FALSE(Body()->GetPopup()) << "a second click on the cell closes it";

    // The popup covers no other cell: another cell's click moves it there
    Click(Screen(cell.Center()));
    ASSERT_TRUE(Body()->GetPopup());
    Click(Screen(Body()->GetGrid()->GetCellRect("kit-title").Center()));
    ASSERT_TRUE(Body()->GetPopup());
    EXPECT_EQ(Body()->GetPopup()->portId, "kit-title");
    EXPECT_FALSE(Body()->GetPopup()->GetGuides()) << "a style has no source";
    ShotBoth("uicard_kit_popup_style");
    Click(Screen(cell.Center()));
    ASSERT_TRUE(Body()->GetPopup());
    EXPECT_EQ(Body()->GetPopup()->portId, "kit-btn") << "another cell moves it";

    Body()->GetPopup()->Click("close");
    Step(3);
    EXPECT_FALSE(Body()->GetPopup()) << "its close button";

    Click(Screen(cell.Center()));
    ASSERT_TRUE(Body()->GetPopup());
    RectF card = editor->GetNodeWidget(kitNode->id)->GetCardRect();
    Click(Screen(Vec2F(card.left - 200, card.bottom - 120)));
    EXPECT_FALSE(Body()->GetPopup()) << "a click outside";
}

// The size in inches and the own DPI apply on Enter or when the field is left; Escape drops what was typed. The sides keep
// their proportion while locked; "auto" goes back to the size found
TEST_F(PipelineKitCardFixture, SizeAndDpiApplyOnEnterOnly)
{
    OpenKit();
    Body()->OpenCell("kit-btn");
    Step(3);
    auto popup = Body()->GetPopup();
    ASSERT_TRUE(popup);
    auto el = Body()->ElementOf("kit-btn");
    ASSERT_TRUE(el && el->inches);
    double ratio = el->inches->h/el->inches->w;

    Type(popup->GetField("sizeW"), "2", 0);
    Key(VK_RETURN);
    Step(3);
    auto item = Item("kit-btn");
    auto size = item.Get("size");
    ASSERT_TRUE(size && size->IsObject());
    EXPECT_EQ(size->Get("w")->Num(), 2.0);
    EXPECT_NEAR(size->Get("h")->Num(), std::round(2.0*ratio*1000.0)/1000.0, 0.0011) << "the height follows while locked";

    Type(Body()->GetPopup()->GetField("sizeH"), "3", VK_ESCAPE);
    EXPECT_EQ(Item("kit-btn").Get("size")->Get("w")->Num(), 2.0) << "Escape drops it";
    EXPECT_NE(Item("kit-btn").Get("size")->Get("h")->Num(), 3.0);
    ASSERT_TRUE(Body()->GetPopup());

    Body()->GetPopup()->Click("lock");
    EXPECT_FALSE(Body()->GetPopup()->IsLocked());
    double h = Item("kit-btn").Get("size")->Get("h")->Num();
    Type(Body()->GetPopup()->GetField("sizeW"), "1", VK_RETURN);
    EXPECT_EQ(Item("kit-btn").Get("size")->Get("w")->Num(), 1.0);
    EXPECT_EQ(Item("kit-btn").Get("size")->Get("h")->Num(), h) << "unlocked: the other side stays";

    Body()->GetPopup()->Click("sizeAuto");
    Step(3);
    EXPECT_TRUE(Item("kit-btn").Get("size") && Item("kit-btn").Get("size")->IsNull()) << "auto";

    // DPI: the kit's device, or the element's own
    auto device = Body()->Device();
    Body()->GetPopup()->Click("dpi:own");
    Step(3);
    ASSERT_TRUE(Item("kit-btn").Get("dpi"));
    EXPECT_EQ(Item("kit-btn").Get("dpi")->Num(), device.dpi);
    auto dpiField = Body()->GetPopup()->GetField("dpi");
    ASSERT_TRUE(dpiField) << "the own density's field";
    Type(dpiField, "300", VK_ESCAPE);
    EXPECT_EQ(Item("kit-btn").Get("dpi")->Num(), device.dpi);
    Type(Body()->GetPopup()->GetField("dpi"), "300", VK_RETURN);
    EXPECT_EQ(Item("kit-btn").Get("dpi")->Num(), 300.0);
    Body()->GetPopup()->Click("dpi:kit");
    Step(3);
    EXPECT_TRUE(Item("kit-btn").Get("dpi")->IsNull());

    // The purpose typed is the user's: the AI's mark goes
    Body()->SetItemFields("kit-btn", { { "purpose", UiDocJson("backing of buttons") }, { "purposeFrom", UiDocJson("ai") } });
    Step(3);
    Body()->GetPopup()->GetField("purpose")->Focus();
    Step(2);
    Body()->GetPopup()->GetField("purpose")->SetText("main action button");
    Step();
    Body()->GetPopup()->GetField("purpose")->Unfocus();
    Step(3);
    EXPECT_EQ(Item("kit-btn").Get("purpose")->ToJsString(), "main action button");
    EXPECT_FALSE(Item("kit-btn").Get("purposeFrom"));
}

// The device chip opens the devices by kind and the custom screen; its numbers apply on Enter, Escape drops them
TEST_F(PipelineKitCardFixture, DeviceMenuPicksADeviceAndTypesAScreen)
{
    OpenKit(false);
    auto chip = Body()->GetDeviceChip();
    ASSERT_TRUE(chip);
    EXPECT_EQ(chip->Target().id, UiDoc::DefaultTarget().id);
    auto tablet = UiDoc::UiDevices().Find([](const UiDoc::UiDevice& d) { return d.kind == "tablet"; });
    ASSERT_TRUE(tablet);

    chip->ToggleMenu();
    Step(3);
    ASSERT_TRUE(chip->IsMenuOpen());
    Shot("kit_device_menu");
    chip->GetMenuPanel()->ClickHot("device:" + tablet->id);
    Step(3);
    EXPECT_EQ(kitNode->GetConfigString("kitDevice", ""), tablet->id);
    EXPECT_FALSE(chip->IsMenuOpen()) << "a pick closes the menu";

    chip = Body()->GetDeviceChip();
    chip->ToggleMenu();
    Step(3);
    auto width = chip->GetMenuPanel()->FindChildByTypeAndName<EditBox>("custom w");
    ASSERT_TRUE(width);
    Type(width, "1000", VK_ESCAPE);
    EXPECT_EQ(kitNode->GetConfigString("kitDevice", ""), tablet->id) << "Escape drops it";

    chip = Body()->GetDeviceChip();
    if (!chip->IsMenuOpen())
    {
        chip->ToggleMenu();
        Step(3);
    }
    width = chip->GetMenuPanel()->FindChildByTypeAndName<EditBox>("custom w");
    Type(width, "1000", VK_RETURN);
    EXPECT_EQ(kitNode->GetConfigString("kitDevice", ""), "custom");
    auto custom = UiDoc::FromDataValue(*kitNode->GetConfigValue("kitDeviceCustom"));
    EXPECT_EQ(custom.Get("w")->Num(), 1000.0);
    EXPECT_EQ(custom.Get("h")->Num(), tablet->h) << "the rest from the device chosen before";
    EXPECT_EQ(UiDoc::TargetOfConfig(UiDoc::FromDataValue(kitNode->config)).w, 1000.0);
}

// The nine-slice guides of the open element drag on its source: the borders found become borders set by hand
TEST_F(PipelineKitCardFixture, GuidesInThePopupDragTheNineSlice)
{
    OpenKit();
    Body()->OpenCell("kit-btn");
    Step(4);
    auto guides = Body()->GetPopup()->GetGuides();
    ASSERT_TRUE(guides && guides->HasImage());
    auto el = Body()->ElementOf("kit-btn");
    ASSERT_TRUE(el && el->source.Get("detected"));
    double left = el->source.Get("detected")->Get("left")->Num();
    RectF image = guides->GetImageRect();
    float k = image.Width()/160.0f;
    Vec2F from(image.left + (float)left*k, image.Center().y);
    Drag(Screen(from), Screen(from + Vec2F(10.0f*k, 0.0f)));
    Step(3);
    auto nine = UiDoc::NormalizeNine(Item("kit-btn").Get("nineSlice"));
    ASSERT_TRUE(nine.has_value());
    EXPECT_NEAR(nine->left, left + 10.0, 1.0);
    EXPECT_FALSE(Item("kit-btn").Get("nineFrom"));
}

// The Text style card: the fill a colour or a gradient (stops on a bar), the effects in rows; every edit writes the mirrors.
// The sample's height follows the card's width
TEST_F(PipelineKitCardFixture, TextStyleFillAndEffects)
{
    PipelineGraph graph;
    auto made = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    made->SetConfigString("font", "Lilita One");
    made->SetConfigNumber("size", 64);
    graph.nodes.Add(made);
    OpenGraph(graph);
    auto style = Node(made->id);
    auto card = editor->GetNodeWidget(style->id);
    editor->SetView(card->GetCardRect().Center(), 1.0f);
    Step(3);
    auto panel = [&](const char* name) { return editor->GetNodeWidget(style->id)->FindChildByTypeAndName<PipelineTextFxPanel>(name); };
    auto config = [&]() { return UiDoc::FromDataValue(style->config); };
    ASSERT_TRUE(panel("style fill") && panel("style effects"));

    panel("style fill")->Click("gradient");
    Step(3);
    ASSERT_TRUE(config().Get("fill")->IsObject());
    EXPECT_EQ(config().Get("fill")->Get("stops")->Count(), 2);
    EXPECT_EQ(config().Get("gradient")->ToJsString(), "#FF8C00") << "the mirror: the last stop";

    auto fill = panel("style fill");
    fill->PressBar(0.5f);
    fill->DragStopTo(0.6f);
    Step(2);
    auto now = config();
    auto stops = now.Get("fill")->Get("stops");
    ASSERT_EQ(stops->Count(), 3);
    EXPECT_EQ(stops->At(1)->Get("at")->Num(), 0.6);
    EXPECT_EQ(fill->GetSelectedStop(), 1);
    fill->Click("stopRemove");
    Step(2);
    EXPECT_EQ(config().Get("fill")->Get("stops")->Count(), 2);
    fill->Slide("angle", 0.0f);
    EXPECT_EQ(config().Get("fill")->Get("angle")->Num(), 0.0);

    panel("style effects")->AddEffect("stroke");
    Step(3);
    ASSERT_EQ(config().Get("effects")->Count(), 1);
    ASSERT_TRUE(config().Get("stroke")->IsObject()) << "the mirror: the first outline";
    panel("style effects")->Slide("fx0:width", 1.0f);
    EXPECT_EQ(config().Get("effects")->At(0)->Get("width")->Num(), 30.0);
    EXPECT_EQ(config().Get("stroke")->Get("width")->Num(), 30.0);
    panel("style effects")->AddEffect("shadow");
    panel("style effects")->AddEffect("stroke");
    Step(3);
    ASSERT_EQ(config().Get("effects")->Count(), 3);
    panel("style effects")->Click("fx2:up");
    EXPECT_EQ(config().Get("effects")->At(0)->Get("width")->Num(), UiDoc::DefaultEffect("stroke", UiDoc::TextStyleFromConfig(style->config)).width);
    panel("style effects")->Click("fx0:toggle");
    Step(2);
    EXPECT_EQ(config().Get("stroke")->Get("width")->Num(), 30.0) << "the first live outline";
    panel("style effects")->Click("fx1:remove");
    Step(3);
    EXPECT_EQ(config().Get("effects")->Count(), 2);
    EXPECT_TRUE(config().Get("shadow")->IsNull());

    for (auto kind : { PipelineThemeKind::Light, PipelineThemeKind::Dark })
    {
        PipelineTheme::SetKind(kind);
        Step(4);
        editor->SetView(editor->GetNodeWidget(style->id)->GetCardRect().Center(), 1.0f);
        editor->FinishBackgroundWork();
        Step(4);
        ShotCard(style, String("uicard_textstyle_fx") + (kind == PipelineThemeKind::Dark ? "_dark" : "_light"));
    }
    PipelineTheme::FollowEditor();
    Step(3);

    // A wider card shows the sample larger: its own shape across the body
    auto sample = editor->GetNodeWidget(style->id)->FindChildByTypeAndName<PipelineImageView>("style sample");
    ASSERT_TRUE(sample);
    float before = sample->layout->GetHeight();
    Resize(style, Vec2F(640, 0));
    sample = editor->GetNodeWidget(style->id)->FindChildByTypeAndName<PipelineImageView>("style sample");
    EXPECT_GE(sample->layout->GetHeight(), Math::Round((640.0f - 22.0f)*224.0f/680.0f) - 0.5f);
    EXPECT_GT(sample->layout->GetHeight(), before);
}
