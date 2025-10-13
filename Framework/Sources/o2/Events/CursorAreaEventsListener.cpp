#include "o2/stdafx.h"
#include "CursorAreaEventsListener.h"

#include "o2/Events/EventSystem.h"
#include "o2/Render/IDrawable.h"
#include "o2/Render/Render.h"

namespace o2
{
    CursorAreaEventsListener::CursorAreaEventsListener()
    {}

    CursorAreaEventsListener::CursorAreaEventsListener(const CursorAreaEventsListener& other)
    {}

    CursorAreaEventsListener::~CursorAreaEventsListener()
    {
        if (EventSystem::IsSingletonInitialzed())
            o2Events.UnregCursorAreaListener(this);
    }

    bool CursorAreaEventsListener::IsUnderPoint(const Vec2F& point)
    {
        return true;
    }

    bool CursorAreaEventsListener::IsScrollable() const
    {
        return false;
    }

    void CursorAreaEventsListener::SetInteractable(bool interactable)
    {
        if (mInteractable == interactable)
            return;

        mInteractable = interactable;

        if (mInteractable)
            OnBecomeInteractable();
        else
            OnBecomeNotInteractable();
    }

    bool CursorAreaEventsListener::IsInteractable() const
    {
        return mInteractable;
    }

    bool CursorAreaEventsListener::IsInputTransparent() const
    {
        return false;
    }

    void CursorAreaEventsListener::OnDrawn()
    {
        if (!mInteractable)
            return;

        mScissorRect = o2Render.GetResScissorRect();
        o2Events.DrawnCursorAreaListener(Ref(this));
    }

    void CursorAreaEventsListener::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorPressed(cursor);
    }

    void CursorAreaEventsListener::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorReleased(cursor);
    }

    void CursorAreaEventsListener::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorPressBreak(cursor);
    }

    void CursorAreaEventsListener::OnCursorPressedOutside(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorPressedOutside(cursor);
    }

    void CursorAreaEventsListener::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorReleasedOutside(cursor);
    }

    void CursorAreaEventsListener::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorStillDown(cursor);
    }

    void CursorAreaEventsListener::OnCursorMoved(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorMoved(cursor);
    }

    void CursorAreaEventsListener::OnCursorEnter(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorEnter(cursor);
    }

    void CursorAreaEventsListener::OnCursorExit(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorExit(cursor);
    }

    void CursorAreaEventsListener::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorDblClicked(cursor);
    }

    void CursorAreaEventsListener::OnCursorRightMousePressed(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorRightMousePressed(cursor);
    }

    void CursorAreaEventsListener::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorRightMouseReleased(cursor);
    }

    void CursorAreaEventsListener::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorRightMouseStayDown(cursor);
    }

    void CursorAreaEventsListener::OnCursorMiddleMousePressed(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorMiddleMousePressed(cursor);
    }

    void CursorAreaEventsListener::OnCursorMiddleMouseStayDown(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorMiddleMouseStayDown(cursor);
    }

    void CursorAreaEventsListener::OnCursorMiddleMouseReleased(const Input::Cursor& cursor)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnCursorMiddleMouseReleased(cursor);
    }

    void CursorAreaEventsListener::OnScrolled(float scroll)
    {
        if (messageFallDownListener)
            messageFallDownListener->OnScrolled(scroll);
    }

	String CursorAreaEventsListener::GetDebugName() const
	{
		return typeid(this).name();
	}

	bool CursorAreaEventsListener::IsPressed() const
    {
        return mIsPressed;
    }

    CursorEventsListener::CursorEventsListener(RefCounter* refCounter):
        RefCounterable(refCounter)
    {
        if (EventSystem::IsSingletonInitialzed())
            EventSystem::RegCursorListener(Ref(this));
    }

    CursorEventsListener::CursorEventsListener(RefCounter* refCounter, const CursorAreaEventsListener& other):
        RefCounterable(refCounter)
    {
        if (EventSystem::IsSingletonInitialzed())
            EventSystem::RegCursorListener(Ref(this));
    }

    CursorEventsListener::~CursorEventsListener()
    {
        EventSystem::UnregCursorListener(WeakRef(this));
    }
}
