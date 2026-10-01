#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "PipelinePairUiFixture.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Pipeline/PipelineValue.h"

// The image cards' rows: the transparent switch beside Crop, the parameters starting with the model and the seed, no
// captions under the pictures, and a hand-sized card whose picture area stays put when its content changes

namespace
{
    // Returns the text of the first label in the widget, or the caption of its first toggle
    String RowCaption(const Ref<Widget>& widget)
    {
        if (auto label = DynamicCast<Label>(widget))
            return label->GetText();

        Ref<Toggle> firstToggle = DynamicCast<Toggle>(widget);
        for (auto& child : widget->GetChildWidgets())
        {
            String text = RowCaption(child);
            if (!text.IsEmpty() && !DynamicCast<Toggle>(child))
                return text;
            if (!firstToggle)
                firstToggle = DynamicCast<Toggle>(child);
        }
        return firstToggle ? String(firstToggle->GetCaption()) : String();
    }

    bool HasLabel(const Ref<Widget>& widget, const String& text)
    {
        if (auto label = DynamicCast<Label>(widget))
        {
            if (String(label->GetText()) == text)
                return true;
        }
        return widget->GetChildWidgets().Any([&](const Ref<Widget>& child) { return HasLabel(child, text); });
    }

    struct PipelinePolishUiFixture : PipelinePairUiFixture
    {
        Ref<PipelineNodeWidget> Card(const Ref<PipelineNode>& node) { return editor->GetNodeWidget(node->id); }

        // The row that holds the node's pictures
        Ref<Widget> PictureArea(const Ref<PipelineNode>& node)
        {
            auto card = Card(node);
            if (auto stage = card->FindChildByTypeAndName<PipelinePaintEditor>("draw stage"))
                return stage;
            if (auto pair = card->GetBody()->GetPair())
                return pair;
            return card->FindChildByType<PipelineCropEditor>();
        }

        // Presses a toggle the way a click does
        void Press(const Ref<Toggle>& toggle)
        {
            ASSERT_TRUE(toggle);
            toggle->SetValue(!toggle->GetValue());
            toggle->onToggleByUser(toggle->GetValue());
            Step(3);
        }

        // Captions of the rows of the open parameter list, top to bottom
        Vector<String> ParamRows(const Ref<PipelineNode>& node)
        {
            Vector<String> rows;
            if (auto list = Card(node)->FindChildByTypeAndName<Widget>("params list"))
            {
                for (auto& row : list->GetChildWidgets())
                    rows.Add(RowCaption(row));
            }
            return rows;
        }

        String ParamsHeader(const Ref<PipelineNode>& node)
        {
            auto head = Card(node)->FindChildByTypeAndName<Button>("params");
            auto text = head ? head->GetLayerDrawable<Text>("caption") : nullptr;
            return text ? String(text->GetText()) : String();
        }

        // A press and a release at a screen point, seen by the widgets that poll the input too
        void Click(const Vec2F& screenPoint)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(350));
            o2Input.OnCursorMoved(screenPoint, 0, false);
            o2Input.OnCursorPressed(screenPoint);
            o2Input.PreUpdate();
            Step();
            o2Input.OnCursorReleased();
            Step(2);
        }

        // A press and a release the way the event system alone sees them, as a click inside a menu
        void Press(const Vec2F& screenPoint)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(350));
            o2Input.OnCursorMoved(screenPoint, 0, false);
            o2Input.OnCursorPressed(screenPoint);
            Step();
            o2Input.OnCursorReleased();
            Step(2);
        }

        void ClickWidget(const Ref<Widget>& widget) { Click(editor->LocalToScreenPoint(widget->layout->GetWorldRect().Center())); }

        void Key(KeyboardKey key)
        {
            o2Input.OnKeyPressed(key);
            Step();
            o2Input.OnKeyReleased(key);
            Step(2);
        }

        Ref<Toggle> TransparentSwitch(const Ref<PipelineNode>& node)
        {
            return Card(node)->FindChildByTypeAndName<Toggle>("transparent switch");
        }

        // Opens a card of the type with a result, sized by hand taller than its content
        Ref<PipelineNode> OpenSized(const String& type, float width = 480.0f)
        {
            auto node = OpenFed(type, width, type != "nanoBananaGen");
            SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
            auto card = Card(node);
            node->size = Vec2F(width, card->GetAutoHeight() + 100.0f);
            card->UpdateFromNode();
            Step(3);
            return node;
        }
    };
}

