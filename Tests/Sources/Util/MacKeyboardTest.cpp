#include "o2/stdafx.h"

#ifdef PLATFORM_MAC

#include <gtest/gtest.h>

#include "o2/Application/Mac/MacKeyboard.h"

using namespace o2;

namespace {

using Modifiers = MacKeyboardHandler::Modifiers;
using KeyEvent = MacKeyboardHandler::KeyEvent;

// Hardware key codes (kVK_*), same on any keyboard layout
constexpr int kHwW = 0x0D;
constexpr int kHwA = 0x00;
constexpr int kHwS = 0x01;
constexpr int kHwD = 0x02;
constexpr int kHwQ = 0x0C;
constexpr int kHwE = 0x0E;
constexpr int kHwZ = 0x06;
constexpr int kHwEscape = 0x35;
constexpr int kHwSpace = 0x31;
constexpr int kHwLeftArrow = 0x7B;
constexpr int kHwComma = 0x2B;
constexpr int kHwDigit1 = 0x12;

Modifiers NoMods() { return Modifiers(); }
Modifiers Shift() { Modifiers m; m.shift = true; return m; }
Modifiers Cmd() { Modifiers m; m.command = true; return m; }
Modifiers CmdShift() { Modifiers m; m.command = true; m.shift = true; return m; }

bool Contains(const Vector<KeyEvent>& events, KeyboardKey key, bool pressed)
{
    return events.Contains({ key, pressed });
}

} // namespace

// ===== Layout-independent key mapping =====

TEST(MacKeyboard, HardwareKeysMapToVKCodesRegardlessOfLayout) {
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwW), 'W');
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwA), 'A');
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwS), 'S');
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwD), 'D');
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwQ), 'Q');
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwE), 'E');
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwDigit1), '1');
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwEscape), VK_ESCAPE);
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwSpace), VK_SPACE);
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwLeftArrow), VK_LEFT);
    EXPECT_EQ(MacHardwareKeyToKeyboardKey(kHwComma), VK_OEM_COMMA);
}

TEST(MacKeyboard, MappingRoundTripsForAllTypingKeys) {
    for (char c = 'A'; c <= 'Z'; c++)
        EXPECT_EQ(MacHardwareKeyToKeyboardKey(KeyboardKeyToMacHardwareKey(c)), c);

    for (char c = '0'; c <= '9'; c++)
        EXPECT_EQ(MacHardwareKeyToKeyboardKey(KeyboardKeyToMacHardwareKey(c)), c);

    EXPECT_EQ(KeyboardKeyToMacHardwareKey(VK_ESCAPE), kHwEscape);
    EXPECT_EQ(KeyboardKeyToMacHardwareKey(VK_SPACE), kHwSpace);
}

// ===== Sticky keys protection =====

// Regression: press 'W', press Shift, release 'W' - the release used to come with a different
// character code ('w' vs 'W') and the key stayed down forever
TEST(MacKeyboard, LetterDoesNotStickWhenShiftChangesDuringPress) {
    MacKeyboardHandler handler;

    auto down = handler.OnKeyDown(kHwW, NoMods());
    EXPECT_TRUE(Contains(down, 'W', true));

    handler.OnModifiersChanged(Shift());

    auto up = handler.OnKeyUp(kHwW, Shift());
    EXPECT_TRUE(Contains(up, 'W', false));
}

TEST(MacKeyboard, ShiftedLetterReleasesWithSameCode) {
    MacKeyboardHandler handler;

    auto down = handler.OnKeyDown(kHwZ, Shift());
    EXPECT_TRUE(Contains(down, VK_SHIFT, true));
    EXPECT_TRUE(Contains(down, 'Z', true));

    auto flags = handler.OnModifiersChanged(NoMods());
    EXPECT_TRUE(Contains(flags, VK_SHIFT, false));

    auto up = handler.OnKeyUp(kHwZ, NoMods());
    EXPECT_TRUE(Contains(up, 'Z', false));
}

TEST(MacKeyboard, RepeatedKeyDownEmitsSinglePress) {
    MacKeyboardHandler handler;

    auto first = handler.OnKeyDown(kHwW, NoMods());
    EXPECT_TRUE(Contains(first, 'W', true));

    auto repeat = handler.OnKeyDown(kHwW, NoMods());
    EXPECT_TRUE(repeat.IsEmpty());
}

TEST(MacKeyboard, KeyUpWithoutDownStillEmitsRelease) {
    MacKeyboardHandler handler;

    auto up = handler.OnKeyUp(kHwW, NoMods());
    EXPECT_TRUE(Contains(up, 'W', false));
}

// ===== Cmd combinations: macOS suppresses keyUp while Cmd is held =====

