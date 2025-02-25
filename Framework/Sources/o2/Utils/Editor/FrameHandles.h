#pragma once

#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Color.h"

namespace o2
{
    // -------------------
    // Frame basis handles
    // -------------------
    class FrameHandles: public CursorAreaEventsListener, public RefCounterable
    {
    public:
        Function<void(const Basis&)> onTransformed;     // Basis transformed event
        Function<void()>             onPressed;         // Some handle pressed event
        Function<void()>             onReleased;        // Some handle released event
        Function<void()>             onChangeCompleted; // Basis changing completed, all handles released and transform was changed

        bool isInputTransparent = false;

    public:
        // Default constructor. Initializes handles
        FrameHandles();

        // Sets frame basis
        void SetBasis(const Basis& basis);

        // Returns current basis
        const Basis& GetCurrentBasis() const;

        // Sets pivot point
        void SetPivot(const Vec2F& position);

        // Returns pivot
        Vec2F GetPivot() const;

        // Draws frame and handles
        void Draw();

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Sets pivot editing available
        void SetPivotEnabled(bool enabled);

        // Is pivot editing available
        bool IsPivotEnabled() const;

        // Set rotation editing available
        void SetRotationEnabled(bool enabled);

        // Returns is rotation available
        bool IsRotationEnabled() const; 

        // Returns frame drawing color
        const Color4& GetFrameColor() const;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

    protected:
        const Color4 mFrameColor = Color4(44, 62, 80, 255); // Regular handle color
        const float  mFrameHandlesSize = 9.0f;              // Frame corners handles size
        const float  mHandlesRotateSize = 17.0f;            // Rotation frame handles size

        Ref<DragHandle> mLeftTopRotateHandle;      // Left top rotation handle
        Ref<DragHandle> mLeftBottomRotateHandle;   // Left bottom rotation handle
        Ref<DragHandle> mRightTopRotateHandle;     // Right top rotation handle
        Ref<DragHandle> mRightBottomRotateHandle;  // Right bottom rotation handle
        Ref<DragHandle> mLeftTopHandle;            // Left top corner frame handle
        Ref<DragHandle> mLeftHandle;               // Left corner frame handle
        Ref<DragHandle> mLeftBottomHandle;         // Left bottom corner frame handle
        Ref<DragHandle> mTopHandle;                // Top corner frame handle
        Ref<DragHandle> mBottomHandle;             // Bottom corner frame handle
        Ref<DragHandle> mRightTopHandle;           // Right top corner frame handle
        Ref<DragHandle> mRightHandle;              // Right corner frame handle
        Ref<DragHandle> mRightBottomHandle;        // Right bottom corner frame handle
        Ref<DragHandle> mPivotHandle;              // Frame pivot handle

        Basis mFrame;                // Frame basis
        Basis mBeginDraggingFrame;   // Frame before dragging any handle
        Vec2F mBeginDraggingOffset;  // Offset at beginning dragging from frame origin to cursor

        bool mIsPivotAvailable = true;    // Is pivot editing available
        bool mIsRotationAvailable = true; // Is rotation editing available

        bool mIsDragging = false;      // Is frame dragging
        bool mChangedFromThis = false; // Is actors changed from this, needs to break circular updating

    protected:
        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Left top handle moved
        void OnLeftTopHandle(const Vec2F& position);

        // Left handle moved
        void OnLeftHandle(const Vec2F& position);

        // Left bottom handle moved
        void OnLeftBottomHandle(const Vec2F& position);

        // Top handle moved
        void OnTopHandle(const Vec2F& position);

        // Bottom handle moved
        void OnBottomHandle(const Vec2F& position);

        // Right top handle moved
        void OnRightTopHandle(const Vec2F& position);

        // Right handle moved
        void OnRightHandle(const Vec2F& position);

        // Right bottom handle moved
        void OnRightBottomHandle(const Vec2F& position);

        // Left top rotation handle moved
        void OnLeftTopRotateHandle(const Vec2F& position);

        // Left bottom rotation handle moved
        void OnLeftBottomRotateHandle(const Vec2F& position);

        // Right top rotation handle moved
        void OnRightTopRotateHandle(const Vec2F& position);

        // Right bottom rotation handle moved
        void OnRightBottomRotateHandle(const Vec2F& position);

        // Rotate handle moved
        void OnRotateHandle(const Vec2F& position, Vec2F lastHandleCoords);

        // Called when one of handles was pressed, stores frame basis
        void OnHandlePressed();

        // Called when one of handles was released, calls event
        void OnHandleReleased();

        // Sets all handles enable
        void SetHandlesEnable(bool enable);

        // Updates handles position and angle
        void UpdateHandlesTransform();

        // Called when frame transformed or moved
        void OnTransformed(const Basis& transform);

        // Called when handle change completed, calling basis change completed event
        void OnChangeCompleted();

        REF_COUNTERABLE_IMPL(RefCounterable);
    };
}
