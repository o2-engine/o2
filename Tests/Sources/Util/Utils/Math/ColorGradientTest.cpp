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

// Coincident keys form a zero-length segment: the first key's color wins, as on a step inside the range
TEST(ColorGradient, CoincidentKeysGiveTheFirstKeyColor)
{
    ColorGradient gradient;
    gradient.RemoveAllKeys();
    gradient.InsertKey(0.0f, Color4::Red());
    gradient.InsertKey(0.0f, Color4::Blue());
    gradient.InsertKey(1.0f, Color4::Blue());

    EXPECT_EQ(gradient.Evaluate(0.0f), Color4::Red());

    int cacheKey = 1;
    EXPECT_EQ(gradient.Evaluate(0.0f, false, cacheKey), Color4::Red());

    gradient.InsertKey(0.5f, Color4::Green());
    gradient.InsertKey(0.5f, Color4::Red());
    EXPECT_EQ(gradient.Evaluate(0.5f), Color4::Green());
}
