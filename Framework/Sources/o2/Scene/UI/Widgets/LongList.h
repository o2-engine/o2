#pragma once

#include "o2/Render/Sprite.h"
#include "ScrollArea.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"

namespace o2
{
    // ---------------------------------------------
    // List view widget with selection and many data
    // ---------------------------------------------
    class LongList: public ScrollArea
    {

    public:
        PROPERTIES(LongList);
        PROPERTY(int, selectedItemPos, SelectItemAt, GetSelectedItemPosition); // Selected item position property    

    public:
        Function<void(int)> onFocused; // Select item position event

        Function<int()>                           getItemsCountFunc; // Items count getting function
        Function<Vector<void*>(int, int)>         getItemsRangeFunc; // Items getting in range function
        Function<void(const Ref<Widget>&, void*)> setupItemFunc;     // Setup item widget function

    public:
        // Default constructor
        explicit LongList(RefCounter* refCounter);

        // Copy-constructor
        LongList(RefCounter* refCounter, const LongList& other);

        // Destructor
        ~LongList();

        // Copy-operator
        LongList& operator=(const LongList& other);

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Draws widget
        void Draw() override;

        // Sets item sample widget. WARNING: Removing all old items!
        void SetItemSample(const Ref<Widget>& sample);

        // Returns item sample widget
        const Ref<Widget>& GetItemSample() const;

        // Selects item at position
        void SelectItemAt(int position);

        // Returns selected item position
        int GetSelectedItemPosition() const;

        // Sets selection drawable
        void SetSelectionDrawable(const Ref<IRectDrawable>& drawable);

        // Returns selection drawable as sprite, null when it is a drawable of another type
        Ref<Sprite> GetSelectionDrawable() const;

        // Returns selection drawable
        const Ref<IRectDrawable>& GetSelectionRectDrawable() const;

        // Sets hover drawable
        void SetHoverDrawable(const Ref<IRectDrawable>& drawable);

        // Returns hover drawable as sprite, null when it is a drawable of another type
        Ref<Sprite> GetHoverDrawable() const;

        // Returns hover drawable
        const Ref<IRectDrawable>& GetHoverRectDrawable() const;

        // Sets selection drawable layout (result rectangle will be calculated by item widget absolute rectangle)
        void SetSelectionDrawableLayout(const Layout& layout);

        // Returns selection drawable layout
        Layout GetSelectionDrawableLayout() const;

        // Sets hover drawable layout (result rectangle will be calculated by item widget absolute rectangle)
        void SetHoverDrawableLayout(const Layout& layout);

        // Returns hover drawable layout
        Layout GetHoverDrawableLayout() const;

        // Updates items
        void OnItemsUpdated(bool itemsRearranged = false);

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(LongList);
        CLONEABLE_REF(LongList);

    protected:
        Ref<Widget> mItemSample; // Item sample widget @SERIALIZABLE

        Ref<IRectDrawable> mSelectionDrawable;            // Selection drawable @SERIALIZABLE
        Ref<IRectDrawable> mHoverDrawable;                // Item hover drawable @SERIALIZABLE
        Layout  mSelectionLayout = Layout::BothStretch(); // Selection layout, result selection area depends on selected item @SERIALIZABLE
        Layout  mHoverLayout = Layout::BothStretch();     // Hover layout, result selection area depends on selected item @SERIALIZABLE
                                                 
        int mMinVisibleItemIdx = -1; // Visible item with minimal index
        int mMaxVisibleItemIdx = -1; // Visible item with maximal index
        int mSelectedItem = -1;      // Position of current selected item (-1 if no item isn't selected)
                                                 
        RectF mCurrentSelectionRect; // Current selection rectangle (for smoothing)
        RectF mTargetSelectionRect;  // Target selection rectangle (over selected item)
        RectF mCurrentHoverRect;     // Current hover rectangle (for smoothing)
        RectF mTargetHoverRect;      // Target hover rectangle (over selected item)
                                                 
        Vec2F mLastHoverCheckCursor;  // Last cursor position on hover check
        Vec2F mLastSelectCheckCursor; // Last cursor position on selection check
                                                 
        Vector<Ref<Widget>> mItemsPool; // Items pool

    protected:
        // Called when object was deserialized and trying to reattach states animations target
        void OnDeserialized(const DataValue& node) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Updates transparency for this and children widgets
        void UpdateTransparency() override;

        // Calculates scroll area
        void CalculateScrollArea() override;

        // Moves scroll position and updates children widgets clipping and layout
        void MoveScrollPosition(const Vec2F& delta) override;

        // Updates visible items
        void UpdateVisibleItems();

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

        // Returns item widget under point and stores index in idxPtr, if not null
        Ref<Widget> GetItemUnderPoint(const Vec2F& point, int* idxPtr);

        // Updates hover
        void UpdateHover(const Vec2F& point);

        // Updates selection
        void UpdateSelection(int position);

        // Called when selected item index was changed
        void OnSelectionChanged();

        friend class DropDown;
        friend class CustomDropDown;
    };
}
// --- META ---

CLASS_BASES_META(o2::LongList)
{
    BASE_CLASS(o2::ScrollArea);
}
END_META;
CLASS_FIELDS_META(o2::LongList)
{
    FIELD().PUBLIC().NAME(selectedItemPos);
    FIELD().PUBLIC().NAME(onFocused);
    FIELD().PUBLIC().NAME(getItemsCountFunc);
    FIELD().PUBLIC().NAME(getItemsRangeFunc);
    FIELD().PUBLIC().NAME(setupItemFunc);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mItemSample);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHoverDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mSelectionLayout);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mHoverLayout);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mMinVisibleItemIdx);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mMaxVisibleItemIdx);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mSelectedItem);
    FIELD().PROTECTED().NAME(mCurrentSelectionRect);
    FIELD().PROTECTED().NAME(mTargetSelectionRect);
    FIELD().PROTECTED().NAME(mCurrentHoverRect);
    FIELD().PROTECTED().NAME(mTargetHoverRect);
    FIELD().PROTECTED().NAME(mLastHoverCheckCursor);
    FIELD().PROTECTED().NAME(mLastSelectCheckCursor);
    FIELD().PROTECTED().NAME(mItemsPool);
}
END_META;
CLASS_METHODS_META(o2::LongList)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const LongList&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemSample, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Widget>&, GetItemSample);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItemAt, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelectedItemPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Sprite>, GetSelectionDrawable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetSelectionRectDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoverDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Sprite>, GetHoverDrawable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetHoverRectDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionDrawableLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetSelectionDrawableLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoverDrawableLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetHoverDrawableLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, OnItemsUpdated, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransparency);
    FUNCTION().PROTECTED().SIGNATURE(void, CalculateScrollArea);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveScrollPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibleItems);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, GetItemUnderPoint, const Vec2F&, int*);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHover, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSelection, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
}
END_META;
// --- END META ---
