#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/Tracks/AnimationColor4Track.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Animation/Tracks/AnimationVec3FTrack.h"

using namespace o2;

// A clip can be longer than its tracks (sub-tracks, staggered keys): outside the keys every
// track type must hold its end values instead of extrapolating the last segment
TEST(AnimationTrackRange, FloatTrackHoldsEndValues)
{
    auto track = AnimationTrack<float>::Linear(0.0f, 1.0f, 1.0f);
    EXPECT_NEAR(track.GetValue(-1.0f), 0.0f, 0.001f);
    EXPECT_NEAR(track.GetValue(1.0f), 1.0f, 0.001f);
    EXPECT_NEAR(track.GetValue(3.0f), 1.0f, 0.001f);
}

TEST(AnimationTrackRange, FloatKeysHoldEndValues)
{
    AnimationTrack<float> track;
    track.AddKey(0.0f, 1.0f);
    track.AddKey(0.5f, 1.0f);
    track.AddKey(0.8f, 0.0f);

    EXPECT_NEAR(track.GetValue(0.9f), 0.0f, 0.001f);
    EXPECT_NEAR(track.GetValue(2.0f), 0.0f, 0.001f);
    EXPECT_NEAR(track.GetValue(-0.5f), 1.0f, 0.001f);
}

TEST(AnimationTrackRange, Vec3FTrackHoldsEndValues)
{
    AnimationTrack<Vec3F> track;
    track.AddKey(0.0f, Vec3F(0, 0, 0));
    track.AddKey(1.0f, Vec3F(1, 2, 3));

    Vec3F after = track.GetValue(2.0f);
    EXPECT_NEAR(after.x, 1.0f, 0.001f);
    EXPECT_NEAR(after.y, 2.0f, 0.001f);
    EXPECT_NEAR(after.z, 3.0f, 0.001f);

    Vec3F before = track.GetValue(-1.0f);
    EXPECT_NEAR(before.x, 0.0f, 0.001f);
}

TEST(AnimationTrackRange, Color4TrackHoldsEndValues)
{
    AnimationTrack<Color4> track;
    track.AddKey(0.0f, Color4(0, 0, 0, 255));
    track.AddKey(1.0f, Color4(255, 255, 255, 255));

    Color4 after = track.GetValue(3.0f);
    EXPECT_EQ(after.r, 255);
    EXPECT_EQ(after.g, 255);

    Color4 before = track.GetValue(-3.0f);
    EXPECT_EQ(before.r, 0);
}

TEST(AnimationTrackRange, Vec2FTrackHoldsEndValues)
{
    auto track = AnimationTrack<Vec2F>::EaseInOut(Vec2F(0, 0), Vec2F(10, 20), 1.0f);
    Vec2F after = track.GetValue(2.0f);
    EXPECT_NEAR(after.x, 10.0f, 0.01f);
    EXPECT_NEAR(after.y, 20.0f, 0.01f);
}
