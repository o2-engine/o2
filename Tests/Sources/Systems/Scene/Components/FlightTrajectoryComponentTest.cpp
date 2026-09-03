#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Math.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Components/FlightTrajectoryComponent.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

static Ref<Spline> MakeArcSpline(float width = 0.0f)
{
    auto spline = mmake<Spline>();
    spline->AppendKey(Vec2F(0, 0), 0.0f);
    spline->AppendKey(Vec2F(200, 130), width);
    spline->AppendKey(Vec2F(400, 0), 0.0f);
    return spline;
}

TEST(FlightTrajectory, EndpointsMapToStartAndFinish)
{
    auto trajectory = mmake<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();
    trajectory->SetPoints(-100, 50, 300, -200);

    Vec2F start = trajectory->EvaluatePoint(0.0f);
    Vec2F finish = trajectory->EvaluatePoint(1.0f);

    EXPECT_NEAR(start.x, -100.0f, 1.0f);
    EXPECT_NEAR(start.y, 50.0f, 1.0f);
    EXPECT_NEAR(finish.x, 300.0f, 1.0f);
    EXPECT_NEAR(finish.y, -200.0f, 1.0f);
}

TEST(FlightTrajectory, ArcBendsAwayFromStraightLine)
{
    auto trajectory = mmake<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();
    trajectory->SetPoints(0, 0, 400, 0);

    Vec2F mid = trajectory->EvaluatePoint(0.5f);
    EXPECT_GT(mid.y, 50.0f);
}

TEST(FlightTrajectory, FewKeysFallsBackToLerp)
{
    auto trajectory = mmake<FlightTrajectoryComponent>();
    trajectory->spline = mmake<Spline>();
    trajectory->SetPoints(0, 0, 100, 200);

    Vec2F mid = trajectory->EvaluatePoint(0.5f);
    EXPECT_NEAR(mid.x, 50.0f, 0.01f);
    EXPECT_NEAR(mid.y, 100.0f, 0.01f);
}

TEST(FlightTrajectory, PointsChangeRetargetsBasis)
{
    auto trajectory = mmake<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();

    trajectory->SetPoints(0, 0, 400, 0);
    trajectory->EvaluatePoint(1.0f);

    trajectory->SetPoints(10, 20, -300, 500);
    Vec2F finish = trajectory->EvaluatePoint(1.0f);
    EXPECT_NEAR(finish.x, -300.0f, 1.0f);
    EXPECT_NEAR(finish.y, 500.0f, 1.0f);
}

