#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayerLayout.h"
#include "o2/Utils/Math/Layout.h"

using namespace o2;

// ===== Construction =====

TEST(WidgetLayerLayout, DefaultConstructionIsBothStretch)
{
    WidgetLayerLayout l;
    EXPECT_EQ(l.GetAnchorMin(), Vec2F(0, 0));
    EXPECT_EQ(l.GetAnchorMax(), Vec2F(1, 1));
    EXPECT_EQ(l.GetOffsetMin(), Vec2F(0, 0));
    EXPECT_EQ(l.GetOffsetMax(), Vec2F(0, 0));
}

TEST(WidgetLayerLayout, CopyConstructorPreservesValues)
{
    WidgetLayerLayout src;
    Layout custom;
    custom.anchorMin = Vec2F(0.1f, 0.2f);
    custom.anchorMax = Vec2F(0.9f, 0.8f);
    custom.offsetMin = Vec2F(1, 2);
    custom.offsetMax = Vec2F(3, 4);
    src = custom;
    WidgetLayerLayout copy(src);
    EXPECT_TRUE(copy == src);
}

// ===== Operators =====

TEST(WidgetLayerLayout, AssignFromLayoutRoundTrip)
{
    WidgetLayerLayout l;
    Layout other;
    other.anchorMin = Vec2F(0.25f, 0.5f);
    other.anchorMax = Vec2F(0.75f, 1.0f);
    other.offsetMin = Vec2F(1, 2);
    other.offsetMax = Vec2F(3, 4);
    l = other;
    EXPECT_EQ(l.GetAnchorMin(), other.anchorMin);
    EXPECT_EQ(l.GetAnchorMax(), other.anchorMax);
    EXPECT_EQ(l.GetOffsetMin(), other.offsetMin);
    EXPECT_EQ(l.GetOffsetMax(), other.offsetMax);
}

TEST(WidgetLayerLayout, CastToLayoutPreservesValues)
{
    WidgetLayerLayout l;
    Layout custom;
    custom.anchorMin = Vec2F(0.1f, 0.2f);
    custom.anchorMax = Vec2F(0.7f, 0.8f);
    custom.offsetMin = Vec2F(1, 2);
    custom.offsetMax = Vec2F(3, 4);
    l = custom;

    Layout asLayout = static_cast<Layout>(l);
    EXPECT_EQ(asLayout.anchorMin, l.GetAnchorMin());
    EXPECT_EQ(asLayout.anchorMax, l.GetAnchorMax());
    EXPECT_EQ(asLayout.offsetMin, l.GetOffsetMin());
    EXPECT_EQ(asLayout.offsetMax, l.GetOffsetMax());
}

TEST(WidgetLayerLayout, EqualsAndNotEqualsWithSelfType)
{
    WidgetLayerLayout a;
    WidgetLayerLayout b;
    EXPECT_TRUE(a == b);

    Layout custom;
    custom.anchorMin = Vec2F(0.5f, 0.5f);
    custom.anchorMax = Vec2F(1, 1);
    b = custom;
    EXPECT_TRUE(a != b);
}

TEST(WidgetLayerLayout, EqualsWithLayoutChecksFields)
{
    WidgetLayerLayout a;
    Layout b;
    b.anchorMin = Vec2F(0, 0);
    b.anchorMax = Vec2F(1, 1);
    b.offsetMin = Vec2F(0, 0);
    b.offsetMax = Vec2F(0, 0);
    EXPECT_TRUE(a == b);
    b.offsetMax = Vec2F(5, 5);
    EXPECT_TRUE(a != b);
}

// ===== Anchors via owner WidgetLayer =====

TEST(WidgetLayerLayout, SetAnchorMinMaxRoundTripWithOwner)
{
    auto layer = mmake<WidgetLayer>();
    layer->layout.SetAnchorMin(Vec2F(0.1f, 0.2f));
    layer->layout.SetAnchorMax(Vec2F(0.7f, 0.8f));
    EXPECT_EQ(layer->layout.GetAnchorMin(), Vec2F(0.1f, 0.2f));
    EXPECT_EQ(layer->layout.GetAnchorMax(), Vec2F(0.7f, 0.8f));
}

