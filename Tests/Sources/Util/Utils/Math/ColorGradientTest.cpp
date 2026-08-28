#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/ColorGradient.h"

using namespace o2;

// Like curves and tracks, a gradient holds its end colors outside the key range instead of extrapolating
TEST(ColorGradient, EvaluateOutsideKeysHoldsEndColors)
{
    ColorGradient gradient;
    gradient.RemoveAllKeys(); // the default gradient already holds white keys at 0 and 1
    gradient.InsertKey(0.0f, Color4::White());
    gradient.InsertKey(1.0f, Color4::Blue());

    Color4 before = gradient.Evaluate(-0.5f);
    EXPECT_EQ(before.r, 255);
    EXPECT_EQ(before.b, 255);

    Color4 after = gradient.Evaluate(1.5f);
    EXPECT_EQ(after.r, 0);
    EXPECT_EQ(after.b, 255);

    Color4 mid = gradient.Evaluate(0.5f);
    EXPECT_GT(mid.r, 0);
    EXPECT_LT(mid.r, 255);
}
