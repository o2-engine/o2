#include "o2Editor/stdafx.h"
#include "PipelinePaintEditor.h"

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/PngFormat.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2/Assets/Types/FontAsset.h"
#include "o2/Scene/UI/WidgetState.h"

#include <cstring>
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    // The tool row and the colour palette of the paint editor (AssetsLine DrawingEditor .draw-toolbar): tools in a framed
    // group, the compact colour well, the brush size and opacity bars sharing the spare width, undo / redo / clear
    static const float paletteHeight = 20.0f;
    static const float toolSize = 22.0f;
    static const float toolGap = 2.0f;
    static const float rowGap = 4.0f;
    static const float sliderMinWidth = 36.0f;

    static const Vector<String> drawPalette = {
        "#ff3b30", "#ff9500", "#ffcc00", "#34c759", "#00c7be", "#007aff", "#5856d6", "#af52de", "#ffffff", "#8e8e93", "#000000"
    };

    // A 22 px square button of the tool row: no fill at rest, the active wash when hovered, an icon in the dim text
    static Ref<Button> MakeToolButton(const String& icon, const String& name)
    {
        auto& theme = PipelineTheme::Get();
        auto button = mmake<Button>();
        button->name = name;
        auto hover = PipelineControls::AddFillLayer(*button, "hover", theme.active, 6.0f, Layout::BothStretch(), -1.0f);
        float strength = hover->transparency;
        hover->transparency = 0.0f;
        button->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, strength, 0.12f))->offStateAnimationSpeed = 0.5f;
        auto sprite = mmake<VectorSprite>(icon);
        sprite->color = theme.fgDim;
        button->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(12, 12)));
        return button;
    }

    // A tool of the framed group: the active one gets the --active pill and the regular text colour
    static Ref<Toggle> MakeToolToggle(const String& icon, const String& name)
    {
        auto& theme = PipelineTheme::Get();
        auto toggle = mmake<Toggle>();
        toggle->name = name;
        auto on = PipelineControls::AddFillLayer(*toggle, "value", theme.active, 6.0f, Layout::BothStretch(), -1.0f);
        float strength = on->transparency;
        on->transparency = 0.0f;
        toggle->AddState("value", AnimationClip::EaseInOut("layer/value/transparency", 0.0f, strength, 0.12f))->offStateAnimationSpeed = 1.0f;
        auto sprite = mmake<VectorSprite>(icon);
        sprite->color = theme.fgDim;
        toggle->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(12, 12)));
        return toggle;
    }

    // A cell of the tool row: lineHeight tall, the control centred in it
    static Ref<Widget> MakeCell(const String& name, float width, bool flexible)
    {
        auto cell = mmake<Widget>();
        cell->name = name;
        cell->layout->minWidth = width;
        if (!flexible)
            cell->layout->maxWidth = width;
        return cell;
    }

    void PipelinePaintEditor::BuildToolbar()
    {
        auto& theme = PipelineTheme::Get();
        WeakRef<PipelinePaintEditor> weakThis(this);

        auto toolbar = mmake<PipelineWrapRow>();
        toolbar->name = "toolbar";
        toolbar->spacing = rowGap;
        toolbar->lineHeight = toolbarHeight;
        *toolbar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, toolbarHeight, 0);
        AddChild(toolbar);
        mToolbar = toolbar;

        // .draw-tools: field fill and line border, r 8, 2 px padding, 22 px tools 2 apart
        mToolGroup = MakeCell("tools", 52.0f, false);
        PipelineControls::AddFillLayer(*mToolGroup, "back", theme.field, 8.0f, Layout::BothStretch(), -2.0f);
        PipelineControls::AddBorderLayer(*mToolGroup, "border", theme.line, 8.0f, 1.0f, Layout::BothStretch(), -1.0f);
        toolbar->AddChild(mToolGroup);

        auto makeTool = [&](const String& icon, const String& tool, const String& name)
        {
            auto toggle = MakeToolToggle(icon, name);
            toggle->onToggleByUser = [weakThis, tool](bool) { if (auto self = weakThis.Lock()) self->SetTool(tool); };
            mToolGroup->AddChild(toggle);
            return toggle;
        };

        mBrushToggle = makeTool("ui/pipeline/al_brush.svg", "brush", "brush");
        mEraserToggle = makeTool("ui/pipeline/al_eraser.svg", "eraser", "eraser");
        mRegionToggle = makeTool("ui/pipeline/al_crop.svg", "roi", "region");
        LayoutToolGroup();

        // Compact colour well: a 22 px box with a 16 px swatch and its faint inner ring
        auto colorCell = MakeCell("color cell", toolSize, false);
        mColorButton = mmake<Button>();
        mColorButton->name = "color";
        *mColorButton->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(1, 0.5f), Vec2F(0, -toolSize * 0.5f), Vec2F(0, toolSize * 0.5f));
        PipelineControls::AddFillLayer(*mColorButton, "back", theme.field, 6.0f, Layout::BothStretch(), -2.0f);
        PipelineControls::AddBorderLayer(*mColorButton, "border", theme.line, 6.0f, 1.0f, Layout::BothStretch(), -1.0f);
        auto swatch = mmake<PipelineRoundedRect>();
        swatch->radius = 4.0f;
        swatch->roundBottom = true;
        mColorButton->AddLayer("swatch", swatch, Layout::BothStretch(3, 3, 3, 3));
        PipelineControls::AddBorderLayer(*mColorButton, "swatchRing", Color4(255, 255, 255, 41), 4.0f, 1.0f, Layout::BothStretch(3, 3, 3, 3), 1.0f);
        mColorButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->TogglePalette(); };
        colorCell->AddChild(mColorButton);
        toolbar->AddChild(colorCell);

        // The bars keep their marks: the brush for the size, α for the opacity; the slider's own caption is empty and its
        // 10 px caption gap sits under the mark, so the track starts right after it
        auto makeBarCell = [&](const String& name, const Ref<IRectDrawable>& mark, float markWidth, float markHeight,
                               Ref<PipelineSlider>& slider, const String& key)
        {
            auto cell = MakeCell(name + " cell", sliderMinWidth, true);
            cell->AddLayer("mark", mark, Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(0, -markHeight * 0.5f), Vec2F(markWidth, markHeight * 0.5f)));
            slider = mmake<PipelineSlider>();
            slider->name = name;
            slider->SetCompact(true);
            *slider->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(1, 0.5f), Vec2F(markWidth + rowGap - 10.0f, -7.5f), Vec2F(0, 7.5f));
            slider->onChanged = [weakThis, key](float value, bool completed)
            {
                if (auto self = weakThis.Lock())
                {
                    self->mNode->SetConfigNumber(key.Data(), value);
                    if (self->onConfigChanged) self->onConfigChanged(key, completed);
                }
            };
            cell->AddChild(slider);
            toolbar->AddChild(cell);
        };

        auto brushMark = mmake<VectorSprite>("ui/pipeline/al_brush.svg");
        brushMark->color = theme.fgDim;
        makeBarCell("size", brushMark, 12.0f, 12.0f, mSizeSlider, "brushSize");
        // Plex Mono has no α: the browser falls back to a UI face, so does this
        auto alpha = PipelineControls::MakeThemeText("\xCE\xB1", 11.0f, 600, theme.fgDim, HorAlign::Middle);
        alpha->dotsEngings = false;
        makeBarCell("opacity", alpha, 10.0f, 14.0f, mOpacitySlider, "brushOpacity");

        // .draw-actions: undo, redo and clear, 22 px each, 2 apart, faded while there is nothing to do
        auto actions = MakeCell("history", toolSize * 3 + toolGap * 2, false);
        auto makeAction = [&](const String& icon, const String& name, int index, const Function<void()>& action)
        {
            auto button = MakeToolButton(icon, name);
            float x = index * (toolSize + toolGap);
            *button->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(x, -toolSize * 0.5f), Vec2F(x + toolSize, toolSize * 0.5f));
            button->onClick = action;
            actions->AddChild(button);
            return button;
        };
        mUndoButton = makeAction("ui/pipeline/al_undo.svg", "undo", 0, [weakThis]() { if (auto self = weakThis.Lock()) self->OnUndo(); });
        mRedoButton = makeAction("ui/pipeline/al_redo.svg", "redo", 1, [weakThis]() { if (auto self = weakThis.Lock()) self->OnRedo(); });
        mClearButton = makeAction("ui/pipeline/al_trash.svg", "clear", 2, [weakThis]() { if (auto self = weakThis.Lock()) self->OnClear(); });
        toolbar->AddChild(actions);

        mPaletteRow = mmake<PipelineWrapRow>();
        mPaletteRow->name = "palette";
        mPaletteRow->spacing = 3;
        mPaletteRow->lineHeight = paletteHeight;
        *mPaletteRow->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, paletteHeight, toolbarHeight + 2);
        mPaletteRow->enabled = false;
        AddChild(mPaletteRow);

        for (auto& hex : drawPalette)
        {
            Color4 color;
            PipelineUtils::ParseHexColor(hex, color);
            auto swatchButton = mmake<Button>();
            swatchButton->layout->minWidth = 20;
            swatchButton->layout->maxWidth = 20;
            PipelineControls::AddFillLayer(*swatchButton, "swatch", color, 5.0f);
            PipelineControls::AddBorderLayer(*swatchButton, "ring", Color4(255, 255, 255, 41), 5.0f, 1.0f, Layout::BothStretch(), 1.0f);
            String value = hex;
            swatchButton->onClick = [weakThis, value]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->mNode->SetConfigString("brushColor", value);
                    if (self->onConfigChanged) self->onConfigChanged("brushColor", true);
                    self->UpdateToolbar();
                }
            };
            mPaletteRow->AddChild(swatchButton);
        }

        auto pick = PipelineControls::MakeButton("...");
        pick->layout->minWidth = 28;
        pick->layout->maxWidth = 28;
        pick->onClick = [weakThis]()
        {
            auto self = weakThis.Lock();
            if (!self)
                return;

            ColorPickerDlg::Show(self->GetBrushColor(), [weakThis](const Color4& value, bool)
            {
                if (auto self = weakThis.Lock())
                {
                    self->mNode->SetConfigString("brushColor", PipelineUtils::ColorToHex(Color4(value.r, value.g, value.b, 255)));
                    if (self->onConfigChanged) self->onConfigChanged("brushColor", false);
                    self->UpdateToolbar();
                }
            }, [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    if (self->onConfigChanged) self->onConfigChanged("brushColor", true);
            });
        };
        mPaletteRow->AddChild(pick);
    }

    void PipelinePaintEditor::LayoutToolGroup()
    {
        // 2 px padding and the 1 px border round the tools: 52 wide for two, 76 for three
        int count = 0;
        for (auto& tool : { mBrushToggle, mEraserToggle, mRegionToggle })
        {
            if (!tool || !tool->IsEnabled())
                continue;

            float x = 3.0f + count * (toolSize + toolGap);
            *tool->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(x, -toolSize * 0.5f), Vec2F(x + toolSize, toolSize * 0.5f));
            count++;
        }
        float width = 6.0f + count * toolSize + Math::Max(0, count - 1) * toolGap;
        mToolGroup->layout->minWidth = width;
        mToolGroup->layout->maxWidth = width;
    }

    void PipelinePaintEditor::UpdateToolbar()
    {
        auto& theme = PipelineTheme::Get();
        String tool = GetTool();
        mBrushToggle->SetValue(tool == "brush");
        mEraserToggle->SetValue(tool == "eraser");
        mRegionToggle->SetValue(tool == "roi");
        for (auto& toggle : { mBrushToggle, mEraserToggle, mRegionToggle })
        {
            if (auto icon = toggle->GetLayerDrawableBasedOn<IRectDrawable>("icon"))
                icon->color = toggle->GetValue() ? theme.fg : theme.fgDim;
        }

        if (auto swatch = mColorButton->GetLayerDrawableBasedOn<IRectDrawable>("swatch"))
            swatch->color = GetBrushColor();

        mSizeSlider->Setup("", 1, 80, 1, GetBrushSize());
        mOpacitySlider->Setup("", 0, 1, 0.01f, GetBrushOpacity());

        bool canClear = mNode && !mNode->GetConfigString("drawing", "").IsEmpty();
        mUndoButton->interactable = !mUndo.IsEmpty();
        mRedoButton->interactable = !mRedo.IsEmpty();
        mClearButton->interactable = canClear;
        mUndoButton->transparency = mUndo.IsEmpty() ? 0.55f : 1.0f;
        mRedoButton->transparency = mRedo.IsEmpty() ? 0.55f : 1.0f;
        mClearButton->transparency = canClear ? 1.0f : 0.55f;
    }

    void PipelinePaintEditor::TogglePalette()
    {
        mPaletteOpen = !mPaletteOpen;
        mPaletteRow->enabled = mPaletteOpen;
    }
}
