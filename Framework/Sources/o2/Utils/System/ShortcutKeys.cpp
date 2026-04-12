#include "o2/stdafx.h"
#include "ShortcutKeys.h"

#include "o2/Application/Input.h"
#include "o2/Application/VKCodes.h"

#if defined PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace o2
{
    const std::unordered_map<KeyboardKey, String> ShortcutKeys::mKeyNames = {
        // Function keys
        { VK_F1, "F1" }, { VK_F2, "F2" }, { VK_F3, "F3" }, { VK_F4, "F4" },
        { VK_F5, "F5" }, { VK_F6, "F6" }, { VK_F7, "F7" }, { VK_F8, "F8" },
        { VK_F9, "F9" }, { VK_F10, "F10" }, { VK_F11, "F11" }, { VK_F12, "F12" },
        
        // Number keys
        { VK_0, "0" }, { VK_1, "1" }, { VK_2, "2" }, { VK_3, "3" }, { VK_4, "4" },
        { VK_5, "5" }, { VK_6, "6" }, { VK_7, "7" }, { VK_8, "8" }, { VK_9, "9" },
        
        // Navigation keys
        { VK_LEFT, "Left" }, { VK_UP, "Up" }, { VK_RIGHT, "Right" }, { VK_DOWN, "Down" },
        { VK_HOME, "Home" }, { VK_END, "End" }, { VK_PRIOR, "PageUp" }, { VK_NEXT, "PageDown" },
        { VK_DELETE, "Delete" },
        
        // Special keys
        { VK_ESCAPE, "Escape" }, { VK_RETURN, "Enter" }, { VK_SPACE, "Space" },
        { VK_BACK, "Backspace" }, { VK_TAB, "Tab" },
        { VK_NUMLOCK, "NumLock" },
        
        // Numpad keys
        { VK_NUMPAD0, "Numpad0" }, { VK_NUMPAD1, "Numpad1" }, { VK_NUMPAD2, "Numpad2" },
        { VK_NUMPAD3, "Numpad3" }, { VK_NUMPAD4, "Numpad4" }, { VK_NUMPAD5, "Numpad5" },
        { VK_NUMPAD6, "Numpad6" }, { VK_NUMPAD7, "Numpad7" }, { VK_NUMPAD8, "Numpad8" },
        { VK_NUMPAD9, "Numpad9" },
        
        // Numpad operators
        { VK_MULTIPLY, "Numpad*" }, { VK_ADD, "Numpad+" }, { VK_SUBTRACT, "Numpad-" },
        { VK_DECIMAL, "Numpad." }, { VK_DIVIDE, "Numpad/" },
        
        // OEM keys
        { VK_OEM_3, "`" }, { VK_OEM_MINUS, "-" }, { VK_OEM_PLUS, "=" },
        { VK_OEM_4, "[" }, { VK_OEM_6, "]" }, { VK_OEM_5, "\\" },
        { VK_OEM_1, ";" }, { VK_OEM_7, "'" },
        { VK_OEM_COMMA, "," }, { VK_OEM_PERIOD, "." }, { VK_OEM_2, "/" },
        
#if defined PLATFORM_WINDOWS || defined PLATFORM_LINUX
        { VK_INSERT, "Insert" },
        { VK_CAPITAL, "CapsLock" },
        { VK_PAUSE, "Pause" }, 
        { VK_SCROLL, "ScrollLock" },
        { VK_LWIN, "LeftWin" }, 
        { VK_RWIN, "RightWin" },
#endif
        
#if defined PLATFORM_MAC || defined PLATFORM_IOS
        { VK_COMMAND, "Cmd" },
        { VK_SELECT, "Select" },
        { VK_SNAPSHOT, "Snapshot" },
#endif
    };

    ShortcutKeys::ShortcutKeys()
    {}

    ShortcutKeys::ShortcutKeys(const Vector<KeyboardKey>& keys) :
        keys(keys)
    {}

    ShortcutKeys ShortcutKeys::FromCustomString(const String& str)
    {
        ShortcutKeys res;
        res.custromString = str;
        return res;
    }

    KeyboardKey ShortcutKeys::NormalizeKey(KeyboardKey key) const
    {
#if defined PLATFORM_MAC || defined PLATFORM_IOS
        if (key >= 'A' && key <= 'Z') {
            return key + ('a' - 'A');
        }
#endif
        return key;
    }

    bool ShortcutKeys::IsPressed() const
    {
        if (keys.IsEmpty())
            return false;

        for (auto key : keys) {
            auto normalizedKey = NormalizeKey(key);
            bool isPressed = o2Input.IsKeyPressed(normalizedKey) || o2Input.IsKeyPressed(key) || 
                             o2Input.IsKeyDown(normalizedKey) || o2Input.IsKeyDown(key);
                             
            if (!isPressed)
                return false;
        }

        return true;
    }

    bool ShortcutKeys::IsDown() const
    {
        if (keys.IsEmpty())
            return false;

        for (auto key : keys) {
            if (!o2Input.IsKeyDown(NormalizeKey(key)) && !o2Input.IsKeyDown(key))
                return false;
        }

        return true;
    }

    String ShortcutKeys::AsString() const
    {
        if (!custromString.IsEmpty())
            return custromString;

        if (keys.IsEmpty())
            return "";

        String mainKey;
        Vector<String> modifiers;
        
        for (auto key : keys) {
            if (key == VK_SHIFT) {
                modifiers.Add("Shift");
            }
            else if (key == VK_CONTROL) {
                modifiers.Add("Ctrl");
            }
            else if (key == VK_CTRL_CMD) {
#if defined PLATFORM_MAC || defined PLATFORM_IOS
                modifiers.Add("Cmd");
#else
                modifiers.Add("Ctrl");
#endif
            }
            else if (key == VK_MENU) {
                modifiers.Add("Alt");
            }
#if defined PLATFORM_MAC || defined PLATFORM_IOS
            else if (key == VK_COMMAND) {
                modifiers.Add("Cmd");
            }
#endif
            else {
                char c = (char)key;
                if ((c >= 'A' && c <= 'Z')) {
                    mainKey = String(c);
                }
                else if ((c >= 'a' && c <= 'z')) {
                    char capital = c - 'a' + 'A';
                    mainKey = String(capital);
                }
                else {
                    auto it = mKeyNames.find(key);
                    if (it != mKeyNames.end()) {
                        mainKey = it->second;
                    }
                    else {
                        mainKey = String((int)key);
                    }
                }
            }
        }

        String result;
        for (const String& modifier : modifiers) {
            if (!result.IsEmpty())
                result += "+";
            result += modifier;
        }
        
        if (!modifiers.IsEmpty() && !mainKey.IsEmpty())
            result += "+";

        result += mainKey;
        
        return result;
    }

    bool ShortcutKeys::IsEmpty() const
    {
        return keys.IsEmpty();
    }

    bool ShortcutKeys::operator<(const ShortcutKeys& other) const
    {
        if (keys.Count() != other.keys.Count())
            return keys.Count() < other.keys.Count();

        for (int i = 0; i < keys.Count(); i++) {
            if (keys[i] != other.keys[i])
                return keys[i] < other.keys[i];
        }

        return false;
    }

    bool ShortcutKeys::operator==(const ShortcutKeys& other) const
    {
        if (keys.Count() != other.keys.Count())
            return false;

        // Sort both arrays for comparison
        Vector<KeyboardKey> sortedKeys1 = keys;
        Vector<KeyboardKey> sortedKeys2 = other.keys;
        
        sortedKeys1.Sort([](const KeyboardKey& a, const KeyboardKey& b) { return a < b; });
        sortedKeys2.Sort([](const KeyboardKey& a, const KeyboardKey& b) { return a < b; });

        for (int i = 0; i < sortedKeys1.Count(); i++) {
            if (sortedKeys1[i] != sortedKeys2[i])
                return false;
        }

        return true;
    }



}
// --- META ---

DECLARE_CLASS(o2::ShortcutKeys, o2__ShortcutKeys);
// --- END META ---
