#pragma once

#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

// The parts of the Text style card that edit a style's look beyond one colour (port of AssetsLine TextStyleFx.tsx): the
// fill - a colour, or a gradient whose stops are dragged along a bar (a click on the bar adds one) - and the effects block:
// the style's layer effects (outlines, depth, shadows, glow, inner shadows, a bevel) in a frame of their own, a palette on
// its head that adds each kind, then the stack, its first effect on top. An effect is dragged by its grip to another place
// in the stack, shown or hidden by its eye, duplicated or removed; an outline may be a gradient. Every effect takes a row
// or two of the card's pinned rows
namespace Editor
{
    class PipelineEditor;

    // ------------------------------------------------------------------------------------------------
    // A drawn block of the Text style card's rows: the fill (Fill and, for a gradient, its colours) or
    // the effects block (its head with the palette, then a plate of a row or two per effect). Every edit
    // hands the whole style over: the card writes fill and effects together with their mirrors
    // ------------------------------------------------------------------------------------------------
    class PipelineTextFxPanel : public UiWsPanel
    {
    public:
        static constexpr float rowH = 26.0f;    // A row (.uts-row)
        static constexpr float rowGap = 5.0f;   // Between the rows
        static constexpr float labelW = 62.0f;  // The caption (.uts-l)
        static constexpr float boxExtra = 10.0f; // What the effects block's frame adds to its rows: border and padding

        bool effects = false; // The effects block, else the fill

        Function<UiDoc::TextStyle()>                       getStyle; // The style now
        Function<void(const UiDoc::TextStyle&, bool done)> onLook;   // A new look; done ends an edit (a drag, a colour pick)
        Function<void()>                                   onRows;   // The rows changed: the card lays itself out again
        WeakRef<PipelineEditor>                            editor;   // For the menus in screen space

    public:
        // Default constructor
        explicit PipelineTextFxPanel(RefCounter* refCounter);

        // Rows of the card an effect takes: an outline one (two with a gradient), depth and glow one, the rest two
        static int EffectRows(const UiDoc::TextEffect& effect);

        // The rows the block (the effects, else the fill) takes for the style
        static int Rows(bool effects, const UiDoc::TextStyle& style);

        // The height the block (the effects with their frame, else the fill) takes for the style
        static float Height(bool effects, const UiDoc::TextStyle& style);

        // Writes the style's fill and effects into a Text style node's config together with their mirrors (gradient, stroke,
        // shadow), which the engine exports and the older editors read. The effects are taken in the order the card shows
        // them (the stack): fxStack stays in the config only when that order is not the canonical one
        static void WriteLook(DataValue& config, const UiDoc::TextStyle& style);

        // The gradient with a stop added where the bar was pressed (a share of its width, kept at 0.01), in the colour the
        // gradient has there; index receives its place among the stops
        static UiDoc::TextGradient WithStopAt(const UiDoc::TextGradient& gradient, double at, int& index);

        // The effects with one moved to another place in the stack: to is its place among the others (0 = on top)
        static Vector<UiDoc::TextEffect> Moved(const Vector<UiDoc::TextEffect>& effects, int from, int to);

        // Returns the selected stop of the fill's gradient ("") or of an outline's ("fx<i>")
        int GetSelectedStop(const String& owner = "") const;

        // Adds an effect as the palette does: where the canonical stack has it
        void AddEffect(const String& type);

        // Presses a gradient bar - the fill's ("") or an outline's ("fx<i>") - at a share of its width as the cursor would: on
        // a stop it selects it, elsewhere it adds one there (tests)
        void PressBar(float at, const String& owner = "");

        // Moves the stop being dragged to a share of the bar's width and lets it go (tests)
        void DragStopTo(float at);

        // Drags effect from by its grip to another place in the stack and drops it there, as one edit (tests)
        void DragEffect(int from, int to);

