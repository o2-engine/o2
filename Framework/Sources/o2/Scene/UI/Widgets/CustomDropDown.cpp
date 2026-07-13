#include "o2/stdafx.h"
#include "CustomDropDown.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widgets/CustomList.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"

namespace o2
{
    CustomDropDown::CustomDropDown(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this)
    {
        mItemsList = mmake<CustomList>();
        mItemsList->SetInternalParent(Ref(this), false);
        mItemsList->onSelectedItem += [&](auto x) { OnItemSelected(); };
        mItemsList->SetMultiselectionAvailable(false);
    }

    CustomDropDown::CustomDropDown(RefCounter* refCounter, const CustomDropDown& other):
        Widget(refCounter, other), DrawableCursorEventsListener(this), mClipLayout(other.mClipLayout),
        mMaxListItems(other.mMaxListItems)
    {
        mItemsList = FindInternalWidgetByType<CustomList>();
        mItemsList->onSelectedItem += [&](auto x) { OnItemSelected(); };
        mItemsList->Hide(true);
        mItemsList->SetMultiselectionAvailable(false);

        RetargetStatesAnimations();
    }

    CustomDropDown::~CustomDropDown()
    {}

    CustomDropDown& CustomDropDown::operator=(const CustomDropDown& other)
    {
        Widget::operator=(other);

        mItemsList = FindInternalWidgetByType<CustomList>();
        mItemsList->onSelectedItem += [&](auto x) { OnItemSelected(); };
        mItemsList->Hide(true);
        mItemsList->SetMultiselectionAvailable(false);

        mClipLayout = other.mClipLayout;
        mMaxListItems = other.mMaxListItems;

        RetargetStatesAnimations();

        return *this;
    }

