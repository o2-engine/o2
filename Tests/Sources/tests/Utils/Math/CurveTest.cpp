#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Curve.h"

using namespace o2;

TEST(Curve, DefaultIsEmpty)
{
    Curve c;
    EXPECT_EQ(c.GetKeys().Count(), 0);
    EXPECT_FLOAT_EQ(c.Length(), 0.0f);
}

TEST(Curve, AppendKeyExtendsLength)
{
    Curve c;
    c.AppendKey(1.0f, 0.0f);
    c.AppendKey(2.0f, 1.0f);
    EXPECT_EQ(c.GetKeys().Count(), 2);
    EXPECT_FLOAT_EQ(c.Length(), 3.0f);
}

TEST(Curve, RemoveKeyByPosition)
{
    Curve c;
    c.InsertKey(0.0f, 0.0f);
    c.InsertKey(1.0f, 1.0f);
    EXPECT_TRUE(c.RemoveKey(0.0f));
    EXPECT_EQ(c.GetKeys().Count(), 1);
    EXPECT_FALSE(c.RemoveKey(99.0f));
}

TEST(Curve, ContainsKeyMatchesInserted)
{
    Curve c;
    c.InsertKey(0.5f, 0.0f);
    EXPECT_TRUE(c.ContainsKey(0.5f));
    EXPECT_FALSE(c.ContainsKey(0.51f));
}

TEST(Curve, EvaluateAtKeyPositionReturnsKeyValue)
{
    Curve c;
    c.InsertKey(0.0f, 10.0f);
    c.InsertKey(1.0f, 20.0f);
    EXPECT_NEAR(c.Evaluate(0.0f), 10.0f, 0.001f);
    EXPECT_NEAR(c.Evaluate(1.0f), 20.0f, 0.001f);
}

TEST(Curve, LinearFactoryEvaluatesLinearly)
{
    Curve c = Curve::Linear(0.0f, 1.0f, 1.0f);
    EXPECT_NEAR(c.Evaluate(0.0f), 0.0f, 0.01f);
    EXPECT_NEAR(c.Evaluate(0.5f), 0.5f, 0.01f);
    EXPECT_NEAR(c.Evaluate(1.0f), 1.0f, 0.01f);
}

TEST(Curve, MoveKeysShiftsAllPositions)
{
    Curve c;
    c.AppendKey(1.0f, 0.0f);
    c.AppendKey(2.0f, 1.0f);
    float originalLength = c.Length();
    c.MoveKeys(5.0f);
    EXPECT_FLOAT_EQ(c.Length(), originalLength + 5.0f);
}

TEST(Curve, OnKeysChangedFiresOnInsertKey)
{
    Curve c;
    int callCount = 0;
    c.onKeysChanged = [&]() { callCount++; };
    c.InsertKey(0.0f, 0.0f);
    EXPECT_GE(callCount, 1);
}

TEST(Curve, EqualsComparesKeys)
{
    Curve a;
    a.InsertKey(0.0f, 1.0f);
    Curve b;
    b.InsertKey(0.0f, 1.0f);
    EXPECT_TRUE(a == b);
    b.InsertKey(2.0f, 3.0f);
    EXPECT_TRUE(a != b);
}
