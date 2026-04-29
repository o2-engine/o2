#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Border.h"

using namespace o2;

TEST(Border, DefaultConstructorZeroes)
{
    BorderF b;
    EXPECT_FLOAT_EQ(b.left, 0);
    EXPECT_FLOAT_EQ(b.bottom, 0);
    EXPECT_FLOAT_EQ(b.right, 0);
    EXPECT_FLOAT_EQ(b.top, 0);
}

TEST(Border, FieldsConstructor)
{
    BorderF b(1, 2, 3, 4);
    EXPECT_FLOAT_EQ(b.left, 1);
    EXPECT_FLOAT_EQ(b.bottom, 2);
    EXPECT_FLOAT_EQ(b.right, 3);
    EXPECT_FLOAT_EQ(b.top, 4);
}

TEST(Border, ConvertingConstructorIntToFloat)
{
    BorderI src(1, 2, 3, 4);
    BorderF dst(src);
    EXPECT_FLOAT_EQ(dst.left, 1);
    EXPECT_FLOAT_EQ(dst.top, 4);
}

TEST(Border, EqualsCompareAllFields)
{
    BorderF a(1, 2, 3, 4);
    BorderF b(1, 2, 3, 4);
    BorderF c(1, 2, 3, 5);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

TEST(Border, AddSubtractMultiplyDivideAreFieldwise)
{
    BorderF a(10, 20, 30, 40);
    BorderF b(1, 2, 3, 4);

    auto sum = a + b;
    EXPECT_FLOAT_EQ(sum.left, 11);
    EXPECT_FLOAT_EQ(sum.top, 44);

    BorderF c(10, 20, 30, 40);
    auto diff = c - b;
    EXPECT_FLOAT_EQ(diff.left, 9);
    EXPECT_FLOAT_EQ(diff.top, 36);

    BorderF d(10, 20, 30, 40);
    auto prod = d * b;
    EXPECT_FLOAT_EQ(prod.right, 90);

    BorderF e(10, 20, 30, 40);
    auto quot = e / b;
    EXPECT_FLOAT_EQ(quot.left, 10);
    EXPECT_FLOAT_EQ(quot.bottom, 10);
}

TEST(Border, CornerAccessorsReturnExpectedVec2)
{
    BorderF b(1, 2, 3, 4);
    EXPECT_EQ(b.LeftBottom(), Vec2F(1, 2));
    EXPECT_EQ(b.RightBottom(), Vec2F(3, 2));
    EXPECT_EQ(b.LeftTop(), Vec2F(1, 4));
    EXPECT_EQ(b.RightTop(), Vec2F(3, 4));
}
