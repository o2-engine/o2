#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "PipelinePairUiFixture.h"

// Input | result rows of the image-to-image cards and the compare view. Screenshots go to Work/Pipelines/shots/pairs/

// Every pair card shows the image feeding its input on the left and its result on the right; the compare setting puts
// both into one view at the same height
TEST_F(PipelinePairUiFixture, EveryPairCardShowsItsInputAndResultInBothViews)
{
    for (auto& type : pairTypes)
    {
        PipelinePairLayout::SetIoView(PipelineIoView::SideBySide);
        auto node = OpenFed(type);
        SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
        auto card = editor->GetNodeWidget(node->id);
        ASSERT_TRUE(card) << type.Data();
        auto body = card->GetBody();
        auto stage = card->FindChildByTypeAndName<PipelinePaintEditor>("draw stage");

        if (type == "imageEdit" || type == "imageExtract")
        {
            ASSERT_TRUE(stage) << type.Data();
            ASSERT_TRUE(stage->GetBackground()) << type.Data() << ": the input is the drawing stage";
            RectF stageArea = stage->GetStageRectangle();
            EXPECT_LT(stageArea.right, stage->layout->GetWorldRect().Center().x + 1.0f) << type.Data() << ": the stage takes the left pane";
            EXPECT_FALSE(stage->IsComparing());
        }
        else
        {
            auto pair = body->GetPair();
            ASSERT_TRUE(pair) << type.Data();
            EXPECT_TRUE(pair->GetInputView()->HasImage()) << type.Data();
            EXPECT_FALSE(pair->IsComparing()) << type.Data();
            RectF input = pair->GetInputView()->layout->GetWorldRect(), result = pair->GetResultWidget()->layout->GetWorldRect();
            EXPECT_NEAR(input.Width(), result.Width(), 0.5f) << type.Data();
            EXPECT_NEAR(result.left - input.right, PipelinePairLayout::paneGap, 0.5f) << type.Data();
            EXPECT_NEAR(input.Height(), PipelinePairLayout::resultHeight, 0.5f) << type.Data();

            auto capture = Capture();
            EXPECT_TRUE(IsNear(PixelAt(capture, input.Center()), inputColor)) << type.Data() << ": input pane";
            if (type == "removeBackground" || type == "aiRemoveBg" || type == "imageColor" || type == "imageGradient" ||
                type == "imageOutline" || type == "imageShadow")
            {
                EXPECT_TRUE(IsNear(PixelAt(capture, result.Center()), resultColor)) << type.Data() << ": result pane";
            }
        }

        float sideHeight = card->GetCardRect().Height();
        ShotCard(node, type + "_side");

        PipelinePairLayout::SetIoView(PipelineIoView::Compare);
        Step(3);
        card = editor->GetNodeWidget(node->id);
        body = card->GetBody();
        stage = card->FindChildByTypeAndName<PipelinePaintEditor>("draw stage");
        if (type == "imageEdit")
            EXPECT_TRUE(stage && stage->IsComparing());
        else if (type == "imageExtract")
            EXPECT_TRUE(stage && !stage->IsComparing()) << "one source, many parts: never compared";
        else
            EXPECT_TRUE(body->GetPair() && body->GetPair()->IsComparing()) << type.Data();

        EXPECT_NEAR(card->GetCardRect().Height(), sideHeight, 0.5f) << type.Data() << ": switching never resizes a card";
        ShotCard(node, type + "_compare");
    }
}

