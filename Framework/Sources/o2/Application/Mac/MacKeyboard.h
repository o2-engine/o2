#pragma once

#ifdef PLATFORM_MAC

#include "o2/Application/VKCodes.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // Maps hardware (layout-independent) Mac key code to engine keyboard key
    KeyboardKey MacHardwareKeyToKeyboardKey(int hardwareKeyCode);

    // Maps engine keyboard key back to hardware Mac key code. Returns -1 when unknown
    int KeyboardKeyToMacHardwareKey(KeyboardKey key);

    // Returns unicode character produced by key in the current keyboard layout, 0 when key doesn't type text
    UInt16 GetUnicodeFromMacKey(KeyboardKey key);

    // ----------------------------------------------------------------------------------------
    // Converts raw Mac keyboard events into engine key press/release events. Tracks pressed
    // keys and modifiers to protect from stuck keys: missed modifier transitions are healed on
    // every event, keys pressed with Cmd are released with it (macOS suppresses their keyUp)
    // ----------------------------------------------------------------------------------------
    class MacKeyboardHandler
    {
    public:
        // ---------------------
        // Modifier keys state
        // ---------------------
        struct Modifiers
        {
            bool shift = false;   // Shift key state
            bool alt = false;     // Option key state
            bool control = false; // Control key state
            bool command = false; // Command key state
        };

        // ------------------------------
        // Key press or release to emit
        // ------------------------------
        struct KeyEvent
        {
            KeyboardKey key;     // Engine keyboard key
            bool        pressed; // True - press, false - release

            // Equals operator
            bool operator==(const KeyEvent& other) const;
        };

    public:
        // Called on keyDown event, returns events to pass into input
        Vector<KeyEvent> OnKeyDown(int hardwareKeyCode, const Modifiers& modifiers);

        // Called on keyUp event, returns events to pass into input
        Vector<KeyEvent> OnKeyUp(int hardwareKeyCode, const Modifiers& modifiers);

        // Called on flagsChanged event, returns events to pass into input
        Vector<KeyEvent> OnModifiersChanged(const Modifiers& modifiers);

        // Called when window loses focus, releases all keys and modifiers
        Vector<KeyEvent> OnFocusLost();

    protected:
        Vector<KeyboardKey> mPressedKeys;        // Keys currently held by keyDown events
        Vector<KeyboardKey> mPressedWithCommand; // Held keys pressed while Cmd was down
        Modifiers           mModifiers;          // Last known modifiers state

    protected:
        // Emits press/release for changed modifiers; on Cmd release also releases keys pressed with it
        void SyncModifiers(const Modifiers& modifiers, Vector<KeyEvent>& out);
    };
}

#endif // PLATFORM_MAC
