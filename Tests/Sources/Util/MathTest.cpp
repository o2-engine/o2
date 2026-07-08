#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include <cmath>
#include "o2/Utils/Math/Math.h"

using namespace o2;

TEST(Math, LerpEndpointsAndMidpoint) {
    EXPECT_FLOAT_EQ(Math::Lerp(2.0f, 8.0f, 0.0f), 2.0f);
    EXPECT_FLOAT_EQ(Math::Lerp(2.0f, 8.0f, 1.0f), 8.0f);
    EXPECT_FLOAT_EQ(Math::Lerp(2.0f, 8.0f, 0.5f), 5.0f);

    EXPECT_FLOAT_EQ(Math::Lerp(-4.0f, 4.0f, 0.25f), -2.0f);
}

TEST(Math, LerpcClampsCoefficient) {
    EXPECT_FLOAT_EQ(Math::Lerpc(2.0f, 8.0f, -1.0f), 2.0f);
    EXPECT_FLOAT_EQ(Math::Lerpc(2.0f, 8.0f, 2.0f), 8.0f);
    EXPECT_FLOAT_EQ(Math::Lerpc(2.0f, 8.0f, 0.5f), 5.0f);
}

TEST(Math, ClampInsideAndOutside) {
    EXPECT_EQ(Math::Clamp(5, 0, 10), 5);
    EXPECT_EQ(Math::Clamp(-5, 0, 10), 0);
    EXPECT_EQ(Math::Clamp(15, 0, 10), 10);
    EXPECT_EQ(Math::Clamp(0, 0, 10), 0);
    EXPECT_EQ(Math::Clamp(10, 0, 10), 10);

    EXPECT_FLOAT_EQ(Math::Clamp01(-0.5f), 0.0f);
    EXPECT_FLOAT_EQ(Math::Clamp01(1.5f), 1.0f);
    EXPECT_FLOAT_EQ(Math::Clamp01(0.5f), 0.5f);
}

TEST(Math, EqualsWithEpsilon) {
    EXPECT_TRUE(Math::Equals(1.0f, 1.0f + FLT_EPSILON * 0.5f));
    EXPECT_FALSE(Math::Equals(1.0f, 1.001f));

    EXPECT_TRUE(Math::Equals(1.0f, 1.005f, 0.01f));
    EXPECT_FALSE(Math::Equals(1.0f, 1.05f, 0.01f));
}

TEST(Math, SignReturnsPlusOrMinusOne) {
    EXPECT_EQ(Math::Sign(5), 1);
    EXPECT_EQ(Math::Sign(-5), -1);
    EXPECT_EQ(Math::Sign(0), 1);

    EXPECT_FLOAT_EQ(Math::Sign(3.5f), 1.0f);
    EXPECT_FLOAT_EQ(Math::Sign(-3.5f), -1.0f);
}

TEST(Math, AbsForSignedTypes) {
    EXPECT_EQ(Math::Abs(-7), 7);
    EXPECT_EQ(Math::Abs(7), 7);
    EXPECT_FLOAT_EQ(Math::Abs(-3.5f), 3.5f);
}

TEST(Math, MinMaxAndSwap) {
    EXPECT_EQ(Math::Min(3, 5), 3);
    EXPECT_EQ(Math::Max(3, 5), 5);
    EXPECT_FLOAT_EQ(Math::Min(-2.0f, -1.0f), -2.0f);

    int a = 1, b = 2;
    Math::Swap(a, b);
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 1);
}

TEST(Math, RoundFloorCeilOnBoundaries) {
    EXPECT_FLOAT_EQ(Math::Floor(2.7f), 2.0f);
    EXPECT_FLOAT_EQ(Math::Floor(-2.3f), -3.0f);

    EXPECT_FLOAT_EQ(Math::Ceil(2.3f), 3.0f);
    EXPECT_FLOAT_EQ(Math::Ceil(-2.7f), -2.0f);

    EXPECT_FLOAT_EQ(Math::Round(2.4f), 2.0f);
    EXPECT_FLOAT_EQ(Math::Round(2.6f), 3.0f);

    EXPECT_EQ(Math::FloorToInt(2.9f), 2);
    EXPECT_EQ(Math::CeilToInt(2.1f), 3);
    EXPECT_EQ(Math::RoundToInt(2.6f), 3);
}

TEST(Math, ModWrapsValue) {
    EXPECT_NEAR(Math::Mod(7.5f, 3.0f), 1.5f, 1e-5f);
    EXPECT_NEAR(Math::Mod(0.5f, 1.0f), 0.5f, 1e-5f);
}

TEST(Math, SqrtAndSqr) {
    EXPECT_FLOAT_EQ(Math::Sqrt(9.0f), 3.0f);
    EXPECT_FLOAT_EQ(Math::Sqrt(0.0f), 0.0f);

    EXPECT_EQ(Math::Sqr(5), 25);
    EXPECT_FLOAT_EQ(Math::Sqr(2.5f), 6.25f);
}

TEST(Math, DegRadRoundtrip) {
    const float angles[] = { 0.0f, 90.0f, 180.0f, 270.0f, 360.0f, 45.0f, -90.0f };
    for (float deg : angles)
        EXPECT_NEAR(Math::Rad2deg(Math::Deg2rad(deg)), deg, 1e-3f);

    EXPECT_NEAR(Math::Deg2rad(180.0f), Math::PI(), 1e-5f);
    EXPECT_NEAR(Math::Rad2deg(Math::PI()), 180.0f, 1e-3f);
}

TEST(Math, TrigBasicIdentities) {
    EXPECT_NEAR(Math::Sin(0.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(Math::Cos(0.0f), 1.0f, 1e-5f);
    EXPECT_NEAR(Math::Sin(Math::PI() * 0.5f), 1.0f, 1e-5f);
    EXPECT_NEAR(Math::Cos(Math::PI() * 0.5f), 0.0f, 1e-5f);

    const float a = 0.7f;
    EXPECT_NEAR(Math::Sqr(Math::Sin(a)) + Math::Sqr(Math::Cos(a)), 1.0f, 1e-5f);
}

TEST(Math, ASinACosInverseOfSinCos) {
    const float values[] = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };
    for (float v : values)
    {
        EXPECT_NEAR(Math::Sin(Math::ASin(v)), v, 1e-5f);
        EXPECT_NEAR(Math::Cos(Math::ACos(v)), v, 1e-5f);
    }
}

TEST(Math, Atan2FQuadrants) {
    EXPECT_NEAR(Math::Atan2F(0.0f, 1.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(Math::Atan2F(1.0f, 0.0f), Math::PI() * 0.5f, 1e-5f);
}

TEST(Math, RandomInRange) {
    for (int i = 0; i < 100; ++i)
    {
        int v = Math::Random(10, 20);
        EXPECT_GE(v, 10);
        EXPECT_LE(v, 20);
    }
}

TEST(Math, WrapAngleStaysInPiRange) {
    EXPECT_NEAR(Math::WrapAngle(0.5f), 0.5f, 1e-6f);
    EXPECT_NEAR(Math::WrapAngle(Math::PI() + 0.5f), -Math::PI() + 0.5f, 1e-5f);
    EXPECT_NEAR(Math::WrapAngle(-Math::PI() - 0.5f), Math::PI() - 0.5f, 1e-5f);
    EXPECT_NEAR(Math::WrapAngle(4.0f*Math::PI() + 0.1f), 0.1f, 1e-4f);
}
