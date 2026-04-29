#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(HorizontalProgress, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    ASSERT_TRUE(p);
    EXPECT_FLOAT_EQ(p->GetMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(p->GetMaxValue(), 1.0f);
}

TEST(HorizontalProgress, CopyPreservesOrientation)
{
    SceneCleanGuard guard;
    auto src = mmake<HorizontalProgress>();
    src->SetOrientation(HorizontalProgress::Orientation::Left);
    auto copy = src->CloneAsRef<HorizontalProgress>();
    EXPECT_EQ(copy->GetOrientation(), HorizontalProgress::Orientation::Left);
}

// ===== Range =====

TEST(HorizontalProgress, SetValueRangeStoresEndpoints)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    p->SetValueRange(5, 50);
    EXPECT_LE(p->GetMinValue(), 5);
    EXPECT_GE(p->GetMaxValue(), 50);
}

TEST(HorizontalProgress, SetMinValueRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    p->SetMinValue(2);
    EXPECT_FLOAT_EQ(p->GetMinValue(), 2);
}

TEST(HorizontalProgress, SetMaxValueRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    p->SetMaxValue(20);
    EXPECT_FLOAT_EQ(p->GetMaxValue(), 20);
}

// ===== Value =====

TEST(HorizontalProgress, SetValueForcibleRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    p->SetValueForcible(0.7f);
    EXPECT_NEAR(p->GetValue(), 0.7f, 0.001f);
}

TEST(HorizontalProgress, SetValueClampedToRange)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    p->SetValueRange(0, 100);
    p->SetValueForcible(150);
    EXPECT_LE(p->GetValue(), 100);
    p->SetValueForcible(-10);
    EXPECT_GE(p->GetValue(), 0);
}

// ===== Orientation =====

TEST(HorizontalProgress, SetOrientationRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    p->SetOrientation(HorizontalProgress::Orientation::Left);
    EXPECT_EQ(p->GetOrientation(), HorizontalProgress::Orientation::Left);
    p->SetOrientation(HorizontalProgress::Orientation::Right);
    EXPECT_EQ(p->GetOrientation(), HorizontalProgress::Orientation::Right);
}

// ===== Scroll sense =====

TEST(HorizontalProgress, SetScrollSenseRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    p->SetScrollSense(2.0f);
    EXPECT_FLOAT_EQ(p->GetScrollSense(), 2.0f);
}

// ===== Events =====

TEST(HorizontalProgress, OnChangeFiresOnSetValueForcible)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    int count = 0;
    p->onChange = [&](float) { count++; };
    p->SetValueForcible(0.5f);
    EXPECT_EQ(count, 1);
}

TEST(HorizontalProgress, OnChangeFiresOnSetValue)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    int count = 0;
    p->onChange = [&](float) { count++; };
    p->SetValue(0.3f);
    EXPECT_EQ(count, 1);
}

TEST(HorizontalProgress, OnChangeByUserFiresOnlyWhenByUserFlag)
{
    SceneCleanGuard guard;
    auto p = mmake<HorizontalProgress>();
    int byUserCount = 0;
    p->onChangeByUser = [&](float) { byUserCount++; };
    p->SetValue(0.4f, false);
    EXPECT_EQ(byUserCount, 0);
    p->SetValue(0.6f, true);
    EXPECT_EQ(byUserCount, 1);
}
