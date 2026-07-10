#include "o2/stdafx.h"
#include "CustomList.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
	CustomList::CustomList(RefCounter* refCounter) :
		ScrollArea(refCounter)
	{
		mItemSample = mmake<Widget>();
		mItemSample->RemoveFromScene();

		mSelectionDrawable = mmake<Sprite>();
		mHoverDrawable = mmake<Sprite>();

		mVerLayout = mmake<VerticalLayout>();
		mVerLayout->baseCorner = BaseCorner::LeftTop;
		mVerLayout->name = "layout";
		mVerLayout->expandHeight = false;
		mVerLayout->expandWidth = true;
		mVerLayout->layout->anchorMin = Vec2F(0, 0);
		mVerLayout->layout->anchorMax = Vec2F(1, 1);
		mVerLayout->layout->offsetMin = Vec2F();
		mVerLayout->layout->offsetMax = Vec2F();
		mVerLayout->fitByChildren = true;

		AddChild(mVerLayout);
	}

	CustomList::CustomList(RefCounter* refCounter, const CustomList& other) :
		ScrollArea(refCounter, other), mHoverLayout(other.mHoverLayout),
		mSelectionLayout(other.mSelectionLayout)
	{
		mVerLayout = FindChildByType<VerticalLayout>();

		mItemSample = other.mItemSample->CloneAsRef<Widget>();
		mItemSample->RemoveFromScene();
		mItemSample->UpdateSelfTransform();
		mItemSample->UpdateChildrenTransforms();

		mSelectionDrawable = other.mSelectionDrawable->CloneAsRef<Sprite>();
		mHoverDrawable = other.mHoverDrawable->CloneAsRef<Sprite>();

		RetargetStatesAnimations();
		SetLayoutDirty();
	}

	CustomList::~CustomList()
	{
	}

	CustomList& CustomList::operator=(const CustomList& other)
	{
		mVerLayout = nullptr;

		mSelectionDrawable = other.mSelectionDrawable->CloneAsRef<Sprite>();
		mHoverDrawable = other.mHoverDrawable->CloneAsRef<Sprite>();

		mSelectionLayout = other.mSelectionLayout;
		mHoverLayout = other.mHoverLayout;

		ScrollArea::operator=(other);

		mVerLayout = FindChildByType<VerticalLayout>();
		mItemSample = other.mItemSample->CloneAsRef<Widget>();
		mItemSample->RemoveFromScene();
		mItemSample->SetLayoutDirty();

		RetargetStatesAnimations();
		SetLayoutDirty();

		return *this;
	}

	void CustomList::Update(float dt)
	{
		PROFILE_SAMPLE_FUNC();

		ScrollArea::Update(dt);

		if (!mResEnabledInHierarchy || mIsClipped)
			return;

		const float rectLerpCoef = 20.0f;

		if (mCurrentHoverRect != mTargetHoverRect)
		{
			mCurrentHoverRect = Math::Lerp(mCurrentHoverRect, mTargetHoverRect, dt * rectLerpCoef);
			mHoverDrawable->SetRect(mCurrentHoverRect);
		}
	}

	void CustomList::Draw()
	{
		PROFILE_SAMPLE_FUNC();

		if (!mResEnabledInHierarchy || mIsClipped)
			return;

		for (auto& layer : mDrawingLayers)
			layer->Draw();

		IDrawable::OnDrawn();

		o2Render.EnableScissorTest(mAbsoluteClipArea);

		DrawInheritedDepthChildren();

		for (auto& sel : mSelectedItems)
			sel.selection->Draw();

		mHoverDrawable->Draw();

		o2Render.DisableScissorTest();

		CursorAreaEventsListener::OnDrawn();

		for (auto& layer : mTopDrawingLayers)
			layer->Draw();

		if (mOwnHorScrollBar)
			mHorScrollBar->Draw();

		if (mOwnVerScrollBar)
			mVerScrollBar->Draw();

		DrawDebugFrame();
	}

	void CustomList::SetItemSample(const Ref<Widget>& sample)
	{
		RemoveAllItems();

		mItemSample = sample;

		SetLayoutDirty();
	}

	const Ref<Widget>& CustomList::GetItemSample() const
	{
		return mItemSample;
	}

	const Ref<VerticalLayout>& CustomList::GetItemsLayout() const
	{
		return mVerLayout;
	}

	Ref<Widget> CustomList::AddItem()
	{
		return mVerLayout->AddChildWidget(mItemSample->CloneAsRef<Widget>());
	}

	Ref<Widget> CustomList::AddItem(int position)
	{
		position = Math::Max(0, position);

		for (int i = mVerLayout->GetChildren().Count(); i < position; i++)
			AddItem();

		return mVerLayout->AddChildWidget(mItemSample->CloneAsRef<Widget>(), position);
	}

	void CustomList::RemoveItem(const Ref<Widget>& item)
	{
		mVerLayout->RemoveChild(item);
	}

	void CustomList::RemoveItem(int position)
	{
		if (position < 0 || position >= mVerLayout->GetChildren().Count())
		{
			o2Debug.LogWarning("Failed to remove item at %i: out of range (%i)", position, mVerLayout->GetChildren().Count());
			return;
		}

		auto item = mVerLayout->GetChildWidgets().Get(position);
		mVerLayout->RemoveChild(item);
	}

	void CustomList::MoveItem(int position, int newPosition)
	{
		if (position < 0 || position >= mVerLayout->GetChildren().Count())
		{
			o2Debug.LogWarning("Failed to move item from %i to %i: out of range (%i)", position, newPosition,
							   mVerLayout->GetChildren().Count());
			return;
		}

		auto item = mVerLayout->GetChildWidgets().Get(position);
		mVerLayout->RemoveChild(item);
		mVerLayout->AddChild(item, newPosition);
	}

	void CustomList::MoveItem(const Ref<Widget>& item, int newPosition)
	{
		mVerLayout->RemoveChild(item, false);
		mVerLayout->AddChild(item, newPosition);
	}

	int CustomList::GetItemPosition(const Ref<Widget>& item)
	{
		int i = 0;
		for (auto& child : mVerLayout->GetChildWidgets())
		{
			if (child == item)
				return i;

			i++;
		}

		return -1;
	}

	Ref<Widget> CustomList::GetItem(int position) const
	{
		if (position < 0 || position >= mVerLayout->GetChildWidgets().Count())
			return nullptr;

		return mVerLayout->GetChildWidgets().Get(position);
	}

	void CustomList::RemoveAllItems()
	{
		mVerLayout->RemoveAllChildren();
	}

	void CustomList::SortItems(const Function<bool(const Ref<Widget>&, const Ref<Widget>&)>& sortFunc)
	{
		mVerLayout->mChildWidgets.Sort(sortFunc);
	}

	int CustomList::GetItemsCount() const
	{
		return mVerLayout->GetChildWidgets().Count();
	}

	void CustomList::SelectItem(const Ref<Widget>& item)
	{
		int itemPos = GetItemPosition(item);

		if (mSelectedItems.Contains([=](auto x) { return x.idx == itemPos; }))
			return;

		if (!mMultiSelection)
			ClearSelection();

		Selection selection;
		selection.idx = itemPos;
		selection.selection = GetSelectionSprite();
		mSelectedItems.Add(selection);

		onSelectedPos(itemPos);
		onSelectedItem(item);

		OnSelectionChanged();
		SetLayoutDirty();
	}

	void CustomList::SelectItemAt(int position)
	{
		if (!mMultiSelection)
			ClearSelection();

		if (position >= mVerLayout->GetChildWidgets().Count()) 
		{
			o2Debug.LogWarning("Can't select item at %i: out of range (%i)", position, GetItemsCount());
			return;
		}

		if (mSelectedItems.Contains([=](auto x) { return x.idx == position; }))
			return;

		if (position >= 0)
		{
			Selection selection;
			selection.idx = position;
			selection.selection = GetSelectionSprite();
			mSelectedItems.Add(selection);

			onSelectedPos(position);
			onSelectedItem(GetItem(position));
		}

		OnSelectionChanged();
		SetLayoutDirty();
	}

	void CustomList::SetSelectedItems(const Vector<int>& items)
	{
		for (auto& x : items)
			SelectItemAt(x);
	}

	void CustomList::ClearSelection()
	{
		for (auto& sel : mSelectedItems)
			mSelectionSpritesPool.Add(sel.selection);

		mSelectedItems.Clear();
	}

	Vector<int> CustomList::GetSelectedItems() const
	{
		return mSelectedItems.Convert<int>([](auto x) { return x.idx; });
	}

	int CustomList::GetSelectedItemPos() const
	{
		if (mSelectedItems.IsEmpty())
			return -1;

		return mSelectedItems.Last().idx;
	}

	Ref<Widget> CustomList::GetSelectedItem() const
	{
		if (mSelectedItems.IsEmpty())
			return nullptr;

		return GetItem(mSelectedItems.Last().idx);
	}

	void CustomList::SetMultiselectionAvailable(bool available)
	{
		mMultiSelection = available;

		if (!mMultiSelection) 
		{
			int count = mSelectedItems.Count();
			for (int i = 0; i < count - 1; i++) 
			{
				mSelectionSpritesPool.Add(mSelectedItems.Last().selection);
				mSelectedItems.PopBack();
			}
		}
	}

	bool CustomList::IsMultiselectionAvailable() const
	{
		return mMultiSelection;
	}

	const Ref<Sprite>& CustomList::GetSelectionDrawable() const
	{
		return mSelectionDrawable;
	}

	const Ref<Sprite>& CustomList::GetHoverDrawable() const
	{
		return mHoverDrawable;
	}

	void CustomList::SetSelectionDrawableLayout(const Layout& layout)
	{
		mSelectionLayout = layout;
	}

	Layout CustomList::GetSelectionDrawableLayout() const
	{
		return mSelectionLayout;
	}

	void CustomList::SetHoverDrawableLayout(const Layout& layout)
	{
		mHoverLayout = layout;
	}

	Layout CustomList::GetHoverDrawableLayout() const
	{
		return mHoverLayout;
	}

	bool CustomList::IsScrollable() const
	{
		return mEnableHorScroll || mEnableVerScroll;
	}

	bool CustomList::IsInputTransparent() const
	{
		return false;
	}

	void CustomList::MoveScrollPosition(const Vec2F& delta)
	{
		ScrollArea::MoveScrollPosition(delta);
		UpdateHover(o2Input.GetCursorPos());
		UpdateSelectionSprites();
	}

	void CustomList::OnTransformUpdated()
	{
		ScrollArea::OnTransformUpdated();
		UpdateHover(o2Input.GetCursorPos());
		UpdateSelectionSprites();
	}

	void CustomList::UpdateSelfTransform()
	{
		ScrollArea::UpdateSelfTransform();

		if (Input::IsSingletonInitialzed())
			UpdateHover(o2Input.cursorPos);

		mCurrentHoverRect = mTargetHoverRect;
		mHoverDrawable->SetRect(mCurrentHoverRect);
	}

	String CustomList::GetCreateMenuGroup()
	{
		return "List";
	}

	void CustomList::UpdateSelectionSprites()
	{
		for (auto& sel : mSelectedItems)
		{
			if (auto item = GetItem(sel.idx))
			{
				sel.selection->SetRect(mSelectionLayout.Calculate(item->layout->worldRect));
				sel.selection->SetEnabled(item->mResEnabledInHierarchy && !item->mIsClipped);
			}
		}
	}

	void CustomList::OnCursorPressed(const Input::Cursor& cursor)
	{
		auto pressedState = state["pressed"];
		if (pressedState)
			*pressedState = true;
	}

	void CustomList::OnCursorStillDown(const Input::Cursor& cursor)
	{
		const float checkDeltaThreshold = 2.0f;
		if ((cursor.position - mLastSelectCheckCursor).Length() < checkDeltaThreshold)
			return;

		mLastSelectCheckCursor = cursor.position;
	}

	void CustomList::OnCursorReleased(const Input::Cursor& cursor)
	{
		auto pressedState = state["pressed"];
		if (pressedState)
			*pressedState = false;

		if (!mMultiSelection || !o2Input.IsKeyDown(VK_CONTROL))
			ClearSelection();

		int itemIdx = -1;
		GetItemUnderPoint(cursor.position, &itemIdx);
		SelectItemAt(itemIdx);
	}

	void CustomList::OnCursorPressBreak(const Input::Cursor& cursor)
	{
		auto pressedState = state["pressed"];
		if (pressedState)
			*pressedState = false;
	}

	void CustomList::OnCursorExit(const Input::Cursor& cursor)
	{
		auto hoverState = state["hover"];
		if (hoverState)
			*hoverState = false;
		else
			mHoverDrawable->SetEnabled(false);
	}

	void CustomList::OnCursorMoved(const Input::Cursor& cursor)
	{
		const float checkDeltaThreshold = 2.0f;
		if ((cursor.position - mLastHoverCheckCursor).Length() < checkDeltaThreshold)
			return;

		mLastHoverCheckCursor = cursor.position;

		UpdateHover(cursor.position);
	}

	Ref<Widget> CustomList::GetItemUnderPoint(const Vec2F& point, int* idxPtr)
	{
		if (!mVerLayout)
			return nullptr;

		int idx = 0;
		for (auto& child : mVerLayout->mChildWidgets)
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

	void CustomList::OnDeserialized(const DataValue& node)
	{
		ScrollArea::OnDeserialized(node);
		RetargetStatesAnimations();
	}

	void CustomList::UpdateTransparency()
	{
		Widget::UpdateTransparency();

		if (mHorScrollBar)
			mHorScrollBar->UpdateTransparency();

		if (mVerScrollBar)
			mVerScrollBar->UpdateTransparency();

		if (mHoverDrawable)
			mHoverDrawable->transparency = mResTransparency;

		for (auto& sel : mSelectedItems)
			sel.selection->transparency = mResTransparency;
	}

	void CustomList::UpdateHover(const Vec2F& point)
	{
		int itemIdx = -1;
		auto itemUnderCursor = GetItemUnderPoint(point, &itemIdx);

		if (itemIdx < 0)
		{
			auto hoverState = state["hover"];
			if (hoverState)
			{
				mHoverDrawable->SetEnabled(true);
				*hoverState = false;
			}
			else mHoverDrawable->SetEnabled(false);
		}
		else
		{
			mTargetHoverRect = mHoverLayout.Calculate(itemUnderCursor->layout->worldRect);

			auto hoverState = state["hover"];
			if (hoverState) {
				mHoverDrawable->SetEnabled(true);
				*hoverState = true;
			}
			else mHoverDrawable->SetEnabled(true);
		}
	}

	Ref<Sprite> CustomList::GetSelectionSprite()
	{
		if (mSelectionSpritesPool.IsEmpty())
		{
			const int poolStep = 5;
			for (int i = 0; i < poolStep; i++)
				mSelectionSpritesPool.Add(mSelectionDrawable->CloneAsRef<Sprite>());
		}

		auto sprite = mSelectionSpritesPool.PopBack();
		sprite->SetTransparency(1.0f);

		return sprite;
	}

	void CustomList::OnSelectionChanged()
	{
	}

	void CustomList::OnEnabled()
	{
		Widget::OnEnabled();

		SetInteractable(true);
	}

	void CustomList::OnDisabled()
	{
		Widget::OnDisabled();

		SetInteractable(false);
	}

	bool CustomList::Selection::operator==(const Selection& other) const
	{
		return idx == other.idx;
	}

	void CustomList::OnKeyPressed(const Input::Key& key)
	{
		if (!IsFocused())
			return;

		int currentSelection = GetSelectedItemPos();
		int itemsCount = GetItemsCount();

		if (itemsCount <= 0)
			return;

		if (key.keyCode == VK_DOWN)
		{
			// Move selection down
			int nextItem = currentSelection + 1;
			if (nextItem >= itemsCount)
				nextItem = 0; // Wrap to beginning

			if (!mMultiSelection || !o2Input.IsKeyDown(VK_CONTROL))
				ClearSelection();
				
			SelectItemAt(nextItem);
		}
		else if (key.keyCode == VK_UP)
		{
			// Move selection up
			int prevItem = currentSelection - 1;
			if (prevItem < 0)
				prevItem = itemsCount - 1; // Wrap to end

			if (!mMultiSelection || !o2Input.IsKeyDown(VK_CONTROL))
				ClearSelection();
				
			SelectItemAt(prevItem);
		}
	}
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::CustomList>);
// --- META ---

DECLARE_CLASS(o2::CustomList, o2__CustomList);
// --- END META ---
