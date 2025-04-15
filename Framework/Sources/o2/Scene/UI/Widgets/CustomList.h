#pragma once

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Events/KeyboardEventsListener.h"

namespace o2
{
    // -------------------------------
    // List view widget with selection
    // -------------------------------
    class CustomList : public ScrollArea, public KeyboardEventsListener
    {
    public:
        PROPERTIES(CustomList);
        PROPERTY(Vector<int>, selectedItems, SetSelectedItems, GetSelectedItems); // Selected item widget property
        PROPERTY(Ref<Widget>, selectedItem, SelectItem, GetSelectedItem);         // Selected item widget
        PROPERTY(int, selectedItemPos, SelectItemAt, GetSelectedItemPos);         // Selected item position property
        GETTER(int, itemsCount, GetItemsCount);                                   // All items count getter

    public:
        Function<void(int)>                onSelectedPos;   // Select item position event
        Function<void(const Ref<Widget>&)> onSelectedItem;  // Select item event

    public:
        // Default constructor
        explicit CustomList(RefCounter* refCounter);

        // Copy-constructor
        CustomList(RefCounter* refCounter, const CustomList& other);

        // Destructor
        ~CustomList();

        // Copy-operator
        CustomList& operator=(const CustomList& other);

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Draws widget
        void Draw() override;

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

        // Sets items selection
        void SetSelectedItems(const Vector<int>& items);

        // Clears selection
        void ClearSelection();

        // Returns selected item
        Vector<int> GetSelectedItems() const;

        // Returns selected item position
        int GetSelectedItemPos() const;

        // Returns selected item widget
        Ref<Widget> GetSelectedItem() const;

        // Sets multi selection
        void SetMultiselectionAvailable(bool available);

        // Returns is multi selection available
        bool IsMultiselectionAvailable() const;

        // Returns selection drawable
        const Ref<Sprite>& GetSelectionDrawable() const;

        // Returns hover drawable
        const Ref<Sprite>& GetHoverDrawable() const;

        // Sets selection drawable layout (result rectangle will be calculated by item widget absolute rectangle)
        void SetSelectionDrawableLayout(const Layout& layout);

        // Returns selection drawable layout
        Layout GetSelectionDrawableLayout() const;

        // Sets hover drawable layout (result rectangle will be calculated by item widget absolute rectangle)
        void SetHoverDrawableLayout(const Layout& layout);

        // Returns hover drawable layout
        Layout GetHoverDrawableLayout() const;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(CustomList);
        CLONEABLE_REF(CustomList);

    protected:
        // ------------------
        // Selected item info
        // ------------------
        struct Selection
        {
            int         idx;       // Item index
            Ref<Sprite> selection; // Selection sprite

            // Check equals operator
            bool operator==(const Selection& other) const;
        };

    protected:
        Ref<VerticalLayout> mVerLayout;  // Child vertical layout
        Ref<Widget>         mItemSample; // Item sample widget @SERIALIZABLE

        bool              mMultiSelection = true; // Is multi selection available @SERIALIZABLE
        Vector<Selection> mSelectedItems;         // Current selected items

        Ref<Sprite> mSelectionDrawable; // Selection sprite @SERIALIZABLE
        Ref<Sprite> mHoverDrawable;     // Item hover drawable @SERIALIZABLE

        Layout mSelectionLayout = Layout::BothStretch(); // Selection layout, result selection area depends on selected item @SERIALIZABLE
        Layout mHoverLayout = Layout::BothStretch();     // Hover layout, result selection area depends on selected item @SERIALIZABLE

        RectF mCurrentHoverRect; // Current hover rectangle (for smoothing)
        RectF mTargetHoverRect;  // Target hover rectangle (over selected item)

        Vec2F mLastHoverCheckCursor;  // Last cursor position on hover check
        Vec2F mLastSelectCheckCursor; // Last cursor position on selection check

        Vector<Ref<Sprite>> mSelectionSpritesPool; // Selection sprites pool

    protected:
        // Called when object was deserialized and trying to reattach states animations target
        void OnDeserialized(const DataValue& node) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Updates transparency for this and children widgets
        void UpdateTransparency() override;

        // Called when transformation was changed and updated
        void OnTransformUpdated() override;

        // Called when selected item index was changed
        virtual void OnSelectionChanged();

        // Moves scroll position and updates children widgets clipping and layout
        void MoveScrollPosition(const Vec2F& delta) override;

        // Updates selections sprites rectangles
        void UpdateSelectionSprites();

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when key was pressed when widget is focused
        void OnKeyPressed(const Input::Key& key) override;

        // Returns item widget under point and stores index in idxPtr, if not null
        Ref<Widget> GetItemUnderPoint(const Vec2F& point, int* idxPtr);

        // Updates hover
        void UpdateHover(const Vec2F& point);

        // Returns selection sprite
		Ref<Sprite> GetSelectionSprite();

		REF_COUNTERABLE_IMPL(ScrollArea);

        friend class DropDown;
        friend class CustomDropDown;
    };
}
// --- META ---

CLASS_BASES_META(o2::CustomList)
{
    BASE_CLASS(o2::ScrollArea);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::CustomList)
{
    FIELD().PUBLIC().NAME(selectedItems);
    FIELD().PUBLIC().NAME(selectedItem);
    FIELD().PUBLIC().NAME(selectedItemPos);
    FIELD().PUBLIC().NAME(itemsCount);
    FIELD().PUBLIC().NAME(onSelectedPos);
    FIELD().PUBLIC().NAME(onSelectedItem);
    FIELD().PROTECTED().NAME(mVerLayout);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mItemSample);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mMultiSelection);
    FIELD().PROTECTED().NAME(mSelectedItems);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHoverDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mSelectionLayout);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mHoverLayout);
    FIELD().PROTECTED().NAME(mCurrentHoverRect);
    FIELD().PROTECTED().NAME(mTargetHoverRect);
    FIELD().PROTECTED().NAME(mLastHoverCheckCursor);
    FIELD().PROTECTED().NAME(mLastSelectCheckCursor);
    FIELD().PROTECTED().NAME(mSelectionSpritesPool);
}
END_META;
CLASS_METHODS_META(o2::CustomList)
{

    typedef const Function<bool(const Ref<Widget>&, const Ref<Widget>&)>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const CustomList&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
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
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedItems, const Vector<int>&);
    FUNCTION().PUBLIC().SIGNATURE(void, ClearSelection);
    FUNCTION().PUBLIC().SIGNATURE(Vector<int>, GetSelectedItems);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelectedItemPos);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetSelectedItem);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMultiselectionAvailable, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMultiselectionAvailable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetSelectionDrawable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetHoverDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionDrawableLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetSelectionDrawableLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoverDrawableLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetHoverDrawableLayout);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransparency);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveScrollPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSelectionSprites);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, GetItemUnderPoint, const Vec2F&, int*);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHover, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Sprite>, GetSelectionSprite);
}
END_META;
// --- END META ---
