#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/SkinnedModelAnimation.h"
#include "o2/Animation/Tracks/AnimationVec3FTrack.h"

using namespace o2;

namespace
{
    // Two nodes model: root and a child joint with a rotation clip crossing the ±180 degrees seam
    SkinnedModelData BuildTwoNodeModel()
    {
        SkinnedModelData model;

        SkinnedModelData::Node root;
        root.name = "root";

        SkinnedModelData::Node child;
        child.name = "spine/low"; // Path separator must be sanitized in the actor name
        child.parent = 0;
        child.position = Vec3F(0, 10, 0);

        model.nodes = { root, child };
        model.joints = { 0, 1 };
        model.inverseBindMatrices = { Mat4::Identity(), Mat4::Translation(Vec3F(0, -10, 0)) };

        return model;
    }

    float MaxTransformError(const Quat& reference, const Vec3F& euler)
    {
        Quat fromEuler = Quat::FromEuler(euler);

        float maxError = 0.0f;
        Vec3F probes[3] = { Vec3F(1, 0, 0), Vec3F(0, 1, 0), Vec3F(0, 0, 1) };
        for (auto& probe : probes)
            maxError = Math::Max(maxError, (reference*probe - fromEuler*probe).Length());

        return maxError;
    }
}

TEST(SkinnedModelAnimation, BoneNamesAndPathsAreSanitized)
{
    SkinnedModelData model = BuildTwoNodeModel();

    EXPECT_EQ(SkinnedModelAnimation::GetBoneActorName(model, 0), "root");
    EXPECT_EQ(SkinnedModelAnimation::GetBoneActorName(model, 1), "spine_low");
    EXPECT_EQ(SkinnedModelAnimation::GetBoneActorPath(model, 1), "child/root/child/spine_low");

    SkinnedModelData unnamed;
    unnamed.nodes.Add(SkinnedModelData::Node());
    EXPECT_EQ(SkinnedModelAnimation::GetBoneActorName(unnamed, 0), "node0");
}

// Euler keys converted from quaternions must interpolate along the quaternion arc:
// per-component lerp of the unwrapped eulers stays close to slerp between the keys
TEST(SkinnedModelAnimation, EulerConversionFollowsQuaternionArc)
{
    // A full turn around Z crosses the atan2 ±180 seam twice; extra X tilt engages all components
    Vector<Quat> rotations;
    const int keysCount = 13;
    for (int i = 0; i < keysCount; i++)
    {
        float angle = Math::PI()*2.0f*(float)i/(float)(keysCount - 1);
        rotations.Add(Quat::FromEuler(Vec3F(0.4f, 0.0f, 0.0f))*Quat::FromAxisAngle(Vec3F(0, 0, 1), angle));
    }

    Vector<Vec3F> eulers = SkinnedModelAnimation::ConvertRotationKeysToEuler(rotations);
    ASSERT_EQ(eulers.Count(), rotations.Count());

    for (int i = 0; i + 1 < eulers.Count(); i++)
    {
        // Keys don't jump: neighbour unwrapped eulers stay within the keys angular step
        EXPECT_LT((eulers[i + 1] - eulers[i]).Length(), 1.5f) << "euler keys must not jump at key " << i;

        for (float coef = 0.0f; coef <= 1.001f; coef += 0.25f)
        {
            Quat reference = Quat::Slerp(rotations[i], rotations[i + 1], coef);
            Vec3F interpolated = Math::Lerp(eulers[i], eulers[i + 1], coef);

            EXPECT_LT(MaxTransformError(reference, interpolated), 0.06f)
                << "euler interpolation diverged from slerp at key " << i << " coef " << coef;
        }
    }
}

TEST(SkinnedModelAnimation, ConvertClipBuildsTracksOnBonePaths)
{
    SkinnedModelData model = BuildTwoNodeModel();

    SkinnedModelData::AnimationChannel translation;
    translation.node = 1;
    translation.path = SkinnedModelData::AnimationChannel::Path::Translation;
    translation.times = { 0.0f, 0.5f, 1.5f };
    translation.vectors = { Vec3F(0, 10, 0), Vec3F(5, 10, 0), Vec3F(10, 10, 0) };

    SkinnedModelData::AnimationChannel rotation;
    rotation.node = 1;
    rotation.path = SkinnedModelData::AnimationChannel::Path::Rotation;
    rotation.times = { 0.0f, 1.5f };
    rotation.rotations = { Quat::Identity(), Quat::FromAxisAngle(Vec3F(0, 0, 1), 1.0f) };

    SkinnedModelData::AnimationClip clip;
    clip.name = "move";
    clip.duration = 1.5f;
    clip.channels = { translation, rotation };
    model.animations.Add(clip);

    auto converted = SkinnedModelAnimation::ConvertClip(model, "move");
    ASSERT_TRUE(converted);

    EXPECT_NEAR(converted->GetDuration(), 1.5f, 0.001f);
    ASSERT_EQ(converted->GetTracks().Count(), 2);

    auto positionTrack = converted->GetTrackByType<Vec3F>("child/root/child/spine_low/transform/position");
    ASSERT_TRUE(positionTrack);
    ASSERT_EQ(positionTrack->GetKeys().Count(), 3);
    EXPECT_NEAR(positionTrack->GetValue(0.25f).x, 2.5f, 0.001f);
    EXPECT_NEAR(positionTrack->GetValue(1.0f).x, 7.5f, 0.001f);

    auto rotationTrack = converted->GetTrackByType<Vec3F>("child/root/child/spine_low/transform/eulerAngles");
    ASSERT_TRUE(rotationTrack);
    ASSERT_EQ(rotationTrack->GetKeys().Count(), 2);
    EXPECT_NEAR(rotationTrack->GetValue(1.5f).z, 1.0f, 0.001f);

    EXPECT_FALSE(SkinnedModelAnimation::ConvertClip(model, "missing"));
}

// STEP channels hold the previous value until the next key
TEST(SkinnedModelAnimation, StepChannelHoldsValues)
{
    SkinnedModelData model = BuildTwoNodeModel();

    SkinnedModelData::AnimationChannel channel;
    channel.node = 1;
    channel.path = SkinnedModelData::AnimationChannel::Path::Translation;
    channel.step = true;
    channel.times = { 0.0f, 1.0f };
    channel.vectors = { Vec3F(0, 0, 0), Vec3F(10, 0, 0) };

    SkinnedModelData::AnimationClip clip;
    clip.name = "step";
    clip.duration = 1.0f;
    clip.channels = { channel };
    model.animations.Add(clip);

    auto converted = SkinnedModelAnimation::ConvertClip(model, "step");
    ASSERT_TRUE(converted);

    auto track = converted->GetTrackByType<Vec3F>("child/root/child/spine_low/transform/position");
    ASSERT_TRUE(track);
    EXPECT_NEAR(track->GetValue(0.5f).x, 0.0f, 0.05f) << "STEP must hold the previous key value";
    EXPECT_NEAR(track->GetValue(1.0f).x, 10.0f, 0.001f);
}
