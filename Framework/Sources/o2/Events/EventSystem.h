#pragma once

#include "o2/Application/Input.h"
#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

// Events system accessor macros
#define o2Events o2::EventSystem::Instance()

namespace o2
{
    FORWARD_CLASS_REF(ApplicationEventsListener);
    FORWARD_CLASS_REF(CursorAreaEventsListener);
    FORWARD_CLASS_REF(CursorEventsListener);
    FORWARD_CLASS_REF(DragableObject); 
    FORWARD_CLASS_REF(KeyboardEventsListener);
    FORWARD_CLASS_REF(ShortcutKeysListenersManager);

    // -----------------------
    // Event processing system
    // -----------------------
    class EventSystem: public Singleton<EventSystem>
    {
    public:
        static bool eventsListenersEnabledByDefault; // Then it is true, new events listeners will be enabled on initialization

    public:
        // Default constructor
        EventSystem(RefCounter* refCounter);

        // Destructor
        ~EventSystem();

        // Returns all cursor listeners under cursor arranged by depth
        Vector<Ref<CursorAreaEventsListener>> GetAllCursorListenersUnderCursor(CursorId cursorId) const;

		// Returns current cursor area events listeners layer at time
		Ref<CursorAreaEventListenersLayer> GetCurrentCursorAreaEventsLayer() const;

        // Breaks cursor event. All pressed listeners will be unpressed with specific event OnPressBreak
        void BreakCursorEvent();

        // Updates and processes events
        void Update();

        // Post update events
        void PostUpdate();

    protected:
        // Returns time between clicks for double click reaction
        float GetDoubleClickTime() const;

        // Called when application was started
        void OnApplicationStarted();

        // Called when application closing
        void OnApplicationClosing();

        // Called when application activated
        void OnApplicationActivated();

        // Called when application deactivated
        void OnApplicationDeactivated();

        // Called when application frame was sized
        void OnApplicationSized();

        // Processes key pressed event
        void ProcessKeyPressed(const Input::Key& key);

        // Processes key down event
        void ProcessKeyDown(const Input::Key& key);

        // Processes key released event
        void ProcessKeyReleased(const Input::Key& key);

    protected:
        float mDblClickTime = 0.3f; // Time between clicks for double click reaction

        Vector<WeakRef<CursorEventsListener>> mCursorListeners; // All cursor non area listeners

        Vector<WeakRef<CursorAreaEventListenersLayer>> mCursorAreaEventsListenersLayers; // Drawn cursor area events listeners layers
        
        Ref<CursorAreaEventListenersLayer>         mCursorAreaListenersBasicLayer; // Basic cursor area events listeners layer, for main screen
        Ref<CursorAreaEventListenersLayer>         mCurrentCursorAreaEventsLayer;  // Current list of area listeners
        Vector<Ref<CursorAreaEventListenersLayer>> mLayersStack;                   // Input layers stack, in order they are pushed

        Vector<WeakRef<KeyboardEventsListener>>    mKeyboardListeners;    // Keyboard events listeners
        Vector<WeakRef<ApplicationEventsListener>> mApplicationListeners; // Application events listeners

        Ref<ShortcutKeysListenersManager> mShortcutEventsManager; // Shortcut events manager

    protected:
        // Push current cursor area events listeners layer in layers stack
        static void PushCursorAreaEventsListenersLayer(const Ref<CursorAreaEventListenersLayer>& layer);

        // Pops current cursor area events listeners layer
        static void PopCursorAreaEventsListenersLayer();

        // Unregisters layer
        static void RemoveCursorAreaEventsListenersLayer(const WeakRef<CursorAreaEventListenersLayer>& layer);

        // Registering cursor area events listener
        static void DrawnCursorAreaListener(const Ref<CursorAreaEventsListener>& listener);

        // Unregistering cursor area events listener
        static void UnregCursorAreaListener(CursorAreaEventsListener* listener);

        // Registering cursor events listener
        static void RegCursorListener(const Ref<CursorEventsListener>& listener);

        // Unregistering cursor events listener
        static void UnregCursorListener(const WeakRef<CursorEventsListener>& listener);

        // Registering drag events listener
        static void RegDragListener(const Ref<DragableObject>& listener);

        // Unregistering drag events listener
        static void UnregDragListener(DragableObject* listener);

        // Registering keyboard events listener
        static void RegKeyboardListener(const Ref<KeyboardEventsListener>& listener);

        // Unregistering keyboard events listener
        static void UnregKeyboardListener(KeyboardEventsListener* listener);

        // Registering application events listener
        static void RegApplicationListener(const Ref<ApplicationEventsListener>& listener);

        // Unregistering application events listener
        static void UnregApplicationListener(ApplicationEventsListener* listener);

        friend class Application;
        friend class ApplicationEventsListener;
        friend class CursorAreaEventListenersLayer;
        friend class CursorAreaEventsListener;
        friend class CursorEventsListener;
        friend class DragableObject;
        friend class Integration;
        friend class KeyboardEventsListener;
        friend class WndProcFunc;
    };
}
