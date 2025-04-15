#pragma once

#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/List.h"
#include "o2/Events/KeyboardEventsListener.h"

namespace o2
{
    // -----------------------------------------
    // Edit box with dropdown list of suggestions
    // -----------------------------------------
    class EditBoxDropDown: public Widget, public DrawableCursorEventsListener, public KeyboardEventsListener
    {
    public:
        PROPERTIES(EditBoxDropDown);
        PROPERTY(WString, text, SetText, GetText);                             // Edit box text property
        PROPERTY(int, selectedItemPos, SelectItemAt, GetSelectedItemPosition); // Selected item position property
        GETTER(int, itemsCount, GetItemsCount);                                // Items count getter

    public:
        Function<void(const WString&)> onChanged;         // Called when text is edited
        Function<void(const WString&)> onChangeCompleted; // Called when text changing is completed

        Function<void()> onExpand; // Called before opening dropdown

        Function<void(const WString&)> onSelectedText; // Select text event

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

        // Returns items count
        int GetItemsCount() const;
        
        // Removes all items
        void RemoveAllItems();

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
        
        // Returns list view
        const Ref<List>& GetListView() const;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(EditBoxDropDown);
        CLONEABLE_REF(EditBoxDropDown);

    protected:
        Ref<EditBox> mEditBox;   // Edit box for text input
        Ref<List>    mItemsList; // List view for dropdown items

        int mMaxListItems = 10; // Maximum visible items in list @SERIALIZABLE

        bool mIsFiltering = false;          // Whether we're currently filtering items
        bool mIsKeyboardNavigating = false; // Whether we're navigating with keyboard @SERIALIZABLE

        Vector<WString> mOriginalItems; // Original list of items for filtering

    protected:
        // Returns item by position
        Ref<Widget> GetItem(int position) const;

        // Sets item sample widget
        void SetItemSample(const Ref<Widget>& sample);
        
        // Setup callbacks for edit box and list
        void SetupCallbacks();

        // Updates the list size based on items count
        void UpdateListSize();

        // Moves widget's to delta and checks for clipping
        void MoveAndCheckClipping(const Vec2F& delta, const RectF& clipArea) override;

		// Called when cursor released outside this(only when cursor pressed this at previous time)
		void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

		// Called when key was pressed when widget is focused
		void OnKeyPressed(const Input::Key& key) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Filter items in dropdown list based on current text
        void FilterItems();

        // Navigate to the next item in the list
        void NavigateNext();

        // Navigate to the previous item in the list
        void NavigatePrevious();

        // Callback when edit box text changes
        void OnEditBoxTextChanged(const WString& text);

        // Callback when edit box is completed editing
        void OnEditBoxTextCompleted(const WString& text);
        
        // Called when item text was selected in list
        void OnSelectedText(const WString& text);

        // Called when selection was changed
        virtual void OnSelectionChanged();

        // Called when edit box gets focus
        void OnEditBoxFocused();

        // Called when edit box loses focus
        void OnEditBoxUnfocused();

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(o2::EditBoxDropDown)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::EditBoxDropDown)
{
    FIELD().PUBLIC().NAME(text);
    FIELD().PUBLIC().NAME(selectedItemPos);
    FIELD().PUBLIC().NAME(itemsCount);
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PUBLIC().NAME(onChangeCompleted);
    FIELD().PUBLIC().NAME(onExpand);
    FIELD().PUBLIC().NAME(onSelectedText);
    FIELD().PROTECTED().NAME(mEditBox);
    FIELD().PROTECTED().NAME(mItemsList);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10).NAME(mMaxListItems);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsFiltering);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsKeyboardNavigating);
    FIELD().PROTECTED().NAME(mOriginalItems);
}
END_META;
CLASS_METHODS_META(o2::EditBoxDropDown)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const EditBoxDropDown&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
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
    FUNCTION().PUBLIC().SIGNATURE(int, GetItemsCount);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllItems);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItemAt, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItemText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelectedItemPosition);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetSelectedItemText);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<EditBox>&, GetEditBox);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxListSizeInItems, int);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<List>&, GetListView);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, GetItem, int);
    FUNCTION().PROTECTED().SIGNATURE(void, SetItemSample, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupCallbacks);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateListSize);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveAndCheckClipping, const Vec2F&, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, FilterItems);
    FUNCTION().PROTECTED().SIGNATURE(void, NavigateNext);
    FUNCTION().PROTECTED().SIGNATURE(void, NavigatePrevious);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxTextChanged, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxTextCompleted, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectedText, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxUnfocused);
}
END_META;
// --- END META ---
