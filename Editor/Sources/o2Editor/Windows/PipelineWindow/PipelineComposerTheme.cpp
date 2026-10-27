#include "o2Editor/stdafx.h"
#include "PipelineComposerTheme.h"

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    namespace PipelineComposerTheme
    {
        using namespace PipelineControls;

        float TextWidth(const String& text, float size, bool mono /*= false*/, int weight /*= 400*/)
        {
            auto drawable = mmake<Text>(mono ? PipelineTheme::MonoFont() : PipelineTheme::Font(weight));
            drawable->height = PipelineTheme::Size(size);
            if (auto font = drawable->GetFont())
            {
                font->CheckCharacters(text, drawable->height);
                return Text::GetTextSize(text, font, drawable->height, Vec2F(), HorAlign::Left, VerAlign::Top, false).x;
            }
            return text.Length() * size * 0.55f;
        }

        Ref<Label> MakeText(const String& text, float size, const Color4& color)
        {
            auto label = o2UI.CreateLabel(text);
            StyleLabel(label, size, 400, color);
            label->horAlign = HorAlign::Left;
            label->verAlign = VerAlign::Middle;
            label->horOverflow = Label::HorOverflow::None;
            float width = Math::Ceil(TextWidth(text, size)) + 1.0f;
            label->layout->minWidth = width;
            label->layout->maxWidth = width;
            return label;
        }

        Ref<Label> MakeCaption(const String& text)
        {
            // .cmp-cap: 9.5 px in --fg-3 with a 1 px margin after it
            auto label = MakeText(text, 9.5f, PipelineTheme::Get().fg3);
            float width = label->layout->minWidth + 1.0f;
            label->layout->minWidth = width;
            label->layout->maxWidth = width;
            return label;
        }

        void SkinField(const Ref<EditBox>& edit, float size, bool mono)
        {
            auto& theme = PipelineTheme::Get();
            for (auto name : { "back", "border", "focus", "hover", "focused" })
            {
                if (edit->GetLayer(name))
                    edit->RemoveLayer(String(name));
            }

            // .cmp-name / .cmp-toolbar input: field fill, --border, 3 px, padding 2 / 4
            AddFillLayer(*edit, "back", theme.field, boxRadius, Layout::BothStretch(), -2.0f);
            AddBorderLayer(*edit, "border", theme.border, boxRadius, 1.0f, Layout::BothStretch(), -1.0f);
            auto focus = AddBorderLayer(*edit, "focus", theme.accent, boxRadius, 1.0f, Layout::BothStretch(), -0.5f);
            focus->transparency = 0.0f;
            edit->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.12f))->offStateAnimationSpeed = 0.5f;
            edit->SetViewLayout(Layout::BothStretch(5, 0, 5, 0));

            if (auto text = edit->GetTextDrawable())
            {
                text->SetFontAsset(AssetRef<FontAsset>(mono ? PipelineTheme::MonoFont() : PipelineTheme::Font(400)));
                text->height = PipelineTheme::Size(size);
                text->color = theme.fg;
                text->verAlign = VerAlign::Middle;
                text->linesDistanceCoef = 1.0f;
            }
        }

        Ref<EditBox> MakeNumberField(const String& text, float width)
        {
            auto edit = MakeEditBox(text, false);
            SkinField(edit, 11.0f, true);
            edit->SetFilterInteger();
            edit->layout->minWidth = width;
            edit->layout->maxWidth = width;
            edit->layout->minHeight = boxHeight;
            return edit;
        }

        Ref<Button> MakeToolButton(const String& caption, const String& icon, float fontSize, float width /*= 0.0f*/)
        {
            auto& theme = PipelineTheme::Get();
            auto button = mmake<Button>();
            AddFillLayer(*button, "back", theme.bg3, boxRadius, Layout::BothStretch(), -2.0f);
            AddBorderLayer(*button, "border", theme.border, boxRadius, 1.0f, Layout::BothStretch(), -1.0f);
            auto hover = AddBorderLayer(*button, "hover", theme.accent, boxRadius, 1.0f, Layout::BothStretch(), -0.5f);
            hover->transparency = 0.0f;
            button->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.12f))->offStateAnimationSpeed = 0.5f;

            // .cmp-dl button: padding 0 6, the 12 px icon 3 px before the caption
            float pad = icon.IsEmpty() ? 5.0f : 6.0f;
            float iconWidth = icon.IsEmpty() ? 0.0f : 12.0f + 3.0f;
            float textWidth = caption.IsEmpty() ? 0.0f : Math::Ceil(TextWidth(caption, fontSize));
            float natural = pad + iconWidth + textWidth + pad;
            float total = width > 0.0f ? width : natural;
            float left = Math::Max(pad, (total - iconWidth - textWidth) * 0.5f);
            if (!icon.IsEmpty())
            {
                auto sprite = mmake<VectorSprite>(icon);
                sprite->color = theme.fgDim;
                button->AddLayer("icon", sprite, Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(left, -6), Vec2F(left + 12, 6)));
            }
            auto text = MakeThemeText(caption, fontSize, 400, theme.fgDim, HorAlign::Left);
            text->dotsEngings = false;
            button->AddLayer("caption", text, Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(left + iconWidth, 0), Vec2F(left + iconWidth + textWidth + 2.0f, 0)));
            button->caption = caption;
            button->layout->minWidth = total;
            button->layout->maxWidth = total;
            button->layout->minHeight = boxHeight;
            return button;
        }

        Ref<Button> MakeRowAction(const String& icon, float iconSize, float width /*= 18.0f*/, float height /*= 20.0f*/)
        {
            auto& theme = PipelineTheme::Get();
            auto button = mmake<Button>();
            auto hover = AddFillLayer(*button, "hover", theme.bg2, boxRadius, Layout::BothStretch(), -1.0f);
            hover->transparency = 0.0f;
            button->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.12f))->offStateAnimationSpeed = 0.5f;
            auto sprite = mmake<VectorSprite>(icon);
            sprite->color = theme.fgDim;
            button->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(iconSize, iconSize)));
            button->layout->minWidth = width;
            button->layout->maxWidth = width;
            button->layout->minHeight = height;
            return button;
        }

        Ref<Toggle> MakeCheck(const String& caption, bool value, float& width)
        {
            auto& theme = PipelineTheme::Get();
            auto toggle = mmake<Toggle>();
            Layout box(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(0, -7.5f), Vec2F(15, 7.5f));
            AddBorderLayer(*toggle, "box", theme.fg3, 4.0f, 1.0f, box, -2.0f);
            auto on = AddFillLayer(*toggle, "value", theme.accent, 4.0f, box, -1.0f);
            on->transparency = value ? 1.0f : 0.0f;
            auto check = mmake<VectorSprite>("ui/pipeline/al_cmp_check.svg");
            check->color = theme.accentInk;
            toggle->AddLayer("check", check, Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(2.5f, -5), Vec2F(12.5f, 5)))->transparency = value ? 1.0f : 0.0f;
            auto anim = AnimationClip::EaseInOut("layer/value/transparency", 0.0f, 1.0f, 0.12f);
            *anim->AddTrack<float>("layer/check/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.12f);
            toggle->AddState("value", anim)->offStateAnimationSpeed = 1.0f;

            float textWidth = Math::Ceil(TextWidth(caption, 10.5f));
            auto text = MakeThemeText(caption, 10.5f, 400, theme.fgDim, HorAlign::Left);
            text->dotsEngings = false;
            toggle->AddLayer("caption", text, Layout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(19, 0), Vec2F(0, 0)));
            toggle->caption = caption;
            width = 15.0f + 4.0f + textWidth;
            toggle->layout->minWidth = width;
            toggle->layout->maxWidth = width;
            toggle->SetValue(value);
            return toggle;
        }

        Ref<Toggle> MakeIconToggle(const String& icon, bool value, float width /*= 22.0f*/)
        {
            auto& theme = PipelineTheme::Get();
            auto toggle = mmake<Toggle>();
            AddFillLayer(*toggle, "back", theme.bg3, boxRadius, Layout::BothStretch(), -2.0f);
            AddBorderLayer(*toggle, "border", theme.border, boxRadius, 1.0f, Layout::BothStretch(), -1.5f);
            auto onBorder = AddBorderLayer(*toggle, "valueBorder", theme.accent, boxRadius, 1.0f, Layout::BothStretch(), -1.0f);
            onBorder->transparency = value ? 1.0f : 0.0f;
            auto sprite = mmake<VectorSprite>(icon);
            sprite->color = theme.fgDim;
            toggle->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(12, 12)));
            auto onIcon = mmake<VectorSprite>(icon);
            onIcon->color = theme.accent;
            toggle->AddLayer("iconOn", onIcon, Layout::Based(BaseCorner::Center, Vec2F(12, 12)))->transparency = value ? 1.0f : 0.0f;
            auto anim = AnimationClip::EaseInOut("layer/valueBorder/transparency", 0.0f, 1.0f, 0.12f);
            *anim->AddTrack<float>("layer/iconOn/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.12f);
            toggle->AddState("value", anim)->offStateAnimationSpeed = 1.0f;
            toggle->layout->minWidth = width;
            toggle->layout->maxWidth = width;
            toggle->layout->minHeight = boxHeight;
            toggle->SetValue(value);
            return toggle;
        }

        void DashedFrame(const RectF& rect, const Color4& color, float dash /*= 3.0f*/, float gap /*= 2.0f*/)
        {
            float px = PipelineDraw::Pixel();
            float line = Math::Max(1.0f, px);
            auto side = [&](const Vec2F& a, const Vec2F& b)
            {
                Vec2F dir = b - a;
                float length = dir.Length();
                if (length <= 0.0f)
                    return;
                dir /= length;
                for (float t = 0.0f; t < length; t += dash + gap)
                {
                    float e = Math::Min(length, t + dash);
                    Vec2F p0 = a + dir * t, p1 = a + dir * e;
                    Vec2F n = dir.Perpendicular() * (line * 0.5f);
                    o2Render.DrawFilledPolygon({ p0 - n, p1 - n, p1 + n, p0 + n }, color);
                }
            };
            float h = line * 0.5f;
            float l = rect.left + h, r = rect.right - h, t = rect.top - h, b = rect.bottom + h;
            side(Vec2F(l + boxRadius, t), Vec2F(r - boxRadius, t));
            side(Vec2F(r, t - boxRadius), Vec2F(r, b + boxRadius));
            side(Vec2F(r - boxRadius, b), Vec2F(l + boxRadius, b));
            side(Vec2F(l, b + boxRadius), Vec2F(l, t - boxRadius));
        }

        void Checker(const RectF& rect, float cell, const Color4& first, const Color4& second)
        {
            // repeating-conic-gradient(first 0 25%, second 0 50%): the second colour on the top-right and bottom-left cells
            PipelineDraw::Checker(rect, PipelineDraw::Corners(0.0f), cell, first, second, false);
        }
    }
}
