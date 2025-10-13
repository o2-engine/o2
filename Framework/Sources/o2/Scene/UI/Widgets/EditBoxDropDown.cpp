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
        mEditBox->SetMultiLine(false);

        mItemsList = mmake<List>();
        mItemsList->SetInternalParent(Ref(this), false);
        mItemsList->SetMultiselectionAvailable(false);
        mItemsList->Hide(true);

        SetupCallbacks();
    }

    EditBoxDropDown::EditBoxDropDown(RefCounter* refCounter, const EditBoxDropDown& other):
        Widget(refCounter, other), DrawableCursorEventsListener(this),
        mMaxListItems(other.mMaxListItems)
    {
        mEditBox = FindInternalWidgetByType<EditBox>();
        
        mItemsList = FindInternalWidgetByType<List>();
        mItemsList->Hide(true);
        mItemsList->SetMultiselectionAvailable(false);

        mOriginalItems = other.mOriginalItems;

        SetupCallbacks();
        RetargetStatesAnimations();
    }

    EditBoxDropDown::~EditBoxDropDown()
    {}

    EditBoxDropDown& EditBoxDropDown::operator=(const EditBoxDropDown& other)
    {
        Widget::operator=(other);

        mEditBox = FindInternalWidgetByType<EditBox>();

        mItemsList = FindInternalWidgetByType<List>();
        mItemsList->Hide(true);
        mItemsList->SetMultiselectionAvailable(false);

        mMaxListItems = other.mMaxListItems;
        mOriginalItems = other.mOriginalItems;

        SetupCallbacks();
        RetargetStatesAnimations();

        return *this;
    }

    void EditBoxDropDown::SetupCallbacks()
    {
        mEditBox->onChanged += [&](auto text) { OnEditBoxTextChanged(text); };
        mEditBox->onChangeCompleted += [&](auto text) { OnEditBoxTextCompleted(text); };
        mEditBox->onFocused += [&]() { OnEditBoxFocused(); };
        mEditBox->onUnfocused += [&]() { OnEditBoxUnfocused(); };
        
        mItemsList->onSelectedText += [&](const WString& text) { OnSelectedText(text); };
    }
    
    void EditBoxDropDown::Draw()
	{
		PROFILE_SAMPLE_FUNC();

		if (!mResEnabledInHierarchy)
			return;

		Widget::Draw();

		o2UI.DrawWidgetAtTop(mItemsList);
	}

    void EditBoxDropDown::SetText(const WString& text)
    {
        mEditBox->SetText(text);
        
        if (IsExpanded())
            FilterItems();
    }

    WString EditBoxDropDown::GetText() const
    {
        return mEditBox->GetText();
    }

    void EditBoxDropDown::Expand()
    {
		onExpand();

		mItemsList->RemoveAllItems();
		mItemsList->AddItems(mOriginalItems);

        auto openedState = state["opened"];
        if (openedState)
            *openedState = true;

		mItemsList->SetEnabled(true);
		
		UpdateListSize();
        SetLayoutDirty();
    }

    void EditBoxDropDown::UpdateListSize()
    {
        float itemHeight = mItemsList->GetItemSample()->layout->minHeight;
        int itemsVisible = Math::Min(mMaxListItems, mItemsList->GetItemsCount());
        float itemsHeight = itemHeight * (float)itemsVisible; 

        mItemsList->layout->minHeight = itemsHeight;
        mItemsList->layout->height = itemsHeight;
		
		mItemsList->UpdateSelfTransform();
		mItemsList->UpdateChildrenTransforms();
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
        mItemsList->AddItem(text);
        mOriginalItems.Add(text);
        return GetItemsCount() - 1;
    }

    int EditBoxDropDown::AddItem(const WString& text, int position)
    {
        mItemsList->AddItem(text, position);        
        mOriginalItems.Insert(text, position);
            
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
            WString text = GetItemText(position);
            mItemsList->RemoveItem(text);
            
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

    const Ref<List>& EditBoxDropDown::GetListView() const
    {
        return mItemsList;
    }

    void EditBoxDropDown::SetMaxListSizeInItems(int itemsCount)
    {
        mMaxListItems = itemsCount;
    }

    void EditBoxDropDown::MoveAndCheckClipping(const Vec2F& delta, const RectF& clipArea)
    {
		Widget::MoveAndCheckClipping(delta, clipArea);

        if (IsExpanded())
            Collapse();
    }

    void EditBoxDropDown::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {
        if (!mItemsList->layout->IsPointInside(o2Input.GetCursorPos()) && IsExpanded())
            Collapse();
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
    }

    String EditBoxDropDown::GetCreateMenuGroup()
    {
        return "Dropping";
    }

    void EditBoxDropDown::FilterItems()
    {
        if (mIsFiltering || mIsKeyboardNavigating)
            return;

        mIsFiltering = true;

        WString filterTextLower = mEditBox->GetText().ToLowerCase();
        mItemsList->RemoveAllItems();

        if (filterTextLower.IsEmpty())
        {
            mItemsList->AddItems(mOriginalItems);
        }
        else
        {
            for (const auto& itemText : mOriginalItems)
            {
                if (itemText.ToLowerCase().Contains(filterTextLower))
                    mItemsList->AddItem(itemText);
            }
        }
        
        UpdateListSize();

        mIsFiltering = false;
    }

    void EditBoxDropDown::OnEditBoxTextChanged(const WString& text)
    {
        if (IsExpanded())
            FilterItems();

        onChanged(text);
    }

    void EditBoxDropDown::OnEditBoxTextCompleted(const WString& text)
	{
		Collapse();
        onChangeCompleted(text);
    }

    void EditBoxDropDown::OnSelectedText(const WString& text)
	{
        if (!mIsKeyboardNavigating)
		    Collapse();

        mEditBox->SetText(text);
		onSelectedText(text);

		if (!mIsKeyboardNavigating)
		    onChangeCompleted(text);
    }

    void EditBoxDropDown::OnSelectionChanged()
    {}

    void EditBoxDropDown::OnEditBoxFocused()
    {
        if (!IsExpanded())
            Expand();
    }

    void EditBoxDropDown::OnEditBoxUnfocused()
    {
        if (!mItemsList->IsUnderPoint(o2Input.GetCursorPos()))
            Collapse();
    }

    void EditBoxDropDown::OnKeyPressed(const Input::Key& key)
    {
        if (!IsExpanded())
            return;

        if (key.keyCode == VK_DOWN)
            NavigateNext();
        else if (key.keyCode == VK_UP)
            NavigatePrevious();
    }

    void EditBoxDropDown::NavigateNext()
    {
        int currentPos = GetSelectedItemPosition();
        int itemsCount = GetItemsCount();
        
        if (itemsCount <= 0)
            return;
        
        int nextPos = currentPos < 0 ? 0 : (currentPos + 1) % itemsCount;

        mIsKeyboardNavigating = true;
        SelectItemAt(nextPos);
		mIsKeyboardNavigating = false;
    }

    void EditBoxDropDown::NavigatePrevious()
    {
        int currentPos = GetSelectedItemPosition();
        int itemsCount = GetItemsCount();
        
        if (itemsCount <= 0)
            return;
        
        int prevPos = currentPos < 0 ? itemsCount - 1 : (currentPos - 1 + itemsCount) % itemsCount;

		mIsKeyboardNavigating = true;
        SelectItemAt(prevPos);
		mIsKeyboardNavigating = false;
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
