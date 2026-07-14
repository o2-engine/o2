#include "o2/stdafx.h"

#ifdef PLATFORM_MAC

#include "o2/Application/Mac/MacKeyboard.h"

namespace o2
{
    KeyboardKey MacHardwareKeyToKeyboardKey(int hardwareKeyCode)
    {
        switch (hardwareKeyCode)
        {
            case 0x00: return VK_A;
            case 0x0B: return VK_B;
            case 0x08: return VK_C;
            case 0x02: return VK_D;
            case 0x0E: return VK_E;
            case 0x03: return VK_F;
            case 0x05: return VK_G;
            case 0x04: return VK_H;
            case 0x22: return VK_I;
            case 0x26: return VK_J;
            case 0x28: return VK_K;
            case 0x25: return VK_L;
            case 0x2E: return VK_M;
            case 0x2D: return VK_N;
            case 0x1F: return VK_O;
            case 0x23: return VK_P;
            case 0x0C: return VK_Q;
            case 0x0F: return VK_R;
            case 0x01: return VK_S;
            case 0x11: return VK_T;
            case 0x20: return VK_U;
            case 0x09: return VK_V;
            case 0x0D: return VK_W;
            case 0x07: return VK_X;
            case 0x10: return VK_Y;
            case 0x06: return VK_Z;

            case 0x1D: return VK_0;
            case 0x12: return VK_1;
            case 0x13: return VK_2;
            case 0x14: return VK_3;
            case 0x15: return VK_4;
            case 0x17: return VK_5;
            case 0x16: return VK_6;
            case 0x1A: return VK_7;
            case 0x1C: return VK_8;
            case 0x19: return VK_9;
        }

        return -hardwareKeyCode;
    }

    int KeyboardKeyToMacHardwareKey(KeyboardKey key)
    {
        switch (key)
        {
            case VK_A: return 0x00;
            case VK_B: return 0x0B;
            case VK_C: return 0x08;
            case VK_D: return 0x02;
            case VK_E: return 0x0E;
            case VK_F: return 0x03;
            case VK_G: return 0x05;
            case VK_H: return 0x04;
            case VK_I: return 0x22;
            case VK_J: return 0x26;
            case VK_K: return 0x28;
            case VK_L: return 0x25;
            case VK_M: return 0x2E;
            case VK_N: return 0x2D;
            case VK_O: return 0x1F;
            case VK_P: return 0x23;
            case VK_Q: return 0x0C;
            case VK_R: return 0x0F;
            case VK_S: return 0x01;
            case VK_T: return 0x11;
            case VK_U: return 0x20;
            case VK_V: return 0x09;
            case VK_W: return 0x0D;
            case VK_X: return 0x07;
            case VK_Y: return 0x10;
            case VK_Z: return 0x06;

            case VK_0: return 0x1D;
            case VK_1: return 0x12;
            case VK_2: return 0x13;
            case VK_3: return 0x14;
            case VK_4: return 0x15;
            case VK_5: return 0x17;
            case VK_6: return 0x16;
            case VK_7: return 0x1A;
            case VK_8: return 0x1C;
            case VK_9: return 0x19;
        }

        if (key < 0)
            return -key;

        return -1;
    }

    bool MacKeyboardHandler::KeyEvent::operator==(const KeyEvent& other) const
    {
        return key == other.key && pressed == other.pressed;
    }

    void MacKeyboardHandler::SyncModifiers(const Modifiers& modifiers, Vector<KeyEvent>& out)
    {
        if (modifiers.shift != mModifiers.shift)
            out.Add({ VK_SHIFT, modifiers.shift });

        if (modifiers.alt != mModifiers.alt)
            out.Add({ VK_MENU, modifiers.alt });

        if (modifiers.control != mModifiers.control)
            out.Add({ VK_CONTROL, modifiers.control });

        if (modifiers.command != mModifiers.command)
        {
            out.Add({ VK_COMMAND, modifiers.command });

            if (!modifiers.command)
            {
                for (auto key : mPressedWithCommand)
                {
                    out.Add({ key, false });
                    mPressedKeys.Remove(key);
                }

                mPressedWithCommand.Clear();
            }
        }

        mModifiers = modifiers;
    }

    Vector<MacKeyboardHandler::KeyEvent> MacKeyboardHandler::OnKeyDown(int hardwareKeyCode, const Modifiers& modifiers)
    {
        Vector<KeyEvent> out;
        SyncModifiers(modifiers, out);

        KeyboardKey key = MacHardwareKeyToKeyboardKey(hardwareKeyCode);
        if (mPressedKeys.Contains(key))
            return out;

        mPressedKeys.Add(key);
        if (mModifiers.command)
            mPressedWithCommand.Add(key);

        out.Add({ key, true });
        return out;
    }

    Vector<MacKeyboardHandler::KeyEvent> MacKeyboardHandler::OnKeyUp(int hardwareKeyCode, const Modifiers& modifiers)
    {
        Vector<KeyEvent> out;
        SyncModifiers(modifiers, out);

        KeyboardKey key = MacHardwareKeyToKeyboardKey(hardwareKeyCode);
        mPressedKeys.Remove(key);
        mPressedWithCommand.Remove(key);

        out.Add({ key, false });
        return out;
    }

    Vector<MacKeyboardHandler::KeyEvent> MacKeyboardHandler::OnModifiersChanged(const Modifiers& modifiers)
    {
        Vector<KeyEvent> out;
        SyncModifiers(modifiers, out);
        return out;
    }

    Vector<MacKeyboardHandler::KeyEvent> MacKeyboardHandler::OnFocusLost()
    {
        Vector<KeyEvent> out;
        for (auto key : mPressedKeys)
            out.Add({ key, false });

        mPressedKeys.Clear();
        mPressedWithCommand.Clear();

        SyncModifiers(Modifiers(), out);
        return out;
    }
}

#endif // PLATFORM_MAC
