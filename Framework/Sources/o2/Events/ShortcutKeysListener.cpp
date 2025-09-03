#include "o2/stdafx.h"
#include "ShortcutKeysListener.h"

namespace o2
{
    DECLARE_SINGLETON(ShortcutKeysListenersManager);

    ShortcutKeysListener::~ShortcutKeysListener()
    {
        ShortcutKeysListenersManager::UnRegister(mShortcut, this);
    }

    void ShortcutKeysListener::SetMaxPriority()
    {
        ShortcutKeysListenersManager::SetMaxPriority(mShortcut, Ref(this));
    }

    void ShortcutKeysListener::SetMinPriority()
    {
        ShortcutKeysListenersManager::SetMinPriority(mShortcut, Ref(this));
    }

    void ShortcutKeysListener::SetShortcut(const ShortcutKeys& shortcut)
    {
        ShortcutKeysListenersManager::UnRegister(mShortcut, this);
        mShortcut = shortcut;
        ShortcutKeysListenersManager::Register(mShortcut, Ref(this));
    }

    const ShortcutKeys& ShortcutKeysListener::GetShortcut() const
    {
        return mShortcut;
    }

    void ShortcutKeysListener::SetEnabled(bool enabled)
    {
        if (enabled == mEnabled)
            return;

        mEnabled = enabled;

        if (mEnabled)
            ShortcutKeysListenersManager::Register(mShortcut, Ref(this));
        else
            ShortcutKeysListenersManager::UnRegister(mShortcut, this);
    }

    bool ShortcutKeysListener::IsEnabled() const
    {
        return mEnabled;
	}

	FunctionalShortcutKeysListener::FunctionalShortcutKeysListener(const Function<void()>& onShortcutPressed):
		onShortcutPressed(onShortcutPressed)
	{}

	void FunctionalShortcutKeysListener::OnShortcutPressed()
	{
        onShortcutPressed();
	}

	RefCounter* FunctionalShortcutKeysListener::GetRefCounter() const
	{
		return RefCounterable::GetRefCounter();
	}

	ShortcutKeysListenersManager::ShortcutKeysListenersManager(RefCounter* refCounter) :
        Singleton<ShortcutKeysListenersManager>(refCounter)
    {}

    void ShortcutKeysListenersManager::Register(const ShortcutKeys& shortcut, const Ref<ShortcutKeysListener>& listener)
    {
        if (!mInstance)
            return;

        if (shortcut.IsEmpty())
            return;

        auto& listeners = mInstance->mListeners;

        if (!listeners.ContainsKey(shortcut))
            listeners.Add(shortcut, {});

        listeners[shortcut].Insert(listener, 0);
    }

    void ShortcutKeysListenersManager::UnRegister(const ShortcutKeys& shortcut, ShortcutKeysListener* listener)
    {
        if (!mInstance)
            return;

        auto& listeners = mInstance->mListeners;

        if (!listeners.ContainsKey(shortcut))
            return;

        listeners[shortcut].RemoveFirst([&](auto& x) { return x == listener; });

        if (listeners[shortcut].IsEmpty())
            listeners.Remove(shortcut);
    }

    void ShortcutKeysListenersManager::SetMinPriority(const ShortcutKeys& shortcut, const Ref<ShortcutKeysListener>& listener)
    {
        if (!mInstance)
            return;

        auto& listeners = mInstance->mListeners;

        if (!listeners.ContainsKey(shortcut))
            return;

        listeners[shortcut].Remove(listener);
        listeners[shortcut].Insert(listener, 0);
    }

    void ShortcutKeysListenersManager::SetMaxPriority(const ShortcutKeys& shortcut, const Ref<ShortcutKeysListener>& listener)
    {
        if (!mInstance)
            return;

        auto& listeners = mInstance->mListeners;

        if (!listeners.ContainsKey(shortcut))
            return;

        listeners[shortcut].Remove(listener);
        listeners[shortcut].Add(listener);
    }

    void ShortcutKeysListenersManager::OnKeyPressed(const Input::Key& key)
    {
        //o2Debug.Log("ShortcutKeysListenersManager::OnKeyPressed: " + String(key.keyCode));

        for (auto& kv : mListeners)
        {
            if (kv.first.IsPressed() && !kv.second.IsEmpty())
            {
                for (int i = kv.second.Count() - 1; i >= 0; i--)
                {
                    auto listener = kv.second[i].Lock();
                    if (listener->IsListeningEvents())
                    {
                        listener->OnShortcutPressed();
                        break;
                    }
                }
            }
        }
    }

}
