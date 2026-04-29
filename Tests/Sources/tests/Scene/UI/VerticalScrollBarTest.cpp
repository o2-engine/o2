#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(VerticalScrollBar, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    ASSERT_TRUE(sb);
    EXPECT_FLOAT_EQ(sb->GetMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(sb->GetMaxValue(), 1.0f);
}

TEST(VerticalScrollBar, CopyPreservesProperties)
{
    SceneCleanGuard guard;
    auto src = mmake<VerticalScrollBar>();
    src->SetValueRange(0, 50);
    src->SetScrollHandleSize(0.4f);
    auto copy = src->CloneAsRef<VerticalScrollBar>();
    EXPECT_FLOAT_EQ(copy->GetMaxValue(), 50);
    EXPECT_FLOAT_EQ(copy->GetScrollHandleSize(), 0.4f);
}

// ===== Range =====

TEST(VerticalScrollBar, SetValueRangeRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    sb->SetValueRange(5, 95);
    EXPECT_FLOAT_EQ(sb->GetMinValue(), 5);
    EXPECT_FLOAT_EQ(sb->GetMaxValue(), 95);
}

// ===== Value =====

TEST(VerticalScrollBar, SetValueForcibleRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    sb->SetValueRange(0, 100);
    sb->SetValueForcible(75);
    EXPECT_FLOAT_EQ(sb->GetValue(), 75);
}

TEST(VerticalScrollBar, GetSmoothValueAfterForcibleEqualsValue)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    sb->SetValueForcible(0.7f);
    EXPECT_NEAR(sb->GetSmoothValue(), 0.7f, 0.001f);
}

// ===== Handle size =====

TEST(VerticalScrollBar, SetScrollHandleSizeRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    sb->SetScrollHandleSize(0.6f);
    EXPECT_FLOAT_EQ(sb->GetScrollHandleSize(), 0.6f);
}

// ===== Scroll sense =====

TEST(VerticalScrollBar, SetScrollSenseRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    sb->SetScrollSense(1.7f);
    EXPECT_FLOAT_EQ(sb->GetScrollSense(), 1.7f);
}

// ===== Events =====

TEST(VerticalScrollBar, OnChangeFiresOnSetValueForcible)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    int count = 0;
    sb->onChange = [&](float) { count++; };
    sb->SetValueForcible(0.5f);
    EXPECT_EQ(count, 1);
}

TEST(VerticalScrollBar, OnChangeFiresOnSetValue)
{
    SceneCleanGuard guard;
    auto sb = mmake<VerticalScrollBar>();
    int count = 0;
    sb->onChange = [&](float) { count++; };
    sb->SetValue(0.25f);
    EXPECT_EQ(count, 1);
}
