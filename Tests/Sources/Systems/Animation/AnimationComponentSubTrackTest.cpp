#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Animation/Tracks/AnimationSubTrack.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"
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

// A sub-track whose window ends before the clip does hands the target its end time once:
// a one-shot emitter must not stay frozen with its last dying particles
TEST(AnimationComponentSubTrack, LeavingSubTrackWindowFinishesTheTarget)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(0.1f);
    emitter->SetParticlesLifetime(0.2f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(50);
    emitter->SetLoop(Loop::None);
    emitter->Stop();

    auto animation = actor->AddComponent<AnimationComponent>();
    auto clip = mmake<AnimationClip>();
    clip->AddTrack<float>("transform/angleDegrees")->AddKey(2.0f, 90.0f);
    auto burst = DynamicCast<AnimationSubTrack>(
        clip->AddTrack("component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent)));
    burst->SetBeginTime(0.5f);

    auto state = animation->AddState("fx", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    auto alive = [&]()
    {
        return emitter->GetParticles().Count([](const Particle& p) { return p.alive; });
    };

    TickFrame();
    ASSERT_TRUE(animation->Play("fx"));

    TickFrame(0.6f);
    EXPECT_GT(alive(), 0) << "inside the window the burst is running";

    TickFrame(0.25f); // 0.85: past the window end (0.5 + 0.1 + 0.2) in one step
    EXPECT_EQ(alive(), 0);
}

// Rewinding the clip ahead of the sub-track window resets the target: a replayed effect
// must not show the particles of the previous run until its window opens again
TEST(AnimationComponentSubTrack, RewindAheadOfSubTrackWindowResetsTheTarget)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(0.1f);
    emitter->SetParticlesLifetime(0.4f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(50);
    emitter->SetLoop(Loop::None);
    emitter->Stop();

    auto animation = actor->AddComponent<AnimationComponent>();
    auto clip = mmake<AnimationClip>();
    clip->AddTrack<float>("transform/angleDegrees")->AddKey(2.0f, 90.0f);
    auto burst = DynamicCast<AnimationSubTrack>(
        clip->AddTrack("component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent)));
    burst->SetBeginTime(0.5f);

    auto state = animation->AddState("fx", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    auto alive = [&]()
    {
        return emitter->GetParticles().Count([](const Particle& p) { return p.alive; });
    };

    TickFrame();
    auto player = animation->Play("fx");
    ASSERT_TRUE(player);
    TickFrame(0.7f);
    ASSERT_GT(alive(), 0) << "inside the window the burst is running";

    animation->RewindAndPlay("fx");
    TickFrame(0.05f);
    EXPECT_EQ(alive(), 0) << "ahead of the window the target holds its start state";

    TickFrame(0.6f);
    EXPECT_GT(alive(), 0) << "the window opens again on the replay";
}

// A state's playback speed is scriptable: the game stretches one clip over a computed
// flight time instead of authoring a clip per distance
TEST(AnimationComponentSubTrack, StateSpeedStretchesPlayback)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto animation = actor->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack<float>("transform/angleDegrees");
    track->AddKey(0.0f, 0.0f);
    track->AddKey(1.0f, 90.0f);

    auto state = animation->AddState("fx", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;
    state->SetSpeed(0.5f); // вдвое медленнее

    TickFrame();
    ASSERT_TRUE(animation->Play("fx"));

    TickFrame(0.5f);
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 22.5f, 3.0f)
        << "half the clip time at half speed is a quarter of the way";

    state->SetSpeed(2.0f);
    TickFrame(0.25f);
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 67.5f, 4.0f);
    EXPECT_NEAR(state->GetSpeed(), 2.0f, 0.001f);
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
