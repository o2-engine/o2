#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Animation/SkinnedModelAnimation.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Components/SkinnedMeshComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    // Two-bone model: 200x200 quad, top vertices bound to the child joint; the "slide"
    // clip moves the child (and the top edge) right by 300 over 1 second
    SkinnedModelData BuildTwoBoneModel()
    {
        SkinnedModelData model;

        model.positions = { Vec3F(-100, -100, 0), Vec3F(-100, 100, 0), Vec3F(100, -100, 0), Vec3F(100, 100, 0) };
        model.normals = { Vec3F(0, 0, 1), Vec3F(0, 0, 1), Vec3F(0, 0, 1), Vec3F(0, 0, 1) };
        model.uvs = { Vec2F(0, 0), Vec2F(0, 1), Vec2F(1, 0), Vec2F(1, 1) };
        model.indices = { 0, 1, 2, 1, 3, 2 };

        SkinnedModelData::Node root;
        root.name = "root";
        root.position = Vec3F(0, -20, 0);
        root.rotation = Quat::FromEuler(Vec3F(0, 0, 0.5f));
        root.scale = Vec3F(1.5f, 1.5f, 1.5f);

        SkinnedModelData::Node child;
        child.name = "child";
        child.parent = 0;
        child.position = Vec3F(0, 100, 0);

        model.nodes = { root, child };
        model.joints = { 0, 1 };

        Mat4 rootBind = Mat4::TRS(root.position, root.rotation, root.scale);
        Mat4 childBind = rootBind*Mat4::Translation(child.position);
        model.inverseBindMatrices = { rootBind.Inverted(), childBind.Inverted() };

        model.influences.Resize(4);
        for (int i = 0; i < 4; i++)
        {
            bool topVertex = model.positions[i].y > 0.0f;
            model.influences[i].joints[0] = topVertex ? 1 : 0;
            model.influences[i].weights[0] = 1.0f;
        }

        SkinnedModelData::AnimationChannel channel;
        channel.node = 1;
        channel.path = SkinnedModelData::AnimationChannel::Path::Translation;
        channel.times = { 0.0f, 1.0f };
        channel.vectors = { Vec3F(0, 100, 0), Vec3F(300, 100, 0) };

        SkinnedModelData::AnimationClip clip;
        clip.name = "slide";
        clip.duration = 1.0f;
        clip.channels.Add(channel);
        model.animations.Add(clip);

        return model;
    }

    Ref<SkinnedMeshComponent> MakeSkinnedActor(const String& name)
    {
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->SetName(name);

        auto component = actor->AddComponent<SkinnedMeshComponent>();

        AssetRef<SkinnedModelAsset> modelRef;
        modelRef.CreateInstance();
        modelRef->SetModelData(BuildTwoBoneModel());
        component->SetModelAsset(modelRef);

        return component;
    }

    bool IsNearIdentity(const Mat4& matrix, float tolerance)
    {
        Mat4 identity = Mat4::Identity();
        for (int i = 0; i < 16; i++)
        {
            if (Math::Abs(matrix.m[i] - identity.m[i]) > tolerance)
                return false;
        }

        return true;
    }
}

// Bone actors replicate the model nodes hierarchy with names and bind pose local TRS
TEST(SkinnedBoneActors, CreateBoneActorsBuildsHierarchyWithBindPose)
{
    SceneCleanGuard guard;

    auto component = MakeSkinnedActor("skinned");
    auto owner = component->GetActor();

    component->CreateBoneActors();
    EXPECT_TRUE(component->IsUsingBoneActors());

    auto root = owner->GetChild("root");
    ASSERT_TRUE(root);

    auto child = root->GetChild("child");
    ASSERT_TRUE(child);
    EXPECT_EQ(owner->GetChildren().Count(), 1);
    EXPECT_EQ(root->GetChildren().Count(), 1);

    EXPECT_LT((root->transform->GetPosition() - Vec3F(0, -20, 0)).Length(), 0.001f);
    EXPECT_LT((root->transform->GetScale() - Vec3F(1.5f, 1.5f, 1.5f)).Length(), 0.001f);
    EXPECT_NEAR(root->transform->GetEulerAngles().z, 0.5f, 0.001f);
    EXPECT_LT((child->transform->GetPosition() - Vec3F(0, 100, 0)).Length(), 0.001f);

    // Repeated call reuses existing actors instead of duplicating them
    component->CreateBoneActors();
    EXPECT_EQ(owner->GetChildren().Count(), 1);
    EXPECT_EQ(root->GetChildren().Count(), 1);
}

// In the bind pose the skinning palette from bone actors is identity for every joint
TEST(SkinnedBoneActors, BindPosePaletteFromActorsIsIdentity)
{
    SceneCleanGuard guard;

    auto component = MakeSkinnedActor("skinned");
    component->GetActor()->transform->SetPosition(Vec3F(50, -30, 20));
    component->GetActor()->transform->SetEulerAngles(Vec3F(0.3f, 0.2f, 0.1f));

    component->CreateBoneActors();
    TickFrame();

    Vector<Mat4> palette;
    component->EvaluateModelPalette(palette);
    ASSERT_EQ(palette.Count(), 2);

    for (int i = 0; i < palette.Count(); i++)
        EXPECT_TRUE(IsNearIdentity(palette[i], 0.001f)) << "bind pose palette matrix " << i << " must be identity";
}

