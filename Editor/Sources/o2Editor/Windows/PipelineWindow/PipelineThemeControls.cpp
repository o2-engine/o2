#include "o2Editor/stdafx.h"
#include "PipelineControls.h"

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/CustomList.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    namespace PipelineControls
    {
        Color4 TextColor() { return PipelineTheme::Get().fg; }
        Color4 DimTextColor() { return PipelineTheme::Get().fgDim; }
        Color4 AccentColor() { return PipelineTheme::Get().accent; }

        static Color4 Opaque(const Color4& color)
        {
            return Color4(color.r, color.g, color.b, 255);
        }

        Ref<Text> MakeThemeText(const String& text, float size, int weight, const Color4& color,
                                HorAlign horAlign /*= HorAlign::Left*/, VerAlign verAlign /*= VerAlign::Middle*/)
        {
            auto drawable = mmake<Text>(PipelineTheme::Font(weight));
            drawable->text = text;
            drawable->height = PipelineTheme::Size(size);
            drawable->color = color;
            drawable->horAlign = horAlign;
            drawable->verAlign = verAlign;
            drawable->dotsEngings = true;
            return drawable;
        }

        void StyleLabel(const Ref<Label>& label, float size, int weight, const Color4& color)
        {
            label->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::Font(weight)));
            label->height = PipelineTheme::Size(size);
            label->color = color;
        }

        Ref<WidgetLayer> AddFillLayer(Widget& widget, const String& name, const Color4& color, float radius,
                                      const Layout& layout /*= Layout::BothStretch()*/, float depth /*= 0.0f*/)
        {
            auto rect = mmake<PipelineRoundedRect>();
            rect->radius = radius;
            rect->roundBottom = true;
            rect->color = Opaque(color);
            auto layer = widget.AddLayer(name, rect, layout, depth);
            layer->transparency = color.a / 255.0f;
            return layer;
        }

        Ref<WidgetLayer> AddBorderLayer(Widget& widget, const String& name, const Color4& color, float radius, float width /*= 1.0f*/,
                                        const Layout& layout /*= Layout::BothStretch()*/, float depth /*= 0.0f*/)
        {
            auto frame = mmake<PipelineRoundedFrame>();
            frame->radius = radius;
            frame->width = width;
            frame->color = Opaque(color);
            auto layer = widget.AddLayer(name, frame, layout, depth);
            layer->transparency = color.a / 255.0f;
            return layer;
        }

        // A state fading a layer from hidden to the strength of its colour
        static void AddFadeState(Widget& widget, const String& state, const String& layer, float strength, float duration)
        {
            widget.AddState(state, AnimationClip::EaseInOut("layer/" + layer + "/transparency", 0.0f, strength, duration))
                ->offStateAnimationSpeed = 0.5f;
        }

        Ref<Label> MakeLabel(const String& text, bool dim /*= false*/)
        {
            auto& theme = PipelineTheme::Get();
            auto label = o2UI.CreateLabel(text);
            label->horAlign = HorAlign::Left;
            label->layout->minHeight = 16;
            StyleLabel(label, smallFont, 400, dim ? theme.fg3 : theme.fgDim);
            if (dim)
                label->horOverflow = Label::HorOverflow::Wrap;
            return label;
        }

        void SkinField(const Ref<EditBox>& edit, bool multiline)
        {
            auto& theme = PipelineTheme::Get();
            for (auto name : { "back", "hover", "focus", "focused", "border" })
            {
                if (edit->GetLayer(name))
                    edit->RemoveLayer(String(name));
            }

            AddFillLayer(*edit, "back", theme.field, fieldRadius, Layout::BothStretch(), -2.0f);
            AddBorderLayer(*edit, "border", theme.line, fieldRadius, 1.0f, Layout::BothStretch(), -1.0f);
            auto focus = AddBorderLayer(*edit, "focus", theme.accent, fieldRadius, 1.0f, Layout::BothStretch(), -0.5f);
            focus->transparency = 0.0f;
            AddFadeState(*edit, "focused", "focus", 1.0f, 0.12f);

            // AssetsLine pads a field 6 px x 8 px and sets its text at 1.4 lines
            edit->SetClippingLayout(Layout::BothStretch(1, 1, 1, 1));
            // o2 puts the cap height at the top of a line, a browser centres the line box: 4 px lower matches its first line
            edit->SetViewLayout(multiline ? Layout::BothStretch(8, 3, 9, 11) : Layout::BothStretch(8, 0, 9, 0));
            edit->SetSelectionColor(PipelineTheme::Alpha(theme.accent, 0.35f));

            if (auto text = edit->GetTextDrawable())
            {
                text->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::Font(400)));
                text->height = PipelineTheme::Size(bodyFont);
                text->color = theme.fg;
                text->horAlign = HorAlign::Left;
                text->verAlign = multiline ? VerAlign::Top : VerAlign::Middle;
                if (auto font = text->GetFont())
                {
                    float natural = font->GetLineHeightPx(text->height);
                    text->linesDistanceCoef = natural > 0.0f ? (bodyFont * 1.4f) / natural : 1.0f;
                }
            }

            if (auto caret = edit->GetCaretDrawable())
            {
                *caret = Sprite();
                caret->size = Vec2F(1, 15);
                caret->pivot = Vec2F(0, 0.2f);
                caret->color = theme.fg;
            }
        }

        Ref<EditBox> MakeEditBox(const String& text, bool multiline, const String& placeholder /*= ""*/)
        {
            auto edit = mmake<PipelineField>();
            edit->SetCaretBlinkingDelay(1.15f);
            edit->SetMultiLine(multiline);
            if (multiline)
            {
                edit->SetWordWrap(true);
                edit->SetEnableScrollsHiding(true);
            }
            else
                edit->layout->minSize = Vec2F(30, 20);

            SkinField(edit, multiline);
            edit->SetPlaceholder(placeholder);
            edit->SetText(text);
            return edit;
        }

        Ref<DropDown> MakeDropDown(const Vector<String>& items, const String& value)
        {
            auto& theme = PipelineTheme::Get();
            auto dropdown = o2UI.CreateDropdown();
            for (auto name : { "back", "hover", "pressed", "arrow" })
            {
                if (dropdown->GetLayer(name))
                    dropdown->RemoveLayer(String(name));
            }

            AddFillLayer(*dropdown, "back", theme.field, controlRadius, Layout::BothStretch(), -2.0f);
            AddBorderLayer(*dropdown, "border", theme.line, controlRadius, 1.0f, Layout::BothStretch(), -1.0f);
            auto hover = AddFillLayer(*dropdown, "hover", theme.active, controlRadius, Layout::BothStretch(), -1.5f);
            hover->transparency = 0.0f;
            AddFadeState(*dropdown, "hover", "hover", theme.active.a / 255.0f, 0.12f);
            auto arrow = mmake<VectorSprite>("ui/pipeline/chevron_down.svg");
            arrow->color = theme.fgDim;
            dropdown->AddLayer("arrow", arrow, Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-17, -5), Vec2F(-7, 5)));
            dropdown->SetClippingLayout(Layout::BothStretch(8, 0, 22, 0));

            if (auto sample = DynamicCast<Label>(dropdown->GetItemSample()))
            {
                StyleLabel(sample, bodyFont, 400, theme.fg);
                dropdown->SetItemSample(sample);
            }

            if (auto list = dropdown->GetListView())
            {
                if (auto back = list->GetLayer("back"))
                {
                    auto rect = mmake<PipelineRoundedRect>();
                    rect->radius = controlRadius;
                    rect->roundBottom = true;
                    rect->color = theme.cardSolid;
                    back->SetDrawable(rect);
                    back->layout = Layout::BothStretch();
                }
                AddBorderLayer(*list, "border", theme.border, controlRadius, 1.0f, Layout::BothStretch(), 1.0f);
                auto hoverRect = mmake<PipelineRoundedRect>();
                hoverRect->radius = 6.0f;
                hoverRect->roundBottom = true;
                hoverRect->color = PipelineTheme::Alpha(theme.active, 1.0f);
                list->SetHoverDrawable(hoverRect);
                auto selectRect = mmake<PipelineRoundedRect>();
                selectRect->radius = 6.0f;
                selectRect->roundBottom = true;
                selectRect->color = theme.accentSoft;
                list->SetSelectionDrawable(selectRect);
            }

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

        float TextWidth(const String& text, float size, int weight /*= 400*/)
        {
            auto drawable = MakeThemeText(text, size, weight, Color4::White());
            if (auto font = drawable->GetFont())
                return Text::GetTextSize(text, font, drawable->height, Vec2F(), HorAlign::Left, VerAlign::Top, false).x;
            return text.Length() * size * 0.55f;
        }

        Ref<Toggle> MakeHeadSwitch(const String& caption, bool value)
        {
            auto toggle = MakeCheckbox(caption, value);
            if (auto layer = toggle->GetLayer("caption"))
            {
                if (auto text = DynamicCast<Text>(layer->GetDrawable()))
                {
                    text->height = PipelineTheme::Size(smallFont);
                    text->color = PipelineTheme::Get().fgDim;
                    text->dotsEngings = false;
                }
                layer->layout = Layout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(switchWidth + 6.0f, 0), Vec2F(0, 0));
            }
            float width = switchWidth + 6.0f + Math::Ceil(TextWidth(caption, smallFont));
            toggle->layout->minWidth = width;
            toggle->layout->maxWidth = width;
            toggle->layout->minHeight = 22.8f;
            toggle->layout->maxHeight = 22.8f;
            return toggle;
        }

        Ref<Toggle> MakeCropToggle(bool value)
        {
            auto& theme = PipelineTheme::Get();
            auto toggle = mmake<Toggle>();
            AddBorderLayer(*toggle, "border", theme.line, fieldRadius, 1.0f, Layout::BothStretch(), -2.0f);
            auto on = AddFillLayer(*toggle, "value", theme.bg3, fieldRadius, Layout::BothStretch(), -1.5f);
            on->transparency = value ? 1.0f : 0.0f;
            auto onBorder = AddBorderLayer(*toggle, "valueBorder", theme.accent, fieldRadius, 1.0f, Layout::BothStretch(), -1.0f);
            onBorder->transparency = value ? 1.0f : 0.0f;
            auto anim = AnimationClip::EaseInOut("layer/value/transparency", 0.0f, 1.0f, 0.12f);
            *anim->AddTrack<float>("layer/valueBorder/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.12f);
            toggle->AddState("value", anim)->offStateAnimationSpeed = 1.0f;

            auto glyph = mmake<VectorSprite>("ui/pipeline/al_crop.svg");
            glyph->color = theme.fg3;
            toggle->AddLayer("icon", glyph, Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(9, -5.5f), Vec2F(20, 5.5f)));
            auto caption = MakeThemeText("Crop", smallFont, 400, value ? theme.fg : theme.fgDim);
            caption->dotsEngings = false;
            toggle->AddLayer("caption", caption, Layout::BothStretch(26, 0, 0, 0));
            toggle->caption = "Crop";
            float width = 9.0f + 11.0f + 6.0f + Math::Ceil(TextWidth("Crop", smallFont)) + 9.0f;
            toggle->layout->minWidth = width;
            toggle->layout->maxWidth = width;
            toggle->layout->minHeight = 22.8f;
            toggle->layout->maxHeight = 22.8f;
            toggle->SetValue(value);
            return toggle;
        }

        Ref<Toggle> MakeCheckbox(const String& caption, bool value)
        {
            auto& theme = PipelineTheme::Get();
            auto toggle = mmake<Toggle>();
            toggle->layout->minHeight = switchHeight;

            // .tg: a 32x18 track, white 14% off / accent on, and a 14 px knob that moves from 2 to 16
            Layout track(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(0, -switchHeight * 0.5f), Vec2F(switchWidth, switchHeight * 0.5f));
            Color4 offTrack = theme.dark ? Color4(255, 255, 255, 36) : theme.fg3;
            AddFillLayer(*toggle, "track", offTrack, switchHeight * 0.5f, track, -2.0f);
            auto on = AddFillLayer(*toggle, "trackOn", theme.accent, switchHeight * 0.5f, track, -1.0f);
            on->transparency = value ? 1.0f : 0.0f;

            auto knob = [&](const String& name, float x, const Color4& color)
            {
                auto layer = AddFillLayer(*toggle, name, color, 7.0f,
                                          Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(x, -7), Vec2F(x + 14, 7)), 0.0f);
                return layer;
            };
            knob("knobOff", 2.0f, theme.dark ? theme.fgDim : theme.cardSolid)->transparency = value ? 0.0f : 1.0f;
            knob("knobOn", 16.0f, theme.dark ? Color4::White() : theme.accentInk)->transparency = value ? 1.0f : 0.0f;

            auto anim = AnimationClip::EaseInOut("layer/trackOn/transparency", 0.0f, 1.0f, 0.16f);
            *anim->AddTrack<float>("layer/knobOn/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.16f);
            *anim->AddTrack<float>("layer/knobOff/transparency") = AnimationTrack<float>::EaseInOut(1.0f, 0.0f, 0.16f);
            toggle->AddState("value", anim)->offStateAnimationSpeed = 1.0f;

            toggle->AddLayer("caption", MakeThemeText(caption, bodyFont, 400, theme.fg),
                             Layout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(switchWidth + rowGap, 0), Vec2F(0, 0)));
            toggle->caption = caption;
            toggle->SetValue(value);
            return toggle;
        }

        // .actions button: r 9, 5 / 8 padding, 11.5 px; the three looks differ in fill, border and text only
        static Ref<Button> MakeActionButton(const String& caption, const Color4& fill, const Color4& border, const Color4& text,
                                            int weight, const Color4& hoverFill)
        {
            auto button = mmake<Button>();
            button->layout->minSize = Vec2F(20, 20);
            if (fill.a > 0)
                AddFillLayer(*button, "back", fill, fieldRadius, Layout::BothStretch(), -2.0f);
            if (border.a > 0)
                AddBorderLayer(*button, "border", border, fieldRadius, 1.0f, Layout::BothStretch(), -1.0f);
            auto hover = AddFillLayer(*button, "hover", hoverFill, fieldRadius, Layout::BothStretch(), -1.5f);
            float strength = hover->transparency;
            hover->transparency = 0.0f;
            AddFadeState(*button, "hover", "hover", strength, 0.12f);
            button->AddLayer("caption", MakeThemeText(caption, bodyFont, weight, text, HorAlign::Middle), Layout::BothStretch(8, 0, 8, 0));
            button->caption = caption;
            return button;
        }

        Ref<Button> MakeButton(const String& caption)
        {
            auto& theme = PipelineTheme::Get();
            return MakeActionButton(caption, theme.field, theme.line, theme.fg, 400, theme.active);
        }

        Ref<Button> MakeGhostButton(const String& caption)
        {
            auto& theme = PipelineTheme::Get();
            return MakeActionButton(caption, Color4(0, 0, 0, 0), theme.line, theme.fgDim, 400, theme.active);
        }

        Ref<Button> MakePrimaryButton(const String& caption)
        {
            auto& theme = PipelineTheme::Get();
            return MakeActionButton(caption, theme.accent, Color4(0, 0, 0, 0), theme.accentInk, 600,
                                    Color4(theme.accentHover.r, theme.accentHover.g, theme.accentHover.b, 255));
        }

        Ref<Button> MakeIconButton(const String& icon, const Color4& iconColor, const Color4& backColor)
        {
            auto& theme = PipelineTheme::Get();
            auto button = mmake<Button>();
            button->layout->minSize = Vec2F(20, 20);
            if (backColor.a > 0)
                AddFillLayer(*button, "back", theme.dark ? Color4(33, 36, 46, backColor.a) : Color4(251, 251, 251, backColor.a), 6.0f,
                             Layout::BothStretch(-2, -2, -2, -2), -2.0f);
            auto hover = AddFillLayer(*button, "hover", theme.active, 6.0f, Layout::BothStretch(-2, -2, -2, -2), -1.0f);
            hover->transparency = 0.0f;
            AddFadeState(*button, "hover", "hover", theme.active.a / 255.0f, 0.12f);
            auto pressed = AddFillLayer(*button, "pressed", theme.active, 6.0f, Layout::BothStretch(-2, -2, -2, -2), -0.9f);
            pressed->transparency = 0.0f;
            AddFadeState(*button, "pressed", "pressed", theme.active.a / 255.0f, 0.05f);
            auto sprite = mmake<VectorSprite>(icon);
            sprite->color = iconColor;
            button->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
            return button;
        }

        Ref<Toggle> MakeSegment(const String& caption, bool value)
        {
            auto& theme = PipelineTheme::Get();
            auto toggle = mmake<Toggle>();
            toggle->layout->minSize = Vec2F(20, 18);
            // .seg-ind: the active option's pill, --active over the field
            auto pill = AddFillLayer(*toggle, "value", theme.dark ? Color4(255, 255, 255, 23) : Color4(96, 125, 139, 41), 7.0f,
                                     Layout::BothStretch(), -1.0f);
            float strength = pill->transparency;
            pill->transparency = value ? strength : 0.0f;
            AddFadeState(*toggle, "value", "value", strength, 0.18f);

            auto text = MakeThemeText(caption, smallFont, 400, value ? theme.fg : theme.fgDim, HorAlign::Middle);
            toggle->AddLayer("caption", text, Layout::BothStretch(2, 0, 2, 0));
            toggle->caption = caption;
            toggle->SetValue(value);
            return toggle;
        }

        void SkinSegmentedRow(Widget& row)
        {
            auto& theme = PipelineTheme::Get();
            AddFillLayer(row, "segBack", theme.field, 9.0f, Layout::BothStretch(-3, -3, -3, -3), -2.0f);
            AddBorderLayer(row, "segBorder", theme.line, 9.0f, 1.0f, Layout::BothStretch(-3, -3, -3, -3), -1.0f);
        }

        Ref<IRectDrawable> MakeMenuHover()
        {
            auto& theme = PipelineTheme::Get();
            auto hover = mmake<PipelineRoundedRect>();
            hover->radius = 6.0f;
            hover->roundBottom = true;
            hover->color = theme.dark ? Color4(255, 255, 255, 255) : Color4(96, 125, 139, 255);
            return hover;
        }

        Ref<IRectDrawable> MakeMenuCheck()
        {
            auto check = mmake<VectorSprite>("ui/pipeline/al_check.svg");
            check->color = PipelineTheme::Get().accent;
            return check;
        }

        Ref<IRectDrawable> MakeMenuSearchIcon()
        {
            auto icon = mmake<VectorSprite>("ui/pipeline/al_search.svg");
            icon->color = PipelineTheme::Get().fg3;
            return icon;
        }

        Ref<IRectDrawable> MakeMenuLine()
        {
            auto& theme = PipelineTheme::Get();
            auto line = mmake<PipelineRoundedRect>();
            line->radius = 0.0f;
            line->color = theme.line;
            return line;
        }

        static void SkinMenuItem(const Ref<ContextMenuItem>& item)
        {
            auto& theme = PipelineTheme::Get();
            if (auto caption = item->GetLayerDrawable<Text>("basic/caption"))
            {
                caption->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::Font(400)));
                caption->height = PipelineTheme::Size(12.0f);
                caption->color = theme.fg;
            }
            if (auto shortcut = item->GetLayerDrawable<Text>("basic/shortcut"))
            {
                shortcut->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
                shortcut->height = PipelineTheme::Size(10.5f);
                shortcut->color = theme.fg3;
            }
            if (auto subIcon = item->GetLayer("basic/subIcon"))
            {
                auto chevron = mmake<VectorSprite>("ui/pipeline/chevron_right.svg");
                chevron->color = theme.fg3;
                subIcon->SetDrawable(chevron);
                subIcon->layout = Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-16, -5), Vec2F(-6, 5));
            }
            if (auto check = item->GetLayerDrawableBasedOn<IRectDrawable>("check"))
                check->color = theme.accent;
        }

        void SkinMenu(const Ref<ContextMenu>& menu)
        {
            if (!menu)
                return;

            auto& theme = PipelineTheme::Get();
            if (auto back = menu->GetLayer("back"))
            {
                back->SetDrawable(mmake<PipelineMenuBack>());
                back->layout = Layout::BothStretch(-2, -2, -2, -2);
            }

            auto hover = mmake<PipelineRoundedRect>();
            hover->radius = 6.0f;
            hover->roundBottom = true;
            hover->color = theme.dark ? Color4(255, 255, 255, 23) : Color4(96, 125, 139, 31);
            menu->SetSelectionDrawable(hover);
            menu->SetSelectionDrawableLayout(Layout::BothStretch(2, 0, 2, 0));

            if (auto separator = menu->GetSeparatorSample())
            {
                if (auto line = separator->GetLayer("line"))
                {
                    auto rect = mmake<PipelineRoundedRect>();
                    rect->radius = 0.0f;
                    rect->color = Color4(theme.line.r, theme.line.g, theme.line.b, 255);
                    line->SetDrawable(rect);
                    line->layout = Layout::HorStretch(VerAlign::Middle, 8, 8, 1, 0);
                    line->transparency = theme.line.a / 255.0f;
                }
            }

            SkinMenuItem(menu->GetItemSample());
            if (auto items = menu->GetItemsLayout())
            {
                for (auto& child : items->GetChildWidgets())
                {
                    if (auto item = DynamicCast<ContextMenuItem>(child))
                        SkinMenuItem(item);
                }
            }

            menu->onSubMenuCreated = [](const Ref<ContextMenu>& subMenu) { SkinMenu(subMenu); };
        }

        Ref<HorizontalLayout> MakeRow(const String& label, const Ref<Widget>& control, float labelWidth /*= rowLabelWidth*/)
        {
            auto row = mmake<HorizontalLayout>();
            row->spacing = rowGap;
            row->expandWidth = true;
            row->expandHeight = true;
            row->baseCorner = BaseCorner::Left;

            auto caption = o2UI.CreateLabel(label);
            caption->horAlign = HorAlign::Left;
            StyleLabel(caption, bodyFont, 400, PipelineTheme::Get().fgDim);
            caption->layout->minWidth = labelWidth;
            caption->layout->maxWidth = labelWidth;
            row->AddChild(caption);
            if (control)
                row->AddChild(control);

            return row;
        }
    }

    PipelineField::PipelineField(RefCounter* refCounter):
        EditBox(refCounter)
    {}

    void PipelineField::SetPlaceholder(const String& text)
    {
        if (text.IsEmpty())
        {
            mPlaceholder = nullptr;
            return;
        }

        if (!mPlaceholder)
            mPlaceholder = PipelineControls::MakeThemeText(text, PipelineControls::bodyFont, 400, PipelineTheme::Get().fg3);
        mPlaceholder->text = text;
        mPlaceholder->dotsEngings = !mMultiLine;
        mPlaceholder->wordWrap = mMultiLine;
        mPlaceholder->verAlign = mMultiLine ? VerAlign::Top : VerAlign::Middle;
    }

    void PipelineField::Draw()
    {
        EditBox::Draw();

        if (!mPlaceholder || !mResEnabledInHierarchy || mIsClipped || !GetText().IsEmpty() || IsFocused())
            return;

        if (PipelineControls::IsFarView())
            return;

        RectF rect = layout->GetWorldRect();
        mPlaceholder->SetRect(RectF(rect.left + 8.0f, rect.top - (mMultiLine ? 11.0f : 0.0f), rect.right - 9.0f, rect.bottom + (mMultiLine ? 3.0f : 0.0f)));
        mPlaceholder->transparency = GetResTransparency();
        mPlaceholder->Draw();
    }
}
