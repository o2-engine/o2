#include "o2Editor/stdafx.h"
#include "SplineEditor.h"

namespace Editor
{
    SplineEditor::SplineEditor()
    {
        mSelectionSprite = mmake<Sprite>("ui/UI_Window_place.png");

        mHandlesSample.position = mmake<DragHandle>(mmake<Sprite>("ui/CurveHandle.png"),
                                                    mmake<Sprite>("ui/CurveHandleHover.png"),
                                                    mmake<Sprite>("ui/CurveHandlePressed.png"),
                                                    mmake<Sprite>("ui/CurveHandleSelected.png"));

        mHandlesSample.prevSupport = mmake<DragHandle>(mmake<Sprite>("ui/CurveSupportHandle.png"),
                                                       mmake<Sprite>("ui/CurveSupportHandleHover.png"),
                                                       mmake<Sprite>("ui/CurveSupportHandlePressed.png"),
                                                       mmake<Sprite>("ui/CurveSupportHandleSelected.png"));

        mHandlesSample.nextSupport = mmake<DragHandle>(mmake<Sprite>("ui/CurveSupportHandle.png"),
                                                       mmake<Sprite>("ui/CurveSupportHandleHover.png"),
                                                       mmake<Sprite>("ui/CurveSupportHandlePressed.png"),
                                                       mmake<Sprite>("ui/CurveSupportHandleSelected.png"));

        mHandlesSample.leftRangeHandle = mmake<DragHandle>(mmake<Sprite>("ui/UI2_handle_side_regular.png"),
                                                           mmake<Sprite>("ui/UI2_handle_side_select.png"),
                                                           mmake<Sprite>("ui/UI2_handle_side_pressed.png"),
                                                           mmake<Sprite>("ui/UI2_handle_side_select.png"));

        mHandlesSample.rightRangeHandle = mmake<DragHandle>(mmake<Sprite>("ui/UI2_handle_side_regular.png"),
                                                            mmake<Sprite>("ui/UI2_handle_side_select.png"),
                                                            mmake<Sprite>("ui/UI2_handle_side_pressed.png"),
                                                            mmake<Sprite>("ui/UI2_handle_side_select.png"));

        mSplineColor = Color4(44, 62, 80, 255);
        mSplineSupportColor = Color4(190, 190, 190, 255);

        typedef SplineEditor thisclass;

        mTransformFrame = mmake<FrameHandles>();
        mTransformFrame->SetPivotEnabled(false);
        mTransformFrame->SetRotationEnabled(false);
        mTransformFrame->onTransformed = THIS_FUNC(OnTransformFrameTransformed);
        mTransformFrame->onPressed = THIS_FUNC(OnTransformBegin);
        mTransformFrame->onChangeCompleted = THIS_FUNC(OnTransformCompleted);
        mTransformFrame->isInputTransparent = true;

        mCurvesMesh = mmake<Mesh>();
        mCurvesMesh->Resize(4, 2);
        mCurvesMesh->vertexCount = 4;
        mCurvesMesh->polyCount = 2;
        mCurvesMesh->indexes[0] = 0; mCurvesMesh->indexes[1] = 1; mCurvesMesh->indexes[2] = 2;
        mCurvesMesh->indexes[3] = 2; mCurvesMesh->indexes[4] = 1; mCurvesMesh->indexes[5] = 3;
    }

    SplineEditor::~SplineEditor()
    {
        auto splineHandles = mSplineHandles;
        mSplineHandles.Clear();

        mSplineHandles.Clear();
    }

