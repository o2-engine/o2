#pragma once

#include "o2/Events/IEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Utils/System/ShortcutKeys.h"

namespace o2
{
    // ----------------------------------------------------------------------------------------
    // Shortcut keys listener. Calls event when hit shortcut and this listener has max priority
    // ----------------------------------------------------------------------------------------
    class ShortcutKeysListener: public IEventsListener
    {
    public:
        // Virtual destructor. Unregistering from manager
        virtual ~ShortcutKeysListener();

        // Sets maximum priority for this listener
        void SetMaxPriority();

        // Sets minimal priority
        void SetMinPriority();

        // Sets shortcut keys and registering in manager
        void SetShortcut(const ShortcutKeys& shortcut);

        // Returns shortcut keys
        const ShortcutKeys& GetShortcut() const;

        // Sets enable. Not enabled listeners doesn't react on shortcut
        void SetEnabled(bool enabled);

        // Returns is enabled. Not enabled listeners doesn't react on shortcut
        bool IsEnabled() const; 

    private:
        ShortcutKeys mShortcut;
        bool         mEnabled = true;

    protected:
        // This event calling when shortcut hit and this listener has max priority
        virtual void OnShortcutPressed() {}

        friend class ShortcutKeysListenersManager;
    };

	// -----------------------------------------------------------------------------------------------
	// Functional shortcut keys listener. Contains onShortcutPressed event, calls it when shortcut hit
	// -----------------------------------------------------------------------------------------------
    class FunctionalShortcutKeysListener : public RefCounterable, public ShortcutKeysListener
    {
    public:
		Function<void()> onShortcutPressed; 

    public:
		// Default constructor
		FunctionalShortcutKeysListener() = default;

		// Constructor with onShortcutPressed event
		FunctionalShortcutKeysListener(const Function<void()>& onShortcutPressed);

		// Returns reference counter
		RefCounter* GetRefCounter() const override;

	protected:
		// This event calling when shortcut hit and this listener has max priority, calls onShortcutPressed event
		void OnShortcutPressed() override;
    };

	// -------------------------------------------------------------------------------------
	// Shortcut keys listeners manager. Sends events to listeners when shortcut keys pressed
	// -------------------------------------------------------------------------------------
    class ShortcutKeysListenersManager: public Singleton<ShortcutKeysListenersManager>, public KeyboardEventsListener
    {
    public:
        // Default constructor
        explicit ShortcutKeysListenersManager(RefCounter* refCounter);

        // Enables or disables dispatching shortcuts to all listeners, e.g. while a modal navigation owns the keyboard
        static void SetSuppressed(bool suppressed);

        // Returns is shortcuts dispatching suppressed
        static bool IsSuppressed();

        // Returns whether the shortcut may fire while a text input is focused: bare printable
        // keys are typing, only combos with Ctrl/Cmd/Alt/Win or function keys stay live
        static bool IsAllowedDuringTextInput(const ShortcutKeys& shortcut);

    protected:
        Map<ShortcutKeys, Vector<WeakRef<ShortcutKeysListener>>> mListeners;

        bool mSuppressed = false; // When true shortcuts are not dispatched to listeners

    protected:
        // Registers listener 
        static void Register(const ShortcutKeys& shortcut, const Ref<ShortcutKeysListener>& listener);

        // Unregisters listener
        static void UnRegister(const ShortcutKeys& shortcut, ShortcutKeysListener* listener);

        // Set listener minimal priority
        static void SetMinPriority(const ShortcutKeys& shortcut, const Ref<ShortcutKeysListener>& listener);

        // Set listener maximal priority
        static void SetMaxPriority(const ShortcutKeys& shortcut, const Ref<ShortcutKeysListener>& listener);

        // Called when key was pressed, send event to most priority listener
        void OnKeyPressed(const Input::Key& key) override;

        REF_COUNTERABLE_IMPL(Singleton<ShortcutKeysListenersManager>);

        friend class ShortcutKeysListener;
    };
}
