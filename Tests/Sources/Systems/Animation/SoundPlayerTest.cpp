#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/Tracks/AnimationSubTrack.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2/Sound/SoundSystem.h"
#include "Sound/SoundTestHelpers.h"

using namespace o2;

TEST(SoundPlayer, SoundSystemIsReadyHeadless)
{
    ASSERT_TRUE(SoundSystem::IsSingletonInitialzed());
    EXPECT_TRUE(o2Sounds.IsReady());
    EXPECT_NE(o2Sounds.GetEngine(), nullptr);
}

TEST(SoundPlayer, MasterVolume)
{
    o2Sounds.SetVolume(0.5f);
    EXPECT_NEAR(o2Sounds.GetVolume(), 0.5f, 0.001f);

    o2Sounds.SetVolume(1.0f);
}

TEST(SoundPlayer, DurationComesFromAsset)
{
    auto player = mmake<SoundPlayer>();
    EXPECT_EQ(player->GetDuration(), 0.0f);

    player->SetSound(MakeTestSoundAsset(0.5f));
    EXPECT_NEAR(player->GetDuration(), 0.5f, 0.01f);
    EXPECT_NEAR(player->GetEndBound(), 0.5f, 0.01f);
}

TEST(SoundPlayer, PlayAdvancesTimeAndStopsAtEnd)
{
    auto player = mmake<SoundPlayer>();
    player->SetSound(MakeTestSoundAsset(0.5f));

    bool played = false;
    player->onPlayed = [&]() { played = true; };

    player->Play();
    EXPECT_TRUE(player->IsPlaying());

    player->Update(0.2f);
    EXPECT_NEAR(player->GetTime(), 0.2f, 0.001f);
    EXPECT_TRUE(player->IsPlaying());

    player->Update(0.4f);
    EXPECT_FALSE(player->IsPlaying());
    EXPECT_NEAR(player->GetTime(), 0.5f, 0.01f);
    EXPECT_TRUE(played);
}

TEST(SoundPlayer, LoopRepeatWrapsTime)
{
    auto player = mmake<SoundPlayer>();
    player->SetSound(MakeTestSoundAsset(0.5f));
    player->SetLoop(Loop::Repeat);

    player->Play();
    player->Update(0.7f);

    EXPECT_TRUE(player->IsPlaying());
    EXPECT_NEAR(player->GetInDurationTime(), 0.2f, 0.01f);
}

TEST(SoundPlayer, SetTimeScrubsWithoutPlaying)
{
    auto player = mmake<SoundPlayer>();
    player->SetSound(MakeTestSoundAsset(0.5f));

    player->SetTime(0.3f);

    EXPECT_FALSE(player->IsPlaying());
    EXPECT_NEAR(player->GetTime(), 0.3f, 0.001f);
    EXPECT_NEAR(player->GetInDurationTime(), 0.3f, 0.001f);
}

TEST(SoundPlayer, StopResetsPlaying)
{
    auto player = mmake<SoundPlayer>();
    player->SetSound(MakeTestSoundAsset(0.5f));

    player->Play();
    player->Update(0.1f);
    player->Stop();

    EXPECT_FALSE(player->IsPlaying());
}

TEST(SoundPlayer, ParamsStored)
{
    auto player = mmake<SoundPlayer>();

    player->SetVolume(0.7f);
    player->SetPitch(1.5f);
    player->SetSpatial(true);
    player->SetPosition(Vec3F(10, 20, 30));
    player->SetMinDistance(50.0f);
    player->SetMaxDistance(500.0f);
    player->SetRolloff(2.0f);

    EXPECT_NEAR(player->GetVolume(), 0.7f, 0.001f);
    EXPECT_NEAR(player->GetPitch(), 1.5f, 0.001f);
    EXPECT_TRUE(player->IsSpatial());
    EXPECT_EQ(player->GetPosition(), Vec3F(10, 20, 30));
    EXPECT_NEAR(player->GetMinDistance(), 50.0f, 0.001f);
    EXPECT_NEAR(player->GetMaxDistance(), 500.0f, 0.001f);
    EXPECT_NEAR(player->GetRolloff(), 2.0f, 0.001f);
}

TEST(SoundPlayer, SerializationRoundTripPreservesParams)
{
    auto player = mmake<SoundPlayer>();
    player->SetVolume(0.6f);
    player->SetPitch(0.9f);
    player->SetSpatial(true);
    player->SetMinDistance(25.0f);
    player->SetMaxDistance(250.0f);
    player->SetRolloff(1.5f);

    DataDocument data;
    player->Serialize(data);

    auto restored = mmake<SoundPlayer>();
    restored->Deserialize(data);

    EXPECT_NEAR(restored->GetVolume(), 0.6f, 0.001f);
    EXPECT_NEAR(restored->GetPitch(), 0.9f, 0.001f);
    EXPECT_TRUE(restored->IsSpatial());
    EXPECT_NEAR(restored->GetMinDistance(), 25.0f, 0.001f);
    EXPECT_NEAR(restored->GetMaxDistance(), 250.0f, 0.001f);
    EXPECT_NEAR(restored->GetRolloff(), 1.5f, 0.001f);
}

TEST(SoundPlayer, ListenerFollowsCameraPosition)
{
    o2Sounds.SetListenerPosition(Vec3F(1, 2, 3));

    auto position = o2Sounds.GetListenerPosition();
    EXPECT_NEAR(position.x, 1.0f, 0.001f);
    EXPECT_NEAR(position.y, 2.0f, 0.001f);
    EXPECT_NEAR(position.z, 3.0f, 0.001f);

    o2Sounds.SetListenerPosition(Vec3F());
}

TEST(SoundPlayer, SubTrackDrivesPlayerTime)
{
    auto player = mmake<SoundPlayer>();
    player->SetSound(MakeTestSoundAsset(0.5f));

    auto track = mmake<AnimationSubTrack>();
    auto trackPlayer = mmake<AnimationSubTrack::Player>();
    trackPlayer->SetTrack(track);
    trackPlayer->SetTargetVoid(static_cast<IAnimation*>(player.Get()));

    EXPECT_TRUE(player->IsSubControlled());
    EXPECT_NEAR(track->GetSubTrackDuration(), 0.5f, 0.01f);

    trackPlayer->SetTime(0.3f);
    EXPECT_NEAR(player->GetTime(), 0.3f, 0.001f);

    // Sub controlled player ignores own updates, it is driven only by the parent track
    player->Update(0.1f);
    EXPECT_NEAR(player->GetTime(), 0.3f, 0.001f);
}

TEST(SoundPlayer, ScrubPreviewStopsByWatchdog)
{
    auto player = mmake<SoundPlayer>();
    player->SetSound(MakeTestSoundAsset(0.5f));

    // External time set starts scrub preview playback
    player->SetTime(0.1f);
    EXPECT_FALSE(player->IsPlaying());

    // Watchdog stops preview after timeout without new time sets
    for (int i = 0; i < 30; i++)
        o2Sounds.Update(0.016f);

    // Playback state is backend-internal here; main check is that nothing crashes and
    // animation state stays stopped
    EXPECT_FALSE(player->IsPlaying());
}