// Folding the parameters, the transparent switch and the transparency method change the card's height by exactly what
// they add or remove, so the pictures of a hand-sized card keep their size
TEST_F(PipelinePolishUiFixture, HandSizedPicturesStayWhenTheContentChanges)
{
    for (String type : { "imageEdit", "nanoBananaGen", "aiRemoveBg", "imageExtract" })
    {
        auto node = OpenSized(type);
        auto height = [&]() { return PictureArea(node)->layout->GetHeight(); };
        float start = height();
        auto expectSame = [&](const char* step)
        {
            EXPECT_NEAR(height(), start, 0.5f) << type.Data() << ": " << step;
        };

        Card(node)->SetParamsOpen(true);
        Step(3);
        expectSame("params opened");

        if (type == "aiRemoveBg")
        {
            Press(FindToggle(Card(node), "Chroma key x1"));
            expectSame("chroma key rows added");
            Press(FindToggle(Card(node), "White / black x2"));
            expectSame("chroma key rows removed");
        }
        else
        {
            Press(TransparentSwitch(node));
            ASSERT_TRUE(node->GetConfigBool("transparentBg", false)) << type.Data();
            expectSame("transparency on");
            Press(FindToggle(Card(node), "Chroma key x1"));
            expectSame("chroma key rows added");
            Press(TransparentSwitch(node));
            ASSERT_FALSE(node->GetConfigBool("transparentBg", false)) << type.Data();
            expectSame("transparency off");
        }

        Card(node)->SetParamsOpen(false);
        Step(3);
        expectSame("params closed");
    }
}

// Image gen and image edit put the switch beside Crop under the pictures; the captions there are gone; remove background
// keeps Crop alone; the extract card puts it at the end of its actions, or under them when the card is narrow
TEST_F(PipelinePolishUiFixture, TheTransparentSwitchSitsBesideCrop)
{
    const Vector<String> goneCaptions = { "Draw over", "Result", "Result (transparent)", "Frame: only it changes",
        "Prompt optional - references on the inputs (+)" };

    for (String type : { "imageEdit", "nanoBananaGen", "aiRemoveBg" })
    {
        auto node = OpenFed(type, 480.0f, type != "nanoBananaGen");
        SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
        auto card = Card(node);
        for (auto& caption : goneCaptions)
            EXPECT_FALSE(HasLabel(card, caption)) << type.Data() << ": " << caption.Data();

        auto crop = card->FindChildByTypeAndName<Toggle>("crop");
        ASSERT_TRUE(crop) << type.Data();
        RectF cropRect = crop->layout->GetWorldRect(), pictures = PictureArea(node)->layout->GetWorldRect();
        EXPECT_NEAR(cropRect.right, pictures.right, 1.0f) << type.Data() << ": right-aligned under the pictures";
        if (auto stage = DynamicCast<PipelinePaintEditor>(PictureArea(node)))
            EXPECT_LE(cropRect.top, stage->GetStageRectangle().bottom + 0.5f) << "under the drawing stage, in the tool row";
        else
        {
            EXPECT_LT(pictures.bottom - cropRect.top, 8.0f) << type.Data() << ": right under the pictures";
            EXPECT_GE(pictures.bottom - cropRect.top, 0.0f) << type.Data();
        }

        auto toggle = TransparentSwitch(node);
        if (type == "aiRemoveBg")
        {
            EXPECT_FALSE(toggle) << "always transparent: no switch";
            continue;
        }

        ASSERT_TRUE(toggle) << type.Data();
        EXPECT_EQ(String(toggle->GetCaption()), "Transparent bg");
        RectF toggleRect = toggle->layout->GetWorldRect();
        EXPECT_NEAR(toggleRect.Center().y, cropRect.Center().y, 1.0f) << type.Data() << ": one row with Crop";
        EXPECT_LE(toggleRect.right, cropRect.left + 0.5f) << type.Data() << ": left of Crop";
        EXPECT_LT(cropRect.left - toggleRect.right, 10.0f) << type.Data() << ": next to it";

        // The parameters keep only the method rows, shown while the switch is on
        Card(node)->SetParamsOpen(true);
        Step(3);
        auto list = Card(node)->FindChildByTypeAndName<Widget>("params list");
        ASSERT_TRUE(list) << type.Data();
        EXPECT_FALSE(FindToggle(list, "Transparent bg") || FindToggle(list, "Transparent background")) << type.Data();
        EXPECT_FALSE(Card(node)->FindChildByTypeAndName<Widget>("transparentMode options")) << type.Data();
        Press(TransparentSwitch(node));
        EXPECT_TRUE(node->GetConfigBool("transparentBg", false)) << type.Data();
        EXPECT_TRUE(Card(node)->FindChildByTypeAndName<Widget>("transparentMode options")) << type.Data();
        ShotCard(node, "polish_" + type);
    }

    for (float width : { 480.0f, 312.0f })
    {
        auto node = OpenFed("imageExtract", width, true, nullptr, 3);
        auto card = Card(node);
        auto add = card->FindChildByTypeAndName<Button>("add part");
        auto toggle = TransparentSwitch(node);
        ASSERT_TRUE(add && toggle) << width;
        RectF addRect = add->layout->GetWorldRect(), toggleRect = toggle->layout->GetWorldRect();
        RectF row = card->FindChildByTypeAndName<Widget>("actions")->layout->GetWorldRect();
        EXPECT_NEAR(toggleRect.right, row.right, 1.0f) << width << ": at the right";
        if (width >= 400.0f)
            EXPECT_NEAR(toggleRect.Center().y, addRect.Center().y, 1.0f) << "in the actions row";
        else
            EXPECT_LE(toggleRect.top, addRect.bottom + 0.5f) << "on its own line under the actions";

        float before = PictureArea(node)->layout->GetHeight();
        Press(toggle);
        EXPECT_TRUE(node->GetConfigBool("transparentBg", false)) << width;
        EXPECT_NEAR(PictureArea(node)->layout->GetHeight(), before, 0.5f) << width;
        ShotCard(node, "polish_imageExtract_" + (String)(int)width);
    }
}

