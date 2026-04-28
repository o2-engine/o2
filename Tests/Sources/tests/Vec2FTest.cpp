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

TEST(Vec2F, EqualityUsesFixedEpsilon) {
    // operator== uses a hard-coded 0.001f epsilon (Vector2.h:214).
    Vec2F base(1.0f, 1.0f);
    Vec2F within(1.0f + 0.0009f, 1.0f);
    Vec2F outside(1.0f + 0.0011f, 1.0f);

    EXPECT_TRUE(base == within);
    EXPECT_FALSE(base == outside);
    EXPECT_TRUE(base != outside);
}

TEST(Vec2F, NormalizeZeroVectorReturnsZero) {
    Vec2F zero(0.0f, 0.0f);
    Vec2F n = zero.Normalized();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);

    zero.Normalize();
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);
}

TEST(Vec2F, PerpendicularIsOrthogonal) {
    const Vec2F vectors[] = { Vec2F(3.0f, 4.0f), Vec2F(-2.0f, 5.0f), Vec2F(0.7f, -0.3f) };
    for (const Vec2F& v : vectors)
    {
        Vec2F p = v.Perpendicular();
        EXPECT_NEAR(v.Dot(p), 0.0f, 1e-5f);
        EXPECT_NEAR(v.Length(), p.Length(), 1e-5f);
    }
}

TEST(Vec2F, RotateFullCircleIsIdentity) {
    Vec2F v(1.0f, 0.0f);
    Vec2F rotated = v.Rotate(Math::PI() * 0.5f)
                     .Rotate(Math::PI() * 0.5f)
                     .Rotate(Math::PI() * 0.5f)
                     .Rotate(Math::PI() * 0.5f);

    EXPECT_NEAR(rotated.x, v.x, 1e-5f);
    EXPECT_NEAR(rotated.y, v.y, 1e-5f);
}

TEST(Vec2F, RotateHalfPiTakesXAxisToYAxis) {
    Vec2F v(1.0f, 0.0f);
    Vec2F rotated = v.Rotate(Math::PI() * 0.5f);
    EXPECT_NEAR(rotated.x, 0.0f, 1e-5f);
    EXPECT_NEAR(rotated.y, 1.0f, 1e-5f);
}

TEST(Vec2F, SignedAngleHasSign) {
    // Cross-product based: positive when the second vector is CCW from the first.
    float a = Vec2F::Right().SignedAngle(Vec2F::Up());
    float b = Vec2F::Up().SignedAngle(Vec2F::Right());

    EXPECT_GT(a, 0.0f);
    EXPECT_LT(b, 0.0f);
    EXPECT_NEAR(Math::Abs(a), Math::Abs(b), 1e-5f);
}

TEST(Vec2F, ProjectOntoAxis) {
    Vec2F v(3.0f, 4.0f);
    Vec2F projOnX = v.Project(Vec2F(2.0f, 0.0f));
    EXPECT_NEAR(projOnX.x, 3.0f, 1e-5f);
    EXPECT_NEAR(projOnX.y, 0.0f, 1e-5f);

    Vec2F projOnY = v.Project(Vec2F(0.0f, 5.0f));
    EXPECT_NEAR(projOnY.x, 0.0f, 1e-5f);
    EXPECT_NEAR(projOnY.y, 4.0f, 1e-5f);
}

TEST(Vec2F, IsParallelDetectsCollinear) {
    Vec2F v(1.0f, 2.0f);
    EXPECT_TRUE(v.IsParallel(Vec2F(2.0f, 4.0f)));
    EXPECT_TRUE(v.IsParallel(Vec2F(-1.0f, -2.0f)));
    EXPECT_FALSE(v.IsParallel(Vec2F(2.0f, 1.0f)));
}

TEST(Vec2F, CrossProductOperators) {
    Vec2F a(1.0f, 0.0f);
    Vec2F b(0.0f, 1.0f);

    // Vec2 ^ Vec2 returns scalar: x1*y2 - y1*x2.
    EXPECT_FLOAT_EQ(a ^ b, 1.0f);

    // Vec2 ^ scalar returns scalar * perpendicular: (-s*y, s*x).
    Vec2F perpScaled = a ^ 2.0f;
    EXPECT_FLOAT_EQ(perpScaled.x, 0.0f);
    EXPECT_FLOAT_EQ(perpScaled.y, 2.0f);
}

TEST(Vec2F, ScaleAroundOrigin) {
    Vec2F p(10.0f, 10.0f);
    Vec2F origin(5.0f, 5.0f);

    Vec2F scaled = p.Scale(Vec2F(2.0f, 2.0f), origin);
    // (p - origin) * 2 + origin = (5,5)*2 + (5,5) = (15,15)
    EXPECT_FLOAT_EQ(scaled.x, 15.0f);
    EXPECT_FLOAT_EQ(scaled.y, 15.0f);

    // Origin itself is invariant under Scale-around-origin.
    Vec2F atOrigin = origin.Scale(Vec2F(3.0f, 7.0f), origin);
    EXPECT_FLOAT_EQ(atOrigin.x, origin.x);
    EXPECT_FLOAT_EQ(atOrigin.y, origin.y);
}

TEST(Vec2F, ClampLengthShrinks) {
    Vec2F v(3.0f, 4.0f); // length 5
    Vec2F clamped = v.ClampLength(2.5f);
    EXPECT_NEAR(clamped.Length(), 2.5f, 1e-5f);
    // Direction preserved.
    Vec2F nv = v.Normalized();
    Vec2F nc = clamped.Normalized();
    EXPECT_NEAR(nv.x, nc.x, 1e-5f);
    EXPECT_NEAR(nv.y, nc.y, 1e-5f);
}

TEST(Vec2F, InvertedFlipsSelectedAxes) {
    Vec2F v(3.0f, 4.0f);
    EXPECT_EQ(v.InvertedX(), Vec2F(-3.0f, 4.0f));
    EXPECT_EQ(v.InvertedY(), Vec2F(3.0f, -4.0f));
    EXPECT_EQ(v.Inverted(true, true), Vec2F(-3.0f, -4.0f));
    EXPECT_EQ(v.Inverted(false, false), v);
}
