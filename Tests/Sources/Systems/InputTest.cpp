#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Input.h"
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

namespace {

// Drains queued messages so just-applied state (pressed/released/wheel/delta)
// is observable. Does NOT advance the frame.
void Apply()
{
    o2Input.PreUpdate();
}

// Full frame advance: drain queue then move pressed -> down, clear released,
// wheel delta and per-cursor delta. State after Tick reflects "previous frame"
// from the perspective of just-applied transitions.
void Tick(float dt = 0.0f)
{
    o2Input.PreUpdate();
    o2Input.Update(dt);
}

// Cleans up any keys/cursors a test may have left dangling so the
// singleton's state can't leak across tests.
class InputResetGuard
{
public:
    ~InputResetGuard()
    {
        for (auto& k : o2Input.GetDownKeys())
            o2Input.OnKeyReleased(k.keyCode);
        for (auto& k : o2Input.GetPressedKeys())
            o2Input.OnKeyReleased(k.keyCode);
        for (auto& c : o2Input.GetCursors())
        {
            if (c.isPressed)
                o2Input.OnCursorReleased(c.id);
        }
        Tick();
        Tick();
    }
};

constexpr KeyboardKey kTestKey = (KeyboardKey)0x70; // VK_F1 on Windows; safe unique code
constexpr KeyboardKey kRightMouseKey = (KeyboardKey)-1;
constexpr KeyboardKey kMiddleMouseKey = (KeyboardKey)-2;

} // namespace

// ===== Input::Key / Input::Cursor structs =====

TEST(InputKey, EqualsByKeyCodeOnlyComparison) {
    Input::Key a(kTestKey);
    Input::Key b(kTestKey);
    a.pressedTime = 5.0f;
    b.pressedTime = 0.0f;

    EXPECT_TRUE(a == kTestKey);
    EXPECT_FALSE(a == (KeyboardKey)('Z'));
}

TEST(InputKey, EqualsByCodeAndPressedTimeForFullEquality) {
    Input::Key a(kTestKey);
    Input::Key b(kTestKey);
    EXPECT_TRUE(a == b);

    a.pressedTime = 1.0f;
    EXPECT_FALSE(a == b);
}

TEST(InputCursor, EqualsByAllFields) {
    Input::Cursor a(Vec2F(10, 20), 0);
    Input::Cursor b(Vec2F(10, 20), 0);
    EXPECT_TRUE(a == b);

    b.position = Vec2F(11, 20);
    EXPECT_FALSE(a == b);
}

// ===== Input keyboard state machine =====

TEST(Input, KeyPressFlowProducesPressedAndDown) {
    InputResetGuard guard;
    ASSERT_FALSE(o2Input.IsKeyPressed(kTestKey));
    ASSERT_FALSE(o2Input.IsKeyDown(kTestKey));

    o2Input.OnKeyPressed(kTestKey);
    Apply();

    EXPECT_TRUE(o2Input.IsKeyPressed(kTestKey));
    EXPECT_TRUE(o2Input.IsKeyDown(kTestKey));
    EXPECT_FALSE(o2Input.IsKeyReleased(kTestKey));
}

TEST(Input, KeyTransitionsFromPressedToDownAfterFrame) {
    InputResetGuard guard;
    o2Input.OnKeyPressed(kTestKey);
    Tick(0.016f);

    EXPECT_FALSE(o2Input.IsKeyPressed(kTestKey));
    EXPECT_TRUE(o2Input.IsKeyDown(kTestKey));
}

TEST(Input, KeyReleaseFlowRemovesFromDownAndAddsToReleased) {
    InputResetGuard guard;
    o2Input.OnKeyPressed(kTestKey);
    Tick(0.016f);
    ASSERT_TRUE(o2Input.IsKeyDown(kTestKey));

    o2Input.OnKeyReleased(kTestKey);
    Apply();

    EXPECT_FALSE(o2Input.IsKeyDown(kTestKey));
    EXPECT_TRUE(o2Input.IsKeyReleased(kTestKey));
}

TEST(Input, ReleasedKeyIsClearedOnNextFrame) {
    InputResetGuard guard;
    o2Input.OnKeyPressed(kTestKey);
    Tick(0.016f);
    o2Input.OnKeyReleased(kTestKey);
    Apply();
    ASSERT_TRUE(o2Input.IsKeyReleased(kTestKey));

    o2Input.Update(0.016f);
    EXPECT_FALSE(o2Input.IsKeyReleased(kTestKey));
    EXPECT_FALSE(o2Input.IsKeyDown(kTestKey));
}

TEST(Input, PressingSameKeyTwiceDoesNotDuplicate) {
    InputResetGuard guard;
    o2Input.OnKeyPressed(kTestKey);
    o2Input.OnKeyPressed(kTestKey);
    Apply();

    int count = 0;
    for (auto& k : o2Input.GetDownKeys())
        if (k.keyCode == kTestKey) count++;
    for (auto& k : o2Input.GetPressedKeys())
        if (k.keyCode == kTestKey) count++;

    EXPECT_EQ(count, 1);
}

TEST(Input, GetKeyPressingTimeAccumulatesAcrossUpdates) {
    InputResetGuard guard;
    o2Input.OnKeyPressed(kTestKey);
    Tick(0.05f);
    Tick(0.05f);

    float time = o2Input.GetKeyPressingTime(kTestKey);
    EXPECT_GT(time, 0.09f);
    EXPECT_LT(time, 0.11f);
}

