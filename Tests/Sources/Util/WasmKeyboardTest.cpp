#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/WebAssembly/WasmKeyboard.h"

using namespace o2;

// The browser reports layout independent DOM codes; they have to land on the very same
// engine keys the native build uses, otherwise nothing the game listens for reacts
TEST(WasmKeyboard, MapsDomCodesToEngineKeys)
{
    EXPECT_EQ(DomKeyCodeToKeyboardKey("ArrowLeft"), VK_LEFT);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("ArrowRight"), VK_RIGHT);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("ArrowUp"), VK_UP);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("ArrowDown"), VK_DOWN);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Space"), VK_SPACE);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Enter"), VK_RETURN);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Escape"), VK_ESCAPE);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Backspace"), VK_BACK);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Tab"), VK_TAB);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Delete"), VK_DELETE);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Home"), VK_HOME);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("End"), VK_END);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("PageUp"), VK_PRIOR);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("PageDown"), VK_NEXT);
}

TEST(WasmKeyboard, MapsLettersDigitsAndModifiers)
{
    EXPECT_EQ(DomKeyCodeToKeyboardKey("KeyA"), VK_A);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("KeyW"), VK_W);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("KeyR"), VK_R);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Digit0"), VK_0);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Digit9"), VK_9);

    EXPECT_EQ(DomKeyCodeToKeyboardKey("ShiftLeft"), VK_SHIFT);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("ShiftRight"), VK_SHIFT);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("ControlLeft"), VK_CONTROL);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("AltRight"), VK_MENU);

    EXPECT_EQ(DomKeyCodeToKeyboardKey("F1"), VK_F1);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("F11"), VK_F11);

    EXPECT_EQ(DomKeyCodeToKeyboardKey("Numpad5"), VK_NUMPAD5);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Comma"), VK_OEM_COMMA);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Minus"), VK_OEM_MINUS);
}

// Unknown codes must stay unmapped: key 0 would press a phantom key and, in the browser
// handler, swallow shortcuts the page still needs
TEST(WasmKeyboard, UnknownCodesMapToNothing)
{
    EXPECT_EQ(DomKeyCodeToKeyboardKey(nullptr), 0);
    EXPECT_EQ(DomKeyCodeToKeyboardKey(""), 0);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("MediaPlayPause"), 0);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("Fn"), 0);
    EXPECT_EQ(DomKeyCodeToKeyboardKey("F13"), 0);
}