    void CustomDropDown::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy)
            return;

        Widget::Draw();

        o2UI.DrawWidgetAtTop(mItemsList);

        auto selectedItem = mItemsList->GetItem(mItemsList->GetSelectedItemPos());
        if (selectedItem)
        {
            o2Render.EnableScissorTest(mAbsoluteClip);
            selectedItem->ForceDraw(mAbsoluteClip, mResTransparency);
            o2Render.DisableScissorTest();
        }

        DrawDebugFrame();
    }

    void CustomDropDown::Expand()
    {
        onBeforeExpand();

        float itemHeight = mItemsList->GetItemSample()->layout->minHeight;
        int itemsVisible = Math::Min(mMaxListItems, mItemsList->GetItemsCount());

        mItemsList->layout->minHeight = itemHeight*(float)itemsVisible +
            mItemsList->mViewAreaLayout.offsetMin.y - mItemsList->mViewAreaLayout.offsetMax.y;

        auto openedState = state["opened"];
        if (openedState)
            *openedState = true;

        mItemsList->SetEnabled(true);
        mItemsList->UpdateSelfTransform();
        mItemsList->UpdateChildrenTransforms();

        SetLayoutDirty();
    }

    void CustomDropDown::Collapse()
    {
        auto openedState = state["opened"];
        if (openedState)
            *openedState = false;

        mItemsList->SetEnabled(false);
    }

    bool CustomDropDown::IsExpanded() const
    {
        return mItemsList->IsEnabled();
    }

    void CustomDropDown::SetItemSample(const Ref<Widget>& sample)
    {
        mItemsList->SetItemSample(sample);
    }

    const Ref<Widget>& CustomDropDown::GetItemSample() const
    {
        return mItemsList->GetItemSample();
    }

    const Ref<VerticalLayout>& CustomDropDown::GetItemsLayout() const
    {
        return mItemsList->GetItemsLayout();
    }

    Ref<Widget> CustomDropDown::AddItem()
    {
        return mItemsList->AddItem();
    }

    Ref<Widget> CustomDropDown::AddItem(int position)
    {
        return mItemsList->AddItem(position);
    }

    void CustomDropDown::RemoveItem(const Ref<Widget>& item)
    {
        mItemsList->RemoveItem(item);
    }

    void CustomDropDown::RemoveItem(int position)
    {
        mItemsList->RemoveItem(position);
    }

    void CustomDropDown::MoveItem(int position, int newPosition)
    {
        mItemsList->MoveItem(position, newPosition);
    }

    void CustomDropDown::MoveItem(const Ref<Widget>& item, int newPosition)
    {
        mItemsList->MoveItem(item, newPosition);
    }

    int CustomDropDown::GetItemPosition(const Ref<Widget>& item)
    {
        return mItemsList->GetItemPosition(item);
    }

    Ref<Widget> CustomDropDown::GetItem(int position) const
    {
        return mItemsList->GetItem(position);
    }

    void CustomDropDown::RemoveAllItems()
    {
        mItemsList->RemoveAllItems();
    }

    void CustomDropDown::SortItems(const Function<bool(const Ref<Widget>&, const Ref<Widget>&)>& sortFunc)
    {
        mItemsList->SortItems(sortFunc);
    }

    int CustomDropDown::GetItemsCount() const
    {
        return mItemsList->GetItemsCount();
    }

    void CustomDropDown::SelectItem(const Ref<Widget>& item)
    {
        mItemsList->SelectItem(item);
    }

    void CustomDropDown::SelectItemAt(int position)
    {
        mItemsList->SelectItemAt(position);
    }

    Ref<Widget> CustomDropDown::GetSelectedItem() const
    {
        return mItemsList->GetSelectedItem();
    }

    int CustomDropDown::GetSelectedItemPosition() const
    {
        return mItemsList->GetSelectedItemPos();
    }

    const Ref<CustomList>& CustomDropDown::GetListView() const
    {
        return mItemsList;
    }

    void CustomDropDown::SetMaxListSizeInItems(int itemsCount)
    {
        mMaxListItems = itemsCount;
    }

    void CustomDropDown::SetClippingLayout(const Layout& layout)
    {
        mClipLayout = layout;
        SetLayoutDirty();
    }

    Layout CustomDropDown::GetClippingLayout()
    {
        return mClipLayout;
    }

    void CustomDropDown::MoveAndCheckClipping(const Vec2F& delta, const RectF& clipArea)
    {
        mBoundsWithChilds += delta;
        mIsClipped = !mBoundsWithChilds.IsIntersects(clipArea);

        if (!mIsClipped)
            UpdateSelfTransform();

        for (auto& child : mChildWidgets)
            child->MoveAndCheckClipping(delta, clipArea);

        if (IsExpanded())
            Collapse();
    }

    void CustomDropDown::OnCursorPressed(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = true;
    }

    void CustomDropDown::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        if (Widget::IsUnderPoint(cursor.position))
        {
            if (!IsExpanded())
                Expand();
        }
    }

    void CustomDropDown::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {
        if (!mItemsList->layout->IsPointInside(o2Input.GetCursorPos()) && IsExpanded())
            Collapse();
    }

    void CustomDropDown::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;
    }

    void CustomDropDown::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;
    }

    void CustomDropDown::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;
    }

    void CustomDropDown::OnEnabled()
    {
        Widget::OnEnabled();

        interactable = true;
    }

    void CustomDropDown::OnDisabled()
    {
        Widget::OnDisabled();

        interactable = false;
    }

    void CustomDropDown::UpdateSelfTransform()
{
        layout->Update();
        mAbsoluteClip = mClipLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
    }

    String CustomDropDown::GetCreateMenuGroup()
    {
        return "Dropping";
    }

    void CustomDropDown::OnItemSelected()
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        Collapse();
        onSelectedPos(mItemsList->GetSelectedItemPos());
        onSelectedItem(mItemsList->GetSelectedItem());

        OnSelectionChanged();
    }

    void CustomDropDown::OnSelectionChanged()
    {}
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::CustomDropDown>);
// --- META ---

DECLARE_CLASS(o2::CustomDropDown, o2__CustomDropDown);
// --- END META ---
