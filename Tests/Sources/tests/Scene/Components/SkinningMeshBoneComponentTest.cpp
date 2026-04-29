#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2/Scene/Components/SkinningMeshComponent.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

TEST(SkinningMeshBoneComponent, DefaultLengthIs100)
{
    SceneCleanGuard guard;
    auto bone = mmake<SkinningMeshBoneComponent>();
    EXPECT_FLOAT_EQ(bone->length, 100.0f);
}

TEST(SkinningMeshBoneComponent, DefaultVertexWeightsEmpty)
{
    SceneCleanGuard guard;
    auto bone = mmake<SkinningMeshBoneComponent>();
    EXPECT_EQ(bone->vertexWeights.Count(), 0);
}

TEST(SkinningMeshBoneComponent, AddVertexWeight)
{
    SceneCleanGuard guard;
    auto bone = mmake<SkinningMeshBoneComponent>();

    bone->vertexWeights.Add({ 0, 0.7f });
    bone->vertexWeights.Add({ 1, 0.3f });

    ASSERT_EQ(bone->vertexWeights.Count(), 2);
    EXPECT_EQ(bone->vertexWeights[0].first, 0);
    EXPECT_FLOAT_EQ(bone->vertexWeights[0].second, 0.7f);
}

TEST(SkinningMeshBoneComponent, FindSkinningMeshReturnsParentMesh)
{
    SceneCleanGuard guard;
    auto meshActor = mmake<Actor>(ActorCreateMode::InScene);
    auto mesh = meshActor->AddComponent<SkinningMeshComponent>();

    auto boneActor = mmake<Actor>(ActorCreateMode::InScene);
    meshActor->AddChild(boneActor);
    auto bone = boneActor->AddComponent<SkinningMeshBoneComponent>();

    auto found = bone->FindSkinningMesh();
    EXPECT_EQ(found, mesh);
}

TEST(SkinningMeshBoneComponent, FindSkinningMeshReturnsNullWhenNoParentMesh)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto bone = a->AddComponent<SkinningMeshBoneComponent>();

    EXPECT_FALSE(bone->FindSkinningMesh());
}

TEST(SkinningMeshBoneComponent, FindSkinningMeshWalksMultipleAncestors)
{
    SceneCleanGuard guard;
    auto root = mmake<Actor>(ActorCreateMode::InScene);
    auto mesh = root->AddComponent<SkinningMeshComponent>();

    auto mid = mmake<Actor>(ActorCreateMode::InScene);
    auto leaf = mmake<Actor>(ActorCreateMode::InScene);
    root->AddChild(mid);
    mid->AddChild(leaf);
    auto bone = leaf->AddComponent<SkinningMeshBoneComponent>();

    EXPECT_EQ(bone->FindSkinningMesh(), mesh);
}

TEST(SkinningMeshBoneComponent, AddedToActorRetrievableByType)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto bone = a->AddComponent<SkinningMeshBoneComponent>();
    EXPECT_EQ(a->GetComponent<SkinningMeshBoneComponent>(), bone);
    EXPECT_EQ(bone->GetActor(), a);
}
