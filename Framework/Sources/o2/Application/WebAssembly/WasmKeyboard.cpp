#include "o2/stdafx.h"
#include "o2/Application/WebAssembly/WasmKeyboard.h"

#include <cstring>

namespace o2
{
    KeyboardKey DomKeyCodeToKeyboardKey(const char* code)
    {
        if (!code)
            return 0;

        // letters and digits are the ASCII codes of their character on every platform
        if (std::strncmp(code, "Key", 3) == 0 && code[3] != 0 && code[4] == 0)
            return (KeyboardKey)code[3];

        if (std::strncmp(code, "Digit", 5) == 0 && code[5] != 0 && code[6] == 0)
            return (KeyboardKey)code[5];

        struct DomKey { const char* code; KeyboardKey key; };
        static const DomKey keys[] = {
            { "ArrowLeft", VK_LEFT }, { "ArrowUp", VK_UP },
            { "ArrowRight", VK_RIGHT }, { "ArrowDown", VK_DOWN },

            { "Space", VK_SPACE }, { "Enter", VK_RETURN }, { "NumpadEnter", VK_RETURN },
            { "Escape", VK_ESCAPE }, { "Backspace", VK_BACK }, { "Tab", VK_TAB },
            { "Delete", VK_DELETE }, { "Home", VK_HOME }, { "End", VK_END },
            { "PageUp", VK_PRIOR }, { "PageDown", VK_NEXT },

            { "ShiftLeft", VK_SHIFT }, { "ShiftRight", VK_SHIFT },
            { "ControlLeft", VK_CONTROL }, { "ControlRight", VK_CONTROL },
            { "AltLeft", VK_MENU }, { "AltRight", VK_MENU },

            { "F1", VK_F1 }, { "F2", VK_F2 }, { "F3", VK_F3 }, { "F4", VK_F4 },
            { "F5", VK_F5 }, { "F6", VK_F6 }, { "F7", VK_F7 }, { "F8", VK_F8 },
            { "F9", VK_F9 }, { "F10", VK_F10 }, { "F11", VK_F11 }, { "F12", VK_F12 },

            { "Numpad0", VK_NUMPAD0 }, { "Numpad1", VK_NUMPAD1 }, { "Numpad2", VK_NUMPAD2 },
            { "Numpad3", VK_NUMPAD3 }, { "Numpad4", VK_NUMPAD4 }, { "Numpad5", VK_NUMPAD5 },
            { "Numpad6", VK_NUMPAD6 }, { "Numpad7", VK_NUMPAD7 }, { "Numpad8", VK_NUMPAD8 },
            { "Numpad9", VK_NUMPAD9 }, { "NumpadMultiply", VK_MULTIPLY },
            { "NumpadAdd", VK_ADD }, { "NumpadSubtract", VK_SUBTRACT },
            { "NumpadDecimal", VK_DECIMAL }, { "NumpadDivide", VK_DIVIDE },
            { "NumLock", VK_NUMLOCK },

            { "Backquote", VK_OEM_3 }, { "Minus", VK_OEM_MINUS }, { "Equal", VK_OEM_PLUS },
            { "BracketLeft", VK_OEM_4 }, { "BracketRight", VK_OEM_6 },
            { "Semicolon", VK_OEM_1 }, { "Quote", VK_OEM_7 }, { "Backslash", VK_OEM_5 },
            { "Comma", VK_OEM_COMMA }, { "Period", VK_OEM_PERIOD }, { "Slash", VK_OEM_2 }
        };

        for (auto& entry : keys)
        {
            if (std::strcmp(code, entry.code) == 0)
                return entry.key;
        }

        return 0;
    }
}