TEST(MacKeyboard, KeysPressedWithCommandReleaseOnCommandRelease) {
    MacKeyboardHandler handler;

    auto cmdDown = handler.OnModifiersChanged(Cmd());
    EXPECT_TRUE(Contains(cmdDown, VK_COMMAND, true));

    auto keyDown = handler.OnKeyDown(kHwZ, Cmd());
    EXPECT_TRUE(Contains(keyDown, 'Z', true));

    auto cmdUp = handler.OnModifiersChanged(NoMods());
    EXPECT_TRUE(Contains(cmdUp, VK_COMMAND, false));
    EXPECT_TRUE(Contains(cmdUp, 'Z', false));

    auto again = handler.OnKeyDown(kHwZ, NoMods());
    EXPECT_TRUE(Contains(again, 'Z', true)) << "key must be pressable again after Cmd released it";
}

TEST(MacKeyboard, CmdShiftComboDoesNotStickAnyKey) {
    MacKeyboardHandler handler;

    auto down = handler.OnKeyDown(kHwZ, CmdShift());
    EXPECT_TRUE(Contains(down, VK_COMMAND, true));
    EXPECT_TRUE(Contains(down, VK_SHIFT, true));
    EXPECT_TRUE(Contains(down, 'Z', true));

    auto shiftUp = handler.OnModifiersChanged(Cmd());
    EXPECT_TRUE(Contains(shiftUp, VK_SHIFT, false));

    auto cmdUp = handler.OnModifiersChanged(NoMods());
    EXPECT_TRUE(Contains(cmdUp, VK_COMMAND, false));
    EXPECT_TRUE(Contains(cmdUp, 'Z', false));

    auto again = handler.OnKeyDown(kHwZ, NoMods());
    EXPECT_TRUE(Contains(again, 'Z', true));
}

TEST(MacKeyboard, NormallyReleasedKeyIsNotReleasedTwiceByCommand) {
    MacKeyboardHandler handler;

    handler.OnModifiersChanged(Cmd());
    handler.OnKeyDown(kHwZ, Cmd());
    handler.OnKeyUp(kHwZ, Cmd());

    auto cmdUp = handler.OnModifiersChanged(NoMods());
    EXPECT_TRUE(Contains(cmdUp, VK_COMMAND, false));
    EXPECT_FALSE(Contains(cmdUp, 'Z', false));
}

// ===== Missed modifier events healing =====

// Regression: modifier release consumed by the system (Cmd+Tab, Cmd+Space, Ctrl+Space layout
// switch) left the modifier stuck in input
TEST(MacKeyboard, MissedModifierReleaseHealsOnNextKeyEvent) {
    MacKeyboardHandler handler;

    handler.OnModifiersChanged(Cmd());

    auto down = handler.OnKeyDown(kHwW, NoMods());
    EXPECT_TRUE(Contains(down, VK_COMMAND, false));
    EXPECT_TRUE(Contains(down, 'W', true));
}

TEST(MacKeyboard, MissedModifierPressHealsOnNextKeyEvent) {
    MacKeyboardHandler handler;

    auto up = handler.OnKeyUp(kHwW, Shift());
    EXPECT_TRUE(Contains(up, VK_SHIFT, true));
    EXPECT_TRUE(Contains(up, 'W', false));
}

// ===== Focus loss =====

TEST(MacKeyboard, FocusLostReleasesAllKeysAndModifiers) {
    MacKeyboardHandler handler;

    handler.OnKeyDown(kHwW, CmdShift());
    handler.OnKeyDown(kHwS, CmdShift());

    auto lost = handler.OnFocusLost();
    EXPECT_TRUE(Contains(lost, 'W', false));
    EXPECT_TRUE(Contains(lost, 'S', false));
    EXPECT_TRUE(Contains(lost, VK_COMMAND, false));
    EXPECT_TRUE(Contains(lost, VK_SHIFT, false));

    auto again = handler.OnKeyDown(kHwW, NoMods());
    EXPECT_TRUE(Contains(again, 'W', true));
}

// ===== Unicode translation for text input =====

TEST(MacKeyboard, LetterKeysProduceCharactersInCurrentLayout) {
    EXPECT_NE(GetUnicodeFromMacKey('A'), 0);
    EXPECT_NE(GetUnicodeFromMacKey('W'), 0);
    EXPECT_NE(GetUnicodeFromMacKey('1'), 0);
}

TEST(MacKeyboard, ServiceKeysDoNotProduceCharacters) {
    EXPECT_EQ(GetUnicodeFromMacKey(VK_ESCAPE), 0);
    EXPECT_EQ(GetUnicodeFromMacKey(VK_LEFT), 0);
    EXPECT_EQ(GetUnicodeFromMacKey(VK_F1), 0);
    EXPECT_EQ(GetUnicodeFromMacKey(VK_BACK), 0);
    EXPECT_EQ(GetUnicodeFromMacKey(VK_SHIFT), 0);
    EXPECT_EQ(GetUnicodeFromMacKey(VK_COMMAND), 0);
}

TEST(MacKeyboard, WhitespaceKeysProduceStableCharacters) {
    EXPECT_EQ(GetUnicodeFromMacKey(VK_SPACE), ' ');
    EXPECT_EQ(GetUnicodeFromMacKey(VK_RETURN), 13);
}

#endif // PLATFORM_MAC
