#pragma once

#include <optional>

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsContext.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsDraw.h"

using namespace o2;

namespace o2
{
    class EditBox;
}

// The building blocks of the AI UI layout workspace: a custom-drawn panel with clickable spots, a popover,
// and the workspace's controls (tool buttons, buttons, segments, number fields)
namespace Editor
{
    // -----------------------------------------------------------------------------------------------
    // A panel that draws itself (DrawPanel, y down from its top-left corner) and registers the spots it
    // drew (AddHot) to be clicked: the spot under the cursor is hovered, a press and a release on the same
    // spot is a click (OnHotClick). Its children (fields) are drawn over it, DrawOverlay over them
    // -----------------------------------------------------------------------------------------------
    class UiWsPanel : public Widget, public CursorAreaEventsListener
    {
    public:
        Ref<UiWsContext> ctx; // What the workspace shows

    public:
        // Default constructor
        explicit UiWsPanel(RefCounter* refCounter);

        // Draws the panel, takes the cursor, draws the children and the overlay
        void Draw() override;

        // Returns true inside the panel
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns the spot under the point, "" for none
        String HotAt(const Vec2F& point) const;

        // Returns the world rectangle of a spot drawn last frame (an empty one for none)
        RectF HotRect(const String& id) const;

        // Clicks a spot as the cursor would (tests)
        void ClickHot(const String& id);

    protected:
        struct Hot
        {
            RectF  rect;
            String id;
        };

        Vector<Hot> mHot;          // Spots drawn this frame, the last on top
        Vector<Hot> mDrawnHot;     // Spots of the frame before (what the cursor meets)
        String      mHover;        // The spot under the cursor
        String      mPressedHot;   // The spot pressed
        Vec2F       mPressPoint;   // Where the press began (world)
        bool        mHoverInside = false;
        bool        mClipChildren = false; // The children are clipped to mChildClip (a scrolled list of fields)
        RectF       mChildClip;

    protected:
        // Draws the content
        virtual void DrawPanel(const UiWsDraw::Box& box) {}

        // Draws over the children
        virtual void DrawOverlay(const UiWsDraw::Box& box) {}

        // A spot was clicked (pressed and released on it)
        virtual void OnHotClick(const String& id, const Input::Cursor& cursor) {}

        // A spot was double-clicked
        virtual void OnHotDoubleClick(const String& id, const Input::Cursor& cursor) {}

        // Registers a spot drawn at the rectangle
        void AddHot(const RectF& rect, const String& id);

        // True when the spot is under the cursor
        bool IsHover(const String& id) const { return !id.IsEmpty() && mHover == id; }

        // True while the spot is pressed
        bool IsPressedHot(const String& id) const { return IsPressed() && mPressedHot == id; }

        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;
        void OnCursorMoved(const Input::Cursor& cursor) override;
        void OnCursorExit(const Input::Cursor& cursor) override;
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // --- the workspace's controls, drawn and made clickable at once ---

        // A 28 x 28 tool (.util-tool): the icon, a wash on hover, the accent while active, faded while disabled
        void Tool(const RectF& rect, const String& icon, const String& id, bool active = false, bool enabled = true, float iconSize = 15.0f);

        // A button (.btn-ghost / .btn-primary): an optional icon before the caption
        void Button(const RectF& rect, const String& caption, const String& id, bool primary = false, bool enabled = true,
                    const String& icon = "");

        // A segmented control (.seg): equal parts, the value's part raised; ids are prefix + option value
        void Segments(const RectF& rect, const Vector<Pair<String, String>>& options, const String& value, const String& prefix,
                      bool enabled = true);

        // A separator (.util-sep)
        void Separator(const Vec2F& center, float height = 16.0f);
    };

    // -----------------------------------------------------------------------------------------------
    // A popover of the workspace (a menu, the kit picker, the coverage list, the screen picker): the menu
    // plate, placed under or above a rectangle, drawn above everything; a press outside or Escape closes it
    // -----------------------------------------------------------------------------------------------
    class UiWsPopup : public PopupWidget
    {
    public:
        Function<void()> onClosed; // Called when it closes

    public:
        // Default constructor
        explicit UiWsPopup(RefCounter* refCounter);

        // Opens it next to the anchor (world space): below it (or above when there is no room), left-aligned or right-aligned
        void OpenAt(const RectF& anchor, float width, float height, bool alignRight = false);

        // Changes the size while open
        void Resize(float width, float height);

        // Closes it
        void Close();

        // Returns true while it is open
        bool IsOpen() const;

        // Returns true when a popover of the workspace closed in this frame (its Escape must not close the workspace too)
        static bool ClosedThisFrame();

        // Returns true while any popover of the workspace is open
        static bool AnyOpen();

        // Sets the content panel, stretched over the popover
        void SetContent(const Ref<Widget>& content);

    protected:
        RectF mAnchor;
        float mWidth = 300.0f, mHeight = 200.0f;
        bool  mAlignRight = false;

    protected:
        void FitSizeAndPosition(const Vec2F& position) override;
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override {}
        void OnDisabled() override;
        void OnEnabled() override;

        // Puts the popover at its anchor
        void Place();
    };

    namespace UiWsControls
    {
        // A number field with its letter inside (.ulw-numf): X, Y, W, H, T, R, B, L, # ("" = none). The value applies while it
        // is typed whenever it is a number (not below min); an empty field applies none. The field shows the stored value
        // again once it loses the focus
        Ref<EditBox> NumberField(const String& letter, std::optional<double> value, const String& placeholder,
                                 const Function<void(std::optional<double>)>& onValue, std::optional<double> min = std::nullopt);

        // A number field whose value applies only on Enter or when the field is left (Escape drops what was typed): for
        // numbers whose every change makes something again ("1000" must not apply 100 on the way)
        Ref<EditBox> LazyNumberField(const String& letter, std::optional<double> value, const String& placeholder,
                                     const Function<void(std::optional<double>)>& onValue, std::optional<double> min = std::nullopt);

        // Shows the value in a number field that does not have the focus
        void SetNumber(const Ref<EditBox>& field, std::optional<double> value);

        // Sets a field's text without its change callbacks (EditBox::SetText calls onChanged)
        void SetTextQuiet(const Ref<EditBox>& field, const String& text);

        // A text field of the workspace (28 px, the field look); onText while typing, onDone when it is left
        Ref<EditBox> TextField(const String& text, const String& placeholder, bool multiline,
                               const Function<void(const String&)>& onText, const Function<void(const String&)>& onDone = {});

        // A number as text: integers without a dot, the rest with up to 4 decimals
        String NumberText(double value);
    }
}