// Every parameter list starts with the model (with the seed next on the image cards); its header follows the same order
TEST_F(PipelinePolishUiFixture, ParametersStartWithTheModel)
{
    struct Case { String type; bool input; int parts; Vector<String> rows; String header; };
    const Vector<Case> cases = {
        { "nanoBananaGen", false, 1, { "Model", "Inherit seed", "Transparent bg mode", "White / black x2" }, "Parameters · Model · Seed · Transparency" },
        { "imageEdit", true, 1, { "Model", "Inherit seed", "Transparent bg mode", "White / black x2" }, "Parameters · Model · Seed · Transparency" },
        { "aiRemoveBg", true, 1, { "Model", "Inherit seed", "Transparent bg mode", "White / black x2" },
          "Parameters · Model · Seed · Transparent bg mode" },
        { "imageExtract", true, 3, { "Model", "Inherit seed", "Settings for", "Transparent bg mode", "White / black x2" },
          "Parameters · Model · Seed · Transparency" },
        { "videoGen", false, 1, { "Model", "Aspect", "Duration" }, "Parameters · Model · Aspect · Duration · Solid background colour" },
        { "sfxGen", false, 1, { "Model", "Seamless loop", "Length", "Influence" }, "Parameters · Model · Seamless loop · Length · Influence" },
        { "ttsSpeech", false, 1, { "Gemini TTS", "Model", "Voice" }, "Parameters · Provider · Model · Voice" },
        { "musicGen", false, 1, { "Lyria 3 clip", "Instrumental only (no vocals)" }, "Parameters · Model · Instrumental only" },
        { "promptGen", false, 1, { "Model", "Target" }, "Parameters · Model · Target · Max chars · System prompt" },
        { "aiText", false, 1, { "Model" }, "Parameters · Model · System prompt" }
    };

    for (auto& c : cases)
    {
        auto node = OpenFed(c.type, 480.0f, c.input, nullptr, c.parts);
        node->SetConfigBool("transparentBg", true);
        Card(node)->SetParamsOpen(true);
        Step(3);

        auto rows = ParamRows(node);
        ASSERT_GE(rows.Count(), c.rows.Count()) << c.type.Data();
        for (int i = 0; i < c.rows.Count(); i++)
            EXPECT_EQ(rows[i], c.rows[i]) << c.type.Data() << " row " << i;
        EXPECT_EQ(ParamsHeader(node), c.header) << c.type.Data();

        // The system prompt field spans the list and wraps its text inside the field
        if (c.type == "promptGen" || c.type == "aiText")
        {
            auto list = Card(node)->FindChildByTypeAndName<Widget>("params list");
            auto field = list ? DynamicCast<EditBox>(list->GetChildWidgets().Last()) : nullptr;
            ASSERT_TRUE(field) << c.type.Data();
            EXPECT_NEAR(field->layout->GetWorldRect().Width(), list->layout->GetWorldRect().Width(), 1.0f) << c.type.Data();
            EXPECT_GE(field->layout->GetWorldRect().Width(), 400.0f) << c.type.Data();
            EXPECT_TRUE(field->IsMultiLine() && field->IsWordWrap()) << c.type.Data();
        }
    }
}