// The compare view needs both images and the whole result pane
TEST_F(PipelinePairUiFixture, CompareFallsBackToTwoPanes)
{
    PipelinePairLayout::SetIoView(PipelineIoView::Compare);
    auto node = OpenFed("aiRemoveBg");
    auto pair = [&]() { return editor->GetNodeWidget(node->id)->GetBody()->GetPair(); };
    EXPECT_FALSE(pair()->IsComparing()) << "no result yet";

    SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
    EXPECT_TRUE(pair()->IsComparing());

    // A crop frame being edited takes the result pane
    node->SetConfigBool("cropEnabled", true);
    editor->GetNodeWidget(node->id)->Rebuild();
    editor->GetNodeWidget(node->id)->UpdateFromNode();
    Step(2);
    EXPECT_FALSE(pair()->IsComparing());

    node->SetConfigBool("cropEnabled", false);
    editor->GetNodeWidget(node->id)->Rebuild();
    Step(2);
    EXPECT_TRUE(pair()->IsComparing());

    SetResult(node, nullptr);
    EXPECT_FALSE(pair()->IsComparing());

    auto alone = OpenFed("imageColor", 480.0f, false);
    SetResult(alone, PipelineImageOps::Blank(96, 64, resultColor));
    EXPECT_FALSE(editor->GetNodeWidget(alone->id)->GetBody()->GetPair()->IsComparing()) << "nothing feeds the input";

    PipelinePairLayout::SetIoView(PipelineIoView::SideBySide);
    Step(2);
    EXPECT_FALSE(editor->GetNodeWidget(alone->id)->GetBody()->GetPair()->IsComparing());
}

// Pressing in the view moves the divider there; each side is cut at it
TEST_F(PipelinePairUiFixture, DraggingTheDividerMovesTheCut)
{
    PipelinePairLayout::SetIoView(PipelineIoView::Compare);
    auto node = OpenFed("imageColor");
    SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
    auto pair = editor->GetNodeWidget(node->id)->GetBody()->GetPair();
    ASSERT_TRUE(pair->IsComparing());

    auto view = pair->GetCompareView();
    RectF box = view->layout->GetWorldRect();
    RectF image = PipelinePairDraw::FitRect(Vec2I(96, 64), box);
    Vec2F probe(image.left + image.Width()*0.4f, image.Center().y);

    auto capture = Capture();
    EXPECT_TRUE(IsNear(PixelAt(capture, probe), inputColor)) << "left of the middle divider: the input";

    Vec2F from = editor->LocalToScreenPoint(Vec2F(box.Center().x, box.Center().y));
    Vec2F to = editor->LocalToScreenPoint(Vec2F(box.left + box.Width()*0.25f, box.Center().y));
    Drag(from, to);
    EXPECT_NEAR(PipelinePairLayout::GetDivider(node->id), 0.25f, 0.02f);
    EXPECT_NEAR(view->GetDividerX(), box.left + box.Width()*0.25f, 1.5f);

    capture = Capture();
    EXPECT_TRUE(IsNear(PixelAt(capture, probe), resultColor)) << "now right of the divider: the result";
    EXPECT_TRUE(IsNear(PixelAt(capture, Vec2F(image.left + 4.0f, image.Center().y)), inputColor));
    ShotCard(node, "imageColor_compare_dragged");
}

// On the drawing stage only the divider strip moves the divider; a stroke starts on the input side only
TEST_F(PipelinePairUiFixture, ImageEditDrawsOnTheInputSideOfTheCompareView)
{
    PipelinePairLayout::SetIoView(PipelineIoView::Compare);
    auto node = OpenFed("imageEdit");
    SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
    auto stage = editor->GetNodeWidget(node->id)->FindChildByTypeAndName<PipelinePaintEditor>("draw stage");
    ASSERT_TRUE(stage && stage->IsComparing());

    RectF area = stage->GetStageRectangle();
    float divider = stage->GetDividerX();
    EXPECT_NEAR(divider, area.Center().x, 1.0f);

    // A stroke on the result side leaves no drawing
    Drag(editor->LocalToScreenPoint(Vec2F(divider + 30.0f, area.Center().y)), editor->LocalToScreenPoint(Vec2F(divider + 60.0f, area.Center().y - 10.0f)));
    EXPECT_TRUE(node->GetConfigString("drawing", "").IsEmpty()) << "the result side takes no strokes";
    EXPECT_NEAR(stage->GetDividerX(), divider, 0.5f);

    // The strip on the divider drags it
    Drag(editor->LocalToScreenPoint(Vec2F(divider + 2.0f, area.Center().y)), editor->LocalToScreenPoint(Vec2F(area.left + area.Width()*0.7f, area.Center().y)));
    EXPECT_NEAR(PipelinePairLayout::GetDivider(node->id), 0.7f, 0.03f);
    EXPECT_TRUE(node->GetConfigString("drawing", "").IsEmpty());

    // The input side draws
    float x = area.left + area.Width()*0.2f;
    Drag(editor->LocalToScreenPoint(Vec2F(x, area.Center().y)), editor->LocalToScreenPoint(Vec2F(x + 20.0f, area.Center().y + 10.0f)));
    EXPECT_FALSE(node->GetConfigString("drawing", "").IsEmpty()) << "the input side draws";
    ShotCard(node, "imageEdit_compare_drawn");
}

