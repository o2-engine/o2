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

#include <cstring>

namespace Editor
{
    // The tool row and the colour palette of the paint editor
    static const float paletteHeight = 20.0f;
    static const float sliderMinWidth = 170.0f;

    static const Vector<String> drawPalette = {
        "#ff3b30", "#ff9500", "#ffcc00", "#34c759", "#00c7be", "#007aff", "#5856d6", "#af52de", "#ffffff", "#8e8e93", "#000000"
    };

    void PipelinePaintEditor::BuildToolbar()
    {
        WeakRef<PipelinePaintEditor> weakThis(this);

        auto toolbar = mmake<PipelineWrapRow>();
        toolbar->name = "toolbar";
        toolbar->spacing = 2;
        toolbar->lineHeight = toolbarHeight;
        *toolbar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, toolbarHeight, 0);
        AddChild(toolbar);
        mToolbar = toolbar;

        auto makeTool = [&](const String& icon, const String& tool, const String& name)
        {
            auto toggle = o2UI.CreateWidget<Toggle>("pipeline segment");
            toggle->name = name;
            toggle->caption = "";
            auto sprite = mmake<Sprite>(icon);
            sprite->color = PipelineControls::textColor;
            toggle->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
            toggle->layout->minWidth = 24;
            toggle->layout->maxWidth = 24;
            toggle->onToggleByUser = [weakThis, tool](bool) { if (auto self = weakThis.Lock()) self->SetTool(tool); };
            toolbar->AddChild(toggle);
            return toggle;
        };

        mBrushToggle = makeTool("ui/pipeline/btn_brush.png", "brush", "brush");
        mEraserToggle = makeTool("ui/pipeline/btn_eraser.png", "eraser", "eraser");
        mRegionToggle = makeTool("ui/pipeline/btn_region.png", "roi", "region");

        mColorButton = o2UI.CreateButton("");
        mColorButton->name = "color";
        mColorButton->layout->minWidth = 24;
        mColorButton->layout->maxWidth = 24;
        mColorButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->TogglePalette(); };
        toolbar->AddChild(mColorButton);

        mSizeSlider = mmake<PipelineSlider>();
        mSizeSlider->name = "size";
        mSizeSlider->layout->minWidth = sliderMinWidth;
        mSizeSlider->onChanged = [weakThis](float value, bool completed)
        {
            if (auto self = weakThis.Lock())
            {
                self->mNode->SetConfigNumber("brushSize", value);
                if (self->onConfigChanged) self->onConfigChanged("brushSize", completed);
            }
        };
        toolbar->AddChild(mSizeSlider);

        mOpacitySlider = mmake<PipelineSlider>();
        mOpacitySlider->name = "opacity";
        mOpacitySlider->layout->minWidth = sliderMinWidth;
        mOpacitySlider->onChanged = [weakThis](float value, bool completed)
        {
            if (auto self = weakThis.Lock())
            {
                self->mNode->SetConfigNumber("brushOpacity", value);
                if (self->onConfigChanged) self->onConfigChanged("brushOpacity", completed);
            }
        };
        toolbar->AddChild(mOpacitySlider);

        auto makeAction = [&](const String& icon, const String& name, const Function<void()>& action)
        {
            auto button = PipelineControls::MakeIconButton(icon, PipelineControls::textColor, Color4(0, 0, 0, 0));
            button->name = name;
            button->layout->minWidth = 20;
            button->layout->maxWidth = 20;
            button->onClick = action;
            toolbar->AddChild(button);
            return button;
        };
        mUndoButton = makeAction("ui/pipeline/btn_undo.png", "undo", [weakThis]() { if (auto self = weakThis.Lock()) self->OnUndo(); });
        mRedoButton = makeAction("ui/pipeline/btn_redo.png", "redo", [weakThis]() { if (auto self = weakThis.Lock()) self->OnRedo(); });
        mClearButton = makeAction("ui/UI4_small_trash_icon.png", "clear", [weakThis]() { if (auto self = weakThis.Lock()) self->OnClear(); });

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
            auto swatch = o2UI.CreateButton("");
            swatch->layout->minWidth = 20;
            swatch->layout->maxWidth = 20;
            if (auto regular = swatch->GetLayerDrawable<Sprite>("regular"))
                regular->color = color;
            String value = hex;
            swatch->onClick = [weakThis, value]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->mNode->SetConfigString("brushColor", value);
                    if (self->onConfigChanged) self->onConfigChanged("brushColor", true);
                    self->UpdateToolbar();
                }
            };
            mPaletteRow->AddChild(swatch);
        }

        auto pick = o2UI.CreateButton("...");
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

    void PipelinePaintEditor::UpdateToolbar()
    {
        String tool = GetTool();
        mBrushToggle->SetValue(tool == "brush");
        mEraserToggle->SetValue(tool == "eraser");
        mRegionToggle->SetValue(tool == "roi");

        if (auto regular = mColorButton->GetLayerDrawable<Sprite>("regular"))
            regular->color = GetBrushColor();

        mSizeSlider->Setup("Size", 1, 80, 1, GetBrushSize());
        mOpacitySlider->Setup("Alpha", 0, 1, 0.05f, GetBrushOpacity());

        mUndoButton->interactable = !mUndo.IsEmpty();
        mRedoButton->interactable = !mRedo.IsEmpty();
        mClearButton->interactable = mNode && !mNode->GetConfigString("drawing", "").IsEmpty();
        mUndoButton->transparency = mUndo.IsEmpty() ? 0.4f : 1.0f;
        mRedoButton->transparency = mRedo.IsEmpty() ? 0.4f : 1.0f;
        mClearButton->transparency = mClearButton->interactable ? 1.0f : 0.4f;
    }

    void PipelinePaintEditor::TogglePalette()
    {
        mPaletteOpen = !mPaletteOpen;
        mPaletteRow->enabled = mPaletteOpen;
    }
}
