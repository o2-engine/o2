#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/SkinningMeshComponent.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

TEST(SkinningMeshComponent, DefaultColorIsWhite)
{
    SceneCleanGuard guard;
    auto m = mmake<SkinningMeshComponent>();
    EXPECT_EQ(m->GetColor(), Color4::White());
}

TEST(SkinningMeshComponent, SetColorRoundTrip)
{
    SceneCleanGuard guard;
    auto m = mmake<SkinningMeshComponent>();
    m->SetColor(Color4(20, 30, 40, 200));
    EXPECT_EQ(m->GetColor(), Color4(20, 30, 40, 200));
}

TEST(SkinningMeshComponent, DefaultExtraPointsEmpty)
{
    SceneCleanGuard guard;
    auto m = mmake<SkinningMeshComponent>();
    EXPECT_EQ(m->GetExtraPoints().Count(), 0);
}

TEST(SkinningMeshComponent, AddExtraPointAppends)
{
    SceneCleanGuard guard;
    auto m = mmake<SkinningMeshComponent>();
    m->AddExtraPoint(Vec2F(5, 5));
    m->AddExtraPoint(Vec2F(7, 7));

    ASSERT_EQ(m->GetExtraPoints().Count(), 2);
    EXPECT_EQ(m->GetExtraPoints()[1], Vec2F(7, 7));
}

TEST(SkinningMeshComponent, RemoveExtraPointByIndex)
{
    SceneCleanGuard guard;
    auto m = mmake<SkinningMeshComponent>();
    m->AddExtraPoint(Vec2F(1, 1));
    m->AddExtraPoint(Vec2F(2, 2));
    m->RemoveExtraPoint(0);

    ASSERT_EQ(m->GetExtraPoints().Count(), 1);
    EXPECT_EQ(m->GetExtraPoints()[0], Vec2F(2, 2));
}

TEST(SkinningMeshComponent, SetMappingFrameRoundTrip)
{
    SceneCleanGuard guard;
    auto m = mmake<SkinningMeshComponent>();
    m->SetMappingFrame(RectF(10, 20, 110, 220));
    EXPECT_EQ(m->GetMappingFrame(), RectF(10, 20, 110, 220));
}

TEST(SkinningMeshComponent, GetBonesEmptyByDefault)
{
    SceneCleanGuard guard;
    auto m = mmake<SkinningMeshComponent>();
    EXPECT_EQ(m->GetBones().Count(), 0);
}

TEST(SkinningMeshComponent, AttachesToActor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<SkinningMeshComponent>();
    EXPECT_EQ(a->GetComponent<SkinningMeshComponent>(), m);
}