// The converted clip played by AnimationComponent moves bone actors and the skinned geometry
TEST(SkinnedBoneActors, AnimationComponentDrivesBonesAndSkinning)
{
    SceneCleanGuard guard;

    auto component = MakeSkinnedActor("skinned");
    auto owner = component->GetActor();

    component->CreateBoneActors();

    auto clip = SkinnedModelAnimation::ConvertClip(component->GetModelAsset()->GetModelData(), "slide");
    ASSERT_TRUE(clip);

    AssetRef<AnimationAsset> animationAsset;
    animationAsset.CreateInstance();
    animationAsset->animation = clip;

    auto animationComponent = owner->AddComponent<AnimationComponent>();
    auto state = mmake<AnimationState>("slide");
    state->SetAnimation(animationAsset);
    state->SetLooped(true);
    animationComponent->AddState(state);

    TickFrame(0.001f);

    auto childBone = owner->GetChild("root/child");
    ASSERT_TRUE(childBone);
    Vec3F startBonePosition = childBone->transform->GetPosition();

    Vector<Mat4> startPalette;
    component->EvaluateModelPalette(startPalette);

    TickFrames(5, 0.1f); // Advance the animation to ~0.5 s

    Vec3F movedBonePosition = childBone->transform->GetPosition();
    EXPECT_GT(movedBonePosition.x - startBonePosition.x, 50.0f)
        << "animation must move the child bone actor along x";

    Vector<Mat4> movedPalette;
    component->EvaluateModelPalette(movedPalette);
    ASSERT_EQ(movedPalette.Count(), 2);
    EXPECT_TRUE(IsNearIdentity(movedPalette[0], 0.01f)) << "static root joint stays at bind";
    EXPECT_FALSE(IsNearIdentity(movedPalette[1], 0.01f)) << "animated child joint must leave the bind pose";

    // CPU skinned geometry follows the bones: the top edge moved right
    AABB bounds;
    ASSERT_TRUE(component->Get3DDrawableLocalBounds(bounds));
    EXPECT_GT(bounds.max.x, 130.0f) << "skinned top edge must slide right after the animation step";
}

// Scene round trip: bone actors hierarchy, skinned component flags and the AnimationComponent
// with the embedded converted clip survive save/load
TEST(SkinnedBoneActors, SceneRoundTripKeepsBonesAndAnimation)
{
    SceneCleanGuard guard;

    auto component = MakeSkinnedActor("skinned");
    auto owner = component->GetActor();

    component->CreateBoneActors();

    auto clip = SkinnedModelAnimation::ConvertClip(component->GetModelAsset()->GetModelData(), "slide");
    ASSERT_TRUE(clip);

    AssetRef<AnimationAsset> animationAsset;
    animationAsset.CreateInstance();
    animationAsset->animation = clip;

    auto animationComponent = owner->AddComponent<AnimationComponent>();
    auto state = mmake<AnimationState>("slide");
    state->SetAnimation(animationAsset);
    state->SetLooped(true);
    animationComponent->AddState(state);

    // Note: dt must be positive, Loop::Repeat evaluates the clip end at exactly zero time
    TickFrame(0.001f);

    auto childBoneBeforeSave = owner->GetChild("root/child");
    ASSERT_TRUE(childBoneBeforeSave);
    EXPECT_LT((childBoneBeforeSave->transform->GetPosition() - Vec3F(0, 100, 0)).Length(), 1.0f)
        << "bone must be near the bind pose before saving";

    DataDocument document;
    o2Scene.Save(document);

    o2Scene.Clear(false);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(document);
    TickFrame(0.001f);

    auto loadedActor = o2Scene.FindActor("skinned");
    ASSERT_TRUE(loadedActor);

    auto loadedComponent = loadedActor->GetComponent<SkinnedMeshComponent>();
    ASSERT_TRUE(loadedComponent);
    EXPECT_TRUE(loadedComponent->IsUsingBoneActors());

    auto loadedChildBone = loadedActor->GetChild("root/child");
    ASSERT_TRUE(loadedChildBone);
    EXPECT_LT((loadedChildBone->transform->GetPosition() - Vec3F(0, 100, 0)).Length(), 1.0f);

    auto loadedAnimation = loadedActor->GetComponent<AnimationComponent>();
    ASSERT_TRUE(loadedAnimation);

    auto loadedState = DynamicCast<AnimationState>(loadedAnimation->GetState("slide"));
    ASSERT_TRUE(loadedState);
    ASSERT_TRUE(loadedState->GetAnimation());
    ASSERT_TRUE(loadedState->GetAnimation()->animation);
    EXPECT_EQ(loadedState->GetAnimation()->animation->GetTracks().Count(), 1);

    // The loaded animation keeps driving the bones
    TickFrames(5, 0.1f);
    EXPECT_GT(loadedChildBone->transform->GetPosition().x, 50.0f)
        << "loaded animation must keep moving the bone actor";
}
