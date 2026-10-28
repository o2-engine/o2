#include "o2Editor/stdafx.h"
#include "PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineRichText.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageLoader.h"

#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

#include "o2/Assets/Types/FontAsset.h"
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
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    PipelineRoundedRect::PipelineRoundedRect(const PipelineRoundedRect& other):
        IRectDrawable(other), radius(other.radius), roundTop(other.roundTop), roundBottom(other.roundBottom),
        opacity(other.opacity)
    {}

    void PipelineRoundedRect::Draw()
    {
        if (!mEnabled || mColor.a == 0)
            return;

        Basis basis = GetBasis();
        RectF rect(basis.origin, basis.origin + basis.xv + basis.yv);
        float r = Math::Clamp(radius, 0.0f, Math::Min(rect.Width(), rect.Height()) * 0.5f);
        Color4 color = mColor;
        color.a = (int)(color.a*opacity);
        PipelineDraw::Fill(rect, PipelineDraw::Corners(roundTop ? r : 0.0f, roundBottom ? r : 0.0f), color);
    }

    PipelineMenuBack::PipelineMenuBack(const PipelineMenuBack& other):
        IRectDrawable(other), radius(other.radius)
    {}

    void PipelineMenuBack::Draw()
    {
        if (!mEnabled || mColor.a == 0)
            return;

        // --shadow-pop: 0 12px 32px, then the solid surface and its border (.ctx-menu)
        auto& theme = PipelineTheme::Get();
        Basis basis = GetBasis();
        RectF rect(basis.origin, basis.origin + basis.xv + basis.yv);
        float alpha = mColor.a / 255.0f;
        PipelineDraw::Corners corners(radius);
        Color4 shadow = theme.dark ? Color4(0, 0, 0, 115) : Color4(38, 50, 56, 56);
        PipelineDraw::Shadow(RectF(rect.left, rect.top - (theme.dark ? 12.0f : 6.0f), rect.right, rect.bottom - (theme.dark ? 12.0f : 6.0f)),
                             corners, PipelineTheme::Alpha(shadow, alpha), theme.dark ? 32.0f : 18.0f);
        PipelineDraw::Fill(rect, corners, PipelineTheme::Alpha(theme.cardSolid, alpha));
        PipelineDraw::Stroke(rect, corners, 0.0f, 1.0f, PipelineDraw::Gradient(PipelineTheme::Alpha(theme.border, alpha)));
        if (theme.dark)
            PipelineDraw::Line(Vec2F(rect.left + radius, rect.top - 1.5f), Vec2F(rect.right - radius, rect.top - 1.5f), 1.0f,
                               Color4(255, 255, 255, (int)(20 * alpha)));
    }

    PipelineRoundedFrame::PipelineRoundedFrame(const PipelineRoundedFrame& other):
        IRectDrawable(other), radius(other.radius), width(other.width)
    {}

    void PipelineRoundedFrame::Draw()
    {
        if (!mEnabled || mColor.a == 0)
            return;

        Basis basis = GetBasis();
        RectF rect(basis.origin, basis.origin + basis.xv + basis.yv);
        float r = Math::Clamp(radius, 0.0f, Math::Min(rect.Width(), rect.Height()) * 0.5f);
        PipelineDraw::Stroke(rect, PipelineDraw::Corners(r), 0.0f, width, PipelineDraw::Gradient(mColor));
    }

    PipelineFoldArrow::PipelineFoldArrow(const PipelineFoldArrow& other):
        IRectDrawable(other), open(other.open)
    {}

    void PipelineFoldArrow::Draw()
    {
        if (!mEnabled || mColor.a == 0)
            return;

        Basis basis = GetBasis();
        Vec2F center = basis.origin + (basis.xv + basis.yv)*0.5f;
        float half = Math::Min(basis.xv.Length(), basis.yv.Length())*0.5f;
        float angle = -Math::Clamp01(open)*Math::PI()*0.5f;
        float c = Math::Cos(angle), s = Math::Sin(angle);
        auto point = [&](float x, float y) { return center + Vec2F(x*c - y*s, x*s + y*c)*half; };

        Vector<Vec2F> points = { point(-0.6f, 0.8f), point(0.6f, 0.0f), point(-0.6f, -0.8f) };
        o2Render.DrawFilledPolygon(points, mColor);
        // A hairline around the triangle smooths its edges; an opaque colour keeps them no darker than the fill
        points.Add(points[0]);
        o2Render.DrawAALine(points, mColor, 1.0f);
    }

    PipelinePortMarker::PipelinePortMarker(const PipelinePortMarker& other):
        IRectDrawable(other), portType(other.portType)
    {}

    void PipelinePortMarker::Draw()
    {
        if (!mEnabled || mColor.a == 0)
            return;

        // AssetsLine marker shapes (NodeView.tsx PortMarker) in a 12 px cell, scaled with the cell
        Basis basis = GetBasis();
        Vec2F center = basis.origin + (basis.xv + basis.yv) * 0.5f;
        float k = Math::Min(basis.xv.Length(), basis.yv.Length()) / 12.0f;

        switch (portType)
        {
            case PipelinePortType::Image:
            {
                RectF rect(center.x - 5.0f * k, center.y + 5.0f * k, center.x + 5.0f * k, center.y - 5.0f * k);
                PipelineDraw::Fill(rect, PipelineDraw::Corners(2.5f * k), mColor);
                break;
            }
            case PipelinePortType::Audio:
            {
                // A 9 px square turned by 45 degrees with slightly rounded corners
                float h = 4.5f * k * Math::Sqrt(2.0f) - 0.4f * k;
                PipelineDraw::Polygon({ center + Vec2F(0, -h), center + Vec2F(h, 0), center + Vec2F(0, h), center + Vec2F(-h, 0) }, mColor);
                break;
            }
            case PipelinePortType::Video:
                PipelineDraw::Polygon({ center + Vec2F(-5.0f * k, -6.0f * k), center + Vec2F(6.0f * k, 0), center + Vec2F(-5.0f * k, 6.0f * k) }, mColor);
                break;
            default:
                PipelineDraw::Circle(center, 5.5f * k, mColor);
                break;
        }
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

    bool PipelineWrapRow::IsFlexible(const Ref<Widget>& child)
    {
        // A layout without a maximum keeps o2's default limit of 10000
        return child->layout->maxWidth <= 0.0f || child->layout->maxWidth >= 10000.0f;
    }

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
                if (IsFlexible(child))
                    flexible++;
            }

            float spare = Math::Max(0.0f, width - fixed - spacing * (line.Count() - 1));
            float share = flexible > 0 ? spare / flexible : 0.0f;
            x = 0.0f;
            for (auto& child : line)
            {
                float itemWidth = ItemWidth(child) + (IsFlexible(child) ? share : 0.0f);
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
        auto& theme = PipelineTheme::Get();
        layout->minHeight = 15;

        // .sb.has-label: caption 44 | track | value 44, gaps 8; the track is a 10 px bar with its fill, no knob
        auto label = PipelineControls::MakeThemeText("", PipelineControls::smallFont, 400, theme.fgDim);
        mLabelLayer = AddLayer("label", label, Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(44, 0)));

        mProgress = mmake<HorizontalProgress>();
        Color4 track = theme.dark ? Color4(255, 255, 255, 26) : Color4(96, 125, 139, 41);
        Color4 fill = theme.dark ? Color4(255, 255, 255, 71) : Color4(96, 125, 139, 115);
        PipelineControls::AddFillLayer(*mProgress, "back", track, 5.0f);
        PipelineControls::AddFillLayer(*mProgress, "bar", fill, 5.0f);
        mFillStrength = fill.a / 255.0f;
        mProgress->onChangeByUser = THIS_FUNC(OnProgressChanged);
        AddChild(mProgress);

        auto value = mmake<Text>(PipelineTheme::MonoFont());
        value->height = PipelineTheme::Size(PipelineControls::monoFont);
        value->horAlign = HorAlign::Right;
        value->verAlign = VerAlign::Middle;
        value->color = theme.fgDim;
        mValueLayer = AddLayer("value", value, Layout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-44, 0), Vec2F(0, 0)));
        LayoutParts();
    }

    void PipelineSlider::Setup(const String& label, float minValue, float maxValue, float step, float value, const String& suffix /*= ""*/)
    {
        mMin = minValue;
        mMax = maxValue;
        mStep = Math::Max(step, 0.0001f);
        mSuffix = suffix;
        if (auto text = DynamicCast<Text>(mLabelLayer->GetDrawable()))
            text->text = label;
        if (mCompact)
            LayoutParts();
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

    void PipelineSlider::SetCompact(bool compact)
    {
        mCompact = compact;
        LayoutParts();
    }

    void PipelineSlider::LayoutParts()
    {
        // Compact, the caption takes just its text
        float label = 44.0f;
        auto text = DynamicCast<Text>(mLabelLayer->GetDrawable());
        if (mCompact && text && text->GetFont())
            label = Math::Ceil(Text::GetTextSize(text->GetText(), text->GetFont(), text->GetFontHeight(), Vec2F(), HorAlign::Left, VerAlign::Top, false).x) + 2.0f;
        if (text)
            text->dotsEngings = !mCompact;

        mLabelLayer->layout = Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(label, 0));
        *mProgress->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(1, 0.5f), Vec2F(label + 8.0f, -5.0f), Vec2F(mCompact ? 0.0f : -52.0f, 5.0f));
        mValueLayer->enabled = !mCompact;
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

        // The fill turns to the accent under the cursor and while dragged
        bool active = mProgress->IsPressed() || (mResEnabledInHierarchy && mProgress->IsUnderPoint(o2Input.GetCursorPos()));
        if (active != mActive)
        {
            mActive = active;
            auto& theme = PipelineTheme::Get();
            if (auto bar = mProgress->GetLayer("bar"))
            {
                bar->GetDrawable()->color = active ? Color4(theme.accent.r, theme.accent.g, theme.accent.b, 255)
                                                   : (theme.dark ? Color4(255, 255, 255, 255) : Color4(96, 125, 139, 255));
                bar->transparency = active ? 1.0f : mFillStrength;
            }
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

        auto image = mmake<Sprite>();
        image->mode = SpriteMode::FixedAspect;
        mImageLayer = AddLayer("image", image, Layout::BothStretch(1, 1, 1, 1), 1.0f);
        mImageLayer->enabled = false;

        auto hint = PipelineControls::MakeThemeText("", PipelineControls::smallFont, 400, PipelineTheme::Get().fg3, HorAlign::Middle);
        hint->wordWrap = true;
        hint->dotsEngings = false;
        mHintLayer = AddLayer("hint", hint, Layout::BothStretch(8, 8, 8, 8), 2.0f);
    }

    void PipelineImageView::SetFramed(bool framed)
    {
        mFramed = framed;
    }

    void PipelineImageView::DrawFrame()
    {
        // .result-card: the card shadow, a ck1 plate with ck2 squares of 8 px from its top-left, a 1 px line border, radius 9
        auto& theme = PipelineTheme::Get();
        RectF rect = layout->GetWorldRect();
        PipelineDraw::Corners corners(PipelineControls::fieldRadius);
        bool far = PipelineControls::IsFarView();

        // .result-empty: no plate under the hint, only the frame, so the hint reads on the light theme too
        if (!mHasImage)
        {
            if (mFramed)
                PipelineDraw::Stroke(rect, corners, 0.0f, 1.0f, PipelineDraw::Gradient(theme.dark ? Color4(255, 255, 255, 36) : theme.border));
            return;
        }

        if (mFramed && !far)
            PipelineDraw::Shadow(RectF(rect.left, rect.top - theme.pictureShadowOffset, rect.right, rect.bottom - theme.pictureShadowOffset), corners,
                                 theme.pictureShadow, theme.pictureShadowBlur);

        if (far)
            PipelineDraw::Fill(rect, corners, theme.checkerA);
        else
            PipelineDraw::Checker(rect, corners, 8.0f, theme.checkerA, theme.checkerB, false);

        if (mFramed)
            PipelineDraw::Stroke(rect, corners, 0.0f, 1.0f, PipelineDraw::Gradient(theme.dark ? Color4(255, 255, 255, 20) : theme.line));
    }

    void PipelineImageView::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        DrawFrame();
        Widget::Draw();
    }

    void PipelineImageView::SetBitmap(const Ref<Bitmap>& bitmap)
    {
        if (mHasImage == (bitmap != nullptr) && mShownBitmap.Lock() == bitmap)
            return;

        mHasImage = bitmap != nullptr;
        mShownBitmap = bitmap ? WeakRef<Bitmap>(bitmap) : WeakRef<Bitmap>();
        if (bitmap)
        {
            mImageSize = bitmap->GetSize();
            // A card-sized copy, shared by every view of the picture, so a multi-megapixel render never lands on the card as is
            Vec2I displaySize;
            auto texture = PipelineDisplayCache::GetTexture(bitmap, &displaySize);
            auto sprite = DynamicCast<Sprite>(mImageLayer->GetDrawable());
            sprite->SetTexture(texture);
            sprite->SetTextureSrcRect(RectI(Vec2I(), displaySize));
            sprite->mode = SpriteMode::FixedAspect;
        }
        mImageLayer->enabled = mHasImage;
        // The source rect resizes the sprite, so it is fitted into the layer again without waiting for a transform update
        UpdateLayersLayouts();
        mHintLayer->enabled = !mHasImage;
    }

    void PipelineImageView::SetHint(const String& hint)
    {
        mHint = hint;
        if (auto text = DynamicCast<Text>(mHintLayer->GetDrawable()))
            text->text = mLoading ? String("loading..") : hint;
    }

    void PipelineImageView::SetLoading(bool loading)
    {
        if (mLoading == loading)
            return;

        mLoading = loading;
        if (auto text = DynamicCast<Text>(mHintLayer->GetDrawable()))
            text->text = loading ? String("loading..") : mHint;
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
        auto& theme = PipelineTheme::Get();
        layout->minHeight = 40;

        // .textout: field fill and line border, r 9, padding 6.5 / 8 / 7.5, 11.5 px at 17 px lines
        PipelineControls::AddFillLayer(*this, "back", theme.field, PipelineControls::fieldRadius, Layout::BothStretch(), -2.0f);
        PipelineControls::AddBorderLayer(*this, "border", theme.line, PipelineControls::fieldRadius, 1.0f, Layout::BothStretch(), -1.0f);

        mScroll = mmake<ScrollArea>();
        mScroll->SetClippingLayout(Layout::BothStretch(1, 1, 1, 1));
        mScroll->SetViewLayout(Layout::BothStretch(8, 7.5f, 9, 8.5f));
        *mScroll->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
        AddChild(mScroll);

        mLabel = mmake<PipelineRichText>();
        mLabel->SetStyle(PipelineControls::bodyFont, 17.0f, theme.fg);
        *mLabel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        mScroll->AddChild(mLabel);

        mHintLabel = o2UI.CreateLabel("");
        PipelineControls::StyleLabel(mHintLabel, PipelineControls::smallFont, 400, theme.fg3);
        mHintLabel->horOverflow = Label::HorOverflow::Wrap;
        *mHintLabel->layout = WidgetLayout::BothStretch(8, 8, 8, 8);
        mHintLabel->horAlign = HorAlign::Middle;
        mHintLabel->verAlign = VerAlign::Middle;
        AddChild(mHintLabel);
    }

    void PipelineTextView::SetText(const String& text)
    {
        mLabel->SetMarkdown(text);
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
        auto& theme = PipelineTheme::Get();
        layout->minHeight = 26;

        // .prow in .fx: caption 44 | 8 | .well 102 x 26 - swatch 18, the hex in mono, a chevron (controls.tsx ColorWell)
        auto label = PipelineControls::MakeThemeText("", PipelineControls::bodyFont, 400, theme.fgDim);
        mLabelLayer = AddLayer("label", label, Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(44, 0)));

        Layout well(Vec2F(0, 0), Vec2F(0, 1), Vec2F(52, 0), Vec2F(52 + 102.2f, 0));
        PipelineControls::AddFillLayer(*this, "wellBack", theme.field, PipelineControls::controlRadius, well, -2.0f);
        PipelineControls::AddBorderLayer(*this, "wellBorder", theme.line, PipelineControls::controlRadius, 1.0f, well, -1.0f);
        auto chevron = mmake<VectorSprite>("ui/pipeline/chevron_down.svg");
        chevron->color = theme.fg3;
        AddLayer("chevron", chevron, Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(52 + 83.2f, -5), Vec2F(52 + 93.2f, 5)));

        mSwatch = mmake<Button>();
        mSwatch->caption = "";
        *mSwatch->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(57, -9), Vec2F(75, 9));
        auto swatch = mmake<PipelineRoundedRect>();
        swatch->radius = 5.0f;
        swatch->roundBottom = true;
        mSwatch->AddLayer("regular", swatch, Layout::BothStretch());
        PipelineControls::AddBorderLayer(*mSwatch, "ring", Color4(255, 255, 255, 41), 5.0f, 1.0f, Layout::BothStretch(), 1.0f);
        mSwatch->onClick = THIS_FUNC(OnSwatchPressed);
        AddChild(mSwatch);

        mHexEdit = PipelineControls::MakeEditBox("", false);
        for (auto name : { "back", "border" })
            if (auto layer = mHexEdit->GetLayer(name))
                layer->transparency = 0.0f;
        if (auto text = mHexEdit->GetTextDrawable())
        {
            text->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
            text->height = PipelineTheme::Size(11.0f);
        }
        mHexEdit->SetViewLayout(Layout::BothStretch(1, 0, 1, 0));
        *mHexEdit->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(52 + 30, -9), Vec2F(52 + 80, 9));
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
        if (auto regular = mSwatch->GetLayerDrawableBasedOn<IRectDrawable>("regular"))
            regular->color = Color4(mColor.r, mColor.g, mColor.b, 255);
        mHexEdit->SetText(PipelineUtils::ColorToHex(mColor).ToLowerCase());
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

DECLARE_CLASS(Editor::PipelineRoundedFrame, Editor__PipelineRoundedFrame);

DECLARE_CLASS(Editor::PipelineMenuBack, Editor__PipelineMenuBack);

DECLARE_CLASS(Editor::PipelinePortMarker, Editor__PipelinePortMarker);

DECLARE_CLASS(Editor::PipelineFoldArrow, Editor__PipelineFoldArrow);

DECLARE_CLASS(Editor::PipelineField, Editor__PipelineField);

DECLARE_CLASS(Editor::PipelineWrapRow, Editor__PipelineWrapRow);

DECLARE_CLASS(Editor::PipelineSlider, Editor__PipelineSlider);

DECLARE_CLASS(Editor::PipelineImageView, Editor__PipelineImageView);

DECLARE_CLASS(Editor::PipelineTextView, Editor__PipelineTextView);

DECLARE_CLASS(Editor::PipelineColorField, Editor__PipelineColorField);
// --- END META ---