    void SplineEditor::Draw()
    {
        if (!mSplineWrapper)
            return;

        OnDrawn();

        auto leftRange = mSplineWrapper->GetDrawPointsLeftRange();
        auto rightRange = mSplineWrapper->GetDrawPointsRightRange();

        Color4 meshColor = mSplineColor;
        meshColor.a /= 2;

        for (int i = 1; i < leftRange.Count(); i++)
        {
            mCurvesMesh->vertices[0] = Vertex(leftRange[i - 1], meshColor.ABGR(), 0, 0);
            mCurvesMesh->vertices[1] = Vertex(leftRange[i], meshColor.ABGR(), 0, 0);
            mCurvesMesh->vertices[2] = Vertex(rightRange[i - 1], meshColor.ABGR(), 0, 0);
            mCurvesMesh->vertices[3] = Vertex(rightRange[i], meshColor.ABGR(), 0, 0);

            mCurvesMesh->Draw();
        }

        o2Render.DrawAALine(leftRange, mSplineColor);
        o2Render.DrawAALine(rightRange, mSplineColor);

        DrawTransformFrame();
        DrawHandles();
        DrawSelection();
    }

    void SplineEditor::DrawHandles()
    {
        for (auto& handles : mSplineHandles)
        {
            if (handles->IsSupportsVisible())
            {
                handles->prevSupport->UpdateScreenPosition();
                handles->nextSupport->UpdateScreenPosition();

                bool broken = (o2Input.IsKeyDown(VK_MENU) || handles->startDragFromZero) && o2Input.IsCursorDown();

                if (!handles->isFirst || mSplineWrapper->IsClosed())
                {
                    o2Render.DrawAALine(handles->position->GetScreenPosition(), handles->prevSupport->GetScreenPosition(),
                                        mSplineSupportColor, 1.0f, broken ? LineType::Dash : LineType::Solid);
                }

                if (!handles->isLast || mSplineWrapper->IsClosed())
                {
                    o2Render.DrawAALine(handles->position->GetScreenPosition(), handles->nextSupport->GetScreenPosition(),
                                        mSplineSupportColor, 1.0f, broken ? LineType::Dash : LineType::Solid);
                }
            }
        }

        if (o2Input.IsKeyDown(VK_MENU))
        {
            DrawMainHandles();
            DrawSupportHandles();
        }
        else
        {
            DrawSupportHandles();
            DrawMainHandles();
        }
    }

    void SplineEditor::DrawSelection()
    {
        if (mIsPressed)
        {
            mSelectionSprite->rect = RectF(mSplineWrapper->LocalToWorld(mSelectingPressedPoint), o2Input.cursorPos);
            mSelectionSprite->Draw();
        }
    }

    void SplineEditor::DrawTransformFrame()
    {
        if (!mTransformFrameVisible)
            return;

        Vec2F worldOrig = mSplineWrapper->LocalToWorld(mTransformFrameBasis.origin);
        Vec2F worldXV = mSplineWrapper->LocalToWorld(mTransformFrameBasis.xv + mTransformFrameBasis.origin) - worldOrig;
        Vec2F worldYV = mSplineWrapper->LocalToWorld(mTransformFrameBasis.yv + mTransformFrameBasis.origin) - worldOrig;
        mTransformFrame->SetBasis(Basis(worldOrig - mTransformBasisOffet,
                                        worldXV + Vec2F(mTransformBasisOffet.x*2.0f, 0),
                                        worldYV + Vec2F(0, mTransformBasisOffet.y*2.0f)));

        mTransformFrame->Draw();
    }

    void SplineEditor::DrawMainHandles()
    {
        int idx = 0;
        for (auto& handles : mSplineHandles)
        {
            handles->position->Draw();

            bool rangeHandlesVisible = !Math::Equals(mSplineWrapper->GetPointRangeValue(idx), 0.0f);

            if (rangeHandlesVisible)
            {
                Vec2F normal = GetRangeHandlesNormal(mSplineHandles.IndexOf(handles), handles);
                float angle = -normal.Angle(Vec2F(1, 0));

                handles->leftRangeHandle->angle = angle + Math::PI();
                handles->rightRangeHandle->angle = angle;

                handles->leftRangeHandle->Draw();
                handles->rightRangeHandle->Draw();
            }

            idx++;
        }
    }

