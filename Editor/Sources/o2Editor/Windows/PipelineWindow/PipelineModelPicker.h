#pragma once

#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineModelMenu.h"

using namespace o2;

namespace o2
{
    class Button;
    class EditBox;
    class Label;
    class ScrollArea;
    class Text;
    class Toggle;
}

namespace Editor
{
    // What a model field asks the model menu for
    struct PipelineModelPickerRequest
    {
        Vector<String>                ids;     // Models the node offers, in its order
        PipelineModelKind             kind = PipelineModelKind::Text; // List kind: Recent bucket and the transparency badge
        String                        current; // The node's value
        RectF                         anchor;  // Field rectangle in screen space
        WeakRef<Widget>               field;   // Field the menu opens from
        Function<void(const String&)> onPick;  // Called with the picked id once the menu is closed
    };

    // ------------------------------------------------------------------------------------------------
    // Model menu of the node model fields: a search, provider chips, groups with fold headers and a row
    // that takes the search text as a model id. Lives in screen space above every card; the grouping
    // and filtering rules are PipelineModelMenu's
    // ------------------------------------------------------------------------------------------------
    class PipelineModelPicker : public PopupWidget
    {
    public:
        // Default constructor
        explicit PipelineModelPicker(RefCounter* refCounter);

        // Opens the menu at the field: builds the groups, highlights the node's value and focuses the search
        void Open(const PipelineModelPickerRequest& request);

        // Closes the menu without a pick
        void Close();

        // Returns true while the menu is shown
        bool IsOpen() const;

        // Returns true while the menu is shown for the field
        bool IsOpenFor(const Ref<Widget>& field) const;

        // Returns true once after the menu was closed by a press on the field it opened from, so the click
        // that ends that press does not open it again
        bool ConsumeClosedByField(const Ref<Widget>& field);

        // Remembers the id in Recent, closes the menu and passes the id to the field
        void Pick(const String& id);

        // Picks the highlighted row
        void PickHighlighted();

        // Moves the highlight over the rows, wrapping around, and scrolls it into view
        void MoveHighlight(int step);

        // Turns the provider chips: any provider, or only the given one
        void SetProviderFilter(bool anyProvider, PipelineModelProvider provider);

        // Turns the transparent background chip
        void SetAlphaOnly(bool alphaOnly);

        // Folds or unfolds a group, as a click on its header does
        void ToggleGroup(const String& key);

        // Returns the search field
        const Ref<EditBox>& GetSearch() const { return mSearch; }

        // Returns the ids of the rows the highlight moves over; the custom id row is its id
        const Vector<String>& GetPickableIds() const { return mPickable; }

        // Returns the highlighted row index
        int GetHighlight() const { return mHighlight; }

        // Returns the id of the highlighted row, empty when there are no rows
        String GetHighlightedId() const;

        // Returns the groups shown
        const Vector<PipelineVisibleGroup>& GetVisibleGroups() const { return mVisible; }

        // Returns the id the custom row offers, empty when it is not shown
        const String& GetCustomId() const { return mCustomId; }

        // Returns the widget of a pickable row: a model row, or the custom row under its id
        Ref<Button> FindRow(const String& id) const;

        // Returns the header of a group
        Ref<Button> FindHeader(const String& key) const;

        // Returns the scrolling list of the rows
        const Ref<ScrollArea>& GetList() const { return mList; }

        // Returns true when the row is inside the visible part of the list
        bool IsRowInView(int index) const;

        // Updates the hover highlight and closes the menu on a wheel outside it
        void Update(float dt) override;

        SERIALIZABLE(PipelineModelPicker);

    protected:
        PipelineModelPickerRequest   mRequest;    // What the open menu was asked for
        Vector<PipelineMenuGroup>    mGroups;     // All groups of the list
        Vector<PipelineVisibleGroup> mVisible;    // Groups after the filter
        PipelineMenuFilter           mFilter;     // Search text and chips
        Map<String, bool>            mToggled;    // Folding set by header clicks, until the menu closes
        String                       mCustomId;   // Id of the custom row, empty when it is not shown
        Vector<String>               mPickable;   // Ids of the rows the highlight moves over
        int                          mHighlight = 0; // Highlighted row
        String                       mPendingToggle; // Group whose header was clicked, folded or unfolded on the next update

        bool  mOpensDown = true;    // The menu hangs below the field, else it stands above it
        float mWidth = 320.0f;      // Menu width
        bool  mRefilling = false;   // Set while the search text is reset, mutes its change callback

        WeakRef<Widget> mClosedByField;        // Field whose press closed the menu
        float           mClosedByFieldTime = -1.0f; // Application time of that press

        Ref<Widget>     mSearchPanel; // Search icon and field
        Ref<EditBox>    mSearch;      // Search field
        Ref<WidgetLayer> mPlaceholder; // "Search models", shown while the field is empty
        Ref<Widget>     mChips;       // Provider and transparency chips
        Ref<Widget>     mSeparator;   // Line between the search and the list
        Ref<ScrollArea> mList;        // Scrolling rows
        Ref<Widget>     mListContent; // Rows, laid out top to bottom
        Ref<Text>       mMeasure;     // Font for measuring captions

        Vector<Ref<Button>> mRows;       // Pickable row widgets, in the order of mPickable
        Vector<float>       mRowTops;    // Top of each pickable row inside the list content
        Vector<float>       mRowHeights; // Height of each pickable row
        Vector<float>       mGroupTops;  // Top of the header of the group each pickable row is in
        Vector<Pair<String, Ref<Button>>> mHeaders; // Group headers by group key
        float               mContentHeight = 0.0f; // Height of the list content
        float               mChipsHeight = 0.0f;   // Height of the chip rows, 0 without chips

