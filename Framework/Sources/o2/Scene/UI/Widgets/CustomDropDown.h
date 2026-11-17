#pragma once

#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Scene/UI/Widgets/CustomList.h"

namespace o2
{
    // ---------------------------------
    // Custom items drop down ui element
    // ---------------------------------
    class CustomDropDown: public Widget, public DrawableCursorEventsListener
    {
    public:
        PROPERTIES(CustomDropDown);
        PROPERTY(Ref<Widget>, selectedItem, SelectItem, GetSelectedItem);      // Selected item widget property
        PROPERTY(int, selectedItemPos, SelectItemAt, GetSelectedItemPosition); // Selected item position property
        GETTER(int, itemsCount, GetItemsCount);                                // All items count getter

    public:
        Function<void()> onBeforeExpand; // Called before opening 

        Function<void(int)>                onSelectedPos;   // Select item position event
        Function<void(const Ref<Widget>&)> onSelectedItem;  // Select item event

    public:
        // Default constructor
        explicit CustomDropDown(RefCounter* refCounter);

        // Copy-constructor
        CustomDropDown(RefCounter* refCounter, const CustomDropDown& other);

        // Destructor
        ~CustomDropDown();

        // Copy operator
        CustomDropDown& operator=(const CustomDropDown& other);

        // Draws widget
        void Draw() override;

        // Expand list
        void Expand();

        // Collapse list
        void Collapse();

        // Returns is list expanded
        bool IsExpanded() const;

        // Sets item sample widget. WARNING: Removing all old items!
        void SetItemSample(const Ref<Widget>& sample);

        // Returns item sample widget
        const Ref<Widget>& GetItemSample() const;

        // Returns layout of items
        const Ref<VerticalLayout>& GetItemsLayout() const;

        // Adds new item and returns it
        Ref<Widget> AddItem();

        // Adds new item at position and returns it
        Ref<Widget> AddItem(int position);

        // Removes item
        void RemoveItem(const Ref<Widget>& item);

        // Removes item in position
        void RemoveItem(int position);

        // Moves item from position to new position
        void MoveItem(int position, int newPosition);

        // Moves item to new position
        void MoveItem(const Ref<Widget>& item, int newPosition);

        // Returns item position
        int GetItemPosition(const Ref<Widget>& item);

        // Returns item by position
        Ref<Widget> GetItem(int position) const;

        // Removes all items
        void RemoveAllItems();

        // Sorts items
        void SortItems(const Function<bool(const Ref<Widget>&, const Ref<Widget>&)>& sortFunc);

        // Returns items count
        int GetItemsCount() const;

        // Selects item
        void SelectItem(const Ref<Widget>& item);

        // Selects item at position
        void SelectItemAt(int position);

        // Returns selected item
        Ref<Widget> GetSelectedItem() const;

        // Returns selected item position
        int GetSelectedItemPosition() const;

        // Returns list view 
        const Ref<CustomList>& GetListView() const;

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

        // Returns the icon of the type in the editor
        static String GetEditorIcon();

        SERIALIZABLE(CustomDropDown);
        CLONEABLE_REF(CustomDropDown);

    protected:
        Ref<CustomList> mItemsList; // List view

        Layout mClipLayout = Layout::BothStretch(); // Clipping layout @SERIALIZABLE
        RectF  mAbsoluteClip;                       // Absolute clipping rectangle

        int mMaxListItems = 10; // Maximum visible items in list @SERIALIZABLE

    protected:
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

        // Called when item was selected in list
        void OnItemSelected();

        // Called when selection was changed
        virtual void OnSelectionChanged();

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(o2::CustomDropDown)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::CustomDropDown)
{
    FIELD().PUBLIC().NAME(selectedItem);
    FIELD().PUBLIC().NAME(selectedItemPos);
    FIELD().PUBLIC().NAME(itemsCount);
    FIELD().PUBLIC().NAME(onBeforeExpand);
    FIELD().PUBLIC().NAME(onSelectedPos);
    FIELD().PUBLIC().NAME(onSelectedItem);
    FIELD().PROTECTED().NAME(mItemsList);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mClipLayout);
    FIELD().PROTECTED().NAME(mAbsoluteClip);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10).NAME(mMaxListItems);
}
END_META;
CLASS_METHODS_META(o2::CustomDropDown)
{

    typedef const Function<bool(const Ref<Widget>&, const Ref<Widget>&)>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const CustomDropDown&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemSample, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Widget>&, GetItemSample);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<VerticalLayout>&, GetItemsLayout);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddItem);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddItem, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, int);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveItem, int, int);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveItem, const Ref<Widget>&, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetItemPosition, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetItem, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllItems);
    FUNCTION().PUBLIC().SIGNATURE(void, SortItems, _tmp1);
    FUNCTION().PUBLIC().SIGNATURE(int, GetItemsCount);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItem, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItemAt, int);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetSelectedItem);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelectedItemPosition);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<CustomList>&, GetListView);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxListSizeInItems, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetClippingLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetClippingLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveAndCheckClipping, const Vec2F&, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnItemSelected);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
}
END_META;
// --- END META ---