// Each part's output port sits on its cell's image corner, at any card width
TEST_F(PipelinePairUiFixture, ExtractPortsSitOnTheCellCorners)
{
    for (float width : { 480.0f, 312.0f })
    {
        PipelineGraph graph;
        auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
        node->size = Vec2F(width, 0.0f);
        Vector<PipelineExtractRegion> regions;
        for (int i = 0; i < 3; i++)
        {
            PipelineExtractRegion region;
            region.id = "part" + (String)i;
            region.name = "part " + (String)i;
            region.x = 0.3f*i; region.w = 0.3f;
            regions.Add(region);
        }
        PipelineRegions::Write(*node, regions);
        PipelineNodeRegistry::SyncNodeWithSchema(node);
        graph.nodes.Add(node);
        graph.SaveToAsset(*asset);
        editor->SetAsset(asset);
        Step(3);
        auto live = editor->GetGraph()->FindNode(node->id);
        auto card = editor->GetNodeWidget(live->id);
        editor->SetView(card->GetCardRect().Center(), 1.0f);
        Step(3);

        auto grid = card->FindChildByTypeAndName<Widget>("parts");
        ASSERT_TRUE(grid);
        auto cells = grid->GetChildWidgets();
        ASSERT_EQ(cells.Count(), 3);
        for (int i = 0; i < 3; i++)
        {
            auto image = cells[i]->FindChildByType<PipelineImageView>();
            ASSERT_TRUE(image);
            RectF imageRect = image->layout->GetWorldRect();
            Vec2F port = card->GetPortPosition(live->outputs[i].id, false);
            EXPECT_NEAR(port.x, imageRect.right, 1.0f) << width << " part " << i;
            EXPECT_NEAR(port.y, imageRect.bottom, 1.0f) << width << " part " << i;
        }

        ShotCard(live, "imageExtract_ports_" + (String)(int)width);
    }
}

// A new pair node gets room for two panes and an automatic height; a side edge keeps the height automatic
TEST_F(PipelinePairUiFixture, NewPairNodesStartWideWithAnAutomaticHeight)
{
    PipelineGraph graph;
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    Step(2);

    auto node = editor->AddNodeAtViewCenter("imageColor");
    ASSERT_TRUE(node);
    EXPECT_EQ(node->size, Vec2F(480.0f, 0.0f));
    auto card = editor->GetNodeWidget(node->id);
    editor->SetView(card->GetCardRect().Center(), 1.0f);
    Step(3);
    float autoHeight = card->GetAutoHeight();
    EXPECT_NEAR(card->GetCardRect().Height(), autoHeight, 0.5f);

    RectF rect = card->GetCardRect();
    Drag(editor->LocalToScreenPoint(Vec2F(rect.right, rect.Center().y)), editor->LocalToScreenPoint(Vec2F(rect.right - 60.0f, rect.Center().y)));
    EXPECT_NEAR(node->size.x, 420.0f, 2.0f);
    EXPECT_FLOAT_EQ(node->size.y, 0.0f) << "a side edge keeps the height automatic";

    rect = card->GetCardRect();
    Drag(editor->LocalToScreenPoint(Vec2F(rect.Center().x, rect.bottom)), editor->LocalToScreenPoint(Vec2F(rect.Center().x, rect.bottom - 80.0f)));
    EXPECT_GT(node->size.y, card->GetAutoHeight() + 40.0f) << "the bottom edge sets a height";
    auto pair = card->GetBody()->GetPair();
    EXPECT_GT(pair->layout->GetHeight(), PipelinePairLayout::emptyHeight + 40.0f) << "the extra height goes to the pair row";
}