    void SplineEditor::DrawSupportHandles()
    {
        for (auto& handles : mSplineHandles)
        {
            if (handles->IsSupportsVisible())
            {
                if (!handles->isFirst || mSplineWrapper->IsClosed())
                    handles->prevSupport->Draw();

                if (!handles->isLast || mSplineWrapper->IsClosed())
                    handles->nextSupport->Draw();
            }
        }
    }

    void SplineEditor::Update(float dt)
    {
    }

    void SplineEditor::SetSpline(const Ref<ISplineWrapper>& wrapper)
    {
        ClearHandles();

        mSplineWrapper = wrapper;
        if (!mSplineWrapper)
            return;

        InitializeHandles();
    }

    void SplineEditor::Reset()
    {
        SetSpline(nullptr);
        onChanged.Clear();
    }

    bool SplineEditor::IsUnderPoint(const Vec2F& point)
    {
        return true;
    }

    bool SplineEditor::IsInputTransparent() const
    {
        return true;
    }

    void SplineEditor::ClearHandles()
    {
        mSplineHandles.Clear();
    }

    void SplineEditor::OnSplineChanged()
    {
        ClearHandles();
        InitializeHandles();
    }

    void SplineEditor::InitializeHandles()
    {
        if (!mSplineWrapper)
            return;

        for (int i = 0; i < mSplineWrapper->GetPointsCount(); i++)
        {
            auto handles = mmake<PointHandles>();

            handles->isFirst = i == 0;
            handles->isLast = i == mSplineWrapper->GetPointsCount() - 1;

            handles->position = mHandlesSample.position->CloneAsRef<DragHandle>();
            handles->position->SetPosition(mSplineWrapper->GetPointPos(i));
            handles->position->SetSelectionGroup(Ref(this));
            handles->position->onPressed = [=]() { OnMainHandlePressed(i, handles); };
            handles->position->onBeganDragging = [=]() { handles->positionDragged = true; };
            handles->position->onReleased = [=]() { if (!handles->positionDragged) OnMainHandleReleasedNoDrag(i, handles); };
            handles->position->onChangedPos = [=](const Vec2F& pos) { OnMainHandleMoved(i, pos, handles); };
            handles->position->localToScreenTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->LocalToWorld(p); };
            handles->position->screenToLocalTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->WorldToLocal(p); };

