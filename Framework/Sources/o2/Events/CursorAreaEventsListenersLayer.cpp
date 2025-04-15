#include "o2/stdafx.h"
#include "CursorAreaEventsListenersLayer.h"

#include "o2/Events/EventSystem.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Utils/Editor/DragAndDrop.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    CursorAreaEventListenersLayer::~CursorAreaEventListenersLayer()
    {
        if (EventSystem::IsSingletonInitialzed())
            o2Events.RemoveCursorAreaEventsListenersLayer(WeakRef(this));
    }

    void CursorAreaEventListenersLayer::OnBeginDraw()
    {
        //PROFILE_SAMPLE_FUNC();

        viewPortBasis = o2Render.GetCamera().GetBasis();
        o2Events.PushCursorAreaEventsListenersLayer(Ref(this));
    }

    void CursorAreaEventListenersLayer::OnEndDraw()
    {
        //PROFILE_SAMPLE_FUNC();

        o2UI.DrawCurrentLayerTopWidgets();

        renderBasis = o2Render.GetCamera().GetBasis();
        o2Events.PopCursorAreaEventsListenersLayer();
    }

    void CursorAreaEventListenersLayer::OnDrawn(const Basis& transform)
    {
        //PROFILE_SAMPLE_FUNC();

        isEditor = EditorScope::IsInScope();

        drawnTransform = transform;
        mLocalToWorldTransform = camera.GetBasis().Inverted()*renderBasis*viewPortBasis.Inverted()*drawnTransform;

        CursorAreaEventsListener::OnDrawn();
    }

    Vec2F CursorAreaEventListenersLayer::ToLocal(const Vec2F& point) const
    {
        return point*mLocalToWorldTransform.Inverted();
    }

    Vec2F CursorAreaEventListenersLayer::FromLocal(const Vec2F& point) const
    {
        return point*mLocalToWorldTransform;
    }

    Vec2F CursorAreaEventListenersLayer::ScreenToLocal(const Vec2F& point) const
    {
        if (mParentLayer)
            return ToLocal(mParentLayer.Lock()->ScreenToLocal(point));
        
        return ToLocal(point);
    }

    Vec2F CursorAreaEventListenersLayer::ScreenFromLocal(const Vec2F& point) const
    {
        if (mParentLayer)
            return FromLocal(mParentLayer.Lock()->ScreenFromLocal(point));

        return FromLocal(point);
    }

    void CursorAreaEventListenersLayer::Update()
    {
        PROFILE_SAMPLE_FUNC();

        int editorScopeDepth = EditorScope::GetDepth();
        if (!isEditor)
            EditorScope::Exit(editorScopeDepth);

        cursorEventAreaListeners.Reverse();
        mDragListeners.Reverse();

        mLastUnderCursorListeners = mUnderCursorListeners;
        mUnderCursorListeners.Clear();

        if (mEnabled)
        {
            for (const Input::Cursor& cursor : o2Input.GetCursors())
                ProcessCursorTracing(cursor);
        }

        ProcessCursorExit();
        ProcessCursorEnter();
        ProcessScrolling();

        for (const Input::Cursor& cursor : o2Input.GetCursors())
        {
            if (cursor.pressedTime < FLT_EPSILON && cursor.isPressed)
                ProcessCursorPressed(cursor);
            else
                ProcessCursorDown(cursor);
        }

        for (const Input::Cursor& cursor : o2Input.GetReleasedCursors())
        {
            ProcessCursorReleased(cursor);
            mPressedListeners.Remove(cursor.id);
        }

        for (const Input::Key& key : o2Input.GetPressedKeys())
        {
            if (key.keyCode == -1)
                ProcessRBPressed();
            else if (key.keyCode == -2)
                ProcessMBPressed();
        }

        for (const Input::Key& key : o2Input.GetDownKeys())
        {
            if (key.keyCode == -1)
                ProcessRBDown();
            else if (key.keyCode == -2)
                ProcessMBDown();
        }

        for (const Input::Key& key : o2Input.GetReleasedKeys())
        {
            if (key.keyCode == -1)
                ProcessRBReleased();
            else if (key.keyCode == -2)
                ProcessMBReleased();
        }

        if (!isEditor)
            EditorScope::Enter(editorScopeDepth);
    }

    void CursorAreaEventListenersLayer::PostUpdate()
    {
        cursorEventAreaListeners.Clear();
        mDragListeners.Clear();
    }

    void CursorAreaEventListenersLayer::BreakCursorEvent()
    {
        for (auto& kv : mPressedListeners)
        {
            for (auto& listener : kv.second)
            {
                listener->OnCursorPressBreak(*o2Input.GetCursor(kv.first));
                listener->mIsPressed = false;
            }
        }

        mPressedListeners.Clear();
    }

    void CursorAreaEventListenersLayer::UnregCursorAreaListener(CursorAreaEventsListener* listener)
    {
        cursorEventAreaListeners.RemoveFirst([&](auto& x) { return x == listener; });
    }

    void CursorAreaEventListenersLayer::UnregDragListener(DragableObject* listener)
    {
        mDragListeners.RemoveFirst([&](auto& x) { return x == listener; });
    }

    Vector<Ref<CursorAreaEventsListener>> CursorAreaEventListenersLayer::GetAllCursorListenersUnderCursor(const Vec2F& cursorPos) const
    {
        Vector<Ref<CursorAreaEventsListener>> res;
        Vec2F localCursorPos = ToLocal(cursorPos);
        for (auto& listenerWeak : cursorEventAreaListeners)
        {
            auto listener = listenerWeak.Lock();
            if (!listener || !listener->IsUnderPoint(localCursorPos) || !listener->mScissorRect.IsInside(localCursorPos) || !listener->mInteractable)
                continue;

            if (auto layer = DynamicCast<CursorAreaEventListenersLayer>(listener))
                res += layer->GetAllCursorListenersUnderCursor(localCursorPos);
            else
                res.Add(listener);
        }

        return res;
    }

    bool CursorAreaEventListenersLayer::IsUnderPoint(const Vec2F& point)
    {
        return drawnTransform.IsPointInside(point);
    }

    bool CursorAreaEventListenersLayer::IsInputTransparent() const
    {
        return isTransparent;
    }

	String CursorAreaEventListenersLayer::GetDebugName() const
	{
		return CursorAreaEventsListener::GetDebugName() + "(" + name = ")";
	}

	void CursorAreaEventListenersLayer::OnCursorEnter(const Input::Cursor& cursor)
    {
        mEnabled = true;
    }

    void CursorAreaEventListenersLayer::OnCursorExit(const Input::Cursor& cursor)
    {
        mEnabled = false;
    }

    Input::Cursor CursorAreaEventListenersLayer::ConvertLocalCursor(const Input::Cursor& cursor) const
    {
        Input::Cursor localCursor = cursor;
        localCursor.position = ScreenToLocal(cursor.position);
        localCursor.delta = ScreenToLocal(cursor.position) - ScreenToLocal(cursor.position - cursor.delta);

        return localCursor;
    }

    void CursorAreaEventListenersLayer::ProcessCursorTracing(const Input::Cursor& cursor)
    {
        auto localCursor = ConvertLocalCursor(cursor);

        for (auto& listenerWeak : cursorEventAreaListeners)
        {
            auto listener = listenerWeak.Lock();
            if (!listener || !listener->IsUnderPoint(localCursor.position) || !listener->mScissorRect.IsInside(localCursor.position))
                continue;

            auto drag = DynamicCast<DragableObject>(listener);
            if (drag && drag->IsDragging())
                continue;

            if (!mUnderCursorListeners.ContainsKey(localCursor.id))
                mUnderCursorListeners.Add(localCursor.id, {});
            
            mUnderCursorListeners[localCursor.id].Add(listener);

            if (!listener->IsInputTransparent())
                return;
        }
    }

    void CursorAreaEventListenersLayer::ProcessCursorEnter()
    {
        for (auto& underCursorListeners : mUnderCursorListeners)
        {
            bool lastListenersHasSameCursor = mLastUnderCursorListeners.ContainsKey(underCursorListeners.first);
            for (auto& listener : underCursorListeners.second)
            {
                if (!lastListenersHasSameCursor || !mLastUnderCursorListeners[underCursorListeners.first].Contains(listener))
                    listener->OnCursorEnter(ConvertLocalCursor(*o2Input.GetCursor(underCursorListeners.first)));
            }
        }
    }

    void CursorAreaEventListenersLayer::ProcessCursorExit()
    {
        for (auto& lastUnderCursorListeners : mLastUnderCursorListeners)
        {
            bool listenersHasSameCursor = mUnderCursorListeners.ContainsKey(lastUnderCursorListeners.first);
            for (auto& listener : lastUnderCursorListeners.second)
            {
                if (!listenersHasSameCursor || !mUnderCursorListeners[lastUnderCursorListeners.first].Contains(listener))
                    listener->OnCursorExit(ConvertLocalCursor(*o2Input.GetCursor(lastUnderCursorListeners.first)));
            }
        }
    }

    void CursorAreaEventListenersLayer::ProcessCursorPressed(const Input::Cursor& cursor)
    {
        auto localCursor = ConvertLocalCursor(cursor);

        for (auto& listenerWeak : cursorEventAreaListeners)
        {
            auto listener = listenerWeak.Lock();
            if (listener && !listener->IsUnderPoint(localCursor.position))
                listener->OnCursorPressedOutside(localCursor);
        }

        if (!mUnderCursorListeners.ContainsKey(localCursor.id))
            return;

        for (auto& listener : mUnderCursorListeners[localCursor.id])
        {
            float time = o2Time.GetApplicationTime();
            float s = time - listener->mLastPressedTime;
            float x = o2Events.GetDoubleClickTime();
            float d = s - x;
            if (d < 0)
                listener->OnCursorDblClicked(localCursor);
            else
            {
                if (!mPressedListeners.ContainsKey(localCursor.id))
                    mPressedListeners.Add(localCursor.id, {});

                mPressedListeners[localCursor.id].Add(listener);
                listener->OnCursorPressed(localCursor);
                listener->mIsPressed = true;
                listener->mLastPressedTime = time;
            }
        }
    }

    void CursorAreaEventListenersLayer::ProcessCursorDown(const Input::Cursor& cursor)
    {
        auto localCursor = ConvertLocalCursor(cursor);

        if (mPressedListeners.ContainsKey(localCursor.id))
        {
            bool broken = false;
            for (auto it = mPressedListeners[localCursor.id].begin(); it != mPressedListeners[localCursor.id].end();)
            {
                if (!broken)
                {
                    (*it)->OnCursorStillDown(localCursor);

                    if (!(*it)->IsInputTransparent())
                        broken = true;

                    ++it;
                }
                else
                {
                    (*it)->OnCursorPressBreak(*o2Input.GetCursor(localCursor.id));
                    (*it)->mIsPressed = false;

                    it = mPressedListeners[localCursor.id].Remove(it);
                }
            }
        }

        if (localCursor.delta.Length() > FLT_EPSILON)
        {
            if (mUnderCursorListeners.ContainsKey(localCursor.id))
            {
                for (auto& listener : mUnderCursorListeners[localCursor.id])
                    listener->OnCursorMoved(localCursor);
            }
        }
    }

    void CursorAreaEventListenersLayer::ProcessCursorReleased(const Input::Cursor& cursor)
    {
        auto localCursor = ConvertLocalCursor(cursor);

        for (auto& listenerWeak : cursorEventAreaListeners)
        {
            auto listener = listenerWeak.Lock();
            if (listener && !listener->IsUnderPoint(localCursor.position))
                listener->OnCursorReleasedOutside(localCursor);
        }

        if (mPressedListeners.ContainsKey(localCursor.id))
        {
            for (auto& listener : mPressedListeners[localCursor.id])
            {
                listener->mIsPressed = false;
                listener->OnCursorReleased(localCursor);
            }

            mPressedListeners.Remove(localCursor.id);
        }
    }

    void CursorAreaEventListenersLayer::ProcessRBPressed()
    {
		auto localCursor = ConvertLocalCursor(*o2Input.GetCursor(0));

		mRightButtonPressedListeners.Clear();

        if (!mUnderCursorListeners.ContainsKey(localCursor.id))
            return;

        auto listeners = mUnderCursorListeners[localCursor.id];
        for (auto& listener : listeners)
        {
            mRightButtonPressedListeners.Add(listener);

            listener->OnCursorRightMousePressed(localCursor);
            listener->mIsRightMousePressed = true;

            if (!listener->IsInputTransparent())
                break;
        }
    }

    void CursorAreaEventListenersLayer::ProcessRBDown()
    {
        auto localCursor = ConvertLocalCursor(*o2Input.GetCursor(0));

        bool broken = false;
        for (auto it = mRightButtonPressedListeners.begin(); it != mRightButtonPressedListeners.end();)
        {
            if (!broken)
            {
                (*it)->OnCursorRightMouseStayDown(localCursor);

                if (!(*it)->IsInputTransparent())
                    broken = true;

                ++it;
            }
            else
            {
                (*it)->OnCursorPressBreak(*o2Input.GetCursor(localCursor.id));
                (*it)->mIsPressed = false;

                it = mRightButtonPressedListeners.Remove(it);
            }
        }
    }

    void CursorAreaEventListenersLayer::ProcessRBReleased()
    {
        auto localCursor = ConvertLocalCursor(*o2Input.GetCursor(0));

        for (auto& listener : mRightButtonPressedListeners)
        {
            listener->OnCursorRightMouseReleased(localCursor);
            listener->mIsRightMousePressed = false;
        }
    }

    void CursorAreaEventListenersLayer::ProcessMBPressed()
    {
		auto localCursor = ConvertLocalCursor(*o2Input.GetCursor(0));

		mMiddleButtonPressedListeners.Clear();

        if (!mUnderCursorListeners.ContainsKey(localCursor.id))
            return;

        auto listeners = mUnderCursorListeners[localCursor.id];
        for (auto& listener : listeners)
        {
            mMiddleButtonPressedListeners.Add(listener);

            listener->OnCursorMiddleMousePressed(localCursor);
            listener->mIsMiddleMousePressed = true;

            if (!listener->IsInputTransparent())
                break;
        }
    }

    void CursorAreaEventListenersLayer::ProcessMBDown()
    {
        auto localCursor = ConvertLocalCursor(*o2Input.GetCursor(0));

        bool broken = false;
        for (auto it = mMiddleButtonPressedListeners.begin(); it != mMiddleButtonPressedListeners.end();)
        {
            if (!broken)
            {
                (*it)->OnCursorMiddleMouseStayDown(localCursor);

                if (!(*it)->IsInputTransparent())
                    broken = true;

                ++it;
            }
            else
            {
                (*it)->OnCursorPressBreak(*o2Input.GetCursor(localCursor.id));
                (*it)->mIsMiddleMousePressed = false;

                it = mMiddleButtonPressedListeners.Remove(it);
            }
        }
    }

    void CursorAreaEventListenersLayer::ProcessMBReleased()
    {
        auto localCursor = ConvertLocalCursor(*o2Input.GetCursor(0));

        for (auto& listener : mMiddleButtonPressedListeners)
        {
            listener->OnCursorRightMouseReleased(localCursor);
            listener->mIsMiddleMousePressed = false;
        }
    }

    void CursorAreaEventListenersLayer::ProcessScrolling()
    {
        float scroll = o2Input.GetMouseWheelDelta();
        if (!Math::Equals(scroll, 0.0f))
        {
            for (auto& kv : mUnderCursorListeners)
            {
                for (auto& listener : kv.second)
                {
                    if (!listener->IsScrollable())
                        continue;

                    listener->OnScrolled(scroll);

                    if (!listener->IsInputTransparent())
                        break;
                }
            }
        }
    }
}
