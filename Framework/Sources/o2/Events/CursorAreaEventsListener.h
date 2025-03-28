#pragma once

#include "o2/Application/Input.h"
#include "o2/Events/IEventsListener.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"

namespace o2
{
    class IDrawable;

    // -----------------------
    // Cursors events listener
    // -----------------------
    class CursorAreaEventsListener: public IEventsListener
    {
    public:
        PROPERTIES(CursorAreaEventsListener);
        PROPERTY(bool, interactable, SetInteractable, IsInteractable); // Interactable flag property

    public:
        CursorAreaEventsListener* messageFallDownListener = nullptr; // Messages fall down listener. 
                                                                     // All messages received in this object 
                                                                     // will be delivered to messageFallDownListener

    public:
        // Default constructor
        CursorAreaEventsListener();

        // Copy-constructor
        CursorAreaEventsListener(const CursorAreaEventsListener& other);

        // Virtual destructor
        virtual ~CursorAreaEventsListener();

        // Returns true if point is in this object
        virtual bool IsUnderPoint(const Vec2F& point);

        // Returns is listener scrollable
        virtual bool IsScrollable() const;

        // Sets interactable flag
        virtual void SetInteractable(bool interactable);

        // Returns interactable flag
        virtual bool IsInteractable() const;

        // Returns true when input events can be handled by down listeners
        virtual bool IsInputTransparent() const;

        // Returns true if this was pressed by some cursor
        bool IsPressed() const;

        // Called when listener was drawn
        virtual void OnDrawn();

    protected:
        // Called when cursor pressed on this
        virtual void OnCursorPressed(const Input::Cursor& cursor);

        // Called when cursor released (only when cursor pressed this at previous time)
        virtual void OnCursorReleased(const Input::Cursor& cursor);

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        virtual void OnCursorPressBreak(const Input::Cursor& cursor);

        // Called when cursor pressed outside this
        virtual void OnCursorPressedOutside(const Input::Cursor& cursor);

        // Called when cursor released outside this(only when cursor pressed this at previous time)
        virtual void OnCursorReleasedOutside(const Input::Cursor& cursor);

        // Called when cursor stay down during frame
        virtual void OnCursorStillDown(const Input::Cursor& cursor);

        // Called when cursor moved on this (or moved outside when this was pressed)
        virtual void OnCursorMoved(const Input::Cursor& cursor);

        // Called when cursor enters this object
        virtual void OnCursorEnter(const Input::Cursor& cursor);

        // Called when cursor exits this object
        virtual void OnCursorExit(const Input::Cursor& cursor);

        // Called when cursor double clicked
        virtual void OnCursorDblClicked(const Input::Cursor& cursor);

        // Called when right mouse button was pressed on this
        virtual void OnCursorRightMousePressed(const Input::Cursor& cursor);

        // Called when right mouse button stay down on this
        virtual void OnCursorRightMouseStayDown(const Input::Cursor& cursor);

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        virtual void OnCursorRightMouseReleased(const Input::Cursor& cursor);

        // Called when middle mouse button was pressed on this
        virtual void OnCursorMiddleMousePressed(const Input::Cursor& cursor);

        // Called when middle mouse button stay down on this
        virtual void OnCursorMiddleMouseStayDown(const Input::Cursor& cursor);

        // Called when middle mouse button was released (only when middle mouse button pressed this at previous time)
        virtual void OnCursorMiddleMouseReleased(const Input::Cursor& cursor);

        // Called when scrolling
        virtual void OnScrolled(float scroll);

		// Returns debug name
		virtual String GetDebugName() const;
        
    protected:
        RectF mScissorRect; // Scissor rect at drawing moment

        bool mInteractable = true; // True when listener is interactable

        bool mIsPressed = false;            // True when was pressed by cursor
        bool mIsRightMousePressed = false;  // True when was pressed by right mouse button
        bool mIsMiddleMousePressed = false; // True when was pressed by middle mouse button

        float mLastPressedTime = -1.0f; // Last cursor pressed time

    protected:
        // Called when listener becomes interactable
        virtual void OnBecomeInteractable() {}

        // Called when listener stops interacting
        virtual void OnBecomeNotInteractable() {}

        friend class EventSystem;
        friend class CursorAreaEventListenersLayer;
    };

    // -----------------------
    // Cursors events listener
    // -----------------------
    class CursorEventsListener: public RefCounterable
    {
    public:
        // Default constructor
        CursorEventsListener(RefCounter* refCounter);

        // Copy-constructor
        CursorEventsListener(RefCounter* refCounter, const CursorAreaEventsListener& other);

        // Virtual destructor
        virtual ~CursorEventsListener();

    protected:
        // Called when cursor pressed on this
        virtual void OnCursorPressed(const Input::Cursor& cursor) {}

        // Called when cursor released (only when cursor pressed this at previous time)
        virtual void OnCursorReleased(const Input::Cursor& cursor) {}

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        virtual void OnCursorPressBreak(const Input::Cursor& cursor) {}

        // Called when cursor stay down during frame
        virtual void OnCursorStillDown(const Input::Cursor& cursor) {}

        // Called when cursor moved on this (or moved outside when this was pressed)
        virtual void OnCursorMoved(const Input::Cursor& cursor) {}

        // Called when cursor double clicked
        virtual void OnCursorDblClicked(const Input::Cursor& cursor) {}

        // Called when right mouse button was pressed on this
        virtual void OnCursorRightMousePressed(const Input::Cursor& cursor) {}

        // Called when right mouse button stay down on this
        virtual void OnCursorRightMouseStillDown(const Input::Cursor& cursor) {}

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        virtual void OnCursorRightMouseReleased(const Input::Cursor& cursor) {}

        // Called when middle mouse button was pressed on this
        virtual void OnCursorMiddleMousePressed(const Input::Cursor& cursor) {}

        // Called when middle mouse button stay down on this
        virtual void OnCursorMiddleMouseStillDown(const Input::Cursor& cursor) {}

        // Called when middle mouse button was released (only when middle mouse button pressed this at previous time)
        virtual void OnCursorMiddleMouseReleased(const Input::Cursor& cursor) {}

        // Called when scrolling
        virtual void OnScrolled(float scroll) {}

        friend class EventSystem;
    };
}
