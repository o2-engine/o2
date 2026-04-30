#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Mesh.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/MeshComponent.h"
#include "o2/Utils/Math/Spline.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(MeshComponent, DefaultColorIsWhite)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    EXPECT_EQ(m->GetColor(), Color4::White());
}

TEST(MeshComponent, SetColorRoundTrip)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->SetColor(Color4(50, 100, 150, 200));
    EXPECT_EQ(m->GetColor(), Color4(50, 100, 150, 200));
}

TEST(MeshComponent, DefaultExtraPointsEmpty)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    EXPECT_EQ(m->GetExtraPoints().Count(), 0);
}

TEST(MeshComponent, AddExtraPointAppends)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->AddExtraPoint(Vec2F(1, 2));
    m->AddExtraPoint(Vec2F(3, 4));

    ASSERT_EQ(m->GetExtraPoints().Count(), 2);
    EXPECT_EQ(m->GetExtraPoints()[0], Vec2F(1, 2));
    EXPECT_EQ(m->GetExtraPoints()[1], Vec2F(3, 4));
}

TEST(MeshComponent, SetExtraPointReplacesByIndex)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->AddExtraPoint(Vec2F(1, 2));
    m->AddExtraPoint(Vec2F(3, 4));

    m->SetExtraPoint(0, Vec2F(99, 99));

    EXPECT_EQ(m->GetExtraPoints()[0], Vec2F(99, 99));
    EXPECT_EQ(m->GetExtraPoints()[1], Vec2F(3, 4));
}

TEST(MeshComponent, RemoveExtraPointByIndex)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->AddExtraPoint(Vec2F(1, 1));
    m->AddExtraPoint(Vec2F(2, 2));
    m->AddExtraPoint(Vec2F(3, 3));

    m->RemoveExtraPoint(1);

    ASSERT_EQ(m->GetExtraPoints().Count(), 2);
    EXPECT_EQ(m->GetExtraPoints()[0], Vec2F(1, 1));
    EXPECT_EQ(m->GetExtraPoints()[1], Vec2F(3, 3));
}

TEST(MeshComponent, SetExtraPointsReplacesAll)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->AddExtraPoint(Vec2F(0, 0));

    Vector<Vec2F> newPoints;
    newPoints.Add(Vec2F(10, 10));
    newPoints.Add(Vec2F(20, 20));
    m->SetExtraPoints(newPoints);

    ASSERT_EQ(m->GetExtraPoints().Count(), 2);
    EXPECT_EQ(m->GetExtraPoints()[0], Vec2F(10, 10));
}

TEST(MeshComponent, MappingFrameRoundTrip)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->SetMappingFrame(RectF(0, 0, 256, 256));
    EXPECT_EQ(m->GetMappingFrame(), RectF(0, 0, 256, 256));
}

TEST(MeshComponent, AttachesToActor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshComponent>();
    EXPECT_EQ(a->GetComponent<MeshComponent>(), m);
}

TEST(MeshComponent, SplineAccessibleAndOwned)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    EXPECT_TRUE(m->spline);
}

// ===== Mesh generation =====

TEST(MeshComponent, EmptyMeshHasNoVertices)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    EXPECT_EQ(m->GetMesh().vertexCount, 0u);
}

TEST(MeshComponent, SplineWithLessThanThreeKeysProducesEmptyMesh)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->spline->AppendKey(Vec2F(0, 0));
    m->spline->AppendKey(Vec2F(10, 0));
    EXPECT_EQ(m->GetMesh().vertexCount, 0u);
}

TEST(MeshComponent, ThirdKeyTriggersMeshGeneration)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshComponent>();
    m->spline->AppendKey(Vec2F(0, 0));
    m->spline->AppendKey(Vec2F(10, 0));
    m->spline->AppendKey(Vec2F(5, 10));
    EXPECT_GT(m->GetMesh().vertexCount, 0u);
    EXPECT_GT(m->GetMesh().polyCount, 0u);
}
