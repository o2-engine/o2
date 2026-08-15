#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Animation/Tracks/AnimationSubTrack.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Components/SoundComponent.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"
#include "Sound/SoundTestHelpers.h"

using namespace o2;

// Sub-tracks target sibling components (sounds, particles) by reflection path, so one
// animation state can drive sprite tracks and effects together
TEST(AnimationComponentSubTrack, AddTrackByAnimatableComponentTypeCreatesSubTrack)
{
    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack("component/o2::SoundComponent", TypeOf(SoundComponent));

    ASSERT_TRUE(track);
    EXPECT_TRUE(DynamicCast<AnimationSubTrack>(track));
}

TEST(AnimationComponentSubTrack, StateBindsSubTracksToComponentsByPath)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
    auto sound = actor->AddComponent<SoundComponent>();
    sound->SetSound(MakeTestSoundAsset(0.5f));

    auto animation = actor->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    clip->AddTrack("component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));
    clip->AddTrack("component/o2::SoundComponent", TypeOf(SoundComponent));

    auto state = animation->AddState("fx", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    EXPECT_TRUE(emitter->IsSubControlled());
    EXPECT_TRUE(sound->IsSubControlled());
}

TEST(AnimationComponentSubTrack, PlayingStateDrivesSubTrackTargets)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto sound = actor->AddComponent<SoundComponent>();
    sound->SetSound(MakeTestSoundAsset(0.5f));

    auto animation = actor->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    clip->AddTrack("component/o2::SoundComponent", TypeOf(SoundComponent));

    auto state = animation->AddState("fx", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    TickFrame();
    EXPECT_NEAR(sound->GetTime(), 0.0f, 0.001f);

    auto played = animation->Play("fx");
    ASSERT_TRUE(played);

    // The clip duration comes from the bound target (the sound length)
    EXPECT_NEAR(played->GetDuration(), 0.5f, 0.01f);

    TickFrame(0.2f);
    EXPECT_NEAR(sound->GetTime(), 0.2f, 0.01f);
}

// The saved scene keeps states with embedded clips; loading must rebind sub-track targets
TEST(AnimationComponentSubTrack, SceneRoundTripRebindsSubTracks)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    actor->SetName("FxActor");
    actor->AddComponent<ParticlesEmitterComponent>();

    auto animation = actor->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    clip->AddTrack("component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto state = animation->AddState("fx", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    TickFrame();

    DataDocument document;
    o2Scene.Save(document);
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
    o2Scene.Load(document);
    TickFrame();

    auto loaded = o2Scene.FindActor("FxActor");
    ASSERT_TRUE(loaded);

    auto loadedEmitter = loaded->GetComponent<ParticlesEmitterComponent>();
    ASSERT_TRUE(loadedEmitter);
    EXPECT_TRUE(loadedEmitter->IsSubControlled());

    auto loadedAnimation = loaded->GetComponent<AnimationComponent>();
    ASSERT_TRUE(loadedAnimation);
    EXPECT_TRUE(loadedAnimation->GetStatesNames().Contains(String("fx")));
}

// Wrong component path (e.g. name without namespace) must warn, not crash on null target
TEST(AnimationComponentSubTrack, UnresolvedSubTrackPathDoesNotCrash)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    actor->AddComponent<ParticlesEmitterComponent>();
    auto animation = actor->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    clip->AddTrack("component/ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto state = animation->AddState("fx", clip, AnimationMask(), 1.0f);
    EXPECT_TRUE(state);
}
