#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Spline.h"

using namespace o2;

TEST(Spline, DefaultIsEmpty)
{
    Spline s;
    EXPECT_TRUE(s.IsEmpty());
    EXPECT_EQ(s.GetKeys().Count(), 0);
    EXPECT_FLOAT_EQ(s.Length(), 0.0f);
}

TEST(Spline, AppendKeyAddsKeyAndReturnsIndex)
{
    Spline s;
    int idx = s.AppendKey(Vec2F(0, 0));
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(s.GetKeys().Count(), 1);
    EXPECT_FALSE(s.IsEmpty());
}

TEST(Spline, AppendThreeKeysReportsThree)
{
    Spline s;
    s.AppendKey(Vec2F(0, 0));
    s.AppendKey(Vec2F(10, 0));
    s.AppendKey(Vec2F(10, 10));
    EXPECT_EQ(s.GetKeys().Count(), 3);
}

TEST(Spline, RemoveKeyByIndex)
{
    Spline s;
    s.AppendKey(Vec2F(0, 0));
    s.AppendKey(Vec2F(1, 0));
    EXPECT_TRUE(s.RemoveKey(0));
    EXPECT_EQ(s.GetKeys().Count(), 1);
}

TEST(Spline, RemoveAllKeysClears)
{
    Spline s;
    s.AppendKey(Vec2F(0, 0));
    s.AppendKey(Vec2F(1, 0));
    s.RemoveAllKeys();
    EXPECT_TRUE(s.IsEmpty());
}

TEST(Spline, OnKeysChangedFiresOnAppend)
{
    Spline s;
    int callCount = 0;
    s.onKeysChanged = [&]() { callCount++; };
    s.AppendKey(Vec2F(0, 0));
    EXPECT_GE(callCount, 1);
}

TEST(Spline, GetKeyByIndexReturnsExpectedValue)
{
    Spline s;
    s.AppendKey(Vec2F(5, 7));
    auto key = s.GetKey(0);
    EXPECT_EQ(key.value, Vec2F(5, 7));
}

TEST(Spline, IndexOperatorReturnsKey)
{
    Spline s;
    s.AppendKey(Vec2F(3, 4));
    EXPECT_EQ(s[0].value, Vec2F(3, 4));
}

// Two keys at one point form a zero-length segment: evaluating there gives that point, not a 0/0 blend
TEST(Spline, CoincidentKeysEvaluateToTheirPoint)
{
    Spline s;
    s.AppendKey(Vec2F(0, 0), 0.0f, Vec2F(), Vec2F());
    s.AppendKey(Vec2F(0, 0), 0.0f, Vec2F(), Vec2F());
    s.AppendKey(Vec2F(4, 0), 0.0f, Vec2F(), Vec2F());

    Vec2F value = s.Evaluate(0.0f);
    EXPECT_FALSE(std::isnan(value.x) || std::isnan(value.y));
    EXPECT_NEAR(value.x, 0.0f, 0.001f);
    EXPECT_NEAR(value.y, 0.0f, 0.001f);
}
