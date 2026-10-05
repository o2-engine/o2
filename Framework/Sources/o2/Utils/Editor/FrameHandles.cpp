#include "o2/stdafx.h"
#include "FrameHandles.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/VectorSprite.h"

namespace o2
{
    FrameHandles::FrameHandles()
    {
        Vec2F rotateHandleSize = Vec2F(mHandlesRotateSize, mHandlesRotateSize);
        Vec2F frameHandleSize = Vec2F(mFrameHandlesSize, mFrameHandlesSize);

        mLeftTopRotateHandle = mmake<DragHandle>();
        mLeftTopRotateHandle->SetRegularDrawable(mmake<Sprite>(Color4(0, 0, 0, 20)));
        mLeftTopRotateHandle->SetHoverDrawable(mmake<Sprite>(Color4(0, 0, 0, 10)));
        mLeftTopRotateHandle->SetPressedDrawable(mmake<Sprite>(Color4(0, 0, 0, 50)));
        mLeftTopRotateHandle->GetRegularDrawable()->size = rotateHandleSize;
        mLeftTopRotateHandle->GetHoverDrawable()->size = rotateHandleSize;
        mLeftTopRotateHandle->GetPressedDrawable()->size = rotateHandleSize;
        mLeftTopRotateHandle->GetRegularDrawable()->szPivot = frameHandleSize*0.5f;
        mLeftTopRotateHandle->GetHoverDrawable()->szPivot = frameHandleSize*0.5f;
        mLeftTopRotateHandle->GetPressedDrawable()->szPivot = frameHandleSize*0.5f;

        mLeftBottomRotateHandle = mLeftTopRotateHandle->CloneAsRef<DragHandle>();
        mRightTopRotateHandle = mLeftTopRotateHandle->CloneAsRef<DragHandle>();
        mRightBottomRotateHandle = mLeftTopRotateHandle->CloneAsRef<DragHandle>();

        mLeftTopHandle = mmake<DragHandle>();
        mLeftTopHandle->SetRegularDrawable(mmake<VectorSprite>("ui/UI2_handle_regular.svg"));
        mLeftTopHandle->SetHoverDrawable(mmake<VectorSprite>("ui/UI2_handle_select.svg"));
        mLeftTopHandle->SetPressedDrawable(mmake<VectorSprite>("ui/UI2_handle_pressed.svg"));

        mLeftBottomHandle = mLeftTopHandle->CloneAsRef<DragHandle>();
        mRightTopHandle = mLeftTopHandle->CloneAsRef<DragHandle>();
        mRightBottomHandle = mLeftTopHandle->CloneAsRef<DragHandle>();

        mLeftHandle = mmake<DragHandle>();
        mLeftHandle->SetRegularDrawable(mmake<VectorSprite>("ui/UI2_handle_side_regular.svg"));
        mLeftHandle->SetHoverDrawable(mmake<VectorSprite>("ui/UI2_handle_side_select.svg"));
        mLeftHandle->SetPressedDrawable(mmake<VectorSprite>("ui/UI2_handle_side_pressed.svg"));

        mTopHandle = mLeftHandle->CloneAsRef<DragHandle>();
        mBottomHandle = mLeftHandle->CloneAsRef<DragHandle>();
        mRightHandle = mLeftHandle->CloneAsRef<DragHandle>();

        mPivotHandle = mmake<DragHandle>();
        mPivotHandle->SetRegularDrawable(mmake<VectorSprite>("ui/UI2_pivot.svg"));
        mPivotHandle->SetHoverDrawable(mmake<VectorSprite>("ui/UI2_pivot_select.svg"));
        mPivotHandle->SetPressedDrawable(mmake<VectorSprite>("ui/UI2_pivot_pressed.svg"));

        mLeftTopHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnLeftTopHandle);
        mLeftHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnLeftHandle);
        mLeftBottomHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnLeftBottomHandle);
        mTopHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnTopHandle);
        mBottomHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnBottomHandle);
        mRightTopHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnRightTopHandle);
        mRightHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnRightHandle);
        mRightBottomHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnRightBottomHandle);
        mLeftTopRotateHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnLeftTopRotateHandle);
        mLeftBottomRotateHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnLeftBottomRotateHandle);
        mRightTopRotateHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnRightTopRotateHandle);
        mRightBottomRotateHandle->onChangedPos = MakeFunction(this, &FrameHandles::OnRightBottomRotateHandle);

        mLeftTopHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mLeftHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mLeftBottomHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mTopHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mBottomHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mRightTopHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mRightHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mRightBottomHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mPivotHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mLeftTopRotateHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mLeftBottomRotateHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mRightTopRotateHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);
        mRightBottomRotateHandle->onPressed = MakeFunction(this, &FrameHandles::OnHandlePressed);

        mLeftTopHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mLeftHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mLeftBottomHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mTopHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mBottomHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mRightTopHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mRightHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mRightBottomHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mPivotHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mLeftTopRotateHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mLeftBottomRotateHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mRightTopRotateHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);
        mRightBottomRotateHandle->onReleased = MakeFunction(this, &FrameHandles::OnHandleReleased);

        mLeftTopHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mLeftHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mLeftBottomHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mTopHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mBottomHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mRightTopHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mRightHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mRightBottomHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mPivotHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mLeftTopRotateHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mLeftBottomRotateHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mRightTopRotateHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
        mRightBottomRotateHandle->onChangeCompleted = MakeFunction(this, &FrameHandles::OnChangeCompleted);
    }

    void FrameHandles::SetBasis(const Basis& basis)
    {
        mFrame = basis;
        mPivotHandle->position = mFrame*Vec2F(0.5f, 0.5f);
        UpdateHandlesTransform();
    }

    const Basis& FrameHandles::GetCurrentBasis() const
    {
        return mFrame;
    }

    void FrameHandles::SetPivot(const Vec2F& position)
    {
        mPivotHandle->position = position;
    }

    Vec2F FrameHandles::GetPivot() const
    {
        return mPivotHandle->GetPosition();
    }

    void FrameHandles::Draw()
    {
        o2Render.DrawAABasis(mFrame, mFrameColor, mFrameColor, mFrameColor);
        CursorAreaEventsListener::OnDrawn();

        if (mIsRotationAvailable)
        {
            mLeftTopRotateHandle->Draw();
            mLeftBottomRotateHandle->Draw();
            mRightTopRotateHandle->Draw();
            mRightBottomRotateHandle->Draw();
        }

        mLeftTopHandle->Draw();
        mLeftHandle->Draw();
        mLeftBottomHandle->Draw();
        mTopHandle->Draw();
        mBottomHandle->Draw();
        mRightTopHandle->Draw();
        mRightHandle->Draw();
        mRightBottomHandle->Draw();

        if (mIsPivotAvailable)
            mPivotHandle->Draw();
    }

    bool FrameHandles::IsUnderPoint(const Vec2F& point)
    {
        return mFrame.IsPointInside(point);
    }

    void FrameHandles::SetPivotEnabled(bool enabled)
    {
        mIsPivotAvailable = enabled;
    }

    bool FrameHandles::IsPivotEnabled() const
    {
        return mIsPivotAvailable;
    }

    void FrameHandles::SetRotationEnabled(bool enabled)
    {
        mIsRotationAvailable = enabled;
    }

    bool FrameHandles::IsRotationEnabled() const
    {
        return mIsRotationAvailable;
    }

    const Color4& FrameHandles::GetFrameColor() const
    {
        return mFrameColor;
    }

    bool FrameHandles::IsInputTransparent() const
    {
        return isInputTransparent;
    }

    void FrameHandles::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (mFrame.IsPointInside(cursor.position))
        {
            mIsDragging = true;
            mBeginDraggingFrame = mFrame;
            mBeginDraggingOffset = cursor.position - mFrame.origin;

            SetHandlesEnable(false);

            onPressed();
        }
    }

    void FrameHandles::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mIsDragging)
        {
            mIsDragging = false;
            SetHandlesEnable(true);

            onReleased();

            if (mBeginDraggingFrame != mFrame)
                onChangeCompleted();
        }
    }

    void FrameHandles::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        if (mIsDragging)
        {
            mIsDragging = false;
            SetHandlesEnable(true);

            onReleased();
        }
    }

    void FrameHandles::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mIsDragging)
        {
            Basis transformed(cursor.position - mBeginDraggingOffset, mFrame.xv, mFrame.yv);

            if (o2Input.IsKeyDown(VK_SHIFT))
            {
                if (Math::Abs(transformed.origin.x - mBeginDraggingFrame.origin.x) > Math::Abs(transformed.origin.y - mBeginDraggingFrame.origin.y))
                    transformed.origin.y = mBeginDraggingFrame.origin.y;
                else
                    transformed.origin.x = mBeginDraggingFrame.origin.x;
            }

            OnTransformed(mFrame.Inverted()*transformed);
        }
    }

    void FrameHandles::OnLeftTopHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(0.0f, 1.0f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mFrame.xv);
        Vec2F frameDeltaY = delta.Project(mFrame.yv);

        transformedFrame.origin += frameDeltaX;
        transformedFrame.xv -= frameDeltaX;
        transformedFrame.yv += frameDeltaY;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();

            if (transformedFrame.xv.Length() < transformedFrame.yv.Length())
            {
                Vec2F xd = transformedFrame.xv.Normalized()*
                    (transformedFrame.yv.Length()*aspect - transformedFrame.xv.Length());

                transformedFrame.origin -= xd;
                transformedFrame.xv += xd;
            }
            else transformedFrame.yv = transformedFrame.yv.Normalized()*transformedFrame.xv.Length()/aspect;
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnLeftHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(0.0f, 0.5f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mFrame.xv);

        transformedFrame.origin += frameDeltaX;
        transformedFrame.xv -= frameDeltaX;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();
            transformedFrame.yv = transformedFrame.yv.Normalized()*transformedFrame.xv.Length()/aspect;
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnLeftBottomHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(0.0f, 0.0f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mFrame.xv);
        Vec2F frameDeltaY = delta.Project(mFrame.yv);

        transformedFrame.origin += frameDeltaX + frameDeltaY;
        transformedFrame.xv -= frameDeltaX;
        transformedFrame.yv -= frameDeltaY;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();

            if (transformedFrame.xv.Length() < transformedFrame.yv.Length())
            {
                Vec2F xd = transformedFrame.xv.Normalized()*
                    (transformedFrame.yv.Length()*aspect - transformedFrame.xv.Length());

                transformedFrame.origin -= xd;
                transformedFrame.xv += xd;
            }
            else
            {
                Vec2F yd = transformedFrame.yv.Normalized()*
                    (transformedFrame.xv.Length()/aspect - transformedFrame.yv.Length());

                transformedFrame.origin -= yd;
                transformedFrame.yv += yd;
            }
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnTopHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(0.5f, 1.0f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaY = delta.Project(mFrame.yv);

        transformedFrame.yv += frameDeltaY;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();
            transformedFrame.xv = transformedFrame.xv.Normalized()*transformedFrame.yv.Length()*aspect;
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnBottomHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(0.5f, 0.0f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaY = delta.Project(mFrame.yv);

        transformedFrame.origin += frameDeltaY;
        transformedFrame.yv -= frameDeltaY;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();
            transformedFrame.xv = transformedFrame.xv.Normalized()*transformedFrame.yv.Length()*aspect;
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnRightTopHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(1.0f, 1.0f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mFrame.xv);
        Vec2F frameDeltaY = delta.Project(mFrame.yv);

        transformedFrame.xv += frameDeltaX;
        transformedFrame.yv += frameDeltaY;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();

            if (transformedFrame.xv.Length() < transformedFrame.yv.Length())
                transformedFrame.xv = transformedFrame.xv.Normalized()*transformedFrame.yv.Length()*aspect;
            else
                transformedFrame.yv = transformedFrame.yv.Normalized()*transformedFrame.xv.Length()/aspect;
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnRightHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(1.0f, 0.5f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mFrame.xv);

        transformedFrame.xv += frameDeltaX;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();
            transformedFrame.yv = transformedFrame.yv.Normalized()*transformedFrame.xv.Length()/aspect;
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnRightBottomHandle(const Vec2F& position)
    {
        Basis transformedFrame = mFrame;
        Vec2F lastHandleCoords = Vec2F(1.0f, 0.0f)*mFrame;
        Vec2F delta = position - lastHandleCoords;
        Vec2F frameDeltaX = delta.Project(mFrame.xv);
        Vec2F frameDeltaY = delta.Project(mFrame.yv);

        transformedFrame.origin += frameDeltaY;
        transformedFrame.xv += frameDeltaX;
        transformedFrame.yv -= frameDeltaY;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float aspect = mBeginDraggingFrame.xv.Length()/mBeginDraggingFrame.yv.Length();

            if (transformedFrame.xv.Length() < transformedFrame.yv.Length())
                transformedFrame.xv = transformedFrame.xv.Normalized()*transformedFrame.yv.Length()*aspect;
            else
            {
                Vec2F yd = transformedFrame.yv.Normalized()*
                    (transformedFrame.xv.Length()/aspect - transformedFrame.yv.Length());

                transformedFrame.origin -= yd;
                transformedFrame.yv += yd;
            }
        }

        OnTransformed(mFrame.Inverted()*transformedFrame);
    }

    void FrameHandles::OnLeftTopRotateHandle(const Vec2F& position)
    {
        Vec2F lastHandleCoords = Vec2F(0.0f, 1.0f)*mFrame;
        OnRotateHandle(position, lastHandleCoords);
    }

    void FrameHandles::OnLeftBottomRotateHandle(const Vec2F& position)
    {
        Vec2F lastHandleCoords = Vec2F(0.0f, 0.0f)*mFrame;
        OnRotateHandle(position, lastHandleCoords);
    }

    void FrameHandles::OnRightTopRotateHandle(const Vec2F& position)
    {
        Vec2F lastHandleCoords = Vec2F(1.0f, 1.0f)*mFrame;
        OnRotateHandle(position, lastHandleCoords);
    }

    void FrameHandles::OnRightBottomRotateHandle(const Vec2F& position)
    {
        Vec2F lastHandleCoords = Vec2F(1.0f, 0.0f)*mFrame;
        OnRotateHandle(position, lastHandleCoords);
    }

    void FrameHandles::OnRotateHandle(const Vec2F& position, Vec2F lastHandleCoords)
    {
        Vec2F rotatePivot = mPivotHandle->position;
        float angle = (position - rotatePivot).SignedAngle(lastHandleCoords - rotatePivot);
        Basis transform = Basis::Translated(rotatePivot*-1.0f)*Basis::Rotated(-angle)*Basis::Translated(rotatePivot);
        Basis transformed = mFrame*transform;

        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            float angle = Math::Rad2deg(transformed.GetAngle());
            float step = 15.0f;
            float targetAngle = Math::Round(angle/step)*step;
            float delta = targetAngle - angle;

            Basis deltaTransform = Basis::Translated(rotatePivot*-1.0f)*Basis::Rotated(Math::Deg2rad(delta))
                *Basis::Translated(rotatePivot);

            transformed = transformed*deltaTransform;
        }

        OnTransformed(mFrame.Inverted()*transformed);
    }

    void FrameHandles::OnHandlePressed()
    {
        mBeginDraggingFrame = mFrame;
        onPressed();
    }

    void FrameHandles::OnHandleReleased()
    {
        onReleased();
    }

    void FrameHandles::SetHandlesEnable(bool enable)
    {
        mLeftTopRotateHandle->enabled = enable;
        mLeftBottomRotateHandle->enabled = enable;
        mRightTopRotateHandle->enabled = enable;
        mRightBottomRotateHandle->enabled = enable;

        mLeftTopHandle->enabled = enable;
        mLeftHandle->enabled = enable;
        mLeftBottomHandle->enabled = enable;
        mTopHandle->enabled = enable;
        mBottomHandle->enabled = enable;
        mRightTopHandle->enabled = enable;
        mRightHandle->enabled = enable;
        mRightBottomHandle->enabled = enable;

        mPivotHandle->enabled = enable;
    }

    void FrameHandles::UpdateHandlesTransform()
    {
        float handlesAngle = mFrame.GetAngle();

        auto getHandleType = [&](const Vec2F& pos) {
            Vec2F v = pos - Vec2F(0.5f, 0.5f)*mFrame;
            float angle = Math::Rad2deg(v.Angle(Vec2F::Right()));

            if (angle < 22.5f || angle > 337.5f)
                return CursorType::SizeWE;
            else if (angle > 22.5f && angle < 67.5f)
                return CursorType::SizeNwSe;
            else if (angle > 67.5f && angle < 112.5f)
                return CursorType::SizeNS;
            else if (angle > 112.5f && angle < 157.5f)
                return CursorType::SizeNeSw;
            else if (angle > 157.5f && angle < 202.5f)
                return CursorType::SizeWE;
            else if (angle > 202.5f && angle < 247.5f)
                return CursorType::SizeNwSe;
            else if (angle > 247.5f && angle < 292.5f)
                return CursorType::SizeNS;
            else if (angle > 292.5f && angle < 337.5f)
                return CursorType::SizeNeSw;

            return CursorType::SizeNeSw;
        };

        mLeftTopHandle->position = Vec2F(0.0f, 1.0f)*mFrame;
        mLeftHandle->position = Vec2F(0.0f, 0.5f)*mFrame;
        mLeftBottomHandle->position = Vec2F(0.0f, 0.0f)*mFrame;
        mTopHandle->position = Vec2F(0.5f, 1.0f)*mFrame;
        mBottomHandle->position = Vec2F(0.5f, 0.0f)*mFrame;
        mRightTopHandle->position = Vec2F(1.0f, 1.0f)*mFrame;
        mRightHandle->position = Vec2F(1.0f, 0.5f)*mFrame;
        mRightBottomHandle->position = Vec2F(1.0f, 0.0f)*mFrame;

        mLeftTopHandle->cursorType = getHandleType(mLeftTopHandle->GetPosition());
        mLeftHandle->cursorType = getHandleType(mLeftHandle->GetPosition());
        mLeftBottomHandle->cursorType = getHandleType(mLeftBottomHandle->GetPosition());
        mTopHandle->cursorType = getHandleType(mTopHandle->GetPosition());
        mBottomHandle->cursorType = getHandleType(mBottomHandle->GetPosition());
        mRightTopHandle->cursorType = getHandleType(mRightTopHandle->GetPosition());
        mRightHandle->cursorType = getHandleType(mRightHandle->GetPosition());
        mRightBottomHandle->cursorType = getHandleType(mRightBottomHandle->GetPosition());

        mLeftTopRotateHandle->position = Vec2F(0.0f, 1.0f)*mFrame;
        mLeftBottomRotateHandle->position = Vec2F(0.0f, 0.0f)*mFrame;
        mRightTopRotateHandle->position = Vec2F(1.0f, 1.0f)*mFrame;
        mRightBottomRotateHandle->position = Vec2F(1.0f, 0.0f)*mFrame;

        mLeftTopHandle->angle = handlesAngle + Math::PI()*0.5f;
        mLeftHandle->angle = handlesAngle + Math::PI();
        mLeftBottomHandle->angle = handlesAngle + Math::PI();
        mTopHandle->angle = handlesAngle + Math::PI()*0.5f;
        mBottomHandle->angle = handlesAngle - Math::PI()*0.5f;
        mRightTopHandle->angle = handlesAngle;
        mRightHandle->angle = handlesAngle;
        mRightBottomHandle->angle = handlesAngle - Math::PI()*0.5f;

        mLeftTopRotateHandle->angle = handlesAngle + Math::PI()*0.5f;
        mLeftBottomRotateHandle->angle = handlesAngle + Math::PI();
        mRightTopRotateHandle->angle = handlesAngle;
        mRightBottomRotateHandle->angle = handlesAngle + Math::PI()*1.5f;
    }

    void FrameHandles::OnTransformed(const Basis& transform)
    {
        mFrame = mFrame*transform;
        mPivotHandle->position = mPivotHandle->position*transform;
        mChangedFromThis = true;
        UpdateHandlesTransform();

        onTransformed(mFrame);
    }

    void FrameHandles::OnChangeCompleted()
    {
        onChangeCompleted();
    }

}
