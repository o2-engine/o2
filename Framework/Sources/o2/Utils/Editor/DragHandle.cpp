#include "o2/stdafx.h"
#include "DragHandle.h"

#include "o2/Application/Application.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/RectDrawable.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    DragHandle::DragHandle(RefCounter* refCounter):
        RefCounterable(refCounter)
    {
        screenToLocalTransformFunc = [](const Vec2F& point) { return point; };
        localToScreenTransformFunc = [](const Vec2F& point) { return point; };
        checkPositionFunc = [](const Vec2F& point) { return point; };
        checkSnappingFunc = [](const Vec2F& point) { return point; };
    }

    DragHandle::DragHandle(RefCounter* refCounter, const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover /*= nullptr*/, const Ref<IRectDrawable>& pressed /*= nullptr*/,
                           const Ref<IRectDrawable>& selected /*= nullptr*/, const Ref<IRectDrawable>& selectedHovered /*= nullptr*/,
                           const Ref<IRectDrawable>& selectedPressed /*= nullptr*/) :
        RefCounterable(refCounter), mRegularDrawable(regular), mHoverDrawable(hover), mPressedDrawable(pressed), mSelectedDrawable(selected),
        mSelectedHoverDrawable(selectedHovered), mSelectedPressedDrawable(selectedPressed)
    {
        screenToLocalTransformFunc = [](const Vec2F& point) { return point; };
        localToScreenTransformFunc = [](const Vec2F& point) { return point; };
        checkPositionFunc = [](const Vec2F& point) { return point; };
        checkSnappingFunc = [](const Vec2F& point) { return point; };
    }

    DragHandle::DragHandle(RefCounter* refCounter, const DragHandle& other) :
        RefCounterable(refCounter), angle(this), position(this), enabled(this)
    {
        if (other.mRegularDrawable)
            mRegularDrawable = other.mRegularDrawable->CloneAsRef<IRectDrawable>();

        if (other.mHoverDrawable)
            mHoverDrawable = other.mHoverDrawable->CloneAsRef<IRectDrawable>();

        if (other.mPressedDrawable)
            mPressedDrawable = other.mPressedDrawable->CloneAsRef<IRectDrawable>();

        if (other.mSelectedDrawable)
            mSelectedDrawable = other.mSelectedDrawable->CloneAsRef<IRectDrawable>();

        if (other.mSelectedHoverDrawable)
            mSelectedHoverDrawable = other.mSelectedHoverDrawable->CloneAsRef<IRectDrawable>();

        if (other.mSelectedPressedDrawable)
            mSelectedPressedDrawable = other.mSelectedPressedDrawable->CloneAsRef<IRectDrawable>();

        onChangedPos = other.onChangedPos;
        screenToLocalTransformFunc = other.screenToLocalTransformFunc;
        localToScreenTransformFunc = other.localToScreenTransformFunc;
        checkPositionFunc = other.checkPositionFunc;
        checkSnappingFunc = other.checkSnappingFunc;
        cursorType = other.cursorType;
        snappingKey = other.snappingKey;
        pixelPerfect = other.pixelPerfect;

        SetPosition(other.mPosition);
    }

    DragHandle::DragHandle():
        DragHandle(nullptr)
    {}

    DragHandle::DragHandle(const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover /*= nullptr*/, const Ref<IRectDrawable>& pressed /*= nullptr*/,
                           const Ref<IRectDrawable>& selected /*= nullptr*/, const Ref<IRectDrawable>& selectedHovered /*= nullptr*/,
                           const Ref<IRectDrawable>& selectedPressed /*= nullptr*/) :
        DragHandle(nullptr, regular, hover, pressed, selected, selectedHovered, selectedPressed)
    {}

    DragHandle::DragHandle(const DragHandle& other) :
        DragHandle(nullptr, other)
    {}

    DragHandle::~DragHandle()
    {
        if (mSelectGroup)
            mSelectGroup->RemoveHandle(this);
    }

    DragHandle& DragHandle::operator=(const DragHandle& other)
    {
        if (other.mRegularDrawable)
            mRegularDrawable = other.mRegularDrawable->CloneAsRef<IRectDrawable>();
        else
            mRegularDrawable = nullptr;

        if (other.mHoverDrawable)
            mHoverDrawable = other.mHoverDrawable->CloneAsRef<IRectDrawable>();
        else
            mHoverDrawable = nullptr;

        if (other.mPressedDrawable)
            mPressedDrawable = other.mPressedDrawable->CloneAsRef<IRectDrawable>();
        else
            mPressedDrawable = nullptr;

        if (other.mSelectedDrawable)
            mSelectedDrawable = other.mSelectedDrawable->CloneAsRef<IRectDrawable>();
        else
            mSelectedDrawable = nullptr;

        if (other.mSelectedHoverDrawable)
            mSelectedHoverDrawable = other.mSelectedHoverDrawable->CloneAsRef<IRectDrawable>();
        else
            mSelectedHoverDrawable = nullptr;

        if (other.mSelectedPressedDrawable)
            mSelectedPressedDrawable = other.mSelectedPressedDrawable->CloneAsRef<IRectDrawable>();
        else
            mSelectedPressedDrawable = nullptr;

        onChangedPos = other.onChangedPos;
        screenToLocalTransformFunc = other.screenToLocalTransformFunc;
        localToScreenTransformFunc = other.localToScreenTransformFunc;
        checkPositionFunc = other.checkPositionFunc;
        checkSnappingFunc = other.checkSnappingFunc;
        cursorType = other.cursorType;
        snappingKey = other.snappingKey;
        pixelPerfect = other.pixelPerfect;

        SetPosition(other.mPosition);

        return *this;
    }

    bool DragHandle::IsSelected() const
    {
        return mIsSelected;
    }

    void DragHandle::SetSelected(bool selected)
    {
        if (mIsSelected == selected)
            return;

        if (mSelectGroup)
        {
            if (selected)
                mSelectGroup->SelectHandle(Ref(this));
            else
                mSelectGroup->DeselectHandle(Ref(this));
        }
        else
        {
            mIsSelected = selected;

            if (mIsSelected)
                OnSelected();
            else
                OnDeselected();
        }
    }

    void DragHandle::Select()
    {
        SetSelected(true);
    }

    void DragHandle::Deselect()
    {
        SetSelected(false);
    }

    void DragHandle::Draw()
    {
        if (!mEnabled)
            return;

        if (mLastScreenPosUpdateFrame != o2Time.GetCurrentFrame())
        {
            UpdateScreenPosition();
            mLastScreenPosUpdateFrame = o2Time.GetCurrentFrame();
        }

        UpdateDrawablesPositions();
        DrawInternal();
    }

    void DragHandle::Draw(const RectF& screenClipRect)
    {
        if (!mEnabled)
            return;

        if (mLastScreenPosUpdateFrame != o2Time.GetCurrentFrame())
        {
            UpdateScreenPosition();
            mLastScreenPosUpdateFrame = o2Time.GetCurrentFrame();
        }

        if (screenClipRect.IsInside(mScreenPosition))
        {
            UpdateDrawablesPositions();
            DrawInternal();
        }
    }

    bool DragHandle::IsUnderPoint(const Vec2F& point)
    {
        if (!isPointInside.IsEmpty())
            return mDrawingScissorRect.IsInside(point) && isPointInside(point);

        if (mRegularDrawable)
            return mDrawingScissorRect.IsInside(point) && mRegularDrawable->IsPointInside(point);

        return false;
    }

    Vec2F DragHandle::ScreenToLocal(const Vec2F& point)
    {
        return screenToLocalTransformFunc(point);
    }

    Vec2F DragHandle::LocalToScreen(const Vec2F& point)
    {
        return localToScreenTransformFunc(point);
    }

    void DragHandle::OnCursorPressed(const Input::Cursor& cursor)
    {
        onPressed();

        mDragOffset = mPosition - ScreenToLocal(cursor.position);
        mDragPosition = mPosition;
        mPressedCursorId = cursor.id;

        if (mSelectGroup)
            mSelectGroup->OnHandleCursorPressed(Ref(this), cursor);

        mPressedCursorPos = cursor.position;
    }

    void DragHandle::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        onDblClicked();
    }

    void DragHandle::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mIsDragging)
        {
            mIsDragging = false;

            if (mDragBeginPosition != mPosition)
            {
                if (mSelectGroup)
                    mSelectGroup->OnHandleCompletedChange(Ref(this));
                else
                    onChangeCompleted();
            }
        }
        else
        {
            if (mSelectGroup)
                mSelectGroup->OnHandleCursorReleased(Ref(this), cursor);
            else
                SetSelected(!IsSelected());
        }

        if (!IsUnderPoint(cursor.position))
            o2Application.SetCursor(CursorType::Arrow);

        onReleased();
    }

    void DragHandle::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        mIsDragging = false;

        onReleased();

        if (mDragBeginPosition != mPosition)
            onChangeCompleted();
    }

    void DragHandle::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!mIsPressed)
            return;

        if (cursor.id != mPressedCursorId)
            return;

        if (cursor.delta.Length() < 0.5f)
            return;

        if (!mIsDragging)
        {
            float delta = (cursor.position - mPressedCursorPos).Length();
            if (delta > mDragDistanceThreshold)
            {
                onBeganDragging();

                mIsDragging = true;
                mDragBeginPosition = mPosition;

                if (mSelectGroup)
                    mSelectGroup->OnHandleBeganDragging(Ref(this));
                else
                    Select();
            }
        }

        if (mIsDragging)
        {
            mDragPosition = ScreenToLocal(cursor.position) + mDragOffset;

            SetPosition(mDragPosition);
            onChangedPos(mPosition);

            if (mSelectGroup)
                mSelectGroup->OnHandleMoved(Ref(this), cursor.position);
        }
    }

    void DragHandle::OnCursorEnter(const Input::Cursor& cursor)
    {
        mIsHovered = true;
		onHoverEnter();

        if (!cursor.isPressed)
            o2Application.SetCursor(cursorType);
    }

    void DragHandle::OnCursorExit(const Input::Cursor& cursor)
    {
        mIsHovered = false;
		onHoverExit();

        if (!IsPressed() && !cursor.isPressed)
            o2Application.SetCursor(CursorType::Arrow);
    }

    void DragHandle::OnCursorRightMousePressed(const Input::Cursor& cursor)
    {
        onRightButtonPressed(cursor);
    }

    void DragHandle::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        onRightButtonReleased(cursor);
    }

    void DragHandle::SetPosition(const Vec2F& position)
    {
        mPosition = checkPositionFunc(position);

        if (mIsDragging && o2Input.IsKeyDown(snappingKey))
            mPosition = checkSnappingFunc(mPosition);

        UpdateScreenPosition();
    }

    const Vec2F& DragHandle::GetScreenPosition() const
    {
        return mScreenPosition;
    }

    void DragHandle::UpdateScreenPosition()
    {
        mScreenPosition = LocalToScreen(mPosition);

        if (pixelPerfect)
        {
            mScreenPosition.x = Math::Round(mScreenPosition.x);
            mScreenPosition.y = Math::Round(mScreenPosition.y);
        }
    }

    void DragHandle::UpdateDrawablesPositions()
    {
        if (mLastDrawnScreenPosition == mScreenPosition)
            return;

        mLastDrawnScreenPosition = mScreenPosition;

        if (mRegularDrawable)
            mRegularDrawable->SetPosition(mScreenPosition);

        if (mHoverDrawable)
            mHoverDrawable->SetPosition(mScreenPosition);

        if (mPressedDrawable)
            mPressedDrawable->SetPosition(mScreenPosition);

        if (mSelectedDrawable)
            mSelectedDrawable->SetPosition(mScreenPosition);

        if (mSelectedHoverDrawable)
            mSelectedHoverDrawable->SetPosition(mScreenPosition);

        if (mSelectedPressedDrawable)
            mSelectedPressedDrawable->SetPosition(mScreenPosition);
    }

    void DragHandle::SetDragPosition(const Vec2F& position)
    {
        mDragPosition = position;
        SetPosition(position);
    }

    Vec2F DragHandle::GetDragPosition() const
    {
        return mDragPosition;
    }

    Vec2F DragHandle::GetPosition() const
    {
        return mPosition;
    }

    Vec2F DragHandle::GetDraggingOffset() const
    {
        return mDragOffset;
    }

    Vec2F DragHandle::GetDraggingBeginPosition() const
    {
        return mDragBeginPosition;
    }

    void DragHandle::BeginDrag(const Vec2F& cursor)
    {
        mDragOffset = mPosition - ScreenToLocal(cursor);
        mDragPosition = mPosition;
    }

    void DragHandle::SetEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    bool DragHandle::IsEnabled() const
    {
        return mEnabled;
    }

    void DragHandle::SetAngle(float rad)
    {
        mAngle = rad;

        if (mRegularDrawable)
            mRegularDrawable->angle = rad;

        if (mHoverDrawable)
            mHoverDrawable->angle = rad;

        if (mPressedDrawable)
            mPressedDrawable->angle = rad;

        if (mSelectedDrawable)
            mSelectedDrawable->angle = rad;

        if (mSelectedHoverDrawable)
            mSelectedHoverDrawable->angle = rad;

        if (mSelectedPressedDrawable)
            mSelectedPressedDrawable->angle = rad;
    }

    float DragHandle::GetAngle() const
    {
        return mAngle;
    }

    const Vec2F& DragHandle::GetPressedCursorPos() const
    {
        return mPressedCursorPos;
    }

    void DragHandle::SetRegularDrawable(const Ref<IRectDrawable>& IRectDrawable)
    {
        mRegularDrawable = IRectDrawable;
    }

    const Ref<IRectDrawable>& DragHandle::GetRegularDrawable() const
    {
        return mRegularDrawable;
    }

    void DragHandle::SetHoverDrawable(const Ref<IRectDrawable>& IRectDrawable)
    {
        mHoverDrawable = IRectDrawable;
    }

    const Ref<IRectDrawable>& DragHandle::GetHoverDrawable() const
    {
        return mHoverDrawable;
    }

    void DragHandle::SetPressedDrawable(const Ref<IRectDrawable>& IRectDrawable)
    {
        mPressedDrawable = IRectDrawable;
    }

    const Ref<IRectDrawable>& DragHandle::GetPressedDrawable() const
    {
        return mPressedDrawable;
    }

    void DragHandle::SetSelectedDrawable(const Ref<IRectDrawable>& IRectDrawable)
    {
        mSelectedDrawable = IRectDrawable;
    }

    const Ref<IRectDrawable>& DragHandle::GetSelectedDrawable() const
    {
        return mSelectedDrawable;
    }

    void DragHandle::SetSelectedHoveredDrawable(const Ref<IRectDrawable>& IRectDrawable)
    {
        mSelectedHoverDrawable = IRectDrawable;
    }

    const Ref<IRectDrawable>& DragHandle::GetSelectedHoveredDrawable() const
    {
        return mSelectedHoverDrawable;
    }

    void DragHandle::SetSelectedPressedDrawable(const Ref<IRectDrawable>& IRectDrawable)
    {
        mSelectedPressedDrawable = IRectDrawable;
    }

    const Ref<IRectDrawable>& DragHandle::GetSelectedPressedDrawable() const
    {
        return mSelectedPressedDrawable;
    }

    void DragHandle::SetDrawablesSize(const Vec2F& size)
    {
        if (mRegularDrawable)
            mRegularDrawable->SetSize(size);

        if (mHoverDrawable)
            mHoverDrawable->SetSize(size);

        if (mPressedDrawable)
            mPressedDrawable->SetSize(size);

        if (mSelectedDrawable)
            mSelectedDrawable->SetSize(size);

        if (mSelectedHoverDrawable)
            mSelectedHoverDrawable->SetSize(size);

        if (mSelectedPressedDrawable)
            mSelectedPressedDrawable->SetSize(size);
    }

    void DragHandle::SetDrawablesColor(const Color4& color)
    {
        if (mRegularDrawable)
            mRegularDrawable->SetColor(color);

        if (mHoverDrawable)
            mHoverDrawable->SetColor(color);

        if (mPressedDrawable)
            mPressedDrawable->SetColor(color);
    }

    void DragHandle::SetDrawablesSizePivot(const Vec2F& pivot)
    {
        if (mRegularDrawable)
            mRegularDrawable->SetSizePivot(pivot);

        if (mHoverDrawable)
            mHoverDrawable->SetSizePivot(pivot);

        if (mPressedDrawable)
            mPressedDrawable->SetSizePivot(pivot);

        if (mSelectedDrawable)
            mSelectedDrawable->SetSizePivot(pivot);

        if (mSelectedHoverDrawable)
            mSelectedHoverDrawable->SetSizePivot(pivot);

        if (mSelectedPressedDrawable)
            mSelectedPressedDrawable->SetSizePivot(pivot);
    }

    bool DragHandle::IsScrollable() const
    {
        return true;
    }

    void DragHandle::SetSelectionGroup(const Ref<ISelectableDragHandlesGroup>& group)
    {
        if (mSelectGroup == group)
            return;

        if (mSelectGroup)
            mSelectGroup->RemoveHandle(this);

        mSelectGroup = group;

        if (mSelectGroup)
            mSelectGroup->AddHandle(Ref(this));
    }

    const Ref<ISelectableDragHandlesGroup>& DragHandle::GetSelectionGroup() const
    {
        return mSelectGroup;
    }

    void DragHandle::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {
        if (!mSelectGroup)
            Deselect();
    }

    void DragHandle::OnSelected()
    {
        onSelected();
    }

    void DragHandle::OnDeselected()
    {
		onDeselected();
    }

    void DragHandle::DrawInternal()
    {
        float alphaChangeCoef = 15.0f;

        if (mRegularDrawable)
            mRegularDrawable->Draw();

        if (mHoverDrawable && !(mIsSelected && mSelectedHoverDrawable))
        {
            float transparency = Math::Lerp(mHoverDrawable->GetTransparency(), mIsHovered ? 1.0f : 0.0f,
                                            o2Time.GetDeltaTime()*alphaChangeCoef);

            if (!Math::Equals(transparency, mHoverDrawable->GetTransparency()))
                mHoverDrawable->SetTransparency(transparency);

            mHoverDrawable->Draw();
        }

        if (mPressedDrawable && !(mIsSelected && mSelectedPressedDrawable))
        {
            float transparency = Math::Lerp(mPressedDrawable->GetTransparency(), mIsPressed ? 1.0f : 0.0f,
                                            o2Time.GetDeltaTime()*alphaChangeCoef);

            if (!Math::Equals(transparency, mPressedDrawable->GetTransparency()))
                mPressedDrawable->SetTransparency(transparency);

            mPressedDrawable->Draw();
        }

        if (mSelectedDrawable)
        {
            float transparency = Math::Lerp(mSelectedDrawable->GetTransparency(), mIsSelected ? 1.0f : 0.0f,
                                            o2Time.GetDeltaTime()*alphaChangeCoef);

            if (!Math::Equals(transparency, mSelectedDrawable->GetTransparency()))
                mSelectedDrawable->SetTransparency(transparency);

            mSelectedDrawable->Draw();
        }

        if (mIsSelected && mSelectedHoverDrawable)
        {
            float transparency = Math::Lerp(mSelectedHoverDrawable->GetTransparency(), mIsHovered ? 1.0f : 0.0f,
                                            o2Time.GetDeltaTime()*alphaChangeCoef);

            if (!Math::Equals(transparency, mSelectedHoverDrawable->GetTransparency()))
                mSelectedHoverDrawable->SetTransparency(transparency);

            mSelectedHoverDrawable->Draw();
        }

        if (mIsSelected && mSelectedPressedDrawable)
        {

            float transparency = Math::Lerp(mSelectedPressedDrawable->GetTransparency(), mIsPressed ? 1.0f : 0.0f,
                                            o2Time.GetDeltaTime()*alphaChangeCoef);

            if (!Math::Equals(transparency, mSelectedPressedDrawable->GetTransparency()))
                mSelectedPressedDrawable->SetTransparency(transparency);

            mSelectedPressedDrawable->Draw();
        }

        CursorAreaEventsListener::OnDrawn();
        IDrawable::OnDrawn();
    }

    WidgetDragHandle::WidgetDragHandle(RefCounter* refCounter) :
        DragHandle(refCounter), Widget(refCounter)
    {
        widgetOffsetToLocalTransformFunc = [](const Vec2F& point) { return point; };
        localToWidgetOffsetTransformFunc = [](const Vec2F& point) { return point; };
    }

    WidgetDragHandle::WidgetDragHandle(RefCounter* refCounter, const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover /*= nullptr*/, const Ref<IRectDrawable>& pressed /*= nullptr*/,
                                       const Ref<IRectDrawable>& selected /*= nullptr*/, const Ref<IRectDrawable>& selectedHovered /*= nullptr*/, 
                                       const Ref<IRectDrawable>& selectedPressed /*= nullptr*/) :
        DragHandle(refCounter, regular, hover, pressed, selected, selectedHovered, selectedPressed), Widget(refCounter)
    {
        widgetOffsetToLocalTransformFunc = [](const Vec2F& point) { return point; };
        localToWidgetOffsetTransformFunc = [](const Vec2F& point) { return point; };
    }

    WidgetDragHandle::WidgetDragHandle(RefCounter* refCounter, const WidgetDragHandle& other) :
        DragHandle(refCounter, other), Widget(refCounter, other)
    {
        widgetOffsetToLocalTransformFunc = other.widgetOffsetToLocalTransformFunc;
        localToWidgetOffsetTransformFunc = other.localToWidgetOffsetTransformFunc;

        SetPosition(mPosition);
    }

    WidgetDragHandle::~WidgetDragHandle()
    {}

    WidgetDragHandle& WidgetDragHandle::operator=(const WidgetDragHandle& other)
    {
        Widget::operator=(other);
        DragHandle::operator=(other);

        widgetOffsetToLocalTransformFunc = other.widgetOffsetToLocalTransformFunc;
        localToWidgetOffsetTransformFunc = other.localToWidgetOffsetTransformFunc;

        SetPosition(mPosition);

        return *this;
    }

    void WidgetDragHandle::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        DragHandle::Draw();
    }

    void WidgetDragHandle::SetEnabled(bool enabled)
    {
        DragHandle::SetEnabled(enabled);
    }

    bool WidgetDragHandle::IsEnabled() const
    {
        return DragHandle::IsEnabled();
    }

    Vec2F WidgetDragHandle::ScreenToLocal(const Vec2F& point)
    {
        if (mParentWidget)
            return widgetOffsetToLocalTransformFunc(point - mParentWidget.Lock()->layout->GetWorldLeftBottom());

        return DragHandle::ScreenToLocal(point);
    }

    Vec2F WidgetDragHandle::LocalToScreen(const Vec2F& point)
    {
        if (mParentWidget)
            return localToWidgetOffsetTransformFunc(point) + mParentWidget.Lock()->layout->GetWorldLeftBottom();

        return DragHandle::ScreenToLocal(point);
    }

    String WidgetDragHandle::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    Ref<o2::RefCounterable> WidgetDragHandle::CastToRefCounterable(const Ref<WidgetDragHandle>& ref)
    {
        return DynamicCast<Widget>(ref);
    }

    void WidgetDragHandle::UpdateLayersLayouts()
    {
        onLayoutUpdated();
        UpdateScreenPosition();
    }

    void WidgetDragHandle::OnSerialize(DataValue& node) const
    {
        Widget::OnSerialize(node);
    }

    void WidgetDragHandle::OnDeserialized(const DataValue& node)
    {
        Widget::OnDeserialized(node);
    }

    void WidgetDragHandle::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Widget::OnDeserializedDelta(node, origin);
    }

    ISelectableDragHandlesGroup::ISelectableDragHandlesGroup()
    {}

    ISelectableDragHandlesGroup::ISelectableDragHandlesGroup(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    void ISelectableDragHandlesGroup::DeselectAll()
    {
        auto handles = GetAllHandles();
        for (auto& handle : handles)
            DeselectHandle(handle);
    }

    void ISelectableDragHandlesGroup::SelectAll()
    {
        auto handles = GetAllHandles();
        for (auto& handle : handles)
            SelectHandle(handle);
    }

    void ISelectableDragHandlesGroup::SetHandleSelectedState(const Ref<DragHandle>& handle, bool selected)
    {
        handle->mIsSelected = selected;
    }

    SelectableDragHandlesGroup::SelectableDragHandlesGroup(RefCounter* refCounter):
        ISelectableDragHandlesGroup(refCounter)
    {}

    SelectableDragHandlesGroup::SelectableDragHandlesGroup()
    {}

    SelectableDragHandlesGroup::~SelectableDragHandlesGroup()
    {
        for (auto& handle : mHandles)
            handle->mSelectGroup = nullptr;
    }

    const Vector<Ref<DragHandle>>& SelectableDragHandlesGroup::GetSelectedHandles() const
    {
        return mSelectedHandles;
    }

    Vector<Ref<DragHandle>> SelectableDragHandlesGroup::GetAllHandles() const
    {
        return mHandles;
    }

    void SelectableDragHandlesGroup::SelectHandle(const Ref<DragHandle>& handle)
    {
        if (mSelectedHandles.Contains(handle))
            return;

        handle->mIsSelected = true;
        mSelectedHandles.Add(handle);
        handle->OnSelected();

        OnSelectionChanged();
    }

    void SelectableDragHandlesGroup::DeselectHandle(const Ref<DragHandle>& handle)
    {
        if (!mSelectedHandles.Contains(handle))
            return;

        handle->mIsSelected = false;
        mSelectedHandles.Remove(handle);
        handle->OnDeselected();

        OnSelectionChanged();
    }

    void SelectableDragHandlesGroup::AddHandle(const Ref<DragHandle>& handle)
    {
        if (mHandles.Contains(handle))
            return;

        mHandles.Add(handle);
    }

    void SelectableDragHandlesGroup::RemoveHandle(DragHandle* handle)
    {
        mHandles.RemoveFirst([&](auto& x) { return x == handle; });

        int idx = mSelectedHandles.IndexOf([&](auto& x) { return x == handle; });
        if (idx >= 0)
        {
            mSelectedHandles.RemoveAt(idx);
            OnSelectionChanged();
        }
    }

    void SelectableDragHandlesGroup::DeselectAll()
    {
        for (auto& handle : mSelectedHandles)
        {
            handle->mIsSelected = false;
            handle->OnDeselected();
        }

        mSelectedHandles.Clear();
        OnSelectionChanged();
    }

    void SelectableDragHandlesGroup::SelectAll()
    {
        for (auto& handle : mHandles) {
            handle->mIsSelected = true;
            handle->OnSelected();
        }

        mSelectedHandles = mHandles;
        OnSelectionChanged();
    }

    void SelectableDragHandlesGroup::OnSelectionChanged()
    {}

    void SelectableDragHandlesGroup::OnHandleCursorPressed(const Ref<DragHandle>& handle, const Input::Cursor& cursor)
    {
        if (!GetSelectedHandles().Contains(handle))
        {
            if (!o2Input.IsKeyDown(VK_CONTROL))
                DeselectAll();

            SelectHandle(handle);
        }

        for (auto& handle : GetSelectedHandles())
            handle->BeginDrag(cursor.position);
    }

    void SelectableDragHandlesGroup::OnHandleCursorReleased(const Ref<DragHandle>& handle, const Input::Cursor& cursor)
    {}

    void SelectableDragHandlesGroup::OnHandleBeganDragging(const Ref<DragHandle>& handle)
    {
        if (handle->IsSelected())
            return;

        if (!o2Input.IsKeyDown(VK_CONTROL))
            DeselectAll();

        SelectHandle(handle);
    }

    void SelectableDragHandlesGroup::OnHandleMoved(const Ref<DragHandle>& handle, const Vec2F& cursorPos)
    {
        for (auto& selectedHandle : GetSelectedHandles())
        {
            if (selectedHandle == handle)
                continue;

            selectedHandle->mDragPosition = selectedHandle->ScreenToLocal(cursorPos) + selectedHandle->mDragOffset;
            selectedHandle->SetPosition(selectedHandle->mDragPosition);
            selectedHandle->onChangedPos(selectedHandle->GetPosition());
        }
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::WidgetDragHandle>);
// --- META ---

DECLARE_CLASS(o2::DragHandle, o2__DragHandle);

DECLARE_CLASS(o2::WidgetDragHandle, o2__WidgetDragHandle);
// --- END META ---
