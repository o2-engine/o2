#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Scene/Actor.h"

using namespace o2;

// Players follow their tracks' keys changes; a rebound or destroyed player must leave no delegate
// behind, otherwise the next keys change writes into freed memory
TEST(AnimationPlayerRebind, KeysChangeAfterRebindTouchesOnlyLivePlayers)
{
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);

    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack<float>("transform/angleDegrees");
    track->AddKey(0.0f, 0.0f);
    track->AddKey(1.0f, 90.0f);

    auto player = mmake<AnimationPlayer>(actor.Get(), clip);
    auto firstPlayers = player->GetTrackPlayers();
    ASSERT_EQ(firstPlayers.Count(), 1);

    // rebinding drops the old track players
    player->SetClip(clip);
    EXPECT_NE(player->GetTrackPlayers()[0], firstPlayers[0]);
    firstPlayers.Clear();

    // and the keys change must reach the live player only
    track->AddKey(2.0f, 180.0f);
    EXPECT_NEAR(player->GetDuration(), 2.0f, 0.001f);
    player->SetTime(2.0f);
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 180.0f, 0.5f);

    // destroyed player: the track must not keep its subscription
    player = nullptr;
    track->AddKey(3.0f, 270.0f);
    EXPECT_NEAR(clip->GetDuration(), 3.0f, 0.001f);
}

// The old lambda captured the track itself: tracks leaked through their own keys event
TEST(AnimationPlayerRebind, BoundTrackIsReleasedWithTheClip)
{
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);

    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack<float>("transform/angleDegrees");
    track->AddKey(0.0f, 0.0f);
    track->AddKey(1.0f, 90.0f);

    WeakRef<AnimationTrack<float>> trackLink = track;
    {
        auto player = mmake<AnimationPlayer>(actor.Get(), clip);
        player->SetTime(0.5f);
    }

    track = nullptr;
    clip = nullptr;
    EXPECT_FALSE(trackLink.IsValid()) << "track must die with its clip and players";
}
