#include "o2/stdafx.h"
#include "GridLayoutScrollArea.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"

namespace o2
{
    GridLayoutScrollArea::GridLayoutScrollArea(RefCounter* refCounter):
        ScrollArea(refCounter)
    {
        mInheritedDrawablesManualOrder = true;

        mItemSample = mmake<Widget>();
    }

    GridLayoutScrollArea::GridLayoutScrollArea(RefCounter* refCounter, const GridLayoutScrollArea& other):
        ScrollArea(refCounter, other), mItemsSpacing(other.mItemsSpacing)
    {
        mInheritedDrawablesManualOrder = true;

        mItemSample = other.mItemSample->CloneAsRef<Widget>();
        mItemSample->UpdateTransform();

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    GridLayoutScrollArea::~GridLayoutScrollArea()
    {}

    GridLayoutScrollArea& GridLayoutScrollArea::operator=(const GridLayoutScrollArea& other)
    {
        ScrollArea::operator=(other);
        mItemsSpacing = other.mItemsSpacing;

        mItemSample = other.mItemSample->CloneAsRef<Widget>();
        mItemSample->UpdateSelfTransform();
        mItemSample->UpdateChildrenTransforms();

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void GridLayoutScrollArea::SetItemSample(const Ref<Widget>& sample)
    {
        mItemSample = sample;

        SetLayoutDirty();
    }

    const Ref<Widget>& GridLayoutScrollArea::GetItemSample() const
    {
        return mItemSample;
    }

    void GridLayoutScrollArea::SetItemsSpacing(const Vec2F& spacing)
    {
        mItemsSpacing = spacing;
        SetLayoutDirty();
    }

    const Vec2F& GridLayoutScrollArea::GetItemsSpacing() const
    {
        return mItemsSpacing;
    }

    void GridLayoutScrollArea::OnItemsUpdated(bool itemsRearranged /*= false*/)
    {
        if (itemsRearranged)
        {
            Vector<Ref<Widget>> removingItems;
            for (int i = mMinVisibleItemIdx; i <= mMaxVisibleItemIdx; i++)
            {
                if (i < 0)
                    continue;

                removingItems.Add(mChildWidgets[i - mMinVisibleItemIdx]);
            }

            for (auto item : removingItems)
                mItemsPool.Add(item);

            mChildren.Clear();
            mChildWidgets.Clear();
            mChildrenInheritedDepth.Clear();

            mMinVisibleItemIdx = -1;
            mMaxVisibleItemIdx = -1;
        }

        SetLayoutDirty();
    }

    void GridLayoutScrollArea::ScrollTo(void* item)
    {
        int idx = GetItemsRange(0, GetItemsCount()).IndexOf(item);
        if (idx < 0)
            return;

        int itemsInLine = GetItemsInLine();
        SetScrollForcible(Vec2F(0, idx/itemsInLine*(mItemSample->layout->GetMinHeight() + mItemsSpacing.y)));
    }

    void GridLayoutScrollArea::CalculateScrollArea()
    {
        Vec2F offset;
        InitializeScrollAreaRectCalculation(offset);

        float itemsLinesHeight = Math::Ceil((float)GetItemsCount()/(float)GetItemsInLine())*
            (mItemSample->layout->GetMinHeight() + mItemsSpacing.y);

        RecalculateScrollAreaRect(RectF(0, mAbsoluteViewArea.Height(), mAbsoluteViewArea.Width(),
                                        mAbsoluteViewArea.Height() - itemsLinesHeight), Vec2F());
    }

    void GridLayoutScrollArea::UpdateSelfTransform()
    {
        UpdateVisibleItems();
        ScrollArea::UpdateSelfTransform();
    }

    String GridLayoutScrollArea::GetCreateMenuGroup()
    {
        return "Layout";
    }

    void GridLayoutScrollArea::MoveScrollPosition(const Vec2F& delta)
    {
        ScrollArea::MoveScrollPosition(delta);
        UpdateVisibleItems();
    }

    int GridLayoutScrollArea::GetItemsCount() const
    {
        return getItemsCountFunc();
    }

    Vector<void*> GridLayoutScrollArea::GetItemsRange(int start, int end) const
    {
        return getItemsRangeFunc(start, end);
    }

    void GridLayoutScrollArea::SetupItemWidget(const Ref<Widget>& widget, void* item)
    {
        setupItemFunc(widget, item);
    }

    void GridLayoutScrollArea::UpdateVisibleItems()
    {
        int lastMinItemIdx = mMinVisibleItemIdx;
        int lastMaxItemIdx = mMaxVisibleItemIdx;

        float itemHeight = mItemSample->layout->minHeight;
        float itemWidth = mItemSample->layout->minWidth;
        int itemsInLine = GetItemsInLine();

        bool rebuild = itemsInLine != mPrevItemsInLine;

        if (itemHeight < FLT_EPSILON)
            return;

        int itemsCount = GetItemsCount();

        mMinVisibleItemIdx = Math::FloorToInt(mScrollPos.y/(itemHeight + mItemsSpacing.y))*itemsInLine;
        mMaxVisibleItemIdx = Math::CeilToInt((mScrollPos.y + mAbsoluteViewArea.Height())/(itemHeight + mItemsSpacing.y))*itemsInLine;

        mMinVisibleItemIdx = Math::Min(mMinVisibleItemIdx, itemsCount);
        mMaxVisibleItemIdx = Math::Min(mMaxVisibleItemIdx, itemsCount - 1);

        auto itemsInRange = GetItemsRange(mMinVisibleItemIdx, mMaxVisibleItemIdx + 1);
        Vector<Ref<Widget>> itemsWidgets;
        itemsWidgets.Resize(mMaxVisibleItemIdx - mMinVisibleItemIdx + 1);

        Vector<Ref<Widget>> removingItems;
        for (int i = lastMinItemIdx; i <= lastMaxItemIdx; i++)
        {
            if (i < 0)
                continue;

            if (i < mMinVisibleItemIdx || i > mMaxVisibleItemIdx || rebuild)
                removingItems.Add(mChildWidgets[i - lastMinItemIdx]);
            else
                itemsWidgets[i - mMinVisibleItemIdx] = mChildWidgets[i - lastMinItemIdx];
        }

        for (auto item : removingItems)
            mItemsPool.Add(item);

        mChildren.Clear();
        mChildWidgets.Clear();
        mChildrenInheritedDepth.Clear();

        auto thisRef = Ref(this);

        for (int i = mMinVisibleItemIdx; i <= mMaxVisibleItemIdx; i++)
        {
            if (i < 0)
                continue;

            if (i >= lastMinItemIdx && i <= lastMaxItemIdx && !rebuild)
                continue;

            if (mItemsPool.IsEmpty())
                mItemsPool.Add(mItemSample->CloneAsRef<Widget>());

            auto newItem = mItemsPool.PopBack();

            SetupItemWidget(newItem, itemsInRange[i - mMinVisibleItemIdx]);

            *newItem->layout = WidgetLayout::Based(BaseCorner::LeftTop, newItem->layout->GetMinimalSize(),
                                                   Vec2F((itemWidth + mItemsSpacing.x)*(float)(i % itemsInLine),
                                                         -(itemHeight + mItemsSpacing.y)*(float)(i/itemsInLine)));

            newItem->layout->minSize = Vec2F(itemWidth, itemHeight);

            newItem->mParent = thisRef;
            newItem->mParentWidget = thisRef;

            itemsWidgets[i - mMinVisibleItemIdx] = newItem;

            newItem->UpdateSelfTransform();
            newItem->UpdateChildrenTransforms();
            newItem->mIsClipped = false;
        }

        mChildren.Add(DynamicCastVector<Actor>(itemsWidgets));
        mChildWidgets.Add(itemsWidgets);
        mChildrenInheritedDepth.Add(DynamicCastVector< ISceneDrawable>(itemsWidgets));

        mPrevItemsInLine = itemsInLine;
    }

    Ref<Widget> GridLayoutScrollArea::GetItemUnderPoint(const Vec2F& point, int* idxPtr)
    {
        int idx = mMinVisibleItemIdx;
        for (auto& child : mChildWidgets)
        {
            if (child->layout->IsPointInside(point))
            {
                if (idxPtr)
                    *idxPtr = idx;

                return child;
            }

            idx++;
        }

        if (idxPtr)
            *idxPtr = -1;

        return nullptr;
    }

    int GridLayoutScrollArea::GetItemsInLine() const
    {
        float itemWidth = mItemSample->layout->GetMinWidth();
        return Math::Max(1, Math::FloorToInt((mAbsoluteViewArea.Width() - itemWidth)/
            (itemWidth + mItemsSpacing.x) + 1.0f));
    }

    void GridLayoutScrollArea::OnDeserialized(const DataValue& node)
    {
        ScrollArea::OnDeserialized(node);
        RetargetStatesAnimations();
    }

    void GridLayoutScrollArea::OnEnabled()
    {
        Widget::OnEnabled();

        SetInteractable(true);
    }

    void GridLayoutScrollArea::OnDisabled()
    {
        Widget::OnDisabled();

        SetInteractable(false);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::GridLayoutScrollArea>);
// --- META ---

DECLARE_CLASS(o2::GridLayoutScrollArea, o2__GridLayoutScrollArea);
// --- END META ---
