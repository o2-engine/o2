#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(HorizontalScrollBar, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    ASSERT_TRUE(sb);
    EXPECT_FLOAT_EQ(sb->GetMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(sb->GetMaxValue(), 1.0f);
}

TEST(HorizontalScrollBar, CopyPreservesProperties)
{
    SceneCleanGuard guard;
    auto src = mmake<HorizontalScrollBar>();
    src->SetValueRange(0, 100);
    src->SetScrollHandleSize(0.5f);
    auto copy = src->CloneAsRef<HorizontalScrollBar>();
    EXPECT_FLOAT_EQ(copy->GetMaxValue(), 100);
    EXPECT_FLOAT_EQ(copy->GetScrollHandleSize(), 0.5f);
}

// ===== Range =====

TEST(HorizontalScrollBar, SetValueRangeRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    sb->SetValueRange(10, 200);
    EXPECT_FLOAT_EQ(sb->GetMinValue(), 10);
    EXPECT_FLOAT_EQ(sb->GetMaxValue(), 200);
}

// ===== Value =====

TEST(HorizontalScrollBar, SetValueForcibleRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    sb->SetValueRange(0, 100);
    sb->SetValueForcible(50);
    EXPECT_FLOAT_EQ(sb->GetValue(), 50);
}

TEST(HorizontalScrollBar, GetSmoothValueAfterForcibleEqualsValue)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    sb->SetValueForcible(0.5f);
    EXPECT_NEAR(sb->GetSmoothValue(), 0.5f, 0.001f);
}

// ===== Handle size =====

TEST(HorizontalScrollBar, SetScrollHandleSizeRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    sb->SetScrollHandleSize(0.3f);
    EXPECT_FLOAT_EQ(sb->GetScrollHandleSize(), 0.3f);
}

// ===== Scroll sense =====

TEST(HorizontalScrollBar, SetScrollSenseRoundTrip)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    sb->SetScrollSense(2.5f);
    EXPECT_FLOAT_EQ(sb->GetScrollSense(), 2.5f);
}

// ===== Events =====

TEST(HorizontalScrollBar, OnChangeFiresOnSetValueForcible)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    int count = 0;
    sb->onChange = [&](float) { count++; };
    sb->SetValueForcible(0.5f);
    EXPECT_EQ(count, 1);
}

TEST(HorizontalScrollBar, OnChangeFiresOnSetValue)
{
    SceneCleanGuard guard;
    auto sb = mmake<HorizontalScrollBar>();
    int count = 0;
    sb->onChange = [&](float) { count++; };
    sb->SetValue(0.25f);
    EXPECT_EQ(count, 1);
}
