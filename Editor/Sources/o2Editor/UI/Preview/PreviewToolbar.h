#pragma once

#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"

using namespace o2;

namespace Editor
{
    // Toolbars of the preview widgets and windows: a row of the editor panel look with buttons, lists and labels
    namespace PreviewToolbar
    {
        constexpr float height = 20.0f; // Height of a toolbar

        // Creates a toolbar stretched along the top of its parent
        Ref<HorizontalLayout> Create(const String& name = "toolbar");

        // Creates a group of controls without a back, put into a toolbar; its width follows the added controls
        Ref<HorizontalLayout> CreateGroup(const String& name);

        // Adds a button of the width, at least as wide as its caption
        Ref<Button> AddButton(const Ref<HorizontalLayout>& toolbar, const WString& caption, float width,
                              const Function<void()>& onClick);

        // Adds a drop down list of the width with the items, the first one selected
        Ref<DropDown> AddDropDown(const Ref<HorizontalLayout>& toolbar, const Vector<WString>& items, float width,
                                  const Function<void(int)>& onSelected);

        // Adds a label; zero width takes the rest of the toolbar
        Ref<Label> AddLabel(const Ref<HorizontalLayout>& toolbar, const WString& text, float width = 0.0f);

        // Adds a gap of the width
        void AddGap(const Ref<HorizontalLayout>& toolbar, float width);

        // Adds toggles of one choice, the first one on: a click turns its toggle on and the others off. A toggle is
        // as wide as its width, the last width goes to the rest, and at least as wide as its caption
        Vector<Ref<Toggle>> AddChoice(const Ref<HorizontalLayout>& toolbar, const Vector<WString>& captions,
                                      const Vector<float>& widths, const Function<void(int)>& onSelected);

        // Turns the toggle of the choice on and the others off, without the callback
        void SetChoice(const Vector<Ref<Toggle>>& toggles, int position);

        // Returns the position of the toggle on, -1 when there is none
        int GetChoice(const Vector<Ref<Toggle>>& toggles);
    }
}