// The image edit card has its drawing tools under the pictures: one line with the switches at its right end from a card
// width of 560, else the switches on a line of their own under the tools; the extract and draw cards keep them on top
TEST_F(PipelinePolishUiFixture, ImageEditToolsSitUnderThePictures)
{
    for (auto view : { PipelineIoView::SideBySide, PipelineIoView::Compare })
    {
        for (float width : { 480.0f, 560.0f, 760.0f })
        {
            PipelinePairLayout::SetIoView(view);
            auto node = OpenFed("imageEdit", width);
            SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
            String label = String(view == PipelineIoView::Compare ? "compare " : "side ") + (String)(int)width;

            auto stage = Card(node)->FindChildByTypeAndName<PipelinePaintEditor>("draw stage");
            ASSERT_TRUE(stage && stage->IsToolbarBelow()) << label.Data();
            EXPECT_EQ(stage->IsComparing(), view == PipelineIoView::Compare) << label.Data();
            auto toolbar = stage->FindChildByTypeAndName<Widget>("toolbar");
            ASSERT_TRUE(toolbar) << label.Data();
            EXPECT_TRUE(toolbar->FindChildByTypeAndName<Toggle>("region")) << label.Data() << ": the frame tool moved with the tools";
            auto clear = toolbar->FindChildByTypeAndName<Button>("clear");
            auto toggle = TransparentSwitch(node);
            auto crop = Card(node)->FindChildByTypeAndName<Toggle>("crop");
            ASSERT_TRUE(clear && toggle && crop) << label.Data();

            RectF tools = toolbar->layout->GetWorldRect(), widget = stage->layout->GetWorldRect();
            RectF switchRect = toggle->layout->GetWorldRect(), cropRect = crop->layout->GetWorldRect();
            EXPECT_LE(tools.top, stage->GetStageRectangle().bottom - 5.5f) << label.Data() << ": under the pictures, past the gap";
            EXPECT_GE(tools.bottom, widget.bottom - 0.5f) << label.Data();
            EXPECT_NEAR(cropRect.right, widget.right, 1.0f) << label.Data() << ": the switches end the row";
            EXPECT_NEAR(cropRect.Center().y, switchRect.Center().y, 1.0f) << label.Data();

            if (width >= 560.0f)
            {
                EXPECT_NEAR(tools.Height(), 22.0f, 0.5f) << label.Data() << ": one line, nothing wraps";
                EXPECT_NEAR(switchRect.Center().y, tools.Center().y, 1.5f) << label.Data() << ": the switches share the tool line";
                EXPECT_GE(switchRect.left, tools.right + 7.5f) << label.Data();
                EXPECT_LE(clear->layout->GetWorldRect().right, switchRect.left) << label.Data();
            }
            else
                EXPECT_LE(switchRect.top, tools.bottom + 0.5f) << label.Data() << ": the switches on their own line under the tools";

            ShotCard(node, "toolbar_imageEdit_" + String(view == PipelineIoView::Compare ? "compare_" : "side_") + (String)(int)width);
        }
    }

    auto draw = OpenFed("drawImage", 480.0f, false);
    auto drawStage = Card(draw)->FindChildByType<PipelinePaintEditor>();
    ASSERT_TRUE(drawStage);
    EXPECT_FALSE(drawStage->IsToolbarBelow());
    auto toolbar = drawStage->FindChildByTypeAndName<Widget>("toolbar");
    ASSERT_TRUE(toolbar && toolbar->IsEnabledInHierarchy());
    EXPECT_NEAR(toolbar->layout->GetWorldRect().top, drawStage->layout->GetWorldRect().top, 0.5f) << "the draw node keeps its tools on top";
}

