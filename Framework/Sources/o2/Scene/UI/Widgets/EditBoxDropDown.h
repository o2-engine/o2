#pragma once

#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/CustomList.h"

namespace o2
{
    // -----------------------------------------
    // Edit box with dropdown list of suggestions
    // -----------------------------------------
    class EditBoxDropDown: public Widget, public DrawableCursorEventsListener
    {
    public:
        PROPERTIES(EditBoxDropDown);
        PROPERTY(WString, text, SetText, GetText);                     // Edit box text property
        PROPERTY(WString, value, SelectItemText, GetSelectedItemText); // Selected item text property
        PROPERTY(int, selectedItemPos, SelectItemAt, GetSelectedItemPosition); // Selected item position property
        GETTER(int, itemsCount, GetItemsCount);                        // Items count getter

    public:
        Function<void(const WString&)> onTextChanged;         // Called when text is edited
        Function<void(const WString&)> onTextChangeCompleted; // Called when text changing is completed

        Function<void()> onBeforeExpand;                      // Called before opening dropdown

        Function<void(int)> onSelectedPos;                    // Select item position event
        Function<void(const WString&)> onSelectedText;        // Select text event

    public:
        // Default constructor
        explicit EditBoxDropDown(RefCounter* refCounter);

        // Copy-constructor
        EditBoxDropDown(RefCounter* refCounter, const EditBoxDropDown& other);

        // Destructor
        ~EditBoxDropDown();

        // Copy operator
        EditBoxDropDown& operator=(const EditBoxDropDown& other);

        // Draws widget
        void Draw() override;

        // Updates widget
        void Update(float dt) override;

        // Sets text in the edit box
        void SetText(const WString& text);

        // Returns text from the edit box
        WString GetText() const;

        // Expand list
        void Expand();

        // Collapse list
        void Collapse();

        // Returns is list expanded
        bool IsExpanded() const;

        // Adds new text item and returns position
        int AddItem(const WString& text);

        // Add new text item at position and returns this position
        int AddItem(const WString& text, int position);

        // Adds array of text items
        void AddItems(const Vector<WString>& data);

        // Removes item by text
        void RemoveItem(const WString& text);

        // Removes item at position
        void RemoveItem(int position);

        // Returns position of item by text. Returns -1 if can't find item
        int FindItem(const WString& text);

        // Returns item text by position
        WString GetItemText(int position) const;

        // Returns array of all text items
        Vector<WString> GetAllItemsText() const;

        // Moves item from position to new position
        void MoveItem(int position, int newPosition);

        // Returns items count
        int GetItemsCount() const;

        // Selects item at position
        void SelectItemAt(int position);

        // Selects item by text
        void SelectItemText(const WString& text);

        // Returns selected item position
        int GetSelectedItemPosition() const;

        // Returns current selected text item
        WString GetSelectedItemText() const;

        // Returns edit box
        const Ref<EditBox>& GetEditBox() const;

        // Sets list view size by items size
        void SetMaxListSizeInItems(int itemsCount);

        // Sets clipping layout
        void SetClippingLayout(const Layout& layout);

        // Returns clipping layout
        Layout GetClippingLayout();

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(EditBoxDropDown);
        CLONEABLE_REF(EditBoxDropDown);

    protected:
        Ref<EditBox>    mEditBox;    // Edit box for text input
        Ref<CustomList> mItemsList;  // List view for dropdown items
        
        Layout mClipLayout = Layout::BothStretch(); // Clipping layout @SERIALIZABLE
        RectF  mAbsoluteClip;                       // Absolute clipping rectangle

        int mMaxListItems = 10; // Maximum visible items in list @SERIALIZABLE

        bool mIsFiltering = false;      // Whether we're currently filtering items
        Vector<WString> mOriginalItems; // Original list of items for filtering

    protected:
        // Returns item by position
        Ref<Widget> GetItem(int position) const;

        // Sets item sample widget
        void SetItemSample(const Ref<Widget>& sample);

        // Moves widget's to delta and checks for clipping
        void MoveAndCheckClipping(const Vec2F& delta, const RectF& clipArea) override;

        // Called when cursor pressed on this. Sets state "pressed" to true
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time). Sets state "pressed" to false.
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor released outside this(only when cursor pressed this at previous time)
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor enters this object. Sets state "select" to true
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object. Sets state "select" to false
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Filter items in dropdown list based on current text
        void FilterItems();

        // Callback when edit box text changes
        void OnEditBoxTextChanged(const WString& text);

        // Callback when edit box is completed editing
        void OnEditBoxTextCompleted(const WString& text);

        // Called when item was selected in list
        void OnItemSelected();

        // Called when selection was changed
        virtual void OnSelectionChanged();

        // Called when edit box gets focus
        void OnEditBoxFocused();

        // Called when edit box loses focus
        void OnEditBoxUnfocused();

        // Removes all items
        void RemoveAllItems();

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(o2::EditBoxDropDown)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::EditBoxDropDown)
{
    FIELD().PUBLIC().NAME(text);
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(selectedItemPos);
    FIELD().PUBLIC().NAME(itemsCount);
    FIELD().PUBLIC().NAME(onTextChanged);
    FIELD().PUBLIC().NAME(onTextChangeCompleted);
    FIELD().PUBLIC().NAME(onBeforeExpand);
    FIELD().PUBLIC().NAME(onSelectedPos);
    FIELD().PUBLIC().NAME(onSelectedText);
    FIELD().PROTECTED().NAME(mEditBox);
    FIELD().PROTECTED().NAME(mItemsList);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mClipLayout);
    FIELD().PROTECTED().NAME(mAbsoluteClip);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10).NAME(mMaxListItems);
    FIELD().PROTECTED().NAME(mIsFiltering);
    FIELD().PROTECTED().NAME(mOriginalItems);
}
END_META;
CLASS_METHODS_META(o2::EditBoxDropDown)
{
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const EditBoxDropDown&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetText);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PUBLIC().SIGNATURE(int, AddItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(int, AddItem, const WString&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, AddItems, const Vector<WString>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, int);
    FUNCTION().PUBLIC().SIGNATURE(int, FindItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetItemText, int);
    FUNCTION().PUBLIC().SIGNATURE(Vector<WString>, GetAllItemsText);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveItem, int, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetItemsCount);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItemAt, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItemText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelectedItemPosition);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetSelectedItemText);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<EditBox>&, GetEditBox);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxListSizeInItems, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetClippingLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetClippingLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, GetItem, int);
    FUNCTION().PROTECTED().SIGNATURE(void, SetItemSample, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveAndCheckClipping, const Vec2F&, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, FilterItems);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxTextChanged, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxTextCompleted, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnItemSelected);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxUnfocused);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveAllItems);
}
END_META;
// --- END META --- 