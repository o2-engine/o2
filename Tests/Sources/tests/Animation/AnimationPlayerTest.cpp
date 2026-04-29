#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"

using namespace o2;

TEST(AnimationPlayer, DefaultIsNotPlaying)
{
    auto player = mmake<AnimationPlayer>();
    EXPECT_FALSE(player->IsPlaying());
    EXPECT_FLOAT_EQ(player->GetTime(), 0.0f);
}

TEST(AnimationPlayer, SetClipUpdatesDuration)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 2.5f);
    player->SetClip(clip);
    EXPECT_EQ(player->GetClip(), clip);
    EXPECT_NEAR(player->GetDuration(), 2.5f, 0.001f);
}

TEST(AnimationPlayer, PlaySetsPlayingTrue)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 1.0f);
    player->SetClip(clip);
    player->Play();
    EXPECT_TRUE(player->IsPlaying());
}

TEST(AnimationPlayer, StopSetsPlayingFalse)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 1.0f);
    player->SetClip(clip);
    player->Play();
    player->Stop();
    EXPECT_FALSE(player->IsPlaying());
}

TEST(AnimationPlayer, UpdateAdvancesTime)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 10.0f);
    player->SetClip(clip);
    player->Play();
    player->Update(0.5f);
    EXPECT_NEAR(player->GetTime(), 0.5f, 0.001f);
    player->Update(0.25f);
    EXPECT_NEAR(player->GetTime(), 0.75f, 0.001f);
}

TEST(AnimationPlayer, UpdateOnStoppedPlayerDoesNotAdvanceTime)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 10.0f);
    player->SetClip(clip);
    // Don't call Play()
    player->Update(0.5f);
    EXPECT_FLOAT_EQ(player->GetTime(), 0.0f);
}

TEST(AnimationPlayer, OnPlayCallbackFiresOnPlay)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 1.0f);
    player->SetClip(clip);
    int playCount = 0;
    player->onPlay = [&]() { playCount++; };
    player->Play();
    EXPECT_GE(playCount, 1);
}

TEST(AnimationPlayer, SetSpeedChangesAdvanceRate)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 10.0f);
    player->SetClip(clip);
    player->SetSpeed(2.0f);
    player->Play();
    player->Update(0.5f);
    EXPECT_NEAR(player->GetTime(), 1.0f, 0.001f);
}

TEST(AnimationPlayer, SetReverseFlipsAdvanceDirection)
{
    auto player = mmake<AnimationPlayer>();
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 10.0f);
    player->SetClip(clip);
    player->SetTime(5.0f);
    player->SetReverse(true);
    player->Play();
    player->Update(1.0f);
    EXPECT_LT(player->GetTime(), 5.0f);
}