// The divider is a hairline one screen pixel wide and its grip 5 x 18 screen pixels at any zoom; the grip is faint
// until the cursor is over it
TEST_F(PipelinePairUiFixture, TheDividerKeepsItsScreenSizeWhenZoomed)
{
    PipelinePairLayout::SetIoView(PipelineIoView::Compare);
    for (auto type : { "imageColor", "imageEdit" })
    {
        auto node = OpenFed(type);
        SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
        auto card = editor->GetNodeWidget(node->id);
        auto stage = card->FindChildByTypeAndName<PipelinePaintEditor>("draw stage");
        auto pair = card->GetBody()->GetPair();
        bool onStage = String(type) == "imageEdit";
        ASSERT_TRUE(onStage ? stage && stage->IsComparing() : pair && pair->IsComparing()) << type;

        RectF box = onStage ? stage->GetStageRectangle() : pair->GetCompareView()->layout->GetWorldRect();
        float divider = onStage ? stage->GetDividerX() : pair->GetCompareView()->GetDividerX();
        RectF image = onStage ? box : PipelinePairDraw::FitRect(Vec2I(96, 64), box);
        Vec2F center(divider, box.Center().y);

        for (float cameraScale : { 1.0f, 1.0f/3.0f, 3.0f })
        {
            editor->SetView(center, cameraScale);
            Vec2F corner = (Vec2F)o2Render.GetResolution()*0.5f - Vec2F(30.0f, 30.0f);
            o2Input.OnCursorMoved(corner, 0, false);
            Step(3);
            auto capture = Capture();
            float captureScale = (float)capture->GetSize().x/Math::Max(1, o2Render.GetResolution().x);
            String label = String(type) + " at camera scale " + (String)cameraScale;

            // Across the line, below the grip: the shade it adds is one screen pixel of the dark line
            Vec2I row = ScreenToCapture(editor->LocalToScreenPoint(Vec2F(divider, image.Center().y - image.Height()*0.35f)), capture);
            float shade = 0.0f;
            int reach = (int)(12*captureScale);
            for (int x = row.x - reach; x <= row.x + reach; x++)
                shade += Darkness(capture, x, row.y);
            float lineWidth = shade/captureScale;
            EXPECT_GT(lineWidth, 0.5f) << label.Data();
            EXPECT_LT(lineWidth, 2.0f) << label.Data() << ": the line never gets thicker when zoomed in";

            if (cameraScale > 1.0f)
            {
                ShotCard(node, String(type) + "_divider_far");
                continue;
            }

            // The grip, brought forward by the cursor: 5 px wide, 18 px tall, measured just right of the line
            Vec2F centerScreen = editor->LocalToScreenPoint(center);
            float restShade = Darkness(capture, ScreenToCapture(centerScreen + Vec2F(1.5f, 0.0f), capture).x,
                                        ScreenToCapture(centerScreen, capture).y);

            o2Input.OnCursorMoved(centerScreen, 0, false);
            Step(3);
            capture = Capture();
            Vec2I mid = ScreenToCapture(centerScreen, capture);
            Vec2I column = ScreenToCapture(centerScreen + Vec2F(1.5f, 0.0f), capture);
            EXPECT_GT(Darkness(capture, column.x, mid.y), restShade + 0.2f) << label.Data() << ": the grip comes forward under the cursor";

            int tall = 0, wide = 0, span = (int)(40*captureScale);
            for (int y = mid.y - span; y <= mid.y + span; y++)
                tall += Darkness(capture, column.x, y) > 0.45f ? 1 : 0;
            for (int x = mid.x - span; x <= mid.x + span; x++)
                wide += Darkness(capture, x, mid.y) > 0.45f ? 1 : 0;
            EXPECT_NEAR(tall/captureScale, PipelinePairDraw::gripHeight, 3.0f) << label.Data();
            EXPECT_NEAR(wide/captureScale, PipelinePairDraw::gripWidth, 2.0f) << label.Data();

            ShotCard(node, String(type) + (cameraScale < 1.0f ? "_divider_zoom3" : "_divider_zoom1"));
        }
    }
}