            handles->prevSupport = mHandlesSample.prevSupport->CloneAsRef<DragHandle>();
            handles->prevSupport->SetPosition(mSplineWrapper->GetPointPrevSupportPos(i));
            handles->prevSupport->SetSelectionGroup(mSupportHandlesGroup);
            handles->prevSupport->onChangedPos = [=](const Vec2F& pos) { OnPrevHandleMoved(i, pos, handles); };
            handles->prevSupport->onPressed = [=]() { handles->prevSupportDragged = false; CheckDragFromZero(i, handles); };
            handles->prevSupport->onBeganDragging = [=]() { handles->prevSupportDragged = true; };
            handles->prevSupport->onReleased = [=]() { if (!handles->prevSupportDragged) OnPrevHandleReleasedNoDrag(i, handles); };
            handles->prevSupport->localToScreenTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->LocalToWorld(p); };
            handles->prevSupport->screenToLocalTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->WorldToLocal(p); };

            handles->nextSupport = mHandlesSample.nextSupport->CloneAsRef<DragHandle>();
            handles->nextSupport->SetPosition(mSplineWrapper->GetPointNextSupportPos(i));
            handles->nextSupport->SetSelectionGroup(mSupportHandlesGroup);
            handles->nextSupport->onChangedPos = [=](const Vec2F& pos) { OnNextHandleMoved(i, pos, handles); };
            handles->nextSupport->onPressed = [=]() { handles->nextSupportDragged = false; CheckDragFromZero(i, handles); };
            handles->nextSupport->onBeganDragging = [=]() { handles->nextSupportDragged = true; };
            handles->nextSupport->onReleased = [=]() { if (!handles->nextSupportDragged) OnNextHandleReleasedNoDrag(i, handles); };
            handles->nextSupport->localToScreenTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->LocalToWorld(p); };
            handles->nextSupport->screenToLocalTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->WorldToLocal(p); };

            handles->leftRangeHandle = mHandlesSample.leftRangeHandle->CloneAsRef<DragHandle>();
            handles->leftRangeHandle->SetPosition(GetRangeHandlePos(i, handles, true));
            handles->leftRangeHandle->onChangedPos = [=](const Vec2F& pos) { OnRangeValueChanged(i, handles, true); };
            handles->leftRangeHandle->checkPositionFunc = [=](const Vec2F& pos) { return CheckRangeHandlePos(i, handles, pos); };
            handles->leftRangeHandle->localToScreenTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->LocalToWorld(p); };
            handles->leftRangeHandle->screenToLocalTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->WorldToLocal(p); };

            handles->rightRangeHandle = mHandlesSample.rightRangeHandle->CloneAsRef<DragHandle>();
            handles->rightRangeHandle->SetPosition(GetRangeHandlePos(i, handles, false));
            handles->rightRangeHandle->onChangedPos = [=](const Vec2F& pos) { OnRangeValueChanged(i, handles, false); };
            handles->rightRangeHandle->checkPositionFunc = [=](const Vec2F& pos) { return CheckRangeHandlePos(i, handles, pos); };
            handles->rightRangeHandle->localToScreenTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->LocalToWorld(p); };
            handles->rightRangeHandle->screenToLocalTransformFunc = [&](const Vec2F& p) { return mSplineWrapper->WorldToLocal(p); };

            mSplineHandles.Add(handles);
        }
    }

    void SplineEditor::OnPrevHandleMoved(int i, const Vec2F& pos, const Ref<PointHandles>& handles)
    {
        Vec2F prev = mSplineWrapper->GetPointPrevSupportPos(i);
        Vec2F curr = mSplineWrapper->GetPointPos(i);
        Vec2F next = mSplineWrapper->GetPointNextSupportPos(i);

        if (!o2Input.IsKeyDown(VK_MENU) || handles->startDragFromZero)
        {
            float distanceDiff = (pos - curr).Length() - (prev - curr).Length();
            float nextDistance = (next - curr).Length();
            Vec2F symmetricNextPos = (curr - pos).Normalized()*(nextDistance + distanceDiff) + curr;
            mSplineWrapper->SetPointNextSupportPos(i, symmetricNextPos);

            handles->nextSupport->SetPosition(mSplineWrapper->GetPointNextSupportPos(i));
        }

        mSplineWrapper->SetPointPrevSupportPos(i, pos);
        mSplineWrapper->OnChanged();
        onChanged();
    }

    void SplineEditor::OnNextHandleMoved(int i, const Vec2F& pos, const Ref<PointHandles>& handles)
    {
        Vec2F prev = mSplineWrapper->GetPointPrevSupportPos(i);
        Vec2F curr = mSplineWrapper->GetPointPos(i);
        Vec2F next = mSplineWrapper->GetPointNextSupportPos(i);

        if (!o2Input.IsKeyDown(VK_MENU) || handles->startDragFromZero)
        {
            float distanceDiff = (pos - curr).Length() - (next - curr).Length();
            float prevDistance = (prev - curr).Length();
            Vec2F symmetricPrevPos = (curr - pos).Normalized()*(prevDistance + distanceDiff) + curr;
            mSplineWrapper->SetPointPrevSupportPos(i, symmetricPrevPos);

            handles->prevSupport->SetPosition(mSplineWrapper->GetPointPrevSupportPos(i));
        }

        mSplineWrapper->SetPointNextSupportPos(i, pos);
        mSplineWrapper->OnChanged();
        onChanged();
    }

    void SplineEditor::OnMainHandleMoved(int i, const Vec2F& pos, const Ref<PointHandles>& handles)
    {
        mSplineWrapper->SetPointPos(i, pos);

        handles->prevSupport->SetPosition(mSplineWrapper->GetPointPrevSupportPos(i));
        handles->nextSupport->SetPosition(mSplineWrapper->GetPointNextSupportPos(i));

        handles->leftRangeHandle->SetPosition(GetRangeHandlePos(i, handles, true));
        handles->rightRangeHandle->SetPosition(GetRangeHandlePos(i, handles, false));

        if (i > 0)
        {
            mSplineHandles[i - 1]->leftRangeHandle->SetPosition(GetRangeHandlePos(i - 1, mSplineHandles[i - 1], true));
            mSplineHandles[i - 1]->rightRangeHandle->SetPosition(GetRangeHandlePos(i - 1, mSplineHandles[i - 1], false));
        }

        if (i < mSplineHandles.Count() - 1)
        {
            mSplineHandles[i + 1]->leftRangeHandle->SetPosition(GetRangeHandlePos(i + 1, mSplineHandles[i + 1], true));
            mSplineHandles[i + 1]->rightRangeHandle->SetPosition(GetRangeHandlePos(i + 1, mSplineHandles[i + 1], false));
        }

        UpdateTransformFrame();
        mSplineWrapper->OnChanged();
        onChanged();
    }

    void SplineEditor::OnPrevHandleReleasedNoDrag(int i, const Ref<PointHandles>& handles)
    {
        if (o2Input.IsKeyDown(VK_MENU))
        {
            mSplineWrapper->SetPointPrevSupportPos(i, mSplineWrapper->GetPointPos(i));
            handles->prevSupport->SetPosition(mSplineWrapper->GetPointPrevSupportPos(i));

            UpdateTransformFrame();
            mSplineWrapper->OnChanged();
            onChanged();
        }
    }

    void SplineEditor::OnNextHandleReleasedNoDrag(int i, const Ref<PointHandles>& handles)
    {
        if (o2Input.IsKeyDown(VK_MENU))
        {
            mSplineWrapper->SetPointNextSupportPos(i, mSplineWrapper->GetPointPos(i));
            handles->nextSupport->SetPosition(mSplineWrapper->GetPointNextSupportPos(i));

            UpdateTransformFrame();
            mSplineWrapper->OnChanged();
            onChanged();
        }
    }

    void SplineEditor::OnMainHandlePressed(int i, const Ref<PointHandles>& handles)
    {
        handles->positionDragged = false;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float rangeValue = mSplineWrapper->GetPointRangeValue(i);

            if (Math::Equals(rangeValue, 0.0f))
                mSplineWrapper->SetPointRangeValue(i, 10.0f);
            else
                mSplineWrapper->SetPointRangeValue(i, 0.0f);

            handles->leftRangeHandle->SetPosition(GetRangeHandlePos(i, handles, true));
            handles->rightRangeHandle->SetPosition(GetRangeHandlePos(i, handles, false));
        }
    }

    void SplineEditor::OnMainHandleReleasedNoDrag(int i, const Ref<PointHandles>& handles)
    {
        if (o2Input.IsKeyDown(VK_MENU))
        {
            mSplineWrapper->SetPointNextSupportPos(i, mSplineWrapper->GetPointPos(i));
            mSplineWrapper->SetPointPrevSupportPos(i, mSplineWrapper->GetPointPos(i));

            handles->nextSupport->SetPosition(mSplineWrapper->GetPointNextSupportPos(i));
            handles->prevSupport->SetPosition(mSplineWrapper->GetPointPrevSupportPos(i));

            UpdateTransformFrame();
            mSplineWrapper->OnChanged();
            onChanged();
        }
    }

    void SplineEditor::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        const float createPointDistanceThreshold = 7.0f;

        Vec2F localCursorPos = mSplineWrapper->WorldToLocal(cursor.position);
        Vec2F newPointPosition = localCursorPos;
        int newPointIdx = Math::Max(0, mSplineWrapper->GetPointsCount());

        for (int i = 1; i < mSplineWrapper->GetPointsCount(); i++)
        {
            bool found = false;

            const ApproximationVec2F* points = mSplineWrapper->GetPointApproximationLeft(i);
            for (int j = 1; j < mSplineWrapper->GetPointApproximationCount(i); j++)
            {
                Vec2F a = mSplineWrapper->LocalToWorld(points[j - 1].value);
                Vec2F b = mSplineWrapper->LocalToWorld(points[j].value);

                Vec2F ab = b - a;
                float abl = ab.Length();
                Vec2F abn = ab/abl;
                Vec2F abnp = abn.Perpendicular();
                Vec2F ac = cursor.position - a;

                float pointDistance = (b - cursor.position).Length();
                float lineDistance = Math::Abs(abnp.Dot(ac));
                float proj = abn.Dot(ac);

                if (pointDistance < createPointDistanceThreshold ||
                    (lineDistance < createPointDistanceThreshold && proj > 0.0f && proj < abl))
                {
                    newPointPosition = mSplineWrapper->WorldToLocal(abnp*(abnp.Dot(ac)) + cursor.position);
                    newPointIdx = i;

                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        mSplineWrapper->AddPoint(newPointIdx, newPointPosition, Vec2F(), Vec2F());
        OnSplineChanged();
        mSplineWrapper->OnChanged();
        onChanged();
    }

    void SplineEditor::OnCursorPressed(const Input::Cursor& cursor)
    {
        mSelectingPressedPoint = mSplineWrapper->WorldToLocal(cursor.position);

        if (!o2Input.IsKeyDown(VK_CONTROL))
        {
            DeselectAll();
            mSupportHandlesGroup->DeselectAll();
        }
    }

    void SplineEditor::OnCursorReleased(const Input::Cursor& cursor)
    {
        for (auto& handle : mSelectingHandlesBuf)
        {
            SetHandleSelectedState(handle, false);
            handle->SetSelected(true);
        }

        mSelectingHandlesBuf.Clear();
        UpdateTransformFrame();
    }

    void SplineEditor::OnCursorStillDown(const Input::Cursor& cursor)
    {
        for (auto& handle : mSelectingHandlesBuf)
            SetHandleSelectedState(handle, false);

        mSelectingHandlesBuf.Clear();

        RectF selectionLocalRect(mSelectingPressedPoint, mSplineWrapper->WorldToLocal(cursor.position));

        for (auto& handle : mHandles)
        {
            if (handle->IsEnabled() && selectionLocalRect.IsInside(handle->GetPosition()) &&
                !mSelectedHandles.Contains(handle))
            {
                mSelectingHandlesBuf.Add(handle);
                SetHandleSelectedState(handle, true);
            }
        }

        for (auto& handle : mSupportHandlesGroup->GetAllHandles())
        {
            if (handle->IsEnabled() && selectionLocalRect.IsInside(handle->GetPosition()) &&
                !mSupportHandlesGroup->GetSelectedHandles().Contains(handle))
            {
                mSelectingHandlesBuf.Add(handle);
                SetHandleSelectedState(handle, true);
            }
        }
    }

    void SplineEditor::OnSelectionChanged()
    {
    }

    void SplineEditor::OnKeyReleased(const Input::Key& key)
    {
        if (!mSplineWrapper)
            return;

        if (key.keyCode == VK_DELETE)
        {
            auto selectedHandles = mSelectedHandles;
            for (auto& handle : selectedHandles)
            {
                int idx = mHandles.IndexOf(handle);
                mSplineWrapper->RemovePoint(idx);
            }

            OnSplineChanged();
            mSplineWrapper->OnChanged();
            onChanged();
        }
    }

    void SplineEditor::CheckDragFromZero(int i, const Ref<PointHandles>& handles)
    {
        float screenThreshold = 3.0f;
        float threshold = mSplineWrapper->WorldToLocal(Vec2F(screenThreshold, screenThreshold)).x -
            mSplineWrapper->WorldToLocal(Vec2F()).x;

        Vec2F prev = mSplineWrapper->GetPointPrevSupportPos(i);
        Vec2F next = mSplineWrapper->GetPointNextSupportPos(i);

        handles->startDragFromZero = (next - prev).Length() < threshold;
    }

    void SplineEditor::OnRangeValueChanged(int i, const Ref<PointHandles>& handles, bool isLeft)
    {
        Vec2F normal = GetRangeHandlesNormal(i, handles);

        o2Debug.DrawLine(mSplineWrapper->GetPointPos(i), mSplineWrapper->GetPointPos(i) + normal*100.0f);

        if (isLeft)
        {
            Vec2F handlePos = handles->leftRangeHandle->GetPosition();
            float value = (handlePos - mSplineWrapper->GetPointPos(i)).Dot(normal);
            mSplineWrapper->SetPointRangeValue(i, -value);

            handles->rightRangeHandle->SetPosition(GetRangeHandlePos(i, handles, false));
        }
        else
        {
            Vec2F handlePos = handles->rightRangeHandle->GetPosition();
            float value = (handlePos - mSplineWrapper->GetPointPos(i)).Dot(normal);
            mSplineWrapper->SetPointRangeValue(i, value);

            handles->leftRangeHandle->SetPosition(GetRangeHandlePos(i, handles, true));
        }
    }

    Vec2F SplineEditor::GetRangeHandlePos(int i, const Ref<PointHandles>& handles, bool isLeft) const
    {
        Vec2F normal = GetRangeHandlesNormal(i, handles);

        if (isLeft)
        {
            float value = mSplineWrapper->GetPointRangeValue(i);
            return mSplineWrapper->GetPointPos(i) - normal*value;
        }
        else
        {
            float value = mSplineWrapper->GetPointRangeValue(i);
            return mSplineWrapper->GetPointPos(i) + normal*value;
        }
    }

    Vec2F SplineEditor::GetRangeHandlesNormal(int i, const Ref<PointHandles>& handles) const
    {
        bool hasPrev = i > 0;
        bool hasNext = i < mSplineWrapper->GetPointsCount() - 1;

        Vec2F prev = hasPrev ? mSplineWrapper->GetPointPos(i - 1) : Vec2F();
        Vec2F next = hasNext ? mSplineWrapper->GetPointPos(i + 1) : Vec2F();

        return Spline::Key::GetRangeNormal(mSplineWrapper->GetPointPos(i),
                                           hasPrev ? &prev : nullptr,
                                           hasNext ? &next : nullptr);
    }

    Vec2F SplineEditor::CheckRangeHandlePos(int i, const Ref<PointHandles>& handles, const Vec2F& pos)
    {
        Vec2F normal = GetRangeHandlesNormal(i, handles);
        return mSplineWrapper->GetPointPos(i) + normal*(normal.Dot(pos - mSplineWrapper->GetPointPos(i)));
    }

    void SplineEditor::UpdateTransformFrame()
    {
        mTransformFrameVisible = IsTransformFrameVisible();

        if (!mTransformFrameVisible || mSelectedHandles.IsEmpty())
            return;

        RectF aabb((mSelectedHandles[0])->GetPosition(), (mSelectedHandles[0])->GetPosition());

        for (auto& handle : mSelectedHandles)
        {
            aabb.left = Math::Min(handle->GetPosition().x, aabb.left);
            aabb.right = Math::Max(handle->GetPosition().x, aabb.right);
            aabb.top = Math::Max(handle->GetPosition().y, aabb.top);
            aabb.bottom = Math::Min(handle->GetPosition().y, aabb.bottom);
        }

        for (auto& handle : mSupportHandlesGroup->GetSelectedHandles())
        {
            aabb.left = Math::Min(handle->GetPosition().x, aabb.left);
            aabb.right = Math::Max(handle->GetPosition().x, aabb.right);
            aabb.top = Math::Max(handle->GetPosition().y, aabb.top);
            aabb.bottom = Math::Min(handle->GetPosition().y, aabb.bottom);
        }

        mTransformFrameBasis = Basis(aabb.LeftBottom(), Vec2F::Right()*aabb.Width(), Vec2F::Up()*aabb.Height());
    }

    bool SplineEditor::IsTransformFrameVisible() const
    {
        int selectedMainHandles = 0;

        for (auto& handles : mSplineHandles)
        {
            if (handles->position->IsSelected())
                selectedMainHandles++;
        }

        return selectedMainHandles > 1;
    }

    void SplineEditor::OnTransformFrameTransformed(const Basis& basis)
    {
        Vec2F localBasisOrig = mSplineWrapper->WorldToLocal(basis.origin + mTransformBasisOffet);
        Vec2F localBasisXV = mSplineWrapper->WorldToLocal(basis.xv - Vec2F(mTransformBasisOffet.x*2.0f, 0) + basis.origin + mTransformBasisOffet) - localBasisOrig;
        Vec2F localBasisYV = mSplineWrapper->WorldToLocal(basis.yv - Vec2F(0, mTransformBasisOffet.y*2.0f) + basis.origin + mTransformBasisOffet) - localBasisOrig;
        Basis localBasis(localBasisOrig, localBasisXV, localBasisYV);

        Basis delta = mTransformFrameBasis.Inverted()*localBasis;

        if (delta.origin.Length() > 0.01f || delta.xv != Vec2F(1, 0) || delta.yv != Vec2F(0, 1))
        {
            Vector<Vec2F> newPosHandlesPositions, newSupportnHandlesPositions;

            for (int i = 0; i < mSelectedHandles.Count(); i++)
                newPosHandlesPositions.Add(mSelectedHandles[i]->GetPosition()*delta);

            auto& supportHandles = mSupportHandlesGroup->GetSelectedHandles();
            for (int i = 0; i < supportHandles.Count(); i++)
                newSupportnHandlesPositions.Add(supportHandles[i]->GetPosition()*delta);

            for (int i = 0; i < mSelectedHandles.Count(); i++)
            {
                mSelectedHandles[i]->SetPosition(newPosHandlesPositions[i]);
                mSelectedHandles[i]->onChangedPos(mSelectedHandles[i]->GetPosition());
            }

            for (int i = 0; i < supportHandles.Count(); i++)
            {
                supportHandles[i]->SetPosition(newSupportnHandlesPositions[i]);
                supportHandles[i]->onChangedPos(supportHandles[i]->GetPosition());
            }

            UpdateTransformFrame();
        }

        mSplineWrapper->OnChanged();
        onChanged();
    }

    void SplineEditor::OnTransformBegin()
    {
    }

    void SplineEditor::OnTransformCompleted()
    {
    }

    Vec2F SplineEditor::ISplineWrapper::WorldToLocal(const Vec2F& point) const
    {
        return point;
    }

    Vec2F SplineEditor::ISplineWrapper::LocalToWorld(const Vec2F& point) const
    {
        return point;
    }

    Vector<Vec2F> SplineEditor::ISplineWrapper::GetDrawPointsLeftRange() const
    {
        Vector<Vec2F> res;

        for (int i = 1; i < GetPointsCount(); i++)
        {
            for (int j = 0; j < GetPointApproximationCount(i) - 1; j++)
                res.Add(LocalToWorld(GetPointApproximationLeft(i)[j].value));
        }

        return res;
    }

    Vector<Vec2F> SplineEditor::ISplineWrapper::GetDrawPointsRightRange() const
    {
        Vector<Vec2F> res;

        for (int i = 1; i < GetPointsCount(); i++)
        {
            for (int j = 0; j < GetPointApproximationCount(i) - 1; j++)
                res.Add(LocalToWorld(GetPointApproximationRight(i)[j].value));
        }

        return res;
    }

    bool SplineEditor::PointHandles::IsSupportsVisible() const
    {
        return position->IsSelected() || prevSupport->IsSelected() ||nextSupport->IsSelected() || o2Input.IsKeyDown(VK_MENU);
    }

}
