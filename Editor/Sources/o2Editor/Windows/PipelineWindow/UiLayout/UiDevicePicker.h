#pragma once

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

namespace Editor
{
    class PipelineEditor;
    class UiDeviceMenuPanel;

    // -----------------------------------------------------------------------------------------------
    // The screen an AI UI layout is previewed on (port of AssetsLine DevicePicker.tsx): the design, the
    // reference picture it rebuilds, one of 14 devices or a custom W x H x DPI, turned if asked, met by
    // DPI (physical size) or by screen (Unity's Canvas Scaler) - a field on the card, a chip on the
    // workspace's toolbar, and its menu. It writes the node's view keys uiDevice, uiDeviceCustom,
    // uiLandscape and uiScale (no undo step, no run). The menu warns about kit graphics drawn larger than
    // their pixels on that screen
    // -----------------------------------------------------------------------------------------------
    class UiDevicePicker : public UiWsPanel
    {
    public:
        bool chip = false; // The toolbar's chip (no border, 16 px icon), else the card's field

    public:
        // Default constructor
        explicit UiDevicePicker(RefCounter* refCounter);

        // Binds it to a node of the editor (the card): it reads the node itself and follows its changes
        void Bind(const Ref<PipelineEditor>& editor, const String& nodeId);

        // Binds it to the workspace's context
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

        // Picks a screen as the menu does: design, reference, custom or a device id
        void Pick(const String& device);

        // Sets the scale mode as the menu's segments do: dpi | fit
        void SetScale(const String& scale);

        // Turns the screen as the menu's button does
        void ToggleLandscape();

        // Sets one side of the custom screen (w, h or dpi) as its fields do; the custom screen becomes the one shown
        void SetCustom(const String& key, double value);

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
    // The screen picker's menu: By DPI | By screen and the turn button, the design and the reference,
    // phones, tablets, desktops, the custom screen's W H DPI fields, the canvas the layout gets and the
    // soft graphics
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

        // Returns the custom screen's field of a key: w, h or dpi
        Ref<EditBox> GetCustomField(const String& key) const;

        void OnScrolled(float scroll) override;

    protected:
        Ref<EditBox> mFields[3];
        float        mScroll = 0.0f;
        float        mListHeight = 0.0f;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };
}
