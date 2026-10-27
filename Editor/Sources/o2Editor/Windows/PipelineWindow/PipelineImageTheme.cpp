#include "o2Editor/stdafx.h"
#include "PipelineImageTheme.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    namespace PipelineImageTheme
    {
        float HandleScale()
        {
            float scale = o2Render.GetCamera().GetScale2D().x;
            return Math::Min(scale > 0.0f ? scale : 1.0f, 1.5f);
        }

        void DrawChecker(const RectF& rect, float cell, const Color4& first, const Color4& second, bool firstSecond, float radius)
        {
            PipelineDraw::Checker(rect, PipelineDraw::Corners(radius), cell, first, second, firstSecond);
        }

        void DrawStagePlate(const RectF& area, bool shadow)
        {
            auto& theme = PipelineTheme::Get();
            PipelineDraw::Corners corners(9.0f);
            // Dark: 0 6px 16px black 35%; light: 0 2px 6px rgba(38, 50, 56, .14)
            float offset = theme.dark ? 6.0f : 2.0f, blur = theme.dark ? 16.0f : 6.0f;
            if (shadow)
                PipelineDraw::Shadow(RectF(area.left, area.top - offset, area.right, area.bottom - offset), corners,
                                     theme.dark ? Color4(0, 0, 0, 89) : Color4(38, 50, 56, 36), blur);

            // repeating-conic-gradient(ck1 0 25%, ck2 0 50%) / 16px: ck2 on the top-left square of each tile
            RectF inner(area.left + 1.0f, area.top - 1.0f, area.right - 1.0f, area.bottom + 1.0f);
            PipelineDraw::Fill(area, corners, theme.checkerA);
            DrawChecker(inner, 8.0f, theme.checkerA, theme.checkerB, true, 8.0f);
            PipelineDraw::Stroke(area, corners, 0.0f, 1.0f, PipelineDraw::Gradient(theme.line));
        }

        void DrawCellPlate(const RectF& cell, bool selected, bool shadow)
        {
            auto& theme = PipelineTheme::Get();
            PipelineDraw::Corners corners(7.0f);
            // Dark: 0 3px 8px black 30%; light: 0 1px 4px rgba(38, 50, 56, .14)
            float offset = theme.dark ? 3.0f : 1.0f, blur = theme.dark ? 8.0f : 4.0f;
            if (shadow)
                PipelineDraw::Shadow(RectF(cell.left, cell.top - offset, cell.right, cell.bottom - offset), corners,
                                     theme.dark ? Color4(0, 0, 0, 77) : Color4(38, 50, 56, 36), blur);

            RectF inner(cell.left + 1.0f, cell.top - 1.0f, cell.right - 1.0f, cell.bottom + 1.0f);
            DrawChecker(inner, 6.0f, theme.checkerA, theme.checkerB, false, 6.0f);
            if (selected)
                PipelineDraw::Stroke(cell, corners, 0.0f, 2.0f, PipelineDraw::Gradient(theme.accent));
            else
                PipelineDraw::Stroke(cell, corners, 0.0f, 1.0f, PipelineDraw::Gradient(theme.dark ? Color4(255, 255, 255, 20) : theme.line));
        }

        void DrawRegionTag(const RectF& box, int index, bool selected, Text& text)
        {
            auto& theme = PipelineTheme::Get();
            float k = HandleScale();
            String label = (String)index;
            text.text = label;
            text.height = PipelineTheme::Size(11.5f);
            float textWidth = 0.0f;
            if (auto font = text.GetFont())
                textWidth = Text::GetTextSize(label, font, text.height).x;
            float width = Math::Max(20.0f, textWidth + 12.0f) * k;

            RectF tag(box.left + k, box.top - k, box.left + k + width, box.top - k - 20.0f * k);
            PipelineDraw::Corners corners;
            corners.rb = 6.0f * k;
            PipelineDraw::Fill(tag, corners, selected ? theme.accent : Color4(0, 0, 0, 158));

            text.color = selected ? theme.accentInk : Color4::White();
            text.horAlign = HorAlign::Middle;
            text.verAlign = VerAlign::Middle;
            text.scale = Vec2F(k, k);
            text.SetRect(RectF(tag.left, tag.top, tag.right, tag.bottom));
            text.Draw();
            text.scale = Vec2F(1, 1);
        }

        RectF RemoveTabRect(const RectF& box)
        {
            float k = HandleScale();
            return RectF(box.right - 22.0f * k, box.top, box.right, box.top - 22.0f * k);
        }

        void DrawRemoveTab(const RectF& box, VectorSprite& trash, bool hovered)
        {
            auto& theme = PipelineTheme::Get();
            float k = HandleScale();
            RectF tab = RemoveTabRect(box);
            PipelineDraw::Corners corners;
            corners.lb = 6.0f * k;
            PipelineDraw::Fill(tab, corners, hovered ? theme.red : Color4(0, 0, 0, 142));

            trash.color = Color4(255, 255, 255, 230);
            Vec2F c = tab.Center();
            trash.rect = RectF(c.x - 6.0f * k, c.y + 6.0f * k, c.x + 6.0f * k, c.y - 6.0f * k);
            trash.Draw();
        }

        void DrawBoxOutline(const RectF& box, bool selected)
        {
            Vector<Vec2F> points = { box.LeftBottom(), Vec2F(box.left, box.top), box.RightTop(), Vec2F(box.right, box.bottom), box.LeftBottom() };
            if (selected)
            {
                o2Render.DrawAALine(points, Color4(255, 255, 255, 230), 1.0f);
                return;
            }

            // A CSS dashed 1 px border: 3 px dashes 3 px apart, on screen
            float px = PipelineDraw::Pixel(), dash = 3.0f * px;
            Color4 color(255, 255, 255, 140);
            for (int i = 0; i < 4; i++)
            {
                Vec2F a = points[i], b = points[i + 1];
                float length = (b - a).Length();
                if (length <= 0.0f)
                    continue;

                Vec2F dir = (b - a) / length;
                for (float t = 0.0f; t < length; t += dash * 2.0f)
                    PipelineDraw::Line(a + dir * t, a + dir * Math::Min(length, t + dash), px, color);
            }
        }

        void SkinCellNameField(const Ref<EditBox>& edit)
        {
            auto& theme = PipelineTheme::Get();
            for (auto name : { "back", "border", "focus" })
            {
                if (auto layer = edit->GetLayer(name))
                {
                    if (auto rect = DynamicCast<PipelineRoundedRect>(layer->GetDrawable()))
                        rect->radius = 5.0f;
                    if (auto frame = DynamicCast<PipelineRoundedFrame>(layer->GetDrawable()))
                        frame->radius = 5.0f;
                }
            }
            if (auto border = edit->GetLayer("border"))
            {
                border->GetDrawable()->color = Color4(theme.accent.r, theme.accent.g, theme.accent.b, 255);
                border->transparency = 1.0f;
            }
            edit->SetViewLayout(Layout::BothStretch(5, 0, 5, 0));
            if (auto text = edit->GetTextDrawable())
            {
                text->height = PipelineTheme::Size(10.5f);
                text->horAlign = HorAlign::Middle;
                text->verAlign = VerAlign::Middle;
            }
        }

        void MakeAccentGhost(const Ref<Button>& button)
        {
            auto& theme = PipelineTheme::Get();
            if (auto caption = button->GetLayerDrawable<Text>("caption"))
            {
                caption->color = theme.accent;
                caption->horAlign = HorAlign::Left;
                caption->dotsEngings = false;
            }
            if (auto layer = button->GetLayer("caption"))
                layer->layout = Layout::BothStretch(22, 0, 4, 0);
            auto sparkle = mmake<VectorSprite>("ui/pipeline/al_sparkle.svg");
            sparkle->color = theme.accent;
            button->AddLayer("sparkle", sparkle, Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(9, -4.5f), Vec2F(18, 4.5f)));
            if (auto border = button->GetLayer("border"))
            {
                border->GetDrawable()->color = Color4(theme.accent.r, theme.accent.g, theme.accent.b, 255);
                border->transparency = 0.35f;
            }
            if (auto hover = button->GetLayer("hover"))
                hover->GetDrawable()->color = Color4(theme.accent.r, theme.accent.g, theme.accent.b, 255);
        }

        void KeepWholeCaption(Widget& widget)
        {
            if (auto caption = widget.GetLayerDrawable<Text>("caption"))
                caption->dotsEngings = false;
            for (auto& child : widget.GetChildWidgets())
                KeepWholeCaption(*child);
        }

        Ref<Toggle> MakeSmallToggle(const String& caption, bool value)
        {
            auto& theme = PipelineTheme::Get();
            auto toggle = mmake<Toggle>();
            PipelineControls::AddBorderLayer(*toggle, "border", theme.line, PipelineControls::fieldRadius, 1.0f, Layout::BothStretch(), -2.0f);
            auto on = PipelineControls::AddFillLayer(*toggle, "value", theme.bg3, PipelineControls::fieldRadius, Layout::BothStretch(), -1.5f);
            on->transparency = value ? 1.0f : 0.0f;
            auto onBorder = PipelineControls::AddBorderLayer(*toggle, "valueBorder", theme.accent, PipelineControls::fieldRadius, 1.0f,
                                                             Layout::BothStretch(), -1.0f);
            onBorder->transparency = value ? 1.0f : 0.0f;
            auto anim = AnimationClip::EaseInOut("layer/value/transparency", 0.0f, 1.0f, 0.12f);
            *anim->AddTrack<float>("layer/valueBorder/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.12f);
            toggle->AddState("value", anim)->offStateAnimationSpeed = 1.0f;

            auto text = PipelineControls::MakeThemeText(caption, PipelineControls::smallFont, 400, value ? theme.fg : theme.fgDim, HorAlign::Middle);
            text->dotsEngings = false;
            toggle->AddLayer("caption", text, Layout::BothStretch(8, 0, 8, 0));
            toggle->caption = caption;
            float width = Math::Ceil(PipelineControls::TextWidth(caption, PipelineControls::smallFont)) + 18.0f;
            toggle->layout->minWidth = width;
            toggle->layout->maxWidth = width;
            toggle->SetValue(value);
            return toggle;
        }

        void DrawBoxHandles(const RectF& box)
        {
            float k = HandleScale();
            Color4 white = Color4::White();
            float arm = 15.0f * k, t = 3.0f * k, off = 2.0f * k;
            auto fill = [&](float l, float top, float r, float b) { PipelineDraw::Fill(RectF(l, top, r, b), PipelineDraw::Corners(0.0f), white); };

            // Brackets: the arms nearest the box run along its edges, 2 px outside
            fill(box.left - off - t, box.top + off + arm, box.left - off, box.top + off);
            fill(box.left - off - arm, box.top + off + t, box.left - off, box.top + off);
            fill(box.right + off, box.top + off + arm, box.right + off + t, box.top + off);
            fill(box.right + off, box.top + off + t, box.right + off + arm, box.top + off);
            fill(box.left - off - t, box.bottom - off, box.left - off, box.bottom - off - arm);
            fill(box.left - off - arm, box.bottom - off, box.left - off, box.bottom - off - t);
            fill(box.right + off, box.bottom - off, box.right + off + t, box.bottom - off - arm);
            fill(box.right + off, box.bottom - off, box.right + off + arm, box.bottom - off - t);

            // Bars outside the middles: 4..7 px above the top, 7..10 below the bottom, as .ch-n / .ch-s / .ch-w / .ch-e place them
            Vec2F c = box.Center();
            float half = 13.0f * k;
            fill(c.x - half, box.top + 7.0f * k, c.x + half, box.top + 4.0f * k);
            fill(c.x - half, box.bottom - 7.0f * k, c.x + half, box.bottom - 10.0f * k);
            fill(box.left - 7.0f * k, c.y + half, box.left - 4.0f * k, c.y - half);
            fill(box.right + 7.0f * k, c.y + half, box.right + 10.0f * k, c.y - half);
        }
    }

    PipelineCellImage::PipelineCellImage(RefCounter* refCounter):
        PipelineImageView(refCounter)
    {}

    void PipelineCellImage::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        bool far = PipelineControls::IsFarView();
        RectF rect = layout->GetWorldRect();
        PipelineImageTheme::DrawCellPlate(rect, selected, !far);
        if (failed)
            PipelineDraw::Stroke(rect, PipelineDraw::Corners(7.0f), 0.0f, 1.0f, PipelineDraw::Gradient(PipelineTheme::Get().red));
        Widget::Draw();

        if (tools && !far)
        {
            PipelineDraw::Corners corners;
            corners.lt = 6.0f;
            corners.rb = 7.0f;
            PipelineDraw::Fill(RectF(rect.left + 1.0f, rect.top - 1.0f, rect.left + 41.0f, rect.top - 19.0f), corners, Color4(0, 0, 0, 158));
        }
    }

    PipelineRegionHandles::PipelineRegionHandles()
    {
        for (auto& handle : { mLeftTopHandle, mLeftHandle, mLeftBottomHandle, mTopHandle, mBottomHandle, mRightTopHandle,
                              mRightHandle, mRightBottomHandle })
        {
            handle->SetRegularDrawable(mmake<Sprite>(Color4(255, 255, 255, 0)));
            handle->SetHoverDrawable(mmake<Sprite>(Color4(255, 255, 255, 0)));
            handle->SetPressedDrawable(mmake<Sprite>(Color4(255, 255, 255, 0)));
        }
    }

    RectF PipelineRegionHandles::GetBox() const
    {
        const Basis& b = GetCurrentBasis();
        RectF frame(b.origin, b.origin + b.xv + b.yv);
        return RectF(Math::Min(frame.left, frame.right), Math::Max(frame.top, frame.bottom),
                     Math::Max(frame.left, frame.right), Math::Min(frame.top, frame.bottom));
    }

    void PipelineRegionHandles::DrawStyled()
    {
        CursorAreaEventsListener::OnDrawn();

        float k = PipelineImageTheme::HandleScale();
        Vec2F size(24.0f * k, 24.0f * k);
        for (auto& handle : { mLeftTopHandle, mLeftHandle, mLeftBottomHandle, mTopHandle, mBottomHandle, mRightTopHandle,
                              mRightHandle, mRightBottomHandle })
        {
            handle->SetDrawablesSize(size);
            handle->SetDrawablesSizePivot(size * 0.5f);
            handle->Draw();
        }

        PipelineImageTheme::DrawBoxHandles(GetBox());
    }
}
