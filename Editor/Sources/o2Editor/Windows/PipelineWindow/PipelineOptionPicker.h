#pragma once

#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "o2/Utils/Function/Function.h"

using namespace o2;

namespace o2
{
    class Button;
    class Text;
}

namespace Editor
{
    // One choice of an option menu
    struct PipelineOption
    {
        String value; // Config value the choice writes
        String label; // Name shown
        String hint;  // Second line under the name
        String icon;  // Icon image

        bool operator==(const PipelineOption& other) const { return value == other.value; }
    };

    // What an option field asks the option menu for
    struct PipelineOptionPickerRequest
    {
        Vector<PipelineOption>        options; // Choices in their order
        String                        current; // The node's value
        RectF                         anchor;  // Field rectangle in screen space
        WeakRef<Widget>               field;   // Field the menu opens from
        Function<void(const String&)> onPick;  // Called with the picked value once the menu is closed
    };

    // -------------------------------------------------------------------------------------------------------
    // Menu of a small fixed choice, the model menu's look without its search: a row per option with its icon,
    // its name and a hint under it, the current one checked. Lives in screen space above every card
    // -------------------------------------------------------------------------------------------------------
    class PipelineOptionPicker : public PopupWidget
    {
    public:
        // Default constructor
        explicit PipelineOptionPicker(RefCounter* refCounter);

        // Opens the menu at the field with the current option highlighted
        void Open(const PipelineOptionPickerRequest& request);

        // Closes the menu without a pick
        void Close();

        // Returns true while the menu is shown
        bool IsOpen() const;

        // Returns true while the menu is shown for the field
        bool IsOpenFor(const Ref<Widget>& field) const;

        // Returns true once after the menu was closed by a press on the field it opened from, so the click that ends
        // that press does not open it again
        bool ConsumeClosedByField(const Ref<Widget>& field);

        // Closes the menu and passes the value to the field
        void Pick(const String& value);

        // Picks the highlighted row
        void PickHighlighted();

        // Moves the highlight over the rows, wrapping around
        void MoveHighlight(int step);

        // Returns the highlighted row index
        int GetHighlight() const { return mHighlight; }

        // Returns the value of the highlighted row
        String GetHighlightedValue() const;

        // Returns the rows in order
        const Vector<Ref<Button>>& GetRows() const { return mRows; }

        // Returns the row of the value
        Ref<Button> FindRow(const String& value) const;

        // Updates the hover highlight and closes the menu on a wheel outside it
        void Update(float dt) override;

        SERIALIZABLE(PipelineOptionPicker);

    protected:
        PipelineOptionPickerRequest mRequest;            // What the open menu was asked for
        Vector<Ref<Button>>         mRows;               // Row widgets, in the order of the options
        int                         mHighlight = 0;      // Highlighted row
        bool                        mOpensDown = true;   // The menu hangs below the field, else it stands above it
        float                       mWidth = 300.0f;     // Menu width
        float                       mContentHeight = 0.0f; // Height of the rows

        WeakRef<Widget> mClosedByField;             // Field whose press closed the menu
        float           mClosedByFieldTime = -1.0f; // Application time of that press

        Ref<Text> mMeasure; // Font for measuring the hints

    protected:
        // Closes the menu on Escape, moves the highlight on the arrows and picks on Enter
        void OnKeyPressed(const Input::Key& key) override;

        // Repeats the arrows while held
        void OnKeyStayDown(const Input::Key& key) override;

        // Gives the focus back to the canvas
        void OnDisabled() override;

        // Keeps the menu open: it closes on a press outside, a pick or Escape
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Places the menu at the field; the position the popup is shown at is not used
        void FitSizeAndPosition(const Vec2F& position) override;

        // Builds the rows for the options
        void RebuildRows();

        // Sets the menu rectangle at the field
        void Place();

        // Shows the highlight on its row only
        void UpdateHighlight();

        // Returns the height of a hint wrapped into the width
        float MeasureHint(const String& text, float width) const;

        // Creates the row of an option
        Ref<Button> MakeRow(const PipelineOption& option, float width, float height, float hintHeight);
    };

    namespace PipelineControls
    {
        // Creates the closed option field: looks like the model field, with the option's icon before its name
        Ref<Button> MakeOptionField();

        // Shows the option on the field
        void SetOptionFieldValue(const Ref<Button>& field, const PipelineOption& option);
    }
}
// --- META ---

CLASS_BASES_META(Editor::PipelineOptionPicker)
{
    BASE_CLASS(o2::PopupWidget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineOptionPicker)
{
    FIELD().PROTECTED().NAME(mRequest);
    FIELD().PROTECTED().NAME(mRows);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mHighlight);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mOpensDown);
    FIELD().PROTECTED().DEFAULT_VALUE(300.0f).NAME(mWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mContentHeight);
    FIELD().PROTECTED().NAME(mClosedByField);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mClosedByFieldTime);
    FIELD().PROTECTED().NAME(mMeasure);
}
END_META;
CLASS_METHODS_META(Editor::PipelineOptionPicker)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Open, const PipelineOptionPickerRequest&);
    FUNCTION().PUBLIC().SIGNATURE(void, Close);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsOpen);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsOpenFor, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(bool, ConsumeClosedByField, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Pick, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, PickHighlighted);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveHighlight, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetHighlight);
    FUNCTION().PUBLIC().SIGNATURE(String, GetHighlightedValue);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Button>>&, GetRows);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, FindRow, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, FitSizeAndPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildRows);
    FUNCTION().PROTECTED().SIGNATURE(void, Place);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHighlight);
    FUNCTION().PROTECTED().SIGNATURE(float, MeasureHint, const String&, float);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Button>, MakeRow, const PipelineOption&, float, float, float);
}
END_META;
// --- END META ---
