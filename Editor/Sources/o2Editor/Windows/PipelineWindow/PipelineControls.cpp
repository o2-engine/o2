#include "o2Editor/stdafx.h"
#include "PipelineControls.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    PipelineRoundedRect::PipelineRoundedRect(const PipelineRoundedRect& other):
        IRectDrawable(other), radius(other.radius), roundTop(other.roundTop), roundBottom(other.roundBottom)
    {}

    void PipelineRoundedRect::Draw()
    {
        if (!mEnabled || mColor.a == 0)
            return;

        Basis basis = GetBasis();
        RectF rect(basis.origin, basis.origin + basis.xv + basis.yv);
        float r = Math::Clamp(radius, 0.0f, Math::Min(rect.Width(), rect.Height()) * 0.5f);
        const int segments = 6;
        float pi = Math::PI();

        Vector<Vec2F> points;
        auto corner = [&](const Vec2F& sharp, const Vec2F& center, float fromAngle, float toAngle, bool rounded)
        {
            if (!rounded || r <= 0.0f)
            {
                points.Add(sharp);
                return;
            }
            for (int i = 0; i <= segments; i++)
            {
                float a = Math::Lerp(fromAngle, toAngle, (float)i / segments);
                points.Add(center + Vec2F(Math::Cos(a), Math::Sin(a)) * r);
            }
        };

        corner(rect.LeftBottom(), Vec2F(rect.left + r, rect.bottom + r), pi, pi * 1.5f, roundBottom);
        corner(Vec2F(rect.right, rect.bottom), Vec2F(rect.right - r, rect.bottom + r), pi * 1.5f, pi * 2.0f, roundBottom);
        corner(rect.RightTop(), Vec2F(rect.right - r, rect.top - r), 0.0f, pi * 0.5f, roundTop);
        corner(Vec2F(rect.left, rect.top), Vec2F(rect.left + r, rect.top - r), pi * 0.5f, pi, roundTop);

        o2Render.DrawFilledPolygon(points, mColor);
    }

    namespace PipelineControls
    {
        static bool sFarView = false;

        bool IsFarView()
        {
            return sFarView;
        }

        void SetFarView(bool enabled)
        {
            sFarView = enabled;
        }

        void DrawRoundedFrame(const RectF& rect, float radius, const Color4& color, float widthPixels)
        {
            float r = Math::Clamp(radius, 0.0f, Math::Min(rect.Width(), rect.Height()) * 0.5f);
            const int segments = 6;
            float pi = Math::PI();
            Vector<Vec2F> points;
            auto corner = [&](const Vec2F& center, float fromAngle, float toAngle)
            {
                for (int i = 0; i <= segments; i++)
                {
                    float a = Math::Lerp(fromAngle, toAngle, (float)i / segments);
                    points.Add(center + Vec2F(Math::Cos(a), Math::Sin(a)) * r);
                }
            };
            corner(Vec2F(rect.left + r, rect.bottom + r), pi, pi * 1.5f);
            corner(Vec2F(rect.right - r, rect.bottom + r), pi * 1.5f, pi * 2.0f);
            corner(Vec2F(rect.right - r, rect.top - r), 0.0f, pi * 0.5f);
            corner(Vec2F(rect.left + r, rect.top - r), pi * 0.5f, pi);
            points.Add(points[0]);
            o2Render.DrawAALine(points, color, widthPixels);
        }

        const Color4 textColor(96, 125, 139, 255);
        const Color4 dimTextColor(96, 125, 139, 170);
        const Color4 accentColor(0, 150, 136, 255);

        Ref<Label> MakeLabel(const String& text, bool dim /*= false*/)
        {
            auto label = o2UI.CreateLabel(text);
            label->horAlign = HorAlign::Left;
            label->layout->minHeight = 16;
            if (dim)
            {
                label->SetColor(dimTextColor);
                label->horOverflow = Label::HorOverflow::Wrap;
            }
            return label;
        }

        Ref<EditBox> MakeEditBox(const String& text, bool multiline, const String& placeholder /*= ""*/)
        {
            auto edit = o2UI.CreateEditBox(multiline ? "standard" : "singleline");
            if (multiline)
            {
                edit->SetMultiLine(true);
                edit->SetWordWrap(true);
                edit->SetEnableScrollsHiding(true);
            }
            else
                edit->layout->minSize = Vec2F(30, 20);
            edit->SetText(text);
            return edit;
        }

        Ref<DropDown> MakeDropDown(const Vector<String>& items, const String& value)
        {
            auto dropdown = o2UI.CreateDropdown();
            Vector<String> all = items;
            if (!value.IsEmpty() && !all.Contains(value))
                all.Insert(value, 0);
            for (auto& item : all)
                dropdown->AddItem(item);
            dropdown->SetMaxListSizeInItems(12);
            if (!value.IsEmpty())
                dropdown->SelectItemText(value);
            else if (!all.IsEmpty())
                dropdown->SelectItemAt(0);
            return dropdown;
        }

        Ref<Toggle> MakeCheckbox(const String& caption, bool value)
        {
            auto toggle = o2UI.CreateToggle(caption);
            toggle->SetValue(value);
            return toggle;
        }

        Ref<Button> MakeButton(const String& caption)
        {
            auto button = o2UI.CreateButton(caption);
            return button;
        }

        Ref<Button> MakeIconButton(const String& icon, const Color4& iconColor, const Color4& backColor)
        {
            auto button = o2UI.CreateWidget<Button>("pipeline icon");
            if (auto ic = button->GetLayerDrawable<Sprite>("icon")) { ic->imageName = icon; ic->color = iconColor; }
            if (backColor.a > 0)
            {
                auto back = mmake<Sprite>("ui/UI4_button_regular.png");
                back->color = backColor;
                button->AddLayer("back", back, Layout::BothStretch(-9, -9, -10, -10), -1.0f);
            }
            return button;
        }

        Ref<Toggle> MakeSegment(const String& caption, bool value)
        {
            auto toggle = o2UI.CreateWidget<Toggle>("pipeline segment");
            toggle->caption = caption;
            toggle->SetValue(value);
            return toggle;
        }

        Ref<HorizontalLayout> MakeRow(const String& label, const Ref<Widget>& control, float labelWidth /*= 64.0f*/)
        {
            auto row = mmake<HorizontalLayout>();
            row->spacing = 6;
            row->expandWidth = true;
            row->expandHeight = true;
            row->baseCorner = BaseCorner::Left;

            auto caption = MakeLabel(label, false);
            caption->layout->minWidth = labelWidth;
            caption->layout->maxWidth = labelWidth;
            row->AddChild(caption);
            if (control)
                row->AddChild(control);

            return row;
        }

        String FormatNumber(float value, float step)
        {
            char buf[32];
            if (step >= 1.0f)
                snprintf(buf, sizeof(buf), "%d", (int)std::lround(value));
            else if (step >= 0.1f)
                snprintf(buf, sizeof(buf), "%.1f", value);
            else
                snprintf(buf, sizeof(buf), "%.2f", value);
            return buf;
        }
    }

    PipelineWrapRow::PipelineWrapRow(RefCounter* refCounter):
        Widget(refCounter)
    {}

    float PipelineWrapRow::ItemWidth(const Ref<Widget>& child)
    {
        float width = child->layout->minWidth;
        return width > 0.0f ? width : 60.0f;
    }

    float PipelineWrapRow::GetHeightForWidth(float width) const
    {
        int lines = 0;
        float x = 0.0f;
        for (auto& child : mChildWidgets)
        {
            if (!child->IsEnabled())
                continue;

            float itemWidth = ItemWidth(child);
            if (lines == 0 || x + itemWidth > width + 0.5f)
            {
                lines++;
                x = 0.0f;
            }
            x += itemWidth + spacing;
        }

        return lines > 0 ? lines * lineHeight + (lines - 1) * lineSpacing : 0.0f;
    }

    void PipelineWrapRow::Update(float dt)
    {
        Widget::Update(dt);

        Vector<int> enabled;
        for (auto& child : mChildWidgets)
            enabled.Add(child->IsEnabled() ? 1 : 0);

        if (enabled != mLaidOutEnabled)
            UpdateSelfTransform();
    }

    void PipelineWrapRow::UpdateSelfTransform()
    {
        Widget::UpdateSelfTransform();

        mLaidOutEnabled.Clear();
        for (auto& child : mChildWidgets)
            mLaidOutEnabled.Add(child->IsEnabled() ? 1 : 0);

        float width = layout->GetWidth();
        Vector<Vector<Ref<Widget>>> lines;
        float x = 0.0f;
        for (auto& child : mChildWidgets)
        {
            if (!child->IsEnabled())
                continue;

            float itemWidth = ItemWidth(child);
            if (lines.IsEmpty() || x + itemWidth > width + 0.5f)
            {
                lines.Add(Vector<Ref<Widget>>());
                x = 0.0f;
            }
            lines.Last().Add(child);
            x += itemWidth + spacing;
        }

        float y = 0.0f;
        for (auto& line : lines)
        {
            float fixed = 0.0f;
            int flexible = 0;
            for (auto& child : line)
            {
                fixed += ItemWidth(child);
                if (child->layout->maxWidth <= 0.0f)
                    flexible++;
            }

            float spare = Math::Max(0.0f, width - fixed - spacing * (line.Count() - 1));
            float share = flexible > 0 ? spare / flexible : 0.0f;
            x = 0.0f;
            for (auto& child : line)
            {
                float itemWidth = ItemWidth(child) + (child->layout->maxWidth <= 0.0f ? share : 0.0f);
                // A whole layout assignment would drop the size limits the next pass relies on
                float minWidth = child->layout->minWidth, maxWidth = child->layout->maxWidth;
                float minHeight = child->layout->minHeight, maxHeight = child->layout->maxHeight;
                *child->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(itemWidth, lineHeight), Vec2F(x, -y));
                child->layout->minWidth = minWidth;
                child->layout->maxWidth = maxWidth;
                child->layout->minHeight = minHeight;
                child->layout->maxHeight = maxHeight;
                x += itemWidth + spacing;
            }
            y += lineHeight + lineSpacing;
        }
    }

    PipelineSlider::PipelineSlider(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 20;

        auto label = mmake<Text>("stdFont.ttf");
        label->horAlign = HorAlign::Left;
        label->verAlign = VerAlign::Middle;
        label->color = PipelineControls::textColor;
        label->dotsEngings = true;
        mLabelLayer = AddLayer("label", label, Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(52, 0)));

        mProgress = o2UI.CreateHorProgress();
        *mProgress->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(56, 0), Vec2F(-46, 0));
        mProgress->onChangeByUser = THIS_FUNC(OnProgressChanged);
        AddChild(mProgress);

        auto value = mmake<Text>("stdFont.ttf");
        value->horAlign = HorAlign::Right;
        value->verAlign = VerAlign::Middle;
        value->color = PipelineControls::textColor;
        mValueLayer = AddLayer("value", value, Layout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-44, 0), Vec2F(0, 0)));
    }

    void PipelineSlider::Setup(const String& label, float minValue, float maxValue, float step, float value, const String& suffix /*= ""*/)
    {
        mMin = minValue;
        mMax = maxValue;
        mStep = Math::Max(step, 0.0001f);
        mSuffix = suffix;
        if (auto text = DynamicCast<Text>(mLabelLayer->GetDrawable()))
            text->text = label;
        mProgress->SetValueRange(mMin, mMax);
        mProgress->SetScrollSense(mStep);
        SetValue(value, false);
    }

    void PipelineSlider::SetValue(float value, bool notify /*= false*/)
    {
        float snapped = Math::Round((value - mMin) / mStep) * mStep + mMin;
        mValue = Math::Clamp(snapped, mMin, mMax);
        UpdateVisuals();
        if (notify && onChanged)
            onChanged(mValue, true);
    }

    void PipelineSlider::OnProgressChanged(float value)
    {
        float snapped = Math::Clamp(Math::Round((value - mMin) / mStep) * mStep + mMin, mMin, mMax);
        bool changed = !Math::Equals(snapped, mValue);
        mValue = snapped;
        UpdateVisuals();
        mPendingComplete = true;
        if (changed && onChanged)
            onChanged(mValue, false);
    }

    float PipelineSlider::GetTrackWidth() const
    {
        return mProgress->layout->GetWidth();
    }

    void PipelineSlider::Update(float dt)
    {
        Widget::Update(dt);
        if (mPendingComplete && !o2Input.IsCursorDown())
        {
            mPendingComplete = false;
            if (onChanged)
                onChanged(mValue, true);
        }
    }

    void PipelineSlider::UpdateVisuals()
    {
        if (!Math::Equals(mProgress->GetValue(), mValue))
            mProgress->SetValue(mValue);
        if (auto text = DynamicCast<Text>(mValueLayer->GetDrawable()))
            text->text = PipelineControls::FormatNumber(mValue, mStep) + mSuffix;
    }

    PipelineImageView::PipelineImageView(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 60;

        mFrameLayer = AddLayer("frame", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9), -1.0f);

        auto checker = mmake<Sprite>("ui/pipeline/checker.png");
        checker->mode = SpriteMode::Tiled;
        mCheckerLayer = AddLayer("checker", checker, Layout::BothStretch(1, 1, 1, 1));
        mCheckerLayer->transparency = 0.45f;

        auto image = mmake<Sprite>();
        image->mode = SpriteMode::FixedAspect;
        mImageLayer = AddLayer("image", image, Layout::BothStretch(2, 2, 2, 2), 1.0f);
        mImageLayer->enabled = false;

        auto hint = mmake<Text>("stdFont.ttf");
        hint->horAlign = HorAlign::Middle;
        hint->verAlign = VerAlign::Middle;
        hint->wordWrap = true;
        hint->color = PipelineControls::dimTextColor;
        mHintLayer = AddLayer("hint", hint, Layout::BothStretch(6, 6, 6, 6), 2.0f);
    }

    void PipelineImageView::Draw()
    {
        if (!PipelineControls::IsFarView())
        {
            Widget::Draw();
            return;
        }

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        // The checker tiles are sub-pixel from afar and cost hundreds of quads per view
        for (auto& layer : mDrawingLayers)
        {
            if (layer != mCheckerLayer)
            {
                layer->Draw();
                continue;
            }

            RectF rect = layer->GetDrawable()->GetRect();
            o2Render.DrawFilledPolygon({ rect.LeftBottom(), Vec2F(rect.left, rect.top), rect.RightTop(), Vec2F(rect.right, rect.bottom) },
                                       Color4(61, 63, 69, (int)(mCheckerLayer->transparency * 255.0f)));
        }

        OnDrawn();
        DrawTopLayers();
    }

    void PipelineImageView::SetBitmap(const Ref<Bitmap>& bitmap)
    {
        mHasImage = bitmap != nullptr;
        if (bitmap)
        {
            mImageSize = bitmap->GetSize();
            // Display copies are downscaled so a multi-megapixel render never lands on the card as is
            Ref<Bitmap> display = bitmap;
            const int maxSide = 1024;
            if (mImageSize.x > maxSide || mImageSize.y > maxSide)
            {
                float k = (float)maxSide / Math::Max(mImageSize.x, mImageSize.y);
                display = bitmap->Resized(Vec2I(Math::Max(1, (int)(mImageSize.x * k)), Math::Max(1, (int)(mImageSize.y * k))));
            }
            auto sprite = DynamicCast<Sprite>(mImageLayer->GetDrawable());
            sprite->SetTexture(TextureRef(*display));
            sprite->SetTextureSrcRect(RectI(Vec2I(), display->GetSize()));
            sprite->mode = SpriteMode::FixedAspect;
        }
        mImageLayer->enabled = mHasImage;
        // The source rect resizes the sprite, so it is fitted into the layer again without waiting for a transform update
        UpdateLayersLayouts();
        mHintLayer->enabled = !mHasImage;
        mCheckerLayer->enabled = true;
    }

    void PipelineImageView::SetHint(const String& hint)
    {
        if (auto text = DynamicCast<Text>(mHintLayer->GetDrawable()))
            text->text = hint;
    }

    RectF PipelineImageView::GetImageRect() const
    {
        RectF area = mImageLayer->GetRect();
        if (!mHasImage || mImageSize.x <= 0 || mImageSize.y <= 0)
            return area;

        float k = Math::Min(area.Width() / mImageSize.x, area.Height() / mImageSize.y);
        Vec2F size(mImageSize.x * k, mImageSize.y * k);
        Vec2F center = area.Center();
        return RectF(center - size * 0.5f, center + size * 0.5f);
    }

    void PipelineImageView::UpdateLayersLayouts()
    {
        Widget::UpdateLayersLayouts();
    }

    PipelineTextView::PipelineTextView(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 40;

        mScroll = o2UI.CreateScrollArea();
        *mScroll->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
        AddChild(mScroll);

        mLabel = o2UI.CreateLabel("");
        mLabel->horAlign = HorAlign::Left;
        mLabel->verAlign = VerAlign::Top;
        mLabel->horOverflow = Label::HorOverflow::Wrap;
        mLabel->verOverflow = Label::VerOverflow::Expand;
        *mLabel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        mScroll->AddChild(mLabel);

        mHintLabel = o2UI.CreateLabel("");
        mHintLabel->SetColor(PipelineControls::dimTextColor);
        mHintLabel->horOverflow = Label::HorOverflow::Wrap;
        *mHintLabel->layout = WidgetLayout::BothStretch(6, 6, 6, 6);
        mHintLabel->horAlign = HorAlign::Middle;
        mHintLabel->verAlign = VerAlign::Middle;
        AddChild(mHintLabel);
    }

    void PipelineTextView::SetText(const String& text)
    {
        mLabel->text = text;
        mLabel->enabled = !text.IsEmpty();
        mHintLabel->enabled = text.IsEmpty();
    }

    void PipelineTextView::SetHint(const String& hint)
    {
        mHintLabel->text = hint;
    }

    PipelineColorField::PipelineColorField(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 22;

        auto label = mmake<Text>("stdFont.ttf");
        label->horAlign = HorAlign::Left;
        label->verAlign = VerAlign::Middle;
        label->color = PipelineControls::textColor;
        mLabelLayer = AddLayer("label", label, Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(52, 0)));

        mSwatch = o2UI.CreateButton("");
        mSwatch->caption = "";
        *mSwatch->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(56, 0), Vec2F(84, 0));
        mSwatch->onClick = THIS_FUNC(OnSwatchPressed);
        AddChild(mSwatch);

        mHexEdit = PipelineControls::MakeEditBox("", false);
        *mHexEdit->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(90, 0), Vec2F(0, 0));
        mHexEdit->onChangeCompleted = THIS_FUNC(OnHexChanged);
        AddChild(mHexEdit);
    }

    void PipelineColorField::Setup(const String& label, const Color4& color)
    {
        if (auto text = DynamicCast<Text>(mLabelLayer->GetDrawable()))
            text->text = label;
        SetColor(color);
    }

    void PipelineColorField::SetColor(const Color4& color)
    {
        mColor = color;
        UpdateVisuals();
    }

    void PipelineColorField::UpdateVisuals()
    {
        if (auto regular = mSwatch->GetLayerDrawable<Sprite>("regular"))
            regular->color = Color4(mColor.r, mColor.g, mColor.b, 255);
        mHexEdit->SetText(PipelineUtils::ColorToHex(mColor));
    }

    void PipelineColorField::OnSwatchPressed()
    {
        WeakRef<PipelineColorField> weakThis(this);
        ColorPickerDlg::Show(mColor, [weakThis](const Color4& value, bool byUser)
        {
            if (auto self = weakThis.Lock())
            {
                self->SetColor(Color4(value.r, value.g, value.b, 255));
                if (self->onChanged)
                    self->onChanged(self->mColor, false);
            }
        }, [weakThis]()
        {
            if (auto self = weakThis.Lock())
            {
                if (self->onChanged)
                    self->onChanged(self->mColor, true);
            }
        });
    }

    void PipelineColorField::OnHexChanged(const WString& text)
    {
        Color4 color;
        if (PipelineUtils::ParseHexColor((String)text, color))
        {
            SetColor(color);
            if (onChanged)
                onChanged(mColor, true);
        }
        else
            UpdateVisuals();
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineRoundedRect, Editor__PipelineRoundedRect);

DECLARE_CLASS(Editor::PipelineWrapRow, Editor__PipelineWrapRow);

DECLARE_CLASS(Editor::PipelineSlider, Editor__PipelineSlider);

DECLARE_CLASS(Editor::PipelineImageView, Editor__PipelineImageView);

DECLARE_CLASS(Editor::PipelineTextView, Editor__PipelineTextView);

DECLARE_CLASS(Editor::PipelineColorField, Editor__PipelineColorField);
// --- END META ---