// A card resized after its layout switched still fits every image into its area. The web kept a drawing stage measured
// once, so after the result arrived and re-created it, a resize left the image small in the middle
TEST_F(PipelinePairUiFixture, ImagesFillTheirAreasAfterALayoutSwitchAndAResize)
{
    struct Case { String type; int parts; };
    Vector<Case> cases;
    for (auto& type : pairTypes)
        cases.Add({ type, 1 });
    cases.Add({ "imageExtract", 3 });

    auto result = PipelineImageOps::Blank(96, 64, resultColor);
    const Vector<String> scenarios = { "loaded with a result", "result arrived", "crop on and off", "setting flipped" };
    const Vector<Vec2F> sizes = { Vec2F(900.0f, 800.0f), Vec2F(620.0f, 1000.0f) };
    int checks = 0;

    for (auto& c : cases)
    {
        for (auto view : { PipelineIoView::SideBySide, PipelineIoView::Compare })
        {
            for (auto& scenario : scenarios)
            {
                if (scenario == "crop on and off" && c.type != "aiRemoveBg" && c.type != "imageEdit")
                    continue;

                auto other = view == PipelineIoView::Compare ? PipelineIoView::SideBySide : PipelineIoView::Compare;
                PipelinePairLayout::SetIoView(scenario == "setting flipped" ? other : view);
                bool loaded = scenario == "loaded with a result";
                auto node = OpenFed(c.type, 480.0f, true, loaded ? result : nullptr, c.parts);
                if (!loaded)
                    SetResult(node, result);

                if (scenario == "crop on and off")
                {
                    for (int i = 0; i < 2; i++)
                    {
                        auto toggle = FindToggle(editor->GetNodeWidget(node->id), "Crop");
                        ASSERT_TRUE(toggle) << c.type.Data();
                        toggle->onToggleByUser(true);
                        Step(3);
                    }
                    EXPECT_FALSE(node->GetConfigBool("cropEnabled", false));
                }

                if (scenario == "setting flipped")
                {
                    PipelinePairLayout::SetIoView(view);
                    Step(3);
                }

                auto rowOf = [&]() -> Ref<Widget>
                {
                    auto card = editor->GetNodeWidget(node->id);
                    if (auto stage = card->FindChildByTypeAndName<PipelinePaintEditor>("draw stage"))
                        return stage;
                    return card->GetBody()->GetPair();
                };
                ASSERT_TRUE(rowOf()) << c.type.Data();
                float rowBefore = rowOf()->layout->GetHeight();
                bool compare = PipelinePairLayout::ShowsCompare(view, c.type, true, true, false);

                for (auto& size : sizes)
                {
                    Resize(node, size);
                    auto card = editor->GetNodeWidget(node->id);
                    String label = c.type + (c.parts > 1 ? " with parts" : "") + (compare ? ", compare" : ", side by side") + ", " +
                        scenario + ", " + (String)(int)size.x + "x" + (String)(int)size.y;
                    float rowWidth = card->GetCardRect().Width() - 20.0f;
                    auto row = rowOf();
                    EXPECT_NEAR(row->layout->GetWidth(), rowWidth, 2.0f) << label.Data() << ": the row spans the card";
                    EXPECT_GT(row->layout->GetHeight(), rowBefore + 50.0f) << label.Data() << ": the row takes the added height";

                    if (scenario == "result arrived" && size.x > 800.0f)
                        ShotCard(node, "resized_" + c.type + (c.parts > 1 ? "_parts" : "") + (compare ? "_compare" : "_side"));

                    auto capture = Capture();
                    ASSERT_TRUE(capture);
                    auto expectFill = [&](const RectF& area, const Vector<Color4>& colors, const char* what)
                    {
                        EXPECT_GE(FillRatio(capture, area, colors), 0.95f) << label.Data() << ": " << what;
                        checks++;
                    };

                    if (auto stage = DynamicCast<PipelinePaintEditor>(row))
                    {
                        EXPECT_EQ(stage->IsComparing(), compare) << label.Data();
                        RectF area = stage->layout->GetWorldRect();
                        if (stage->IsToolbarBelow())
                            area.bottom += stage->GetBarsHeight(area.Width());
                        else
                            area.top -= stage->GetBarsHeight(area.Width());
                        if (!compare)
                            area.right = area.left + PipelinePairLayout::PaneWidth(area.Width());
                        // The extract stage keeps room round the image for the region handles and shades the source outside
                        // the selected part
                        if (c.type == "imageExtract")
                        {
                            area = RectF(area.left + 18.0f, area.top - 18.0f, area.right - 18.0f, area.bottom + 18.0f);
                            Color4 shaded(inputColor.r*127/255, inputColor.g*127/255, inputColor.b*127/255, 255);
                            expectFill(area, { inputColor, shaded }, "source stage");
                        }
                        else if (compare)
                            expectFill(area, { inputColor, resultColor }, "drawing stage split by the divider");
                        else
                            expectFill(area, { inputColor }, "drawing stage");

                        if (c.type == "imageEdit" && !compare)
                        {
                            auto crop = card->FindChildByType<PipelineCropEditor>();
                            ASSERT_TRUE(crop && crop->IsEnabledInHierarchy()) << label.Data();
                            expectFill(crop->layout->GetWorldRect(), { resultColor }, "result beside the stage");
                        }

                        if (c.type == "imageExtract")
                        {
                            auto grid = card->FindChildByTypeAndName<Widget>("parts");
                            ASSERT_TRUE(grid) << label.Data();
                            auto cells = grid->GetChildWidgets();
                            EXPECT_EQ(cells.Count(), c.parts) << label.Data();
                            for (auto& cell : cells)
                            {
                                auto image = cell->FindChildByType<PipelineImageView>();
                                ASSERT_TRUE(image) << label.Data();
                                expectFill(image->layout->GetWorldRect(), { resultColor }, "part cell");
                            }
                        }
                        continue;
                    }

                    auto pair = DynamicCast<PipelineIoPair>(row);
                    ASSERT_TRUE(pair) << label.Data();
                    EXPECT_EQ(pair->IsComparing(), compare) << label.Data();
                    if (compare)
                    {
                        RectF box = pair->GetCompareView()->layout->GetWorldRect();
                        expectFill(RectF(box.left + 1, box.top - 1, box.right - 1, box.bottom + 1), { inputColor, resultColor }, "compare view");
                    }
                    else
                    {
                        expectFill(pair->GetInputView()->layout->GetWorldRect(), { inputColor }, "input pane");
                        expectFill(pair->GetResultWidget()->layout->GetWorldRect(), { resultColor }, "result pane");
                    }
                }
            }
        }
    }
    EXPECT_GT(checks, 100);
}

