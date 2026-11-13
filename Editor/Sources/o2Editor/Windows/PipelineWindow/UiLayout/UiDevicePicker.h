#pragma once

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

namespace Editor
{
    class PipelineEditor;
    class UiDeviceMenuPanel;

    // -----------------------------------------------------------------------------------------------
    // The screen an AI UI layout is shown on (port of AssetsLine DevicePicker.tsx): the kit's device (the
    // one it is laid out for - the default), the design itself, the reference picture it rebuilds, another
    // device or a custom W x H, turned if asked. The layout meets every screen the same way - fitted, the
    // room left over added to the canvas - so another screen changes the aspect ratio only: the list names
    // the devices by it, and every row says the canvas it gives in the layout's pixels. A field on the card
    // ("iPhone 16 · 1179×2556 · +460 px height"), a chip on the workspace's toolbar, and its menu. It writes
    // the node's view keys uiDevice, uiDeviceCustom and uiLandscape (no undo step, no run). The menu warns
    // about kit graphics drawn larger than their pixels on that screen
    // -----------------------------------------------------------------------------------------------
    class UiDevicePicker : public UiWsPanel
    {
    public:
        bool chip = false;     // The toolbar's chip (no border, 16 px icon), else the card's field
        bool onCanvas = false; // It sits on the editor's canvas (a card): its menu opens in screen space at it

    public:
        // Default constructor
        explicit UiDevicePicker(RefCounter* refCounter);

        // Binds it to a node of the editor (the card): it reads the node itself and follows its changes
        void Bind(const Ref<PipelineEditor>& editor, const String& nodeId);

        // Binds it to a context read by another (the workspace's, the card's)
        void Bind(const Ref<UiWsContext>& context);

        // Reads the node again (the card calls it when the node changed)
        void Refresh();

        // Returns the width it wants for its text
        float GetPreferredWidth() const;

        // Opens or closes the menu
        void ToggleMenu();

        // Returns true while the menu is open
        bool IsMenuOpen() const;

        // Returns the menu's panel (tests), null before the first open
        Ref<UiDeviceMenuPanel> GetMenuPanel() const;

        // Picks a screen as the menu does: kit, design, reference, custom or a device id
        void Pick(const String& device);

        // Turns the screen as the menu's button does (not the design nor the reference)
        void ToggleLandscape();

        // Sets one side of the custom screen (w or h) as its fields do on Enter / leaving them; the custom screen becomes
        // the one shown. The same value as stored changes nothing
        void SetCustom(const String& key, double value);

        // The screen picked: what the menu marks (the reference without one is the kit's device)
        String Chosen() const;

        // Polls the bound node for changes (card mode)
        void Update(float dt) override;

    protected:
        Ref<UiDeviceMenuPanel> mMenuPanel;
        Ref<UiWsPopup>         mMenu;
        bool                   mOwnContext = false;
        float                  mPoll = 0.0f;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };

    // -----------------------------------------------------------------------------------------------
    // The screen picker's menu: the hint and the turn button, the kit's device, the design and the
    // reference, phones, tablets, desktops (each named with its aspect ratio and the canvas it gives),
    // the custom screen's W H fields, the canvas the layout gets, what the screen adds, the comparison
    // note and the soft graphics
    // -----------------------------------------------------------------------------------------------
    class UiDeviceMenuPanel : public UiWsPanel
    {
    public:
        WeakRef<UiDevicePicker> picker;

    public:
        // Default constructor
        explicit UiDeviceMenuPanel(RefCounter* refCounter);

        // The height it wants
        float GetContentHeight() const;

        // Builds the custom screen's fields with the current values
        void SyncFields();

        // Returns the custom screen's field of a key: w or h
        Ref<EditBox> GetCustomField(const String& key) const;

        // The text of a list row (tests): its name with the aspect ratio, and what it says on the right
        String RowText(const String& id) const;

        void OnScrolled(float scroll) override;

    protected:
        Ref<EditBox> mFields[2];
        float        mScroll = 0.0f;
        float        mListHeight = 0.0f;
        Map<String, String> mRowTexts; // id -> the row's words, as drawn last

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };
}
