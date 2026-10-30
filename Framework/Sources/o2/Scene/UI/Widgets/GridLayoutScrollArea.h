#pragma once

#include "o2/Render/Sprite.h"
#include "ScrollArea.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"

namespace o2
{
    // ---------------------------------
    // Optimized grid layout with scroll
    // ---------------------------------
    class GridLayoutScrollArea: public ScrollArea
    {
    public:
        Function<int()>                   getItemsCountFunc; // Items count getting function
        Function<Vector<void*>(int, int)> getItemsRangeFunc; // Items getting in range function

        Function<void(const Ref<Widget>&, void*)> setupItemFunc; // Setup item widget function

    public:
        // Default constructor
        explicit GridLayoutScrollArea(RefCounter* refCounter);

        // Copy-constructor
        GridLayoutScrollArea(RefCounter* refCounter, const GridLayoutScrollArea& other);

        // Destructor
        ~GridLayoutScrollArea();

        // Copy-operator
        GridLayoutScrollArea& operator=(const GridLayoutScrollArea& other);

        // Sets item sample widget. WARNING: Removing all old items!
        void SetItemSample(const Ref<Widget>& sample);

        // Returns item sample widget
        const Ref<Widget>& GetItemSample() const;

        // Sets spacing between cell widgets
        void SetItemsSpacing(const Vec2F& spacing);

        // Returns spacing between cell widgets
        const Vec2F& GetItemsSpacing() const;

        // Sets are the items stretched to the view width, one in a line
        void SetItemsFitWidth(bool fit);

        // Returns are the items stretched to the view width, one in a line
        bool IsItemsFitWidth() const;

        // Updates items
        void OnItemsUpdated(bool itemsRearranged = false);

        // Scroll view to item
        void ScrollTo(void* item);

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(GridLayoutScrollArea);
        CLONEABLE_REF(GridLayoutScrollArea);

    protected:
        Ref<Widget>         mItemSample; // Item sample widget @SERIALIZABLE
        Vector<Ref<Widget>> mItemsPool;  // Items pool

        Vec2F mItemsSpacing; // Spacing between cell widgets @SERIALIZABLE

        bool  mItemsFitWidth = false; // Are the items stretched to the view width, one in a line @SERIALIZABLE
        float mPrevItemsWidth = 0.0f; // Width of the items at the previous update
        
        int mMinVisibleItemIdx = -1; // Visible item with minimal index
        int mMaxVisibleItemIdx = -1; // Visible item with maximal index

        int mPrevItemsInLine = 0; // Previous visible items on line count                                                 

    protected:
        // Called when object was deserialized and trying to reattach states animations target
        void OnDeserialized(const DataValue& node) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Calculates scroll area
        void CalculateScrollArea() override;

        // Moves scroll position and updates children widgets clipping and layout
        void MoveScrollPosition(const Vec2F& delta) override;

        // Returns items count, calls getItemsCountFunc
        virtual int GetItemsCount() const;

        // Returns items in range from start to end, calls getItemsRangeFunc
        virtual Vector<void*> GetItemsRange(int start, int end) const;

        // Sets item widget, calls setupItemFunc
        virtual void SetupItemWidget(const Ref<Widget>& widget, void* item);

        // Updates visible items
        virtual void UpdateVisibleItems();

        // Returns item widget under point and stores index in idxPtr, if not null
        Ref<Widget> GetItemUnderPoint(const Vec2F& point, int* idxPtr);

        // Returns count of items in one line
        int GetItemsInLine() const;

        // Puts the item into the pool: it leaves this, so it can't register in the drawables of this while pooled
        void FreeItemToPool(const Ref<Widget>& item);

        friend class DropDown;
        friend class CustomDropDown;
    };
}
// --- META ---

CLASS_BASES_META(o2::GridLayoutScrollArea)
{
    BASE_CLASS(o2::ScrollArea);
}
END_META;
CLASS_FIELDS_META(o2::GridLayoutScrollArea)
{
    FIELD().PUBLIC().NAME(getItemsCountFunc);
    FIELD().PUBLIC().NAME(getItemsRangeFunc);
    FIELD().PUBLIC().NAME(setupItemFunc);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mItemSample);
    FIELD().PROTECTED().NAME(mItemsPool);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mItemsSpacing);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mItemsFitWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPrevItemsWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mMinVisibleItemIdx);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mMaxVisibleItemIdx);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevItemsInLine);
}
END_META;
CLASS_METHODS_META(o2::GridLayoutScrollArea)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const GridLayoutScrollArea&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemSample, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Widget>&, GetItemSample);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemsSpacing, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(const Vec2F&, GetItemsSpacing);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemsFitWidth, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsItemsFitWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, OnItemsUpdated, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, ScrollTo, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, CalculateScrollArea);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveScrollPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(int, GetItemsCount);
    FUNCTION().PROTECTED().SIGNATURE(Vector<void*>, GetItemsRange, int, int);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupItemWidget, const Ref<Widget>&, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibleItems);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, GetItemUnderPoint, const Vec2F&, int*);
    FUNCTION().PROTECTED().SIGNATURE(int, GetItemsInLine);
    FUNCTION().PROTECTED().SIGNATURE(void, FreeItemToPool, const Ref<Widget>&);
}
END_META;
// --- END META ---
