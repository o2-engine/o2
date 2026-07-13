#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationMask.h"
#include "o2/Animation/Tracks/AnimationSubTrack.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Components/SoundComponent.h"
#include "o2/Sound/SoundSystem.h"
#include "Scene/SceneTestHelpers.h"
#include "Sound/SoundTestHelpers.h"

using namespace o2;

TEST(SoundComponent, AddToActor)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = actor->AddComponent<SoundComponent>();

    ASSERT_TRUE(comp);
    EXPECT_EQ(comp->GetDuration(), 0.0f);
}

TEST(SoundComponent, TransformUpdatesSpatialPosition)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = actor->AddComponent<SoundComponent>();
    comp->SetSpatial(true);

    actor->transform->SetPosition(Vec3F(100, 50, 25));
    TickFrame();

    auto position = comp->GetPosition();
    EXPECT_NEAR(position.x, 100.0f, 0.001f);
    EXPECT_NEAR(position.y, 50.0f, 0.001f);
    EXPECT_NEAR(position.z, 25.0f, 0.001f);
}

TEST(SoundComponent, PlaysSoundViaComponentUpdate)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = actor->AddComponent<SoundComponent>();

    comp->SetSound(MakeTestSoundAsset(0.5f));
    comp->Play();

    comp->OnUpdate(0.2f);
    EXPECT_TRUE(comp->IsPlaying());
    EXPECT_NEAR(comp->GetTime(), 0.2f, 0.001f);

    comp->OnUpdate(0.4f);
    EXPECT_FALSE(comp->IsPlaying());
}

TEST(SoundComponent, SerializationRoundTrip)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = actor->AddComponent<SoundComponent>();

    comp->SetVolume(0.4f);
    comp->SetSpatial(true);

    DataDocument data;
    comp->Serialize(data);

    auto restored = mmake<SoundComponent>();
    restored->Deserialize(data);

    EXPECT_NEAR(restored->GetVolume(), 0.4f, 0.001f);
    EXPECT_TRUE(restored->IsSpatial());
}

TEST(SoundComponent, CloneCopiesParams)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = actor->AddComponent<SoundComponent>();

    comp->SetSound(MakeTestSoundAsset(0.5f));
    comp->SetVolume(0.3f);

    auto clone = comp->CloneAsRef<SoundComponent>();

    EXPECT_NEAR(clone->GetVolume(), 0.3f, 0.001f);
    EXPECT_NEAR(clone->GetDuration(), 0.5f, 0.01f);
}

TEST(SoundComponent, AnimationClipCreatesSubTrackForSoundComponent)
{
    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack("components/Sound", TypeOf(SoundComponent));

    ASSERT_TRUE(track);
    EXPECT_EQ(&track->GetType(), &TypeOf(AnimationSubTrack));
}

TEST(SoundComponent, BackendStopsWhenRemovedFromScene)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = actor->AddComponent<SoundComponent>();

    comp->SetSound(MakeTestSoundAsset(0.5f));
    comp->SetLoop(Loop::Repeat);
    comp->Play();
    comp->OnUpdate(0.05f);
    ASSERT_TRUE(comp->IsBackendPlaying());

    // Editor stop reloads the scene; some window may still hold a component reference,
    // so the destructor is not enough to silence the sound
    Ref<SoundComponent> zombie = comp;
    o2Scene.DestroyActor(actor);
    o2Scene.UpdateDestroyingEntities();

    EXPECT_FALSE(zombie->IsBackendPlaying());
}

TEST(SoundComponent, BackendStopsWhenDisabledAndResumesOnEnable)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = actor->AddComponent<SoundComponent>();

    comp->SetSound(MakeTestSoundAsset(0.5f));
    comp->SetLoop(Loop::Repeat);
    comp->Play();
    TickFrame(0.05f);
    ASSERT_TRUE(comp->IsBackendPlaying());

    comp->SetEnabled(false);
    EXPECT_FALSE(comp->IsBackendPlaying());

    comp->SetEnabled(true);
    comp->OnUpdate(0.05f);
    EXPECT_TRUE(comp->IsBackendPlaying());
}

TEST(SoundComponent, AnimationStateDrivesSoundViaSubTrack)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto sound = actor->AddComponent<SoundComponent>();
    sound->SetSound(MakeTestSoundAsset(0.5f));

    auto clip = mmake<AnimationClip>();
    auto track = DynamicCast<AnimationSubTrack>(clip->AddTrack("component/o2::SoundComponent", TypeOf(SoundComponent)));
    ASSERT_TRUE(track);
    track->SetBeginTime(1.0f);

    auto animComp = actor->AddComponent<AnimationComponent>();
    auto state = animComp->AddState("sound", clip, AnimationMask(), 1.0f);
    state->GetPlayer().Play();

    animComp->OnUpdate(0.5f);
    EXPECT_TRUE(sound->IsSubControlled());
    EXPECT_FALSE(sound->IsBackendPlaying());

    animComp->OnUpdate(0.7f);
    EXPECT_NEAR(sound->GetTime(), 0.2f, 0.01f);
    EXPECT_TRUE(sound->IsBackendPlaying());

    // Playhead left the sub track window: time sets stop coming, watchdog silences the sound
    for (int i = 0; i < 30; i++)
        o2Sounds.Update(0.016f);

    EXPECT_FALSE(sound->IsBackendPlaying());
}