TEST(Input, GetKeyPressingTimeReturnsZeroForUnpressedKey) {
    InputResetGuard guard;
    EXPECT_EQ(o2Input.GetKeyPressingTime(kTestKey), 0.0f);
}

TEST(Input, GetPressedKeysContainsKeyAfterPress) {
    InputResetGuard guard;
    o2Input.OnKeyPressed(kTestKey);
    Apply();

    auto& pressed = o2Input.GetPressedKeys();
    EXPECT_TRUE(pressed.Any([&](const Input::Key& k) { return k.keyCode == kTestKey; }));
}

// ===== Input cursor state machine =====

TEST(Input, CursorPressedSetsPressedAndDownWithPosition) {
    InputResetGuard guard;
    Vec2F pos(123.0f, 45.0f);
    o2Input.OnCursorPressed(pos);
    Apply();

    EXPECT_TRUE(o2Input.IsCursorPressed());
    EXPECT_TRUE(o2Input.IsCursorDown());
    EXPECT_EQ(o2Input.GetCursorPos(), pos);
}

TEST(Input, CursorTransitionsFromPressedToDownAfterFrame) {
    InputResetGuard guard;
    o2Input.OnCursorPressed(Vec2F(0, 0));
    Tick(0.016f);

    EXPECT_FALSE(o2Input.IsCursorPressed());
    EXPECT_TRUE(o2Input.IsCursorDown());
}

TEST(Input, CursorReleaseProducesReleasedThenClears) {
    InputResetGuard guard;
    o2Input.OnCursorPressed(Vec2F(50, 50));
    Tick(0.016f);

    o2Input.OnCursorReleased();
    Apply();
    EXPECT_TRUE(o2Input.IsCursorReleased());
    EXPECT_FALSE(o2Input.IsCursorDown());

    o2Input.Update(0.016f);
    EXPECT_FALSE(o2Input.IsCursorReleased());
}

TEST(Input, CursorMovedUpdatesPositionAndAccumulatesDelta) {
    InputResetGuard guard;
    o2Input.OnCursorPressed(Vec2F(0, 0));
    Tick(0.016f); // settle: position (0,0), delta cleared

    o2Input.OnCursorMoved(Vec2F(10, 5));
    Apply();

    EXPECT_EQ(o2Input.GetCursorPos(), Vec2F(10, 5));
    EXPECT_EQ(o2Input.GetCursorDelta(), Vec2F(10, 5));
}

TEST(Input, CursorDeltaResetsAfterUpdate) {
    InputResetGuard guard;
    o2Input.OnCursorPressed(Vec2F(0, 0));
    Tick(0.016f);
    o2Input.OnCursorMoved(Vec2F(10, 0));
    Tick(0.016f);

    EXPECT_EQ(o2Input.GetCursorDelta(), Vec2F(0, 0));
}

TEST(Input, GetCursorPressingTimeAccumulatesAcrossUpdates) {
    InputResetGuard guard;
    o2Input.OnCursorPressed(Vec2F(0, 0));
    Tick(0.05f);
    Tick(0.05f);

    float time = o2Input.GetCursorPressingTime();
    EXPECT_GT(time, 0.09f);
    EXPECT_LT(time, 0.11f);
}

// ===== Right/middle mouse mapped onto sentinel key codes =====

TEST(Input, AltCursorPressedTriggersRightMouseSentinel) {
    InputResetGuard guard;
    ASSERT_FALSE(o2Input.IsRightMousePressed());

    o2Input.OnAltCursorPressed(Vec2F(20, 30));
    Apply();

    EXPECT_TRUE(o2Input.IsRightMousePressed());
    EXPECT_TRUE(o2Input.IsRightMouseDown());
    EXPECT_TRUE(o2Input.IsKeyPressed(kRightMouseKey));
}

TEST(Input, AltCursorReleasedClearsRightMouse) {
    InputResetGuard guard;
    o2Input.OnAltCursorPressed(Vec2F(0, 0));
    Tick(0.016f);

    o2Input.OnAltCursorReleased();
    Apply();

    EXPECT_TRUE(o2Input.IsRightMouseReleased());
    EXPECT_FALSE(o2Input.IsRightMouseDown());
}

TEST(Input, Alt2CursorPressedTriggersMiddleMouseSentinel) {
    InputResetGuard guard;
    o2Input.OnAlt2CursorPressed(Vec2F(0, 0));
    Apply();

    EXPECT_TRUE(o2Input.IsMiddleMousePressed());
    EXPECT_TRUE(o2Input.IsKeyPressed(kMiddleMouseKey));
}

// ===== Mouse wheel =====

TEST(Input, MouseWheelDeltaAccumulatesAndResetsAfterUpdate) {
    InputResetGuard guard;
    EXPECT_FLOAT_EQ(o2Input.GetMouseWheelDelta(), 0.0f);

    o2Input.OnMouseWheel(1.5f);
    o2Input.OnMouseWheel(-0.5f);
    Apply();

    EXPECT_FLOAT_EQ(o2Input.GetMouseWheelDelta(), 1.0f);

    o2Input.Update(0.016f);
    EXPECT_FLOAT_EQ(o2Input.GetMouseWheelDelta(), 0.0f);
}
