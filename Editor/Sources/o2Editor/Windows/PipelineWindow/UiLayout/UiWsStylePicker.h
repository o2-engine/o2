#pragma once

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

namespace Editor
{
    // -----------------------------------------------------------------------------------------------
    // The text style of a block picked from previews (port of AssetsLine uiLayout/StylePicker.tsx): every
    // style of the kit drawn by the layout's own renderer - its font, fill, outlines, depth, shadows - on the
    // Text style card's dark paper, with the block's own words when it has a short text, so the choice is
    // made by the look and not by a name. "Auto" shows the style the block takes by its role. Each preview's
    // screen is fitted to the lettering (its effects included), so the lettering fills the plate
    // -----------------------------------------------------------------------------------------------
    namespace UiWsStylePicker
    {
        // Draws a style's preview on its plate: the dark paper (radius), the lettering fitted inside the padding
        void DrawPreview(const UiDoc::TextStyle& style, const String& text, const RectF& plate, float radius, float pad);

        // Draws the inspector's button: a small preview, the style's name (a name the kit lacks in yellow, "auto · <its
        // style>" without one) and the chevron
        void DrawButton(const RectF& rect, const UiDoc::UiNode& node, const UiDoc::UiKit* kit, bool hover, bool enabled);

        // How many previews are kept drawn (tests)
        int CachedPreviews();
    }

    // -----------------------------------------------------------------------------------------------
    // The style picker's list: "Auto" first (the style the block takes by its role), then every style of
    // the kit, each a preview with its name, role, font and size; the block's choice marked. A pick closes
    // the popover. Rows scroll when they do not fit
    // -----------------------------------------------------------------------------------------------
    class UiWsStyleListPanel : public UiWsPanel
    {
    public:
        String                        blockId; // The block whose style is picked
        Function<void(const String&)> onPick;  // The style's name, "" = auto

    public:
        explicit UiWsStyleListPanel(RefCounter* refCounter): UiWsPanel(refCounter) {}

        // The height the list wants
        float ContentHeight() const;

        // The rows' ids in order (tests): auto, then the styles' names
        Vector<String> RowIds() const;

        void OnScrolled(float scroll) override;

    protected:
        float mScroll = 0.0f;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;

        // The block shown
        const UiDoc::UiNode* Block() const;
    };
}