// The region tool of the image edit card creates the frame, moves it with the handles and clears it with the trash tab;
// the frame shades what stays as it is, over both halves of the compare view
TEST_F(PipelinePairUiFixture, ImageEditRegionIsCreatedMovedAndCleared)
{
    auto click = [&](const Ref<Widget>& widget)
    {
        Vec2F point = editor->LocalToScreenPoint(widget->layout->GetWorldRect().Center());
        Drag(point, point, 1);
    };

    for (auto view : { PipelineIoView::SideBySide, PipelineIoView::Compare })
    {
        bool compare = view == PipelineIoView::Compare;
        const char* mode = compare ? "compare" : "side by side";
        PipelinePairLayout::SetIoView(view);
        auto node = OpenFed("imageEdit");
        SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
        auto stage = [&]() { return editor->GetNodeWidget(node->id)->FindChildByTypeAndName<PipelinePaintEditor>("draw stage"); };
        // A frame keeps the background, so the card offers the transparent switch only without one
        auto hasSwitch = [&]() { return editor->GetNodeWidget(node->id)->FindChildByTypeAndName<Toggle>("transparent switch") != nullptr; };
        ASSERT_TRUE(stage()) << mode;
        EXPECT_EQ(stage()->IsComparing(), compare) << mode;
        EXPECT_FALSE(stage()->HasRegion()) << mode;
        EXPECT_TRUE(hasSwitch()) << mode;

        // The tool is always there; picking it makes the centred frame
        auto tool = stage()->FindChildByTypeAndName<Toggle>("region");
        ASSERT_TRUE(tool && tool->IsEnabledInHierarchy()) << mode;
        click(tool);
        Step(3);
        PipelineImageOps::CropRect region;
        ASSERT_TRUE(PipelineEditRegion::Of(*node, region)) << mode;
        EXPECT_FLOAT_EQ(region.x, 0.25f);
        EXPECT_FLOAT_EQ(region.y, 0.25f);
        EXPECT_FLOAT_EQ(region.w, 0.5f);
        EXPECT_FLOAT_EQ(region.h, 0.5f);
        EXPECT_EQ(node->GetConfigString("drawTool", ""), "roi") << mode;
        EXPECT_EQ(stage()->GetTool(), "roi") << mode;
        EXPECT_FALSE(hasSwitch()) << mode;

        // The parameters trade the transparency rows for the hint
        auto card = editor->GetNodeWidget(node->id);
        card->SetParamsOpen(true);
        Step(3);
        card = editor->GetNodeWidget(node->id);
        EXPECT_TRUE(card->FindChildByTypeAndName<Label>("edit region hint")) << mode;
        EXPECT_FALSE(card->FindChildByTypeAndName<Label>("native transparency")) << mode;

        // Dragging inside the frame moves it; comparing, left of the divider, which lies under the frame's middle
        RectF area = stage()->GetStageRectangle();
        RectF box = stage()->GetRegionRectangle();
        Vec2F from(box.left + box.Width()*0.25f, box.Center().y);
        Drag(editor->LocalToScreenPoint(from), editor->LocalToScreenPoint(from + Vec2F(-20.0f, 10.0f)));
        ASSERT_TRUE(PipelineEditRegion::Of(*node, region)) << mode;
        EXPECT_NEAR(region.x, 0.25f - 20.0f/area.Width(), 0.01f) << mode;
        EXPECT_NEAR(region.y, 0.25f - 10.0f/area.Height(), 0.01f) << mode;
        EXPECT_NEAR(region.w, 0.5f, 0.01f) << mode;

        // Outside the frame both halves are shaded; inside, the images show as they are
        box = stage()->GetRegionRectangle();
        auto capture = Capture();
        Vec2F outsideLeft(area.left + 3.0f, area.Center().y), outsideRight(area.right - 3.0f, area.Center().y);
        Color4 dimInput(inputColor.r/2, inputColor.g/2, inputColor.b/2, 255), dimResult(resultColor.r/2, resultColor.g/2, resultColor.b/2, 255);
        EXPECT_TRUE(IsNear(PixelAt(capture, outsideLeft), dimInput, 20)) << mode;
        float insideY = box.Center().y + box.Height()*0.25f;
        EXPECT_TRUE(IsNear(PixelAt(capture, Vec2F(box.left + 6.0f, insideY)), inputColor, 20)) << mode;
        if (compare)
        {
            EXPECT_TRUE(IsNear(PixelAt(capture, outsideRight), dimResult, 20)) << "the shade covers the result half too";
            EXPECT_TRUE(IsNear(PixelAt(capture, Vec2F(box.right - 6.0f, insideY)), resultColor, 20));
        }
        ShotCard(node, String("imageEdit_region_") + (compare ? "compare" : "side"));

        // The trash tab clears the frame and brings the brush back
        auto trash = stage()->FindChildByTypeAndName<Button>("remove region");
        ASSERT_TRUE(trash && trash->IsEnabledInHierarchy()) << mode;
        click(trash);
        Step(3);
        EXPECT_FALSE(PipelineEditRegion::Of(*node, region)) << mode;
        EXPECT_EQ(node->GetConfigString("drawTool", ""), "brush") << mode;
        EXPECT_FALSE(stage()->HasRegion()) << mode;
        EXPECT_TRUE(hasSwitch()) << mode;
    }
}
