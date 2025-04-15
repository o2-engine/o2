#include "o2/stdafx.h"
#include "EditBoxDropDown.h"

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"

namespace o2
{
    EditBoxDropDown::EditBoxDropDown(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this)
    {
        mEditBox = mmake<EditBox>();
        mEditBox->SetInternalParent(Ref(this), false);
        mEditBox->onChanged += [&](auto text) { OnEditBoxTextChanged(text); };
        mEditBox->onChangeCompleted += [&](auto text) { OnEditBoxTextCompleted(text); };
        mEditBox->onFocused += [&]() { OnEditBoxFocused(); };
        mEditBox->onUnfocused += [&]() { OnEditBoxUnfocused(); };
        mEditBox->SetMultiLine(false);

        mItemsList = mmake<CustomList>();
        mItemsList->SetInternalParent(Ref(this), false);
        mItemsList->onSelectedItem += [&](auto x) { OnItemSelected(); };
        mItemsList->SetMultiselectionAvailable(false);
        mItemsList->Hide(true);

        auto itemSample = mmake<Label>();
        itemSample->horOverflow = Label::HorOverflow::Dots;
        SetItemSample(itemSample);
    }

    EditBoxDropDown::EditBoxDropDown(RefCounter* refCounter, const EditBoxDropDown& other):
        Widget(refCounter, other), DrawableCursorEventsListener(this), mClipLayout(other.mClipLayout),
        mMaxListItems(other.mMaxListItems), text(this), value(this), selectedItemPos(this), itemsCount(this)
    {
        mEditBox = FindInternalWidgetByType<EditBox>();
        mEditBox->onChanged += [&](auto text) { OnEditBoxTextChanged(text); };
        mEditBox->onChangeCompleted += [&](auto text) { OnEditBoxTextCompleted(text); };
        mEditBox->onFocused += [&]() { OnEditBoxFocused(); };
        mEditBox->onUnfocused += [&]() { OnEditBoxUnfocused(); };
        
        mItemsList = FindInternalWidgetByType<CustomList>();
        mItemsList->onSelectedItem += [&](auto x) { OnItemSelected(); };
        mItemsList->Hide(true);
        mItemsList->SetMultiselectionAvailable(false);

        // Copy original items
        mOriginalItems = other.mOriginalItems;

        RetargetStatesAnimations();
    }

    EditBoxDropDown::~EditBoxDropDown()
    {}

    EditBoxDropDown& EditBoxDropDown::operator=(const EditBoxDropDown& other)
    {
        Widget::operator=(other);

        mEditBox = FindInternalWidgetByType<EditBox>();
        mEditBox->onChanged += [&](auto text) { OnEditBoxTextChanged(text); };
        mEditBox->onChangeCompleted += [&](auto text) { OnEditBoxTextCompleted(text); };
        mEditBox->onFocused += [&]() { OnEditBoxFocused(); };
        mEditBox->onUnfocused += [&]() { OnEditBoxUnfocused(); };

        mItemsList = FindInternalWidgetByType<CustomList>();
        mItemsList->onSelectedItem += [&](auto x) { OnItemSelected(); };
        mItemsList->Hide(true);
        mItemsList->SetMultiselectionAvailable(false);

        mClipLayout = other.mClipLayout;
        mMaxListItems = other.mMaxListItems;
        mOriginalItems = other.mOriginalItems;

        RetargetStatesAnimations();

        return *this;
    }

