#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/Tracks/AnimationTrack.h"

using namespace o2;

TEST(AnimationClip, DefaultConstructionIsEmpty)
{
    auto clip = mmake<AnimationClip>();
    EXPECT_EQ(clip->GetTracks().Count(), 0);
    EXPECT_FLOAT_EQ(clip->GetDuration(), 0.0f);
    EXPECT_EQ(clip->GetLoop(), Loop::None);
}

TEST(AnimationClip, SetLoopRoundTrip)
{
    auto clip = mmake<AnimationClip>();
    clip->SetLoop(Loop::Repeat);
    EXPECT_EQ(clip->GetLoop(), Loop::Repeat);
    clip->SetLoop(Loop::PingPong);
    EXPECT_EQ(clip->GetLoop(), Loop::PingPong);
}

TEST(AnimationClip, AddTrackByTypeAddsAndContains)
{
    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack<float>("position");
    ASSERT_TRUE(track);
    EXPECT_EQ(clip->GetTracks().Count(), 1);
    EXPECT_TRUE(clip->ContainsTrack("position"));
}

TEST(AnimationClip, GetTrackByPathReturnsAdded)
{
    auto clip = mmake<AnimationClip>();
    auto added = clip->AddTrack<float>("alpha");
    EXPECT_EQ(clip->GetTrack("alpha"), added);
}

TEST(AnimationClip, GetTrackByTypeReturnsTypedReference)
{
    auto clip = mmake<AnimationClip>();
    auto added = clip->AddTrack<float>("alpha");
    auto fetched = clip->GetTrackByType<float>("alpha");
    EXPECT_EQ(fetched, added);
}

TEST(AnimationClip, GetTrackOnUnknownPathReturnsNull)
{
    auto clip = mmake<AnimationClip>();
    EXPECT_FALSE(clip->GetTrack("missing"));
    EXPECT_FALSE(clip->ContainsTrack("missing"));
}

TEST(AnimationClip, RemoveTrackByPathRemoves)
{
    auto clip = mmake<AnimationClip>();
    clip->AddTrack<float>("a");
    clip->AddTrack<float>("b");
    clip->RemoveTrack("a");
    EXPECT_EQ(clip->GetTracks().Count(), 1);
    EXPECT_FALSE(clip->ContainsTrack("a"));
    EXPECT_TRUE(clip->ContainsTrack("b"));
}

TEST(AnimationClip, ClearRemovesAllTracks)
{
    auto clip = mmake<AnimationClip>();
    clip->AddTrack<float>("a");
    clip->AddTrack<float>("b");
    clip->Clear();
    EXPECT_EQ(clip->GetTracks().Count(), 0);
}

TEST(AnimationClip, OnTrackAddedCallbackFires)
{
    auto clip = mmake<AnimationClip>();
    int addedCount = 0;
    clip->onTrackAdded = [&](const Ref<IAnimationTrack>&) { addedCount++; };
    clip->AddTrack<float>("x");
    EXPECT_EQ(addedCount, 1);
}

TEST(AnimationClip, LinearFactoryCreatesClipWithSingleTrackAndDuration)
{
    auto clip = AnimationClip::Linear<float>("alpha", 0.0f, 1.0f, 2.0f);
    ASSERT_TRUE(clip);
    EXPECT_EQ(clip->GetTracks().Count(), 1);
    EXPECT_TRUE(clip->ContainsTrack("alpha"));
    EXPECT_NEAR(clip->GetDuration(), 2.0f, 0.001f);
}

TEST(AnimationClip, CopyConstructorClonesTracksAndLoop)
{
    auto src = mmake<AnimationClip>();
    src->SetLoop(Loop::Repeat);
    src->AddTrack<float>("a");

    auto copy = src->CloneAsRef<AnimationClip>();
    EXPECT_EQ(copy->GetLoop(), Loop::Repeat);
    EXPECT_EQ(copy->GetTracks().Count(), 1);
}
