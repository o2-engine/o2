#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

TEST(Vec2F, ConstructorAndAccess) {
    Vec2F v(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);

    Vec2F zero;
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);

    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 2.0f);
}

TEST(Vec2F, Arithmetic) {
    Vec2F a(1.0f, 2.0f);
    Vec2F b(3.0f, 4.0f);

    Vec2F sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 4.0f);
    EXPECT_FLOAT_EQ(sum.y, 6.0f);

    Vec2F diff = b - a;
    EXPECT_FLOAT_EQ(diff.x, 2.0f);
    EXPECT_FLOAT_EQ(diff.y, 2.0f);

    Vec2F scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 4.0f);

    Vec2F mul = a * b;
    EXPECT_FLOAT_EQ(mul.x, 3.0f);
    EXPECT_FLOAT_EQ(mul.y, 8.0f);
}

TEST(Vec2F, LengthAndNormalize) {
    Vec2F v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.Length(), 5.0f);
    EXPECT_FLOAT_EQ(v.SqrLength(), 25.0f);

    Vec2F n = v.Normalized();
    EXPECT_NEAR(n.Length(), 1.0f, 1e-5f);
    EXPECT_FLOAT_EQ(n.x, 0.6f);
    EXPECT_FLOAT_EQ(n.y, 0.8f);
}

TEST(Vec2F, DotAndCross) {
    Vec2F a(1.0f, 0.0f);
    Vec2F b(0.0f, 1.0f);

    EXPECT_FLOAT_EQ(a.Dot(b), 0.0f);
    EXPECT_FLOAT_EQ(a.Cross(b), 1.0f);

    Vec2F perp = a.Perpendicular();
    EXPECT_FLOAT_EQ(perp.x, 0.0f);
    EXPECT_FLOAT_EQ(perp.y, 1.0f);
}

TEST(Vec2F, StaticMethods) {
    EXPECT_FLOAT_EQ(Vec2F::Up().x, 0.0f);
    EXPECT_FLOAT_EQ(Vec2F::Up().y, 1.0f);
    EXPECT_FLOAT_EQ(Vec2F::Down().y, -1.0f);
    EXPECT_FLOAT_EQ(Vec2F::Left().x, -1.0f);
    EXPECT_FLOAT_EQ(Vec2F::Right().x, 1.0f);
    EXPECT_FLOAT_EQ(Vec2F::One().x, 1.0f);
    EXPECT_FLOAT_EQ(Vec2F::One().y, 1.0f);
}

TEST(Vec2F, Equality) {
    Vec2F a(1.0f, 2.0f);
    Vec2F b(1.0f, 2.0f);
    Vec2F c(1.1f, 2.0f);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

TEST(Vec2F, Vec2IConversion) {
    Vec2I vi(10, 20);
    Vec2F vf = (Vec2F)vi;
    EXPECT_FLOAT_EQ(vf.x, 10.0f);
    EXPECT_FLOAT_EQ(vf.y, 20.0f);

    Vec2F vf2(1.5f, 2.7f);
    Vec2I vi2 = (Vec2I)vf2;
    EXPECT_EQ(vi2.x, 1);
    EXPECT_EQ(vi2.y, 2);
}
