#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(AnimationComponent, NewComponentHasNoStates)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();
    EXPECT_EQ(comp->GetStates().Count(), 0);
    EXPECT_EQ(comp->GetStatesNames().Count(), 0);
}

TEST(AnimationComponent, AddStateByNameCreatesAndReturnsState)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    auto state = comp->AddState("idle");

    ASSERT_TRUE(state);
    EXPECT_EQ(state->name, "idle");
    EXPECT_EQ(comp->GetStates().Count(), 1);
}

TEST(AnimationComponent, GetStateByNameReturnsAdded)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    auto state = comp->AddState("walk");
    EXPECT_EQ(comp->GetState("walk"), state);
}

TEST(AnimationComponent, GetStateMissingReturnsNull)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    EXPECT_FALSE(comp->GetState("missing_state"));
}

TEST(AnimationComponent, GetStatesNamesContainsAddedNames)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    comp->AddState("idle");
    comp->AddState("walk");
    comp->AddState("run");

    auto names = comp->GetStatesNames();
    EXPECT_EQ(names.Count(), 3);
    EXPECT_TRUE(names.Contains("idle"));
    EXPECT_TRUE(names.Contains("walk"));
    EXPECT_TRUE(names.Contains("run"));
}

TEST(AnimationComponent, RemoveStateByNameRemoves)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    comp->AddState("idle");
    comp->AddState("walk");

    comp->RemoveState("idle");

    EXPECT_EQ(comp->GetStates().Count(), 1);
    EXPECT_FALSE(comp->GetState("idle"));
    EXPECT_TRUE(comp->GetState("walk"));
}

TEST(AnimationComponent, RemoveAllStatesEmptiesList)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    comp->AddState("a");
    comp->AddState("b");
    comp->AddState("c");

    comp->RemoveAllStates();

    EXPECT_EQ(comp->GetStates().Count(), 0);
}

TEST(AnimationComponent, GetFirstStateReturnsAddedFirst)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    comp->AddState("first");
    comp->AddState("second");

    auto state = comp->GetFirstState();
    ASSERT_TRUE(state);
    EXPECT_EQ(state->name, "first");
}

TEST(AnimationComponent, GetFirstStateReturnsNullForEmpty)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    EXPECT_FALSE(comp->GetFirstState(false));
}

TEST(AnimationComponent, GetFirstStateCreatesIfMissingWhenFlagSet)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    auto state = comp->GetFirstState(true);
    ASSERT_TRUE(state);
    EXPECT_EQ(comp->GetStates().Count(), 1);
}

TEST(AnimationComponent, AttachesToActor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();
    EXPECT_EQ(a->GetComponent<AnimationComponent>(), comp);
}

// ===== Blending =====

TEST(AnimationComponent, SetWeightOnStateRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();
    auto state = comp->AddState("a");
    state->SetWeight(0.3f);
    EXPECT_FLOAT_EQ(state->GetWeight(), 0.3f);
}

TEST(AnimationComponent, BlendToInterpolatesWeightsLinearly)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();
    auto src = DynamicCast<AnimationState>(comp->AddState("src"));
    auto dst = DynamicCast<AnimationState>(comp->AddState("dst"));
    ASSERT_TRUE(src);
    ASSERT_TRUE(dst);

    // Source has to be playing for BlendTo to enroll it as a "blend off" state.
    src->GetPlayer().Play();

    comp->BlendTo(dst, 1.0f);

    // BlendState::Update interpolates linearly: coef = max(0, time)/duration.
    // After dt=0.5 with duration=1, coef=0.5 → src weight 0.5, dst weight 0.5.
    comp->OnUpdate(0.5f);
    EXPECT_NEAR(src->GetWeight(), 0.5f, 1e-3f);
    EXPECT_NEAR(dst->GetWeight(), 0.5f, 1e-3f);

    // After full duration, source is silent, target is at full weight.
    comp->OnUpdate(0.5f);
    EXPECT_NEAR(src->GetWeight(), 0.0f, 1e-3f);
    EXPECT_NEAR(dst->GetWeight(), 1.0f, 1e-3f);
}

TEST(AnimationComponent, StopAllResetsBlend)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();
    auto src = DynamicCast<AnimationState>(comp->AddState("src"));
    auto dst = DynamicCast<AnimationState>(comp->AddState("dst"));
    src->GetPlayer().Play();
    comp->BlendTo(dst, 1.0f);

    // Snapshot weight after partial advance, then StopAll: subsequent
    // OnUpdate must NOT continue blending.
    comp->OnUpdate(0.25f);
    float dstAfterStop = dst->GetWeight();
    comp->StopAll();
    comp->OnUpdate(0.5f);
    EXPECT_FLOAT_EQ(dst->GetWeight(), dstAfterStop);
}

TEST(AnimationComponent, MismatchedTrackTypeDoesNotCrash)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    // Vec2F track bound to Vec3F field "transform/scale": must warn, not crash
    auto clip = mmake<AnimationClip>();
    clip->AddTrack<Vec2F>("transform/scale");

    auto state = comp->AddState("mismatched", clip, AnimationMask(), 1.0f);
    EXPECT_TRUE(state);
}

TEST(AnimationComponent, Vec2FTrackOnScale2DBinds)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    auto scale = clip->AddTrack<Vec2F>("transform/scale2D");
    scale->spline->AppendKey(Vec2F(1.0f, 1.0f));
    scale->spline->AppendKey(Vec2F(0.5f, 0.5f));
    *scale->timeCurve = Curve::EaseInOut(0.0f, 1.0f, 0.5f);

    auto state = comp->AddState("fly", clip, AnimationMask(), 1.0f);
    ASSERT_TRUE(state);
}
