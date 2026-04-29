#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/Toggle.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Toggle: Construction =====

TEST(Toggle, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto t = mmake<Toggle>();
    ASSERT_TRUE(t);
    EXPECT_FALSE(t->GetValue());
    EXPECT_TRUE(t->IsFocusable());
}

TEST(Toggle, CopyPreservesValue)
{
    SceneCleanGuard guard;
    auto src = mmake<Toggle>();
    src->SetValue(true);
    auto copy = src->CloneAsRef<Toggle>();
    EXPECT_TRUE(copy->GetValue());
}

// ===== Toggle: Value =====

TEST(Toggle, SetValueRoundTrip)
{
    SceneCleanGuard guard;
    auto t = mmake<Toggle>();
    t->SetValue(true);
    EXPECT_TRUE(t->GetValue());
    t->SetValue(false);
    EXPECT_FALSE(t->GetValue());
}

TEST(Toggle, SetValueUnknown)
{
    SceneCleanGuard guard;
    auto t = mmake<Toggle>();
    t->SetValueUnknown();
    EXPECT_TRUE(t->IsValueUnknown());
    t->SetValue(true);
    EXPECT_FALSE(t->IsValueUnknown());
}

// ===== Toggle: Caption =====

TEST(Toggle, SetCaptionWithoutStyleIsNoOp)
{
    SceneCleanGuard guard;
    auto t = mmake<Toggle>();
    t->SetCaption("hello");
    EXPECT_TRUE(t->GetCaption().IsEmpty());
}

// ===== Toggle: Events =====

TEST(Toggle, OnToggleFiresExactlyOnceOnSetValueChange)
{
    SceneCleanGuard guard;
    auto t = mmake<Toggle>();
    int toggleCount = 0;
    t->onToggle = [&](bool) { toggleCount++; };

    t->SetValue(true);
    EXPECT_EQ(toggleCount, 1);
}

TEST(Toggle, OnToggleNotFiredWhenValueDoesNotChange)
{
    SceneCleanGuard guard;
    auto t = mmake<Toggle>();
    t->SetValue(true);
    int toggleCount = 0;
    t->onToggle = [&](bool) { toggleCount++; };

    t->SetValue(true);
    EXPECT_EQ(toggleCount, 0);
}

// ===== ToggleGroup =====

TEST(ToggleGroup, AddRemoveToggleRoundTrip)
{
    SceneCleanGuard guard;
    auto group = mmake<ToggleGroup>(ToggleGroup::Type::OnlySingleTrue);
    auto t = mmake<Toggle>();
    group->AddToggle(t);
    EXPECT_GE(group->GetToggles().Count(), 1);
    group->RemoveToggle(t.Get());
    EXPECT_EQ(group->GetToggles().Count(), 0);
}

TEST(Toggle, SetToggleGroupAssignsGroup)
{
    SceneCleanGuard guard;
    auto t = mmake<Toggle>();
    auto group = mmake<ToggleGroup>(ToggleGroup::Type::OnlySingleTrue);
    t->SetToggleGroup(group);
    EXPECT_EQ(t->GetToggleGroup(), group);
}

TEST(ToggleGroup, OnlySingleTrueDeselectsPreviousWhenSecondActivated)
{
    SceneCleanGuard guard;
    auto group = mmake<ToggleGroup>(ToggleGroup::Type::OnlySingleTrue);
    auto a = mmake<Toggle>();
    auto b = mmake<Toggle>();
    a->SetToggleGroup(group);
    b->SetToggleGroup(group);

    a->SetValue(true);
    b->SetValue(true);

    EXPECT_FALSE(a->GetValue());
    EXPECT_TRUE(b->GetValue());
}

TEST(ToggleGroup, OnlySingleTrueRestoresValueWhenLastActiveDeactivated)
{
    SceneCleanGuard guard;
    auto group = mmake<ToggleGroup>(ToggleGroup::Type::OnlySingleTrue);
    auto a = mmake<Toggle>();
    a->SetToggleGroup(group);

    a->SetValue(true);
    a->SetValue(false);

    // Group resists having no active toggle when nothing else is on.
    EXPECT_TRUE(a->GetValue());
}
