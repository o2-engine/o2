#include "o2Editor/stdafx.h"
#include "PreviewToolbar.h"

#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"

namespace Editor
{
    namespace PreviewToolbar
    {
        namespace
        {
            void SetFixedWidth(const Ref<Widget>& widget, float width)
            {
                widget->layout->minWidth = width;
                widget->layout->maxWidth = width;
            }

            // At least as wide as the caption with the margins of a panel button
            float FitText(const WString& caption, const Ref<Font>& font, int height, float width)
            {
                if (!font)
                    return width;

                Vec2F size = Text::GetTextSize(caption, font, height, Vec2F(), HorAlign::Left, VerAlign::Top, false);
                return Math::Max(width, Math::Ceil(size.x) + 12.0f);
            }

            float FitCaption(const Ref<Widget>& widget, const WString& caption, float width)
            {
                auto text = widget->GetLayerDrawable<Text>("caption");
                return text ? FitText(caption, text->GetFont(), text->GetFontHeight(), width) : width;
            }

            // A group doesn't expand its controls and keeps their width, a toolbar gives the rest to its labels
            void AddToToolbar(const Ref<HorizontalLayout>& toolbar, const Ref<Widget>& widget, float width)
            {
                if (!toolbar->expandWidth)
                {
                    float total = toolbar->layout->GetMinWidth() + (toolbar->GetChildWidgets().IsEmpty() ? 0.0f : toolbar->spacing) + width;
                    SetFixedWidth(toolbar, total);
                }

                toolbar->AddChild(widget);
            }
        }

        Ref<HorizontalLayout> CreateGroup(const String& name)
        {
            auto group = mmake<HorizontalLayout>();
            group->name = name;
            group->baseCorner = BaseCorner::Left;
            group->expandHeight = true;
            group->expandWidth = false;
            group->spacing = 2.0f;
            SetFixedWidth(group, 0.0f);
            return group;
        }

        Ref<HorizontalLayout> Create(const String& name /*= "toolbar"*/)
        {
            auto toolbar = mmake<HorizontalLayout>();
            toolbar->name = name;
            *toolbar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, height, 0);
            toolbar->baseCorner = BaseCorner::Left;
            toolbar->expandHeight = true;
            toolbar->expandWidth = true;
            toolbar->spacing = 2.0f;
            toolbar->AddLayer("back", mmake<VectorSprite>("ui/UI4_small_panel_back.svg"), Layout::BothStretch(-5, -5, -4, -5));
            return toolbar;
        }

        Ref<Button> AddButton(const Ref<HorizontalLayout>& toolbar, const WString& caption, float width,
                              const Function<void()>& onClick)
        {
            auto button = o2UI.CreateButton(caption, onClick);
            button->name = (String)caption;
            width = FitCaption(button, caption, width);
            SetFixedWidth(button, width);
            AddToToolbar(toolbar, button, width);
            return button;
        }

        Ref<DropDown> AddDropDown(const Ref<HorizontalLayout>& toolbar, const Vector<WString>& items, float width,
                                  const Function<void(int)>& onSelected)
        {
            auto dropDown = o2UI.CreateDropdown();
            dropDown->AddItems(items);
            if (!items.IsEmpty())
                dropDown->SelectItemAt(0);

            dropDown->onSelectedPos = onSelected;
            SetFixedWidth(dropDown, width);
            AddToToolbar(toolbar, dropDown, width);
            return dropDown;
        }

        Ref<Label> AddLabel(const Ref<HorizontalLayout>& toolbar, const WString& text, float width /*= 0.0f*/)
        {
            auto label = o2UI.CreateLabel(text);
            label->horAlign = HorAlign::Left;
            label->horOverflow = Label::HorOverflow::Cut;
            if (width > 0.0f)
            {
                width = FitText(text, label->GetFont(), label->GetHeight(), width);
                SetFixedWidth(label, width);
            }

            AddToToolbar(toolbar, label, width);
            return label;
        }

        void AddGap(const Ref<HorizontalLayout>& toolbar, float width)
        {
            auto gap = mmake<Widget>();
            gap->name = "gap";
            SetFixedWidth(gap, width);
            AddToToolbar(toolbar, gap, width);
        }

        Vector<Ref<Toggle>> AddChoice(const Ref<HorizontalLayout>& toolbar, const Vector<WString>& captions,
                                      const Vector<float>& widths, const Function<void(int)>& onSelected)
        {
            Vector<Ref<Toggle>> toggles;
            for (int i = 0; i < captions.Count(); i++)
            {
                auto toggle = o2UI.CreateWidget<Toggle>("preview toolbar toggle");
                toggle->name = (String)captions[i];
                toggle->caption = captions[i];
                float width = FitCaption(toggle, captions[i], i < widths.Count() ? widths[i] : widths.Last());
                SetFixedWidth(toggle, width);
                AddToToolbar(toolbar, toggle, width);
                toggles.Add(toggle);
            }

            Vector<WeakRef<Toggle>> weak;
            for (auto& toggle : toggles)
                weak.Add(WeakRef<Toggle>(toggle));

            for (int i = 0; i < toggles.Count(); i++)
            {
                toggles[i]->onToggleByUser = [weak, i, onSelected](bool) {
                    for (int j = 0; j < weak.Count(); j++)
                    {
                        auto toggle = weak[j].Lock();
                        if (toggle && toggle->GetValue() != (j == i))
                            toggle->SetValue(j == i);
                    }

                    if (onSelected)
                        onSelected(i);
                };
            }

            SetChoice(toggles, 0);
            return toggles;
        }

        void SetChoice(const Vector<Ref<Toggle>>& toggles, int position)
        {
            for (int i = 0; i < toggles.Count(); i++)
            {
                if (toggles[i]->GetValue() != (i == position))
                    toggles[i]->SetValue(i == position);
            }
        }

        int GetChoice(const Vector<Ref<Toggle>>& toggles)
        {
            for (int i = 0; i < toggles.Count(); i++)
            {
                if (toggles[i]->GetValue())
                    return i;
            }

            return -1;
        }
    }
}
