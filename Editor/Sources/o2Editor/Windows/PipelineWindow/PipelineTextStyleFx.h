#pragma once

#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

// The parts of the Text style card that edit a style's look beyond one colour (port of AssetsLine TextStyleFx.tsx): the
// fill - a colour, or a gradient whose stops are dragged along a bar (a click on the bar adds one) - and the list of layer
// effects - outlines, depth, shadows, glow, inner shadows, a bevel - each a row or two of the card's pinned rows
namespace Editor
{
    class PipelineEditor;

    // ------------------------------------------------------------------------------------------------
    // A drawn block of the Text style card's rows: the fill (Fill and, for a gradient, its colours) or
    // the effects (the head with "add an effect", then a row or two per effect). Every edit hands the
    // whole style over: the card writes fill and effects together with their mirrors
    // ------------------------------------------------------------------------------------------------
    class PipelineTextFxPanel : public UiWsPanel
    {
    public:
        static constexpr float rowH = 26.0f;  // A row (.uts-row)
        static constexpr float rowGap = 5.0f; // Between the rows
        static constexpr float labelW = 62.0f; // The caption (.uts-l)

        bool effects = false; // The effects block, else the fill

        Function<UiDoc::TextStyle()>                       getStyle; // The style now
        Function<void(const UiDoc::TextStyle&, bool done)> onLook;   // A new look; done ends an edit (a drag, a colour pick)
        Function<void()>                                   onRows;   // The rows changed: the card lays itself out again
        WeakRef<PipelineEditor>                            editor;   // For the menus in screen space

    public:
        // Default constructor
        explicit PipelineTextFxPanel(RefCounter* refCounter);

        // Rows of the card each kind of effect takes (FX_ROWS)
        static int EffectRows(const String& type);

        // The rows the block (the effects, else the fill) takes for the style
        static int Rows(bool effects, const UiDoc::TextStyle& style);

        // The height the block (the effects, else the fill) takes for the style
        static float Height(bool effects, const UiDoc::TextStyle& style);

        // Writes the style's fill and effects into a Text style node's config together with their mirrors (gradient, stroke,
        // shadow), which the engine exports and the older editors read
        static void WriteLook(DataValue& config, const UiDoc::TextStyle& style);

        // The gradient with a stop added where the bar was pressed (a share of its width, kept at 0.01), in the colour the
        // gradient has there; index receives its place among the stops
        static UiDoc::TextGradient WithStopAt(const UiDoc::TextGradient& gradient, double at, int& index);

        // The effects with one moved before the nearest earlier effect of its kind (order matters only among those);
        // unchanged when there is none
        static Vector<UiDoc::TextEffect> MovedUp(const Vector<UiDoc::TextEffect>& effects, int index);

        // Returns the selected gradient stop
        int GetSelectedStop() const { return mStop; }

        // Adds an effect as the menu does
        void AddEffect(const String& type);

        // Presses the gradient bar at a share of its width as the cursor would: on a stop it selects it, elsewhere it adds
        // one there (tests)
        void PressBar(float at);

        // Moves the stop being dragged to a share of the bar's width and lets it go (tests)
        void DragStopTo(float at);

        // Clicks a spot (tests): gradient, stopRemove, add, fx<i>:toggle, fx<i>:up, fx<i>:remove...
        void Click(const String& id) { ClickHot(id); }

        // Sets a slider as a drag to the share of its track would, done at once (tests): angle, fx<i>:<field>
        void Slide(const String& key, float share);

    protected:
        int    mStop = 0;     // The selected gradient stop
        String mDrag;         // The slider or stop being dragged: "slider:<key>", "stop"
        RectF  mDragTrack;    // Its track (world)
        RectF  mBar;          // The gradient bar drawn last (world)

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;

        void DrawFill(const UiWsDraw::Box& box, const UiDoc::TextStyle& style);
        void DrawEffects(const UiWsDraw::Box& box, const UiDoc::TextStyle& style);

        // The parts of a row
        void Well(const RectF& rect, const String& color, const String& id, bool dim = false);
        void Bar(const RectF& rect, const String& key, float share, const String& text, bool dim = false);
        void Switch(const RectF& rect, bool on, const String& id, const String& caption = "");
        void SmallButton(const RectF& rect, const String& icon, const String& id, bool enabled = true);

        // Sets a slider from a share of its track; done ends the edit
        void SetSlider(const String& key, float share, bool done);

        // Moves the dragged stop to a share of the bar
        void MoveStop(float at, bool done);

        // Opens the colour picker for a well: fill, stop, fx<i>:color|highlight|shade
        void PickColor(const String& key);

        // Hands a new look over
        void Look(const UiDoc::TextStyle& style, bool done);
    };
}