TEST(FlightTrajectory, ResetRandomOffsetVariesCorridorPoint)
{
    auto trajectory = mmake<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline(300.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    // wide corridor: the trajectory middle must vary after explicit rerolls
    bool varied = false;
    Vec2F prev = trajectory->EvaluatePoint(0.5f);
    for (int i = 0; i < 16 && !varied; i++)
    {
        trajectory->ResetRandomOffset();
        Vec2F mid = trajectory->EvaluatePoint(0.5f);
        varied = (mid - prev).Length() > 1.0f;
        prev = mid;
    }
    EXPECT_TRUE(varied);
}

// Entering zero is a flight start and rerolls the corridor offset; staying at zero or
// scrubbing mid-range keeps the trajectory stable
TEST(FlightTrajectory, PositionEnteringZeroRerollsOffset)
{
    Math::RandomScope random(7);

    auto trajectory = mmake<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline(300.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    trajectory->SetPosition(0.7f);
    float offset = trajectory->GetRandomOffset();

    trajectory->SetPosition(0.0f);
    EXPECT_NE(trajectory->GetRandomOffset(), offset) << "flight start picks a fresh offset";

    // repeated zero writes (idle mixers) don't reroll again
    offset = trajectory->GetRandomOffset();
    Vec2F mid = trajectory->EvaluatePoint(0.5f);
    for (int i = 0; i < 16; i++)
    {
        trajectory->SetPosition(0.0f);
        EXPECT_EQ(trajectory->GetRandomOffset(), offset);
    }

    // mid-range scrubbing (editor) keeps the trajectory
    for (float t : { 0.2f, 0.5f, 0.9f, 0.3f, 0.05f })
    {
        trajectory->SetPosition(t);
        EXPECT_EQ(trajectory->GetRandomOffset(), offset);
    }

    Vec2F again = trajectory->EvaluatePoint(0.5f);
    EXPECT_NEAR(mid.x, again.x, 0.001f);
    EXPECT_NEAR(mid.y, again.y, 0.001f);
}

TEST(FlightTrajectory, UpdateWritesTrajectoryPointToActorTransform)
{
    SceneCleanGuard sceneGuard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto trajectory = actor->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();
    trajectory->SetPoints(-50, -50, 350, 250);
    trajectory->position = 1.0f;

    TickFrames(2);

    Vec2F pos = actor->transform->GetPosition2D();
    EXPECT_NEAR(pos.x, 350.0f, 1.0f);
    EXPECT_NEAR(pos.y, 250.0f, 1.0f);
}

TEST(FlightTrajectory, AnimationTrackDrivesPosition)
{
    SceneCleanGuard sceneGuard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto animation = actor->AddComponent<AnimationComponent>();
    auto trajectory = actor->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();
    trajectory->SetPoints(0, 0, 400, 0);

    auto clip = mmake<AnimationClip>();
    *clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position") =
        AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.5f);

    auto state = animation->AddState("flight", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    animation->RewindAndPlay("flight");
    TickFrames(10, 0.05f);

    EXPECT_GT(trajectory->position, 0.5f);
}

TEST(FlightTrajectory, AnimationDrivesPositionOnClonedActor)
{
    SceneCleanGuard sceneGuard;

    auto proto = mmake<Actor>(ActorCreateMode::NotInScene);
    auto animation = proto->AddComponent<AnimationComponent>();
    auto trajectory = proto->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();

    auto clip = mmake<AnimationClip>();
    *clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position") =
        AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.5f);
    auto state = animation->AddState("flight", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto clone = proto->CloneAsRef<Actor>();
    parent->AddChild(clone);
    clone->SetEnabled(true);

    auto cloneTrajectory = clone->GetComponent<FlightTrajectoryComponent>();
    ASSERT_TRUE(cloneTrajectory);
    cloneTrajectory->SetPoints(0, 0, 400, 0);

    clone->GetComponent<AnimationComponent>()->RewindAndPlay("flight");
    TickFrames(10, 0.05f);

    EXPECT_GT(cloneTrajectory->position, 0.5f);
}

TEST(FlightTrajectory, AnimationDrivesPositionOnDeserializedActor)
{
    SceneCleanGuard sceneGuard;

    auto proto = mmake<Actor>(ActorCreateMode::NotInScene);
    auto animation = proto->AddComponent<AnimationComponent>();
    auto trajectory = proto->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();

    auto clip = mmake<AnimationClip>();
    *clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position") =
        AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.5f);
    auto state = animation->AddState("flight", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    DataDocument data;
    proto->Serialize(data);

    auto restored = mmake<Actor>(ActorCreateMode::NotInScene);
    restored->Deserialize(data);

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(restored);
    restored->SetEnabled(true);

    auto restoredTrajectory = restored->GetComponent<FlightTrajectoryComponent>();
    ASSERT_TRUE(restoredTrajectory);
    restoredTrajectory->SetPoints(0, 0, 400, 0);

    auto restoredAnimation = restored->GetComponent<AnimationComponent>();
    ASSERT_TRUE(restoredAnimation);
    restoredAnimation->RewindAndPlay("flight");
    TickFrames(10, 0.05f);

    EXPECT_GT(restoredTrajectory->position, 0.5f);
}

// Editor scrubbing: the player writes position through the property without any scene update
TEST(FlightTrajectory, AnimationPlayerScrubMovesActorWithoutSceneUpdate)
{
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);
    auto trajectory = actor->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();
    trajectory->SetPoints(0, 0, 400, 0);

    auto clip = mmake<AnimationClip>();
    *clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position") =
        AnimationTrack<float>::Linear(0.0f, 1.0f, 1.0f);

    auto player = mmake<AnimationPlayer>(actor.Get(), clip);
    player->SetTime(1.0f);

    EXPECT_NEAR(trajectory->GetPosition(), 1.0f, 0.001f);
    EXPECT_NEAR(actor->transform->GetPosition2D().x, 400.0f, 1.0f);

    player->SetTime(0.0f);
    EXPECT_NEAR(actor->transform->GetPosition2D().x, 0.0f, 1.0f);
}

TEST(FlightTrajectory, PositionSurvivesSerialization)
{
    auto trajectory = mmake<FlightTrajectoryComponent>();
    trajectory->spline = MakeArcSpline();
    trajectory->position = 0.25f;

    DataDocument data;
    trajectory->Serialize(data);

    auto restored = mmake<FlightTrajectoryComponent>();
    restored->Deserialize(data);
    EXPECT_NEAR(restored->GetPosition(), 0.25f, 0.001f);
}
