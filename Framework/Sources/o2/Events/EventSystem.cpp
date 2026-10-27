#include "o2/stdafx.h"
#include "EventSystem.h"

#include "o2/Events/ApplicationEventsListener.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Events/ShortcutKeysListener.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Editor/DragAndDrop.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
#undef DrawText

    DECLARE_SINGLETON(EventSystem);

    FORWARD_REF_IMPL(ApplicationEventsListener);
    FORWARD_REF_IMPL(CursorAreaEventsListener);
    FORWARD_REF_IMPL(CursorEventsListener);
    FORWARD_REF_IMPL(DragableObject);
    FORWARD_REF_IMPL(KeyboardEventsListener);
    FORWARD_REF_IMPL(ShortcutKeysListenersManager);

    EventSystem::EventSystem(RefCounter* refCounter):
        Singleton<EventSystem>(refCounter)
    {
        PushEditorScopeOnStack scope;

        mCursorAreaListenersBasicLayer = mmake<CursorAreaEventListenersLayer>();
        mShortcutEventsManager = mmake<ShortcutKeysListenersManager>();

        mCurrentCursorAreaEventsLayer = mCursorAreaListenersBasicLayer;
        mCursorAreaListenersBasicLayer->mEnabled = true;
    }

    EventSystem::~EventSystem()
    {
        mInstance = nullptr;
    }

    float EventSystem::GetDoubleClickTime() const
    {
        return mDblClickTime;
    }

    void EventSystem::Update()
    {
        for (auto& layer : mCursorAreaEventsListenersLayers)
        {
            if (auto layerPtr = layer.Lock())
            {
                mCurrentCursorAreaEventsLayer = layerPtr;
                layerPtr->Update();
            }
        }

        for (const Input::Cursor& cursor : o2Input.GetCursors())
        {
            if (cursor.pressedTime < FLT_EPSILON && cursor.isPressed)
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorPressed(cursor);
                }
            }
            else
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorStillDown(cursor);
                }
            }
        }

        for (const Input::Cursor& cursor : o2Input.GetReleasedCursors())
        {
            for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
            {
                if (auto locked = listener.Lock())
                    locked->OnCursorReleased(cursor);
            }
        }

        const Input::Cursor& cursor = *o2Input.GetCursor(0);

        for (const Input::Key& key : o2Input.GetPressedKeys())
        {
            if (key.keyCode == -1)
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorRightMousePressed(cursor);
                }
            }
            else if (key.keyCode == -2)
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorMiddleMousePressed(cursor);
                }
            }
            else
            {
                ProcessKeyPressed(key);
            }
        }

        for (const Input::Key& key : o2Input.GetDownKeys())
        {
            if (key.keyCode == -1)
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorRightMouseStillDown(cursor);
                }
            }
            else if (key.keyCode == -2)
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorMiddleMouseStillDown(cursor);
                }
            }
            else
                ProcessKeyDown(key);
        }

        for (const Input::Key& key : o2Input.GetReleasedKeys())
        {
            if (key.keyCode == -1)
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorRightMouseReleased(cursor);
                }
            }
            else if (key.keyCode == -2)
            {
                for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
                {
                    if (auto locked = listener.Lock())
                        locked->OnCursorMiddleMouseReleased(cursor);
                }
            }
            else
                ProcessKeyReleased(key);
        }
        
        float scroll = o2Input.GetMouseWheelDelta();
        if (!Math::Equals(scroll, 0.0f))
        {
            for (auto& listener : Vector<WeakRef<CursorEventsListener>>(mCursorListeners))
            {
                if (auto locked = listener.Lock())
                    locked->OnScrolled(scroll);
            }
        }

        if (o2Input.IsKeyDown(VK_F1))
        {
            int line = 0;
            auto allUnderCursor = o2Input.IsKeyDown(VK_CONTROL) ? 
                GetAllCursorListenersUnderCursor(0) : 
                mCursorAreaListenersBasicLayer->mUnderCursorListeners[0];

            for (auto& listener : allUnderCursor)
            {
                String name = listener->GetDebugName();

                if (auto widget = DynamicCast<Widget>(listener))
                {
                    String path;
                    auto parent = widget;
                    while (parent)
                    {
                        path = parent->GetName() + "/" + path;
                        parent = parent->GetParentWidget().Lock();
                    }

                    name = path + " : " + widget->GetType().GetName();
                    o2Debug.DrawRect(widget->layout->GetWorldRect(), Color4::Red());
                }
                else if (auto handle = DynamicCast<DragHandle>(listener))
                {
                    if (auto drawable = handle->GetRegularDrawable())
                        o2Debug.DrawRect(drawable->GetAxisAlignedRect(), Color4::Red());
                }

                o2Debug.DrawText(Vec2F(-o2Render.GetResolution().x*0.5f, o2Render.GetResolution().y*0.5f - (float)line), name);

                line += 20;
            }
        }

        mCurrentCursorAreaEventsLayer = mCursorAreaListenersBasicLayer;
    }

    void EventSystem::PostUpdate()
    {
        auto layers = mCursorAreaEventsListenersLayers;
        for (auto& layer : layers)
        {
            if (auto locked = layer.Lock())
                locked->PostUpdate();
        }

        mCursorAreaEventsListenersLayers.Clear();
        mCursorAreaEventsListenersLayers.Add(mCursorAreaListenersBasicLayer);
    }

    void EventSystem::OnApplicationStarted()
    {
        auto listeners = mApplicationListeners;
        for (auto& listener : listeners)
        {
            auto locked = listener.Lock();
            if (locked && locked->IsListeningEvents())
                locked->OnApplicationStarted();
        }
    }

    void EventSystem::OnApplicationClosing()
    {
        auto listeners = mApplicationListeners;
        for (auto& listener : listeners)
        {
            auto locked = listener.Lock();
            if (locked && locked->IsListeningEvents())
                locked->OnApplicationClosing();
        }
    }

    void EventSystem::OnApplicationActivated()
    {
        auto listeners = mApplicationListeners;
        for (auto& listener : listeners)
        {
            auto locked = listener.Lock();
            if (locked && locked->IsListeningEvents())
                locked->OnApplicationActivated();
        }
    }

    void EventSystem::OnApplicationDeactivated()
    {
        auto listeners = mApplicationListeners;
        for (auto& listener : listeners)
        {
            auto locked = listener.Lock();
            if (locked && locked->IsListeningEvents())
                locked->OnApplicationDeactivated();
        }
    }

    void EventSystem::OnApplicationSized()
    {
        auto listeners = mApplicationListeners;
        for (auto& listener : listeners)
        {
            auto locked = listener.Lock();
            if (locked && locked->IsListeningEvents())
                locked->OnApplicationSized();
        }
    }

    bool EventSystem::eventsListenersEnabledByDefault = true;

    Vector<Ref<CursorAreaEventsListener>> EventSystem::GetAllCursorListenersUnderCursor(CursorId cursorId) const
    {
        return mCursorAreaListenersBasicLayer->GetAllCursorListenersUnderCursor(o2Input.GetCursorPos(cursorId));
    }

	Ref<CursorAreaEventListenersLayer> EventSystem::GetCurrentCursorAreaEventsLayer() const
	{
		return mCurrentCursorAreaEventsLayer;
	}

	void EventSystem::BreakCursorEvent()
    {
        auto layers = mCursorAreaEventsListenersLayers;
        for (auto& layer : layers)
        {
            if (auto locked = layer.Lock())
                locked->BreakCursorEvent();
        }
    }

    void EventSystem::ProcessKeyPressed(const Input::Key& key)
    {
        auto listeners = mKeyboardListeners;
        for (auto& listener : listeners)
        {
            // A listener the ones before it destroyed is gone
            auto locked = listener.Lock();
            if (locked && locked->mEnabledListeningEvents)
                locked->OnKeyPressed(key);
        }
    }

    void EventSystem::ProcessKeyDown(const Input::Key& key)
    {
        auto listeners = mKeyboardListeners;
        for (auto& listener : listeners)
        {
            // A listener the ones before it destroyed is gone
            auto locked = listener.Lock();
            if (locked && locked->mEnabledListeningEvents)
                locked->OnKeyStayDown(key);
        }
    }

    void EventSystem::ProcessKeyReleased(const Input::Key& key)
    {
        auto listeners = mKeyboardListeners;
        for (auto& listener : listeners)
        {
            // A listener the ones before it destroyed is gone
            auto locked = listener.Lock();
            if (locked && locked->mEnabledListeningEvents)
                locked->OnKeyReleased(key);
        }
    }

    void EventSystem::PushCursorAreaEventsListenersLayer(const Ref<CursorAreaEventListenersLayer>& layer)
    {
        if (layer)
        {
            layer->mParentLayer = mInstance->mCurrentCursorAreaEventsLayer;

            mInstance->mCurrentCursorAreaEventsLayer = layer;
            mInstance->mCursorAreaEventsListenersLayers.Add(layer);
            mInstance->mLayersStack.Add(layer);
        }
    }

    void EventSystem::PopCursorAreaEventsListenersLayer()
    {
        mInstance->mLayersStack.PopBack();
        if (!mInstance->mLayersStack.IsEmpty())
            mInstance->mCurrentCursorAreaEventsLayer = mInstance->mLayersStack.Last();
        else
            mInstance->mCurrentCursorAreaEventsLayer = mInstance->mCursorAreaListenersBasicLayer;
    }

    void EventSystem::RemoveCursorAreaEventsListenersLayer(const WeakRef<CursorAreaEventListenersLayer>& layer)
    {
        mInstance->mCursorAreaEventsListenersLayers.Remove(layer);
    }

    void EventSystem::DrawnCursorAreaListener(const Ref<CursorAreaEventsListener>& listener)
    {
        if (!IsSingletonInitialzed())
            return;

        if (!listener->IsListeningEvents())
            return;

        mInstance->mCurrentCursorAreaEventsLayer->cursorEventAreaListeners.Add(listener);
    }

    void EventSystem::UnregCursorAreaListener(CursorAreaEventsListener* listener)
    {
        // A layer being destroyed is expired already, the listener's own one included
        auto listenerLayer = dynamic_cast<CursorAreaEventListenersLayer*>(listener);
        for (auto& layer : mInstance->mCursorAreaEventsListenersLayers)
        {
            auto locked = layer.Lock();
            if (locked && locked.Get() != listenerLayer)
                locked->UnregCursorAreaListener(listener);
        }
    }

    void EventSystem::RegCursorListener(const Ref<CursorEventsListener>& listener)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance)
            mInstance->mCursorListeners.Add(listener);
    }

    void EventSystem::UnregCursorListener(const WeakRef<CursorEventsListener>& listener)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance)
            mInstance->mCursorListeners.Remove(listener);
    }

    void EventSystem::RegDragListener(const Ref<DragableObject>& listener)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance)
            mInstance->mCurrentCursorAreaEventsLayer->mDragListeners.Add(Ref(listener));
    }

    void EventSystem::UnregDragListener(DragableObject* listener)
    {
        if (mInstance)
        {
            for (auto& layer : mInstance->mCursorAreaEventsListenersLayers)
            {
                if (auto locked = layer.Lock())
                    locked->UnregDragListener(listener);
            }
        }
    }

    void EventSystem::RegKeyboardListener(const Ref<KeyboardEventsListener>& listener)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance)
            mInstance->mKeyboardListeners.Add(Ref(listener));
    }

    void EventSystem::UnregKeyboardListener(KeyboardEventsListener* listener)
    {
        if (mInstance)
            mInstance->mKeyboardListeners.RemoveFirst([&](auto& x) { return x == listener; });
    }

    void EventSystem::RegApplicationListener(const Ref<ApplicationEventsListener>& listener)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance)
            mInstance->mApplicationListeners.Add(listener);
    }

    void EventSystem::UnregApplicationListener(ApplicationEventsListener* listener)
    {
        if (mInstance)
            mInstance->mApplicationListeners.RemoveFirst([&](auto& x) { return x == listener; });
    }
}
