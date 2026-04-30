#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

// Application properties that depend on a real window — must run in the rendered tier
// (o2RenderTests does Application::Initialize without headless, so the platform window
// is created and these queries return meaningful values).

TEST(Application, GetWindowSizeReturnsPositiveDimensions) {
    Vec2I size = o2Application.GetWindowSize();
    EXPECT_GT(size.x, 0);
    EXPECT_GT(size.y, 0);
}

TEST(Application, GetContentSizeReturnsPositiveDimensions) {
    Vec2I content = o2Application.GetContentSize();
    EXPECT_GT(content.x, 0);
    EXPECT_GT(content.y, 0);
}
