#include "o2/stdafx.h"

#ifdef PLATFORM_MAC

#include "o2/Application/Mac/MacKeyboard.h"

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>

namespace o2
{
    // Keys that produce text: letters, digits, punctuation, space, return, tab, numpad
    static bool IsTypingHardwareKey(int hardwareKeyCode)
    {
        if (MacHardwareKeyToKeyboardKey(hardwareKeyCode) >= 0)
            return true;

        switch (hardwareKeyCode)
        {
            case kVK_ANSI_Grave: case kVK_ANSI_Minus: case kVK_ANSI_Equal:
            case kVK_ANSI_LeftBracket: case kVK_ANSI_RightBracket: case kVK_ANSI_Semicolon:
            case kVK_ANSI_Quote: case kVK_ANSI_Backslash: case kVK_ANSI_Comma:
            case kVK_ANSI_Period: case kVK_ANSI_Slash: case kVK_ISO_Section:
            case kVK_Space: case kVK_Return: case kVK_Tab:
            case kVK_ANSI_KeypadDecimal: case kVK_ANSI_KeypadMultiply: case kVK_ANSI_KeypadPlus:
            case kVK_ANSI_KeypadMinus: case kVK_ANSI_KeypadDivide: case kVK_ANSI_KeypadEnter:
            case kVK_ANSI_KeypadEquals:
            case kVK_ANSI_Keypad0: case kVK_ANSI_Keypad1: case kVK_ANSI_Keypad2:
            case kVK_ANSI_Keypad3: case kVK_ANSI_Keypad4: case kVK_ANSI_Keypad5:
            case kVK_ANSI_Keypad6: case kVK_ANSI_Keypad7: case kVK_ANSI_Keypad8:
            case kVK_ANSI_Keypad9:
                return true;
        }

        return false;
    }

    UInt16 GetUnicodeFromMacKey(KeyboardKey key)
    {
        int hardwareKey = KeyboardKeyToMacHardwareKey(key);
        if (hardwareKey < 0 || !IsTypingHardwareKey(hardwareKey))
            return 0;

        NSEventModifierFlags flags = [NSEvent modifierFlags];
        if (flags & NSEventModifierFlagCommand)
            return 0;

        UInt32 modifiers = 0;
        if (flags & NSEventModifierFlagShift)
            modifiers |= (shiftKey >> 8) & 0xFF;
        if (flags & NSEventModifierFlagCapsLock)
            modifiers |= (alphaLock >> 8) & 0xFF;

        UInt16 result = 0;

        TISInputSourceRef source = TISCopyCurrentKeyboardLayoutInputSource();
        CFDataRef layoutData = source
            ? (CFDataRef)TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData)
            : nullptr;

        if (layoutData)
        {
            const UCKeyboardLayout* layout = (const UCKeyboardLayout*)CFDataGetBytePtr(layoutData);
            UInt32 deadKeyState = 0;
            UniChar chars[4];
            UniCharCount length = 0;

            if (UCKeyTranslate(layout, (UInt16)hardwareKey, kUCKeyActionDown, modifiers, LMGetKbdType(),
                               kUCKeyTranslateNoDeadKeysMask, &deadKeyState, 4, &length, chars) == noErr &&
                length > 0)
            {
                result = chars[0];
            }
        }
        else if (key > 0)
            result = (UInt16)key;

        if (source)
            CFRelease(source);

        return result;
    }
}

#endif // PLATFORM_MAC
