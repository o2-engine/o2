#pragma once

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

namespace Editor
{
    // -------------------------------------------------------------------------------------------
    // The list of a select (.ulw-sel's options): a row per option, the current one checked; a pick
    // closes the popover. Rows scroll when they do not fit
    // -------------------------------------------------------------------------------------------
    class UiWsOptionsPanel : public UiWsPanel
    {
    public:
        Vector<Pair<String, String>>  options; // value, label
        String                        current;
        Function<void(const String&)> onPick;

    public:
        explicit UiWsOptionsPanel(RefCounter* refCounter): UiWsPanel(refCounter) {}

        // The height the options want
        float ContentHeight() const { return 12.0f + options.Count()*28.0f; }

        void OnScrolled(float scroll) override;

    protected:
        float mScroll = 0.0f;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };

    // -------------------------------------------------------------------------------------------
    // The kit graphic picker's grid (.ulw-kitgrid): "auto" first, then every graphic of the kit on a
    // checker cell with its name, three across; a pick closes the popover
    // -------------------------------------------------------------------------------------------
    class UiWsKitGridPanel : public UiWsPanel
    {
    public:
        String                        current; // The graphic named now ("" = auto)
        Function<void(const String&)> onPick;  // "" = auto

    public:
        explicit UiWsKitGridPanel(RefCounter* refCounter): UiWsPanel(refCounter) {}

        // The height the grid wants
        float ContentHeight() const;

        void OnScrolled(float scroll) override;

    protected:
        float mScroll = 0.0f;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };

    // ---------------------------------------------------------------------------------------------
    // The kit coverage chip's popover (.ulw-covpop): blocks without a kit graphic (a click selects one),
    // kit graphics nobody uses, icons drawn as characters, and the kit graphics without a description
    // ---------------------------------------------------------------------------------------------
    class UiWsCoveragePanel : public UiWsPanel
    {
    public:
        Function<void(const String&)> onPickBlock; // A block of the list was clicked

    public:
        explicit UiWsCoveragePanel(RefCounter* refCounter): UiWsPanel(refCounter) {}

        // The height it wants at the width
        float ContentHeight(float width);

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;

        // Lays the content out (draw = false only measures); returns its height
        float Lay(const UiWsDraw::Box& box, bool draw);
    };

    namespace UiWsParts
    {
        // One anchor preset as a picture: the parent's box, the block placed in it, the anchor points (both ends of a
        // stretched axis) and the arrows of stretched axes; on / hover change its colours
        void AnchorCell(const RectF& rect, const String& h, const String& v, bool on, bool hover);

        // A switch (.tg): a 32 x 18 track and its knob
        void Toggle(const RectF& rect, bool on, bool hover);

        // A slider bar (.sb): the track and its fill, the value text on the right
        void SliderBar(const RectF& rect, float value, const String& text, bool active);
    }
}
