#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetState.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(WidgetState, DefaultConstructionIsFalse)
{
    auto s = mmake<WidgetState>();
    EXPECT_FALSE(s->GetState());
}

TEST(WidgetState, CopyConstructorPreservesNameAndState)
{
    auto src = mmake<WidgetState>();
    src->name = "active";
    src->SetStateForcible(true);
    auto copy = src->CloneAsRef<WidgetState>();
    EXPECT_EQ(copy->name, "active");
    EXPECT_TRUE(copy->GetState());
}

// ===== Operators =====

TEST(WidgetState, BoolCastReturnsCurrentState)
{
    auto s = mmake<WidgetState>();
    s->SetStateForcible(true);
    EXPECT_TRUE(static_cast<bool>(*s));
    s->SetStateForcible(false);
    EXPECT_FALSE(static_cast<bool>(*s));
}

TEST(WidgetState, AssignFromBoolSetsState)
{
    auto s = mmake<WidgetState>();
    *s = true;
    EXPECT_TRUE(s->GetState());
    *s = false;
    EXPECT_FALSE(s->GetState());
}

// WidgetState::operator== currently always returns false (see WidgetState.cpp).
// Lock that contract here; if the implementation is later changed to compare
// fields, this test should be rewritten to check real equality semantics.
TEST(WidgetState, EqualsOperatorAlwaysReturnsFalse)
{
    auto a = mmake<WidgetState>();
    a->name = "x";
    auto b = mmake<WidgetState>();
    b->name = "x";
    EXPECT_FALSE(*a == *b);

    auto self = mmake<WidgetState>();
    EXPECT_FALSE(*self == *self);
}

// ===== State value =====

TEST(WidgetState, SetStateAndGetStateRoundTrip)
{
    auto s = mmake<WidgetState>();
    s->SetState(true);
    EXPECT_TRUE(s->GetState());
    s->SetState(false);
    EXPECT_FALSE(s->GetState());
}

TEST(WidgetState, SetStateForcibleSetsImmediately)
{
    auto s = mmake<WidgetState>();
    s->SetStateForcible(true);
    EXPECT_TRUE(s->GetState());
}

// ===== Animation =====

TEST(WidgetState, SetAnimationClipRoundTrip)
{
    auto s = mmake<WidgetState>();
    auto clip = mmake<AnimationClip>();
    s->SetAnimationClip(clip);
    EXPECT_EQ(s->GetAnimationClip(), clip);
}

TEST(WidgetState, GetAnimationPlayerNotNull)
{
    auto s = mmake<WidgetState>();
    EXPECT_TRUE(s->GetAnimationPlayer());
}

// ===== Events =====

TEST(WidgetState, OnStateBecomesTrueFiresOnceOnTransitionToTrue)
{
    auto s = mmake<WidgetState>();
    int trueCount = 0;
    int falseCount = 0;
    s->onStateBecomesTrue = [&]() { trueCount++; };
    s->onStateBecomesFalse = [&]() { falseCount++; };

    s->SetState(true);
    EXPECT_EQ(trueCount, 1);
    EXPECT_EQ(falseCount, 0);
    s->SetState(false);
    EXPECT_EQ(trueCount, 1);
    EXPECT_EQ(falseCount, 1);
}

TEST(WidgetState, SetStateForcibleFiresBothBecomesAndFullyCallbacks)
{
    auto s = mmake<WidgetState>();
    int becomes = 0;
    int fully = 0;
    s->onStateBecomesTrue = [&]() { becomes++; };
    s->onStateFullyTrue = [&]() { fully++; };
    s->SetStateForcible(true);
    EXPECT_EQ(becomes, 1);
    EXPECT_EQ(fully, 1);
}
