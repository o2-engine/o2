#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

TEST(Application, IsCursorInfiniteModeDefaultsToFalse) {
    EXPECT_FALSE(o2Application.IsCursorInfiniteModeOn());
}

TEST(Application, SetCursorInfiniteModeTogglesFlag) {
    bool original = o2Application.IsCursorInfiniteModeOn();
    o2Application.SetCursorInfiniteMode(true);
    EXPECT_TRUE(o2Application.IsCursorInfiniteModeOn());
    o2Application.SetCursorInfiniteMode(false);
    EXPECT_FALSE(o2Application.IsCursorInfiniteModeOn());
    o2Application.SetCursorInfiniteMode(original);
}

TEST(Application, GetGraphicsScaleIsPositive) {
    EXPECT_GT(o2Application.GetGraphicsScale(), 0.0f);
}

TEST(Application, GetBinPathReturnsNonEmptyPath) {
    EXPECT_FALSE(o2Application.GetBinPath().IsEmpty());
}

TEST(Application, GetScreenResolutionReturnsPositiveDimensions) {
    Vec2I res = o2Application.GetScreenResolution();
    EXPECT_GT(res.x, 0);
    EXPECT_GT(res.y, 0);
}

TEST(Application, GetWindowSizeReturnsPositiveDimensions) {
    Vec2I size = o2Application.GetWindowSize();
    EXPECT_GT(size.x, 0);
    EXPECT_GT(size.y, 0);
}

TEST(Application, GetContentSizeIsNotLargerThanWindowSize) {
    Vec2I content = o2Application.GetContentSize();
    Vec2I window = o2Application.GetWindowSize();
    EXPECT_LE(content.x, window.x);
    EXPECT_LE(content.y, window.y);
    EXPECT_GT(content.x, 0);
    EXPECT_GT(content.y, 0);
}

TEST(Application, CallbackFunctionsHoldUserClosures) {
    int activated = 0;
    auto savedActivated = o2Application.onActivated;
    o2Application.onActivated = [&] { activated++; };

    o2Application.onActivated();
    o2Application.onActivated();
    EXPECT_EQ(activated, 2);

    o2Application.onActivated = savedActivated;
}
