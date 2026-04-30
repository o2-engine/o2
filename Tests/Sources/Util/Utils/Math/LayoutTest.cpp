#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Layout.h"

using namespace o2;

TEST(Layout, DefaultIsBothStretch)
{
    Layout l;
    EXPECT_EQ(l.anchorMin, Vec2F(0, 0));
    EXPECT_EQ(l.anchorMax, Vec2F(1, 1));
    EXPECT_EQ(l.offsetMin, Vec2F(0, 0));
    EXPECT_EQ(l.offsetMax, Vec2F(0, 0));
}

TEST(Layout, ConstructorStoresAllFields)
{
    Layout l(Vec2F(0.1f, 0.2f), Vec2F(0.7f, 0.8f), Vec2F(1, 2), Vec2F(3, 4));
    EXPECT_EQ(l.anchorMin, Vec2F(0.1f, 0.2f));
    EXPECT_EQ(l.anchorMax, Vec2F(0.7f, 0.8f));
    EXPECT_EQ(l.offsetMin, Vec2F(1, 2));
    EXPECT_EQ(l.offsetMax, Vec2F(3, 4));
}

TEST(Layout, EqualsIsFieldwise)
{
    Layout a(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(0, 0));
    Layout b(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(0, 0));
    Layout c(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(1, 1));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

TEST(Layout, CalculateBothStretchEqualsSourceRect)
{
    Layout l;
    RectF source(0, 0, 100, 50);
    auto result = l.Calculate(source);
    EXPECT_FLOAT_EQ(result.left, 0);
    EXPECT_FLOAT_EQ(result.bottom, 0);
    EXPECT_FLOAT_EQ(result.right, 100);
    EXPECT_FLOAT_EQ(result.top, 50);
}

TEST(Layout, CalculateCenterAnchorWithSizeOffsetReturnsCenteredRect)
{
    auto l = Layout::Based(BaseCorner::Center, Vec2F(40, 20));
    RectF source(0, 0, 200, 100);
    auto result = l.Calculate(source);
    EXPECT_FLOAT_EQ(result.left, 80);
    EXPECT_FLOAT_EQ(result.right, 120);
    EXPECT_FLOAT_EQ(result.bottom, 40);
    EXPECT_FLOAT_EQ(result.top, 60);
}

TEST(Layout, CalculateOffsetTranslatesResultRect)
{
    Layout l(Vec2F(0, 0), Vec2F(0, 0), Vec2F(10, 20), Vec2F(40, 60));
    RectF source(0, 0, 200, 100);
    auto result = l.Calculate(source);
    EXPECT_FLOAT_EQ(result.left, 10);
    EXPECT_FLOAT_EQ(result.bottom, 20);
    EXPECT_FLOAT_EQ(result.right, 40);
    EXPECT_FLOAT_EQ(result.top, 60);
}

TEST(Layout, BothStretchWithBordersInsetsOffsets)
{
    auto l = Layout::BothStretch(10, 20, 30, 40);
    EXPECT_EQ(l.anchorMin, Vec2F(0, 0));
    EXPECT_EQ(l.anchorMax, Vec2F(1, 1));
    EXPECT_EQ(l.offsetMin, Vec2F(10, 20));
    EXPECT_EQ(l.offsetMax, Vec2F(-30, -40));
}

TEST(Layout, BasedLeftBottomZerosAnchors)
{
    auto l = Layout::Based(BaseCorner::LeftBottom, Vec2F(50, 30));
    EXPECT_EQ(l.anchorMin, Vec2F(0, 0));
    EXPECT_EQ(l.anchorMax, Vec2F(0, 0));
    EXPECT_EQ(l.offsetMin, Vec2F(0, 0));
    EXPECT_EQ(l.offsetMax, Vec2F(50, 30));
}

TEST(Layout, HorStretchHasFullHorizontalAnchors)
{
    auto l = Layout::HorStretch(VerAlign::Middle, 10, 20, 50);
    EXPECT_FLOAT_EQ(l.anchorMin.x, 0);
    EXPECT_FLOAT_EQ(l.anchorMax.x, 1);
    EXPECT_FLOAT_EQ(l.offsetMin.x, 10);
    EXPECT_FLOAT_EQ(l.offsetMax.x, -20);
}

TEST(Layout, VerStretchHasFullVerticalAnchors)
{
    auto l = Layout::VerStretch(HorAlign::Middle, 10, 20, 50);
    EXPECT_FLOAT_EQ(l.anchorMin.y, 0);
    EXPECT_FLOAT_EQ(l.anchorMax.y, 1);
    EXPECT_FLOAT_EQ(l.offsetMin.y, 20);
    EXPECT_FLOAT_EQ(l.offsetMax.y, -10);
}
