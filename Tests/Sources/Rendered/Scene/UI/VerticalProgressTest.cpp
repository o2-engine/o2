#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/VerticalProgress.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(VerticalProgress, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    ASSERT_TRUE(p);
    EXPECT_FLOAT_EQ(p->GetMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(p->GetMaxValue(), 1.0f);
}

TEST(VerticalProgress, CopyPreservesProperties)
{
    SceneCleanGuard guard;
    auto src = mmake<VerticalProgress>();
    src->SetValueRange(0, 50);
    src->SetValueForcible(25);
    auto copy = src->CloneAsRef<VerticalProgress>();
    EXPECT_FLOAT_EQ(copy->GetMaxValue(), 50);
}

// ===== Range =====

TEST(VerticalProgress, SetValueRangeStoresEndpoints)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    p->SetValueRange(5, 50);
    EXPECT_LE(p->GetMinValue(), 5);
    EXPECT_GE(p->GetMaxValue(), 50);
}

// ===== Value =====

TEST(VerticalProgress, SetValueForcibleRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    p->SetValueForcible(0.4f);
    EXPECT_NEAR(p->GetValue(), 0.4f, 0.001f);
}

TEST(VerticalProgress, SetValueClampedToRange)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    p->SetValueRange(0, 10);
    p->SetValueForcible(50);
    EXPECT_LE(p->GetValue(), 10);
    p->SetValueForcible(-5);
    EXPECT_GE(p->GetValue(), 0);
}

// ===== Orientation =====

TEST(VerticalProgress, SetOrientationRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    p->SetOrientation(VerticalProgress::Orientation::Down);
    EXPECT_EQ(p->GetOrientation(), VerticalProgress::Orientation::Down);
    p->SetOrientation(VerticalProgress::Orientation::Up);
    EXPECT_EQ(p->GetOrientation(), VerticalProgress::Orientation::Up);
}

// ===== Scroll sense =====

TEST(VerticalProgress, SetScrollSenseRoundTrip)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    p->SetScrollSense(3.0f);
    EXPECT_FLOAT_EQ(p->GetScrollSense(), 3.0f);
}

// ===== Events =====

TEST(VerticalProgress, OnChangeFiresOnSetValueForcible)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    int count = 0;
    p->onChange = [&](float) { count++; };
    p->SetValueForcible(0.5f);
    EXPECT_EQ(count, 1);
}

TEST(VerticalProgress, OnChangeFiresOnSetValue)
{
    SceneCleanGuard guard;
    auto p = mmake<VerticalProgress>();
    int count = 0;
    p->onChange = [&](float) { count++; };
    p->SetValue(0.25f);
    EXPECT_EQ(count, 1);
}