// The prompt writer's Target is a menu of the targets, each with the icon of the node it writes for and a hint under it
TEST_F(PipelinePolishUiFixture, PromptTargetIsAMenuWithIcons)
{
    const Vector<Pair<String, String>> targets = { { "image", "nanoBananaGen" }, { "video", "videoGen" }, { "text", "aiText" },
        { "sfx", "sfxGen" }, { "music", "musicGen" }, { "speech", "ttsSpeech" } };

    auto node = OpenFed("promptGen", 480.0f, false);
    Card(node)->SetParamsOpen(true);
    Step(3);
    auto field = [&]() { return Card(node)->FindChildByTypeAndName<Button>("target"); };
    auto caption = [&]() { return String(field()->GetLayerDrawable<Text>("caption")->GetText()); };
    ASSERT_TRUE(field());
    EXPECT_EQ(caption(), "Image prompt");
    EXPECT_EQ(field()->GetLayerDrawable<Sprite>("icon")->GetImageName(), PipelineNodeWidget::IconForType("nanoBananaGen"));

    ClickWidget(field());
    auto picker = editor->GetOptionPicker();
    ASSERT_TRUE(picker && picker->IsOpen());
    auto& rows = picker->GetRows();
    ASSERT_EQ(rows.Count(), targets.Count());
    for (int i = 0; i < rows.Count(); i++)
    {
        EXPECT_EQ(rows[i]->name, targets[i].first);
        EXPECT_EQ(rows[i]->GetLayerDrawable<Sprite>("icon")->GetImageName(), PipelineNodeWidget::IconForType(targets[i].second)) << i;
        EXPECT_FALSE(String(rows[i]->GetLayerDrawable<Text>("hint")->GetText()).IsEmpty()) << i;
        EXPECT_FLOAT_EQ(rows[i]->FindLayer("check")->transparency, i == 0 ? 1.0f : 0.0f) << "the current target is checked";
    }
    EXPECT_EQ(picker->GetHighlight(), 0) << "opens on the current target";
    RectF menu = picker->layout->GetWorldRect();
    EXPECT_GE(menu.Width(), 300.0f);
    EXPECT_LE(menu.Width(), 420.0f);

    auto capture = Capture();
    ASSERT_TRUE(capture);
    Vec2I lt = ScreenToCapture(Vec2F(menu.left - 8, menu.top + 40), capture), rb = ScreenToCapture(Vec2F(menu.right + 8, menu.bottom - 8), capture);
    lt = Vec2I(Math::Max(0, lt.x), Math::Max(0, lt.y));
    rb = Vec2I(Math::Min(capture->GetSize().x, rb.x), Math::Min(capture->GetSize().y, rb.y));
    String dir = (getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots")) + "/pairs";
    o2FileSystem.FolderCreate(dir, true);
    PipelineImageOps::CropPixels(*capture, lt.x, lt.y, rb.x - lt.x, rb.y - lt.y)->Save(dir + "/prompt_target_menu.png", Bitmap::ImageType::Png);

    // The arrows go through the menu's own key handler; sent as keys they would also reach the scene move tool
    picker->MoveHighlight(1);
    picker->MoveHighlight(1);
    picker->MoveHighlight(1);
    EXPECT_EQ(picker->GetHighlightedValue(), "sfx");
    Key(VK_RETURN);
    EXPECT_FALSE(picker->IsOpen());
    EXPECT_EQ(node->GetConfigString("target", ""), "sfx");
    EXPECT_EQ(caption(), "Sound effect prompt");
    EXPECT_TRUE(HasLabel(Card(node), "Max chars")) << "the budget row follows the target";

    // A click on a row picks it
    ClickWidget(field());
    ASSERT_TRUE(picker->IsOpen());
    EXPECT_EQ(picker->GetHighlightedValue(), "sfx");
    Press(picker->FindRow("music")->layout->GetWorldRect().Center());
    EXPECT_FALSE(picker->IsOpen());
    EXPECT_EQ(node->GetConfigString("target", ""), "music");

    // Escape, a click outside and a click on the field close it without a pick
    ClickWidget(field());
    ASSERT_TRUE(picker->IsOpen());
    Key(VK_ESCAPE);
    EXPECT_FALSE(picker->IsOpen());

    ClickWidget(field());
    ASSERT_TRUE(picker->IsOpen());
    RectF card = Card(node)->GetCardRect();
    Click(editor->LocalToScreenPoint(Vec2F(card.right + 60.0f, card.top - 10.0f)));
    EXPECT_FALSE(picker->IsOpen());

    ClickWidget(field());
    ASSERT_TRUE(picker->IsOpen());
    ClickWidget(field());
    EXPECT_FALSE(picker->IsOpen()) << "a click on the field closes the open menu and does not open it again";
    EXPECT_EQ(node->GetConfigString("target", ""), "music");
}

// AI extract part draws nothing: no tool row, no drawing shown, its boxes editable without picking a tool, a click on
// another box selects that part, and the part ports stay on the cells of the row that moved up
TEST_F(PipelinePolishUiFixture, TheExtractHasNoDrawing)
{
    auto node = OpenFed("imageExtract", 480.0f, true, nullptr, 3);
    auto overlay = PipelineImageOps::Blank(96, 64, Color4(0, 200, 0, 255));
    node->SetConfigString("drawing", PipelineUtils::BytesToDataUrl(EncodeBitmapPng(*overlay), "image/png"));
    node->SetConfigNumber("dw", 96);
    node->SetConfigNumber("dh", 64);
    node->SetConfigString("drawTool", "brush");
    Card(node)->Rebuild();
    Card(node)->UpdateFromNode();
    Step(3);

    auto stage = Card(node)->FindChildByTypeAndName<PipelinePaintEditor>("draw stage");
    ASSERT_TRUE(stage);
    EXPECT_FALSE(stage->IsDrawingEnabled());
    EXPECT_FLOAT_EQ(stage->GetBarsHeight(stage->layout->GetWidth()), 0.0f) << "no tool row";
    auto toolbar = stage->FindChildByTypeAndName<Widget>("toolbar");
    EXPECT_TRUE(!toolbar || !toolbar->IsEnabledInHierarchy());
    EXPECT_EQ(stage->GetTool(), "roi") << "a stored tool is ignored";

    // The source shows through where the old drawing would be
    RectF area = stage->GetStageRectangle();
    auto capture = Capture();
    Color4 middle = PixelAt(capture, Vec2F(area.left + area.Width()*0.15f, area.bottom + area.Height()*0.2f));
    EXPECT_FALSE(IsNear(middle, Color4(0, 200, 0, 255), 60)) << "no drawing layer";

    // The selected box moves under a drag at once
    auto regions = PipelineRegions::Read(*node);
    ASSERT_EQ(regions.Count(), 3);
    EXPECT_EQ(node->GetConfigString("selectedRegion", regions[0].id), regions[0].id);
    RectF box = stage->GetRegionRectangle();
    Vec2F from(box.Center().x, box.bottom + box.Height()*0.3f);
    Drag(editor->LocalToScreenPoint(from), editor->LocalToScreenPoint(from + Vec2F(20.0f, 0.0f)));
    regions = PipelineRegions::Read(*node);
    EXPECT_NEAR(regions[0].x, 20.0f/area.Width(), 0.02f) << "moved without picking the region tool";

    // A click on another box selects that part
    Vec2F third(area.left + area.Width()*(regions[2].x + regions[2].w*0.5f), area.bottom + area.Height()*0.3f);
    Vec2F screen = editor->LocalToScreenPoint(third);
    Drag(screen, screen, 1);
    EXPECT_EQ(node->GetConfigString("selectedRegion", ""), regions[2].id);

    // The part ports sit on their cells
    auto card = Card(node);
    auto grid = card->FindChildByTypeAndName<Widget>("parts");
    ASSERT_TRUE(grid);
    auto cells = grid->GetChildWidgets();
    ASSERT_EQ(cells.Count(), 3);
    for (int i = 0; i < 3; i++)
    {
        RectF imageRect = cells[i]->FindChildByType<PipelineImageView>()->layout->GetWorldRect();
        Vec2F port = card->GetPortPosition(node->outputs[i].id, false);
        EXPECT_NEAR(port.x, imageRect.right, 1.0f) << i;
        EXPECT_NEAR(port.y, imageRect.bottom, 1.0f) << i;
    }
    ShotCard(node, "extract_no_drawing");
}

// Zoomed out past the detail level the cards drop their controls, but a drawing stays: the draw node's and the image
// edit's marks
TEST_F(PipelinePolishUiFixture, DrawingsStayVisibleWhenZoomedOut)
{
    const Color4 green(0, 200, 0, 255);
    for (String type : { "drawImage", "imageEdit" })
    {
        auto node = OpenFed(type, 480.0f, type == "imageEdit");
        node->SetConfigString("drawing", PipelineUtils::BytesToDataUrl(EncodeBitmapPng(*PipelineImageOps::Blank(96, 64, green)), "image/png"));
        node->SetConfigNumber("dw", 96);
        node->SetConfigNumber("dh", 64);
        Card(node)->Rebuild();
        Card(node)->UpdateFromNode();
        Step(3);

        auto stage = Card(node)->FindChildByType<PipelinePaintEditor>();
        ASSERT_TRUE(stage) << type.Data();
        RectF area = stage->GetStageRectangle();
        Vec2F center = area.Center();
        EXPECT_TRUE(IsNear(PixelAt(Capture(), center), green)) << type.Data() << ": the drawing at the detail zoom";

        editor->SetView(center, 6.0f);
        Step(4);
        EXPECT_FALSE(Card(node)->IsDetailed()) << type.Data() << ": the card dropped its controls";
        auto capture = Capture();
        EXPECT_TRUE(IsNear(PixelAt(capture, center), green)) << type.Data() << ": the drawing zoomed out";
        ShotCard(node, "zoomed_out_" + type);
    }
}

// AI upscale: the input beside the result, the size row with its modes, fields and "input → output" info, the details
// prompt, and the note for a model that answers at its own size, which keeps the pictures of a hand-sized card in place
TEST_F(PipelinePolishUiFixture, UpscaleCardShowsTheSizeAndTheNote)
{
    auto node = OpenFed("aiUpscale");
    SetResult(node, PipelineImageOps::Blank(192, 128, resultColor));
    auto card = Card(node);
    ASSERT_TRUE(card->GetBody()->GetPair());
    EXPECT_TRUE(card->GetBody()->GetPair()->GetInputView()->HasImage());
    EXPECT_FALSE(card->FindChildByTypeAndName<Toggle>("crop")) << "no crop";

    auto info = [&]() { auto label = Card(node)->FindChildByTypeAndName<Label>("upscale info"); return label ? String(label->GetText()) : String(); };
    auto mode = [&](const String& value) { return Card(node)->FindChildByTypeAndName<Toggle>("mode " + value); };
    auto edit = [&](const String& name) { return Card(node)->FindChildByTypeAndName<EditBox>(name); };
    auto type = [&](const String& name, const String& text)
    {
        auto field = edit(name);
        ASSERT_TRUE(field) << name.Data();
        field->SetText(text);
        field->onChangeCompleted(text);
        Step(2);
    };

    for (auto value : { "x2", "x3", "x4", "size" })
        ASSERT_TRUE(mode(value)) << value;
    EXPECT_TRUE(mode("x2")->GetValue());
    EXPECT_EQ(info(), "96\xC3\x97" "64 -> 192\xC3\x97" "128");
    EXPECT_FALSE(edit("target width")) << "fields only in Size mode";

    Press(mode("x4"));
    EXPECT_EQ(node->GetConfigString("upscale", ""), "x4");
    EXPECT_EQ(info(), "96\xC3\x97" "64 -> 384\xC3\x97" "256");

    Press(mode("size"));
    ASSERT_TRUE(edit("target width") && edit("target height"));
    EXPECT_EQ(String(edit("target width")->GetText()), "192");
    EXPECT_EQ(String(edit("target height")->GetText()), "128");
    type("target width", "300");
    EXPECT_EQ(String(edit("target height")->GetText()), "200") << "locked: the height follows";
    type("target height", "100");
    EXPECT_EQ(String(edit("target width")->GetText()), "150") << "locked: a height sets the width that gives it";
    EXPECT_EQ(info(), "96\xC3\x97" "64 -> 150\xC3\x97" "100");

    Press(Card(node)->FindChildByTypeAndName<Toggle>("lock aspect"));
    EXPECT_FALSE(node->GetConfigBool("lockAspect", true));
    EXPECT_FLOAT_EQ(node->GetConfigNumber("targetH", 0), 100.0f) << "unlocking keeps the height on view";
    type("target height", "400");
    EXPECT_EQ(info(), "96\xC3\x97" "64 -> 150\xC3\x97" "400");
    ShotCard(node, "upscale_size");

    // A model that answers at its own size brings the note; a hand-sized card keeps its pictures
    node->size = Vec2F(480.0f, Card(node)->GetAutoHeight() + 60.0f);
    Card(node)->SetParamsOpen(true);
    Step(3);
    EXPECT_FALSE(Card(node)->FindChildByTypeAndName<Label>("small model note")) << "Gemini 3 renders 2K / 4K itself";
    float pictures = PictureArea(node)->layout->GetHeight();
    float height = Card(node)->GetCardRect().Height();

    auto field = Card(node)->FindChildByTypeAndName<Button>("model");
    ASSERT_TRUE(field);
    field->onClick();
    Step(2);
    auto picker = editor->GetModelPicker();
    ASSERT_TRUE(picker && picker->IsOpen());
    picker->Pick("gemini-2.5-flash-image");
    Step(3);
    EXPECT_EQ(node->GetConfigString("model", ""), "gemini-2.5-flash-image");
    EXPECT_TRUE(Card(node)->FindChildByTypeAndName<Label>("small model note"));
    EXPECT_NEAR(PictureArea(node)->layout->GetHeight(), pictures, 0.5f) << "the note row adds to the card, not from the pictures";
    EXPECT_GT(Card(node)->GetCardRect().Height(), height + 20.0f);
    ShotCard(node, "upscale_note");
}

// Each part's output port sits on its cell's image corner whether the parameters are folded or open, on a hand-sized
// card and a natural one, with many parts or few, while the transparency rows come and go, after a width change and
// after the pipeline is opened again
TEST_F(PipelinePolishUiFixture, ExtractPortsStayOnTheCellsWhenTheParamsUnfold)
{
    struct Case { float width; int parts; int cols; bool sized; };
    for (auto c : { Case{ 1140.0f, 30, 8, true }, Case{ 1140.0f, 30, 8, false }, Case{ 480.0f, 3, 3, true }, Case{ 480.0f, 3, 3, false } })
    {
        String name = (String)c.parts + " parts, " + (c.sized ? "hand-sized" : "natural");
        Ref<PipelineNode> node = OpenFed("imageExtract", c.width, true, nullptr, c.parts);
        SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
        if (c.sized)
        {
            node->size = Vec2F(c.width, Card(node)->GetAutoHeight() + 60.0f);
            Card(node)->UpdateFromNode();
            Step(3);
        }

        auto expectOnCells = [&](const String& step, int cols = 0)
        {
            auto card = Card(node);
            auto grid = card->FindChildByTypeAndName<Widget>("parts");
            ASSERT_TRUE(grid) << name.Data() << ", " << step.Data();
            auto cells = grid->GetChildWidgets();
            ASSERT_EQ(cells.Count(), c.parts) << name.Data() << ", " << step.Data();
            if (cols > 0)
            {
                float firstTop = cells[0]->layout->GetWorldRect().top;
                EXPECT_EQ(cells.Count([&](const Ref<Widget>& cell) { return Math::Abs(cell->layout->GetWorldRect().top - firstTop) < 0.5f; }), cols)
                    << name.Data() << ", " << step.Data();
            }
            for (int i = 0; i < c.parts; i++)
            {
                auto image = cells[i]->FindChildByType<PipelineImageView>();
                ASSERT_TRUE(image);
                RectF box = image->layout->GetWorldRect();
                Vec2F port = card->GetPortPosition(node->outputs[i].id, false);
                EXPECT_NEAR(port.x, box.right, 0.5f) << name.Data() << ", " << step.Data() << ": part " << i;
                EXPECT_NEAR(port.y, box.bottom, 0.5f) << name.Data() << ", " << step.Data() << ": part " << i;
            }
        };

        expectOnCells("params folded", c.cols);
        Card(node)->SetParamsOpen(true);
        Step(3);
        expectOnCells("params open", c.cols);

        Press(TransparentSwitch(node));
        ASSERT_TRUE(node->GetConfigBool("transparentBg", false)) << name.Data();
        expectOnCells("transparency rows");
        Press(FindToggle(Card(node), "Chroma key x1"));
        expectOnCells("chroma key rows");
        if (c.parts > 1)
        {
            Press(FindToggle(Card(node), "This part"));
            expectOnCells("this part's rows");
        }
        if (c.sized)
            ShotCard(node, "extract_ports_" + (String)c.parts + "_params_open");

        Resize(node, Vec2F(c.width - 220.0f, c.sized ? node->size.y : 0.0f));
        expectOnCells("narrower");

        node->size.x = c.width;
        editor->GetGraph()->SaveToAsset(*asset);
        editor->SetAsset(asset);
        Step(3);
        node = editor->GetGraph()->FindNode(node->id);
        ASSERT_TRUE(node);
        SetResult(node, PipelineImageOps::Blank(96, 64, resultColor));
        editor->SetView(Card(node)->GetCardRect().Center(), 1.0f);
        Step(3);
        expectOnCells("opened again", c.cols);

        Press(TransparentSwitch(node));
        expectOnCells("transparency off");
        Card(node)->SetParamsOpen(false);
        Step(3);
        expectOnCells("params folded again", c.cols);
    }
}