    protected:
        // Closes the menu on Escape, moves the highlight on the arrows and picks on Enter
        void OnKeyPressed(const Input::Key& key) override;

        // Repeats the arrows while held
        void OnKeyStayDown(const Input::Key& key) override;

        // Moves the focus off the menu's own widgets to the canvas when the menu closes: a focused row or chip would
        // act on Enter and Space while the menu is hidden
        void OnDisabled() override;

        // Keeps the menu open: it closes on a press outside, a pick or Escape. A release left over from a click that
        // closed the menu would otherwise close it again right after the next opening
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Keeps the menu open when its list scrolls under a press
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Places the menu at the field; the position the popup is shown at is not used
        void FitSizeAndPosition(const Vec2F& position) override;

        // Creates the search, the chip panel and the list
        void InitializeControls();

        // Rebuilds the chips for the list
        void RebuildChips();

        // Applies the filter: groups, rows, size and highlight; resetHighlight starts from the first row
        void Refilter(bool resetHighlight);

        // Rebuilds the list rows from the filtered groups
        void RebuildRows();

        // Sets the menu rectangle at the field for the current content height
        void Place();

        // Returns the height the header part (search, chips, gaps) takes
        float GetHeaderHeight() const;

        // Shows the highlight on its row only
        void UpdateHighlight();

        // Scrolls the list so the row is visible; withGroup also keeps the header of its group in view when it fits
        void ScrollToRow(int index, bool withGroup);

        // Returns the width of a caption in the font
        float MeasureText(const String& text, int height) const;

        // Creates a model row
        Ref<Button> MakeModelRow(const PipelineMenuModel& model, float width);

        // Creates a group header
        Ref<Button> MakeHeader(const PipelineVisibleGroup& group, float width);

        // Creates the "use as model id" row
        Ref<Button> MakeCustomRow(const String& id);
    };

    namespace PipelineControls
    {
        // Creates the closed model field: looks like a drop down, shows the model name and the transparency badge
        Ref<Button> MakeModelField();

        // Shows the model on the field; the badge is shown when the kind's list marks the model
        void SetModelFieldValue(const Ref<Button>& field, const String& id, PipelineModelKind kind);
    }
}
// --- META ---

CLASS_BASES_META(Editor::PipelineModelPicker)
{
    BASE_CLASS(o2::PopupWidget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineModelPicker)
{
    FIELD().PROTECTED().NAME(mRequest);
    FIELD().PROTECTED().NAME(mGroups);
    FIELD().PROTECTED().NAME(mVisible);
    FIELD().PROTECTED().NAME(mFilter);
    FIELD().PROTECTED().NAME(mToggled);
    FIELD().PROTECTED().NAME(mCustomId);
    FIELD().PROTECTED().NAME(mPickable);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mHighlight);
    FIELD().PROTECTED().NAME(mPendingToggle);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mOpensDown);
    FIELD().PROTECTED().DEFAULT_VALUE(320.0f).NAME(mWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mRefilling);
    FIELD().PROTECTED().NAME(mClosedByField);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mClosedByFieldTime);
    FIELD().PROTECTED().NAME(mSearchPanel);
    FIELD().PROTECTED().NAME(mSearch);
    FIELD().PROTECTED().NAME(mPlaceholder);
    FIELD().PROTECTED().NAME(mChips);
    FIELD().PROTECTED().NAME(mSeparator);
    FIELD().PROTECTED().NAME(mList);
    FIELD().PROTECTED().NAME(mListContent);
    FIELD().PROTECTED().NAME(mMeasure);
    FIELD().PROTECTED().NAME(mRows);
    FIELD().PROTECTED().NAME(mRowTops);
    FIELD().PROTECTED().NAME(mRowHeights);
    FIELD().PROTECTED().NAME(mGroupTops);
    FIELD().PROTECTED().NAME(mHeaders);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mContentHeight);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mChipsHeight);
}
END_META;
CLASS_METHODS_META(Editor::PipelineModelPicker)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Open, const PipelineModelPickerRequest&);
    FUNCTION().PUBLIC().SIGNATURE(void, Close);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsOpen);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsOpenFor, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(bool, ConsumeClosedByField, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Pick, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, PickHighlighted);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveHighlight, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetProviderFilter, bool, PipelineModelProvider);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAlphaOnly, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, ToggleGroup, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<EditBox>&, GetSearch);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<String>&, GetPickableIds);
    FUNCTION().PUBLIC().SIGNATURE(int, GetHighlight);
    FUNCTION().PUBLIC().SIGNATURE(String, GetHighlightedId);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<PipelineVisibleGroup>&, GetVisibleGroups);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetCustomId);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, FindRow, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, FindHeader, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ScrollArea>&, GetList);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsRowInView, int);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, FitSizeAndPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildChips);
    FUNCTION().PROTECTED().SIGNATURE(void, Refilter, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildRows);
    FUNCTION().PROTECTED().SIGNATURE(void, Place);
    FUNCTION().PROTECTED().SIGNATURE(float, GetHeaderHeight);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHighlight);
    FUNCTION().PROTECTED().SIGNATURE(void, ScrollToRow, int, bool);
    FUNCTION().PROTECTED().SIGNATURE(float, MeasureText, const String&, int);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Button>, MakeModelRow, const PipelineMenuModel&, float);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Button>, MakeHeader, const PipelineVisibleGroup&, float);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Button>, MakeCustomRow, const String&);
}
END_META;
// --- END META ---