TEST(WidgetLayerLayout, SetAnchorEdgesWithOwner)
{
    auto layer = mmake<WidgetLayer>();
    layer->layout.SetAnchorLeft(0.1f);
    layer->layout.SetAnchorRight(0.9f);
    layer->layout.SetAnchorBottom(0.2f);
    layer->layout.SetAnchorTop(0.8f);
    EXPECT_FLOAT_EQ(layer->layout.GetAnchorLeft(), 0.1f);
    EXPECT_FLOAT_EQ(layer->layout.GetAnchorRight(), 0.9f);
    EXPECT_FLOAT_EQ(layer->layout.GetAnchorBottom(), 0.2f);
    EXPECT_FLOAT_EQ(layer->layout.GetAnchorTop(), 0.8f);
}

// ===== Offsets via owner WidgetLayer =====

TEST(WidgetLayerLayout, SetOffsetMinMaxRoundTripWithOwner)
{
    auto layer = mmake<WidgetLayer>();
    layer->layout.SetOffsetMin(Vec2F(5, 10));
    layer->layout.SetOffsetMax(Vec2F(15, 20));
    EXPECT_EQ(layer->layout.GetOffsetMin(), Vec2F(5, 10));
    EXPECT_EQ(layer->layout.GetOffsetMax(), Vec2F(15, 20));
}

TEST(WidgetLayerLayout, SetOffsetEdgesWithOwner)
{
    auto layer = mmake<WidgetLayer>();
    layer->layout.SetOffsetLeft(5);
    layer->layout.SetoffsetRight(15);
    layer->layout.SetOffsetBottom(7);
    layer->layout.SetOffsetTop(17);
    EXPECT_FLOAT_EQ(layer->layout.GetOffsetLeft(), 5);
    EXPECT_FLOAT_EQ(layer->layout.GetoffsetRight(), 15);
    EXPECT_FLOAT_EQ(layer->layout.GetOffsetBottom(), 7);
    EXPECT_FLOAT_EQ(layer->layout.GetOffsetTop(), 17);
}

// ===== Calculate =====

TEST(WidgetLayerLayout, CalculateBothStretchEqualsSource)
{
    WidgetLayerLayout l;
    RectF source(0, 0, 100, 50);
    auto result = l.Calculate(source);
    EXPECT_NEAR(result.left, 0, 0.01f);
    EXPECT_NEAR(result.bottom, 0, 0.01f);
    EXPECT_NEAR(result.right, 100, 0.01f);
    EXPECT_NEAR(result.top, 50, 0.01f);
}

TEST(WidgetLayerLayout, CalculateWithCenterAnchorAndZeroSize)
{
    auto layer = mmake<WidgetLayer>();
    layer->layout.SetAnchorMin(Vec2F(0.5f, 0.5f));
    layer->layout.SetAnchorMax(Vec2F(0.5f, 0.5f));
    RectF source(0, 0, 100, 50);
    auto result = layer->layout.Calculate(source);
    EXPECT_NEAR(result.left, 50, 0.01f);
    EXPECT_NEAR(result.bottom, 25, 0.01f);
    EXPECT_NEAR(result.right, 50, 0.01f);
    EXPECT_NEAR(result.top, 25, 0.01f);
}

TEST(WidgetLayerLayout, CalculateWithFixedOffsetFromCorner)
{
    auto layer = mmake<WidgetLayer>();
    layer->layout.SetAnchorMin(Vec2F(0, 0));
    layer->layout.SetAnchorMax(Vec2F(0, 0));
    layer->layout.SetOffsetMin(Vec2F(10, 20));
    layer->layout.SetOffsetMax(Vec2F(40, 60));
    RectF source(0, 0, 200, 100);
    auto result = layer->layout.Calculate(source);
    EXPECT_NEAR(result.left, 10, 0.01f);
    EXPECT_NEAR(result.bottom, 20, 0.01f);
    EXPECT_NEAR(result.right, 40, 0.01f);
    EXPECT_NEAR(result.top, 60, 0.01f);
}

// Position/Size getters depend on owner widget rect; not exercised in isolation.