    void EditBoxDropDown::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy)
            return;

        Widget::Draw();

        o2UI.DrawWidgetAtTop(mItemsList);

        DrawDebugFrame();
    }

    void EditBoxDropDown::Update(float dt)
    {
        Widget::Update(dt);

        if (!mResEnabledInHierarchy || mIsClipped)
            return;
    }

    void EditBoxDropDown::SetText(const WString& text)
    {
        mEditBox->SetText(text);
        
        // Apply filter to items 
        if (IsExpanded())
            FilterItems();
    }

    WString EditBoxDropDown::GetText() const
    {
        return mEditBox->GetText();
    }

    void EditBoxDropDown::Expand()
    {
        onBeforeExpand();

        float itemHeight = mItemsList->GetItemSample()->layout->minHeight;
        int itemsVisible = Math::Min(mMaxListItems, mItemsList->GetItemsCount());

        // Calculate approximate view size offset
        float viewOffsetY = itemHeight * 0.2f; // Approximate offset for spacing

        mItemsList->layout->minHeight = itemHeight * (float)itemsVisible + viewOffsetY;

        auto openedState = state["opened"];
        if (openedState)
            *openedState = true;

        mItemsList->SetEnabled(true);
        mItemsList->UpdateSelfTransform();
        mItemsList->UpdateChildrenTransforms();

        // Filter items based on current text
        FilterItems();

        SetLayoutDirty();
    }

    void EditBoxDropDown::Collapse()
    {
        auto openedState = state["opened"];
        if (openedState)
            *openedState = false;

        mItemsList->SetEnabled(false);
    }

    bool EditBoxDropDown::IsExpanded() const
    {
        return mItemsList->IsEnabled();
    }

    int EditBoxDropDown::AddItem(const WString& text)
    {
        Ref<Widget> widget = mItemsList->AddItem();
        auto item = DynamicCast<Label>(widget);
        if (item)
            item->SetText(text);
        
        mOriginalItems.Add(text);
        return GetItemsCount() - 1;
    }

    int EditBoxDropDown::AddItem(const WString& text, int position)
    {
        Ref<Widget> widget = mItemsList->AddItem(position);
        auto item = DynamicCast<Label>(widget);
        if (item)
            item->SetText(text);
        
        if (position >= 0 && position <= mOriginalItems.Count())
            mOriginalItems.Insert(position, text);
        else
            mOriginalItems.Add(text);
            
        return position;
    }

    void EditBoxDropDown::AddItems(const Vector<WString>& data)
    {
        for (auto& text : data)
            AddItem(text);
    }

    void EditBoxDropDown::RemoveItem(int position)
    {
        if (position >= 0 && position < GetItemsCount())
        {
            mItemsList->RemoveItem(position);
            
            if (position < mOriginalItems.Count())
                mOriginalItems.RemoveAt(position);
        }
    }

    void EditBoxDropDown::RemoveItem(const WString& text)
    {
        int position = FindItem(text);
        if (position >= 0)
            RemoveItem(position);
    }

    int EditBoxDropDown::FindItem(const WString& text)
    {
        for (int i = 0; i < mItemsList->GetItemsCount(); i++)
        {
            auto item = DynamicCast<Label>(GetItem(i));
            if (item && item->GetText() == text)
                return i;
        }

        return -1;
    }

    WString EditBoxDropDown::GetItemText(int position) const
    {
        if (position >= 0 && position < mItemsList->GetItemsCount())
        {
            auto item = DynamicCast<Label>(GetItem(position));
            if (item)
                return item->GetText();
        }
        return WString();
    }

    Vector<WString> EditBoxDropDown::GetAllItemsText() const
    {
        Vector<WString> res;
        for (int i = 0; i < mItemsList->GetItemsCount(); i++)
        {
            auto item = DynamicCast<Label>(GetItem(i));
            if (item)
                res.Add(item->GetText());
        }
        return res;
    }

    void EditBoxDropDown::MoveItem(int position, int newPosition)
    {
        if (position >= 0 && position < GetItemsCount() && 
            newPosition >= 0 && newPosition < GetItemsCount())
        {
            mItemsList->MoveItem(position, newPosition);
            
            // Update original items list
            if (position < mOriginalItems.Count() && newPosition < mOriginalItems.Count())
            {
                WString item = mOriginalItems[position];
                mOriginalItems.RemoveAt(position);
                mOriginalItems.Insert(newPosition, item);
            }
        }
    }

    Ref<Widget> EditBoxDropDown::GetItem(int position) const
    {
        return mItemsList->GetItemsCount() > position ? mItemsList->GetItem(position) : nullptr;
    }

    void EditBoxDropDown::RemoveAllItems()
    {
        mItemsList->RemoveAllItems();
        mOriginalItems.Clear();
    }

    int EditBoxDropDown::GetItemsCount() const
    {
        return mItemsList->GetItemsCount();
    }

    void EditBoxDropDown::SelectItemAt(int position)
    {
        mItemsList->SelectItemAt(position);
    }

    void EditBoxDropDown::SelectItemText(const WString& text)
    {
        int idx = FindItem(text);
        if (idx >= 0)
            mItemsList->SelectItemAt(idx);
    }

    int EditBoxDropDown::GetSelectedItemPosition() const
    {
        return mItemsList->GetSelectedItemPos();
    }

    WString EditBoxDropDown::GetSelectedItemText() const
    {
        auto selectedItem = DynamicCast<Label>(mItemsList->GetSelectedItem());
        if (selectedItem)
            return selectedItem->GetText();

        return WString();
    }

    const Ref<EditBox>& EditBoxDropDown::GetEditBox() const
    {
        return mEditBox;
    }

    void EditBoxDropDown::SetMaxListSizeInItems(int itemsCount)
    {
        mMaxListItems = itemsCount;
    }

    void EditBoxDropDown::SetClippingLayout(const Layout& layout)
    {
        mClipLayout = layout;
        SetLayoutDirty();
    }

    Layout EditBoxDropDown::GetClippingLayout()
    {
        return mClipLayout;
    }

    void EditBoxDropDown::MoveAndCheckClipping(const Vec2F& delta, const RectF& clipArea)
    {
        mBoundsWithChilds += delta;
        mIsClipped = !mBoundsWithChilds.IsIntersects(clipArea);

        if (!mIsClipped)
            UpdateSelfTransform();

        // Update child widgets positions
        for (auto& child : mChildWidgets)
            child->MoveAndCheckClipping(delta, clipArea);

        if (IsExpanded())
            Collapse();
    }

    void EditBoxDropDown::OnCursorPressed(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = true;
    }

    void EditBoxDropDown::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;
    }

    void EditBoxDropDown::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {
        if (!mItemsList->layout->IsPointInside(o2Input.GetCursorPos()) && IsExpanded())
            Collapse();
    }

    void EditBoxDropDown::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;
    }

    void EditBoxDropDown::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;
    }

    void EditBoxDropDown::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;
    }

    void EditBoxDropDown::OnEnabled()
    {
        Widget::OnEnabled();

        interactable = true;
    }

    void EditBoxDropDown::OnDisabled()
    {
        Widget::OnDisabled();

        interactable = false;
    }

    void EditBoxDropDown::UpdateSelfTransform()
    {
        layout->Update();
        mAbsoluteClip = mClipLayout.Calculate(GetLayoutData().worldRectangle);
    }

    String EditBoxDropDown::GetCreateMenuGroup()
    {
        return "Dropping";
    }

    void EditBoxDropDown::FilterItems()
    {
        // Prevent recursive filtering during text updates
        if (mIsFiltering)
            return;

        mIsFiltering = true;

        WString filterText = mEditBox->GetText();
        
        // First remove all items from the visible list
        mItemsList->RemoveAllItems();

        // Add filtered items from original list
        for (auto& itemText : mOriginalItems)
        {
            bool matches = true;
            
            if (!filterText.IsEmpty())
            {
                // Case-insensitive comparison
                WString itemTextLower = itemText;
                WString filterTextLower = filterText;
                
                // Convert to lowercase manually
                for (int i = 0; i < itemTextLower.Length(); i++)
                    itemTextLower[i] = (wchar_t)tolower((int)itemTextLower[i]);
                    
                for (int i = 0; i < filterTextLower.Length(); i++)
                    filterTextLower[i] = (wchar_t)tolower((int)filterTextLower[i]);
                
                matches = itemTextLower.Contains(filterTextLower);
            }

            // Add if filter matches or is empty
            if (matches)
            {
                Ref<Widget> widget = mItemsList->AddItem();
                auto item = DynamicCast<Label>(widget);
                if (item)
                    item->SetText(itemText);
            }
        }

        mIsFiltering = false;
    }

    void EditBoxDropDown::OnEditBoxTextChanged(const WString& text)
    {
        // Filter items when text changes
        if (IsExpanded())
            FilterItems();

        onTextChanged(text);
    }

    void EditBoxDropDown::OnEditBoxTextCompleted(const WString& text)
    {
        onTextChangeCompleted(text);
    }

    void EditBoxDropDown::OnItemSelected()
    {
        auto selectedItem = DynamicCast<Label>(mItemsList->GetSelectedItem());
        if (selectedItem)
        {
            mEditBox->SetText(selectedItem->GetText());
        }

        Collapse();
        onSelectedPos(mItemsList->GetSelectedItemPos());
        
        if (selectedItem)
            onSelectedText(selectedItem->GetText());

        OnSelectionChanged();
    }

    void EditBoxDropDown::OnSelectionChanged()
    {
    }

    void EditBoxDropDown::OnEditBoxFocused()
    {
        if (!IsExpanded())
            Expand();
    }

    void EditBoxDropDown::OnEditBoxUnfocused()
    {
        // Give small delay before collapse to allow list item selection
        o2Application.AddDelayedCall([this]() {
            // Only collapse if not selecting an item
            if (!mItemsList->IsUnderPoint(o2Input.GetCursorPos()))
                Collapse();
        }, 0.1f);
    }

    void EditBoxDropDown::SetItemSample(const Ref<Widget>& sample)
    {
        mItemsList->SetItemSample(sample);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::EditBoxDropDown>);
// --- META ---

DECLARE_CLASS(o2::EditBoxDropDown, o2__EditBoxDropDown);
// --- END META --- 