        // Clicks a spot (tests): gradient, stopRemove, add:<type>, fx<i>:eye, fx<i>:dup, fx<i>:remove, fx<i>:grad,
        // fx<i>:stopRemove...
        void Click(const String& id) { ClickHot(id); }

        // Sets a slider as a drag to the share of its track would, done at once (tests): angle, fx<i>:<field> (fx<i>:angle for
        // an outline's gradient)
        void Slide(const String& key, float share);

    protected:
        // ----------------------------------------------------------
        // An effect being dragged to another place in the stack
        // ----------------------------------------------------------
        struct FxDrag
        {
            int   from = -1;     // The effect; -1: no drag
            int   to = 0;        // Its place among the others where it would land
            float startY = 0;    // Where the press began (world)
            float dy = 0;        // How far it moved, down
            float lineY = 0;     // The drop line (world)
        };

        int           mStop = 0;     // The selected stop of the fill's gradient
        Map<int, int> mFxStops;      // The selected stop of each gradient outline's bar, by the effect's place
        String        mDrag;         // The slider or stop being dragged: "slider:<key>", "stop:<owner>"
        RectF         mDragTrack;    // Its track (world)
        RectF         mBar;          // The fill's gradient bar drawn last (world)
        Map<int, RectF> mFxBars;     // The outlines' gradient bars drawn last (world), by the effect's place
        Vector<RectF> mPlates;       // The effects' plates drawn last (world), in the list's order
        FxDrag        mFxDrag;       // An effect being dragged
        Vec2F         mCursor;       // The cursor over the panel (world)

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;
        void OnCursorMoved(const Input::Cursor& cursor) override;

        void DrawFill(const UiWsDraw::Box& box, const UiDoc::TextStyle& style);
        void DrawEffects(const UiWsDraw::Box& box, const UiDoc::TextStyle& style);

        // The effects block's head: the caption, the count and the palette that adds each kind
        void DrawEffectsHead(const UiWsDraw::Box& box, const UiDoc::TextStyle& style, float x, float y, float w);

        // An effect's plate with its rows from y (box space, y down); lifted: being dragged
        void DrawPlate(const UiWsDraw::Box& box, const UiDoc::TextStyle& style, int i, float x, float y, float w, bool lifted);

        // A gradient bar with its stops as handles; hot spots <prefix>bar and <prefix>stop:<n>
        void DrawStopsBar(const RectF& bar, const UiDoc::TextGradient& gradient, int selected, const String& prefix, bool dim = false);

        // The parts of a row
        void Well(const RectF& rect, const String& color, const String& id, bool dim = false);
        void Bar(const RectF& rect, const String& key, float share, const String& text, bool dim = false);
        void Switch(const RectF& rect, bool on, const String& id, const String& caption = "");
        void SmallButton(const RectF& rect, const String& icon, const String& id, bool enabled = true, bool on = false,
                         float iconSize = 10.0f);

        // An effect's action: eye, dup, remove, grad, stopRemove, a colour (color, highlight, shade, stop); false for none
        bool EffectClick(UiDoc::TextStyle& style, int i, const String& act);

        // The gradient of the fill ("") or of an outline ("fx<i>"); null when it has none
        static UiDoc::TextGradient* GradientOf(UiDoc::TextStyle& style, const String& owner);

        // The selected stop of a gradient's owner
        int& StopOf(const String& owner);

        // Sets a slider from a share of its track; done ends the edit
        void SetSlider(const String& key, float share, bool done);

        // Moves the dragged stop to a share of its bar
        void MoveStop(float at, bool done);

        // Where a dragged effect would land among the others with the cursor at y (world), and the drop line there
        void UpdateFxSlot(float y);

        // Opens the colour picker for a well: fill, stop, fx<i>:color|highlight|shade|stop
        void PickColor(const String& key);

        // Hands a new look over
        void Look(const UiDoc::TextStyle& style, bool done);
    };
}
