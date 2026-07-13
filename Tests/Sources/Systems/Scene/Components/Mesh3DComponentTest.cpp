#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/Mesh3DComponent.h"
#include "o2/Utils/Math/Matrix4.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    AssetRef<Mesh3DAsset> MakeBoxAssetInstance(const Vec3F& size)
    {
        AssetRef<Mesh3DAsset> asset;
        asset.CreateInstance();
        asset->SetFromPrimitive(Mesh3DPrimitives::BuildBox(size));
        return asset;
    }
}

TEST(Mesh3DComponent, DefaultsAndEmptyMesh)
{
    SceneCleanGuard guard;
    auto m = mmake<Mesh3DComponent>();

    EXPECT_FALSE(m->GetMeshAsset());
    EXPECT_EQ(m->GetColor(), Color4::White());
    EXPECT_TRUE(m->IsShaded());
    EXPECT_EQ(m->GetMesh().vertexCount, 0u);
}

TEST(Mesh3DComponent, DrawnGeometryMatchesAsset)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<Mesh3DComponent>();
    m->SetMeshAsset(MakeBoxAssetInstance(Vec3F(40, 60, 80)));
    m->SetShaded(false);
    TickFrame();

    auto expectedData = Mesh3DPrimitives::BuildBox(Vec3F(40, 60, 80));

    const Mesh& mesh = m->GetMesh();
    ASSERT_EQ(mesh.vertexCount, (UInt)expectedData.positions.Count());
    ASSERT_EQ(mesh.polyCount, (UInt)(expectedData.indices.Count()/3));

    const Vertex* verts = mesh.GetVertices<Vertex>();
    for (UInt i = 0; i < mesh.vertexCount; i++)
    {
        EXPECT_NEAR(verts[i].x, expectedData.positions[i].x, kEps);
        EXPECT_NEAR(verts[i].y, expectedData.positions[i].y, kEps);
        EXPECT_NEAR(verts[i].z, expectedData.positions[i].z, kEps);
    }

    const VertexIndex* indexes = mesh.GetIndexes();
    for (int i = 0; i < expectedData.indices.Count(); i++)
        EXPECT_EQ(indexes[i], expectedData.indices[i]);
}

TEST(Mesh3DComponent, WorldTransformAppliedToDrawnMesh)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<Mesh3DComponent>();
    m->SetMeshAsset(MakeBoxAssetInstance(Vec3F(100, 100, 100)));
    m->SetShaded(false);

    Vec3F position(-5, 15, 25);
    Vec3F euler(0.2f, -0.4f, 0.6f);
    a->transform->SetPosition(position);
    a->transform->SetEulerAngles(euler);
    TickFrame();

    Mat4 expected = Mat4::TRS(position, Quat::FromEuler(euler), Vec3F::One());
    auto localData = Mesh3DPrimitives::BuildBox(Vec3F(100, 100, 100));

    const Mesh& mesh = m->GetMesh();
    ASSERT_EQ(mesh.vertexCount, (UInt)localData.positions.Count());

    const Vertex* verts = mesh.GetVertices<Vertex>();
    for (UInt i = 0; i < mesh.vertexCount; i++)
    {
        Vec3F expectedPos = expected.TransformPoint(localData.positions[i]);
        EXPECT_NEAR(verts[i].x, expectedPos.x, kEps);
        EXPECT_NEAR(verts[i].y, expectedPos.y, kEps);
        EXPECT_NEAR(verts[i].z, expectedPos.z, kEps);
    }
}

TEST(Mesh3DComponent, SerializationRoundTripWithInstanceAsset)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<Mesh3DComponent>();
    m->SetMeshAsset(MakeBoxAssetInstance(Vec3F(30, 30, 30)));
    m->SetColor(Color4(5, 6, 7, 8));
    m->SetShaded(false);

    DataDocument data;
    m->Serialize(data);

    auto restored = mmake<Mesh3DComponent>();
    restored->Deserialize(data);

    EXPECT_EQ(restored->GetColor(), Color4(5, 6, 7, 8));
    EXPECT_FALSE(restored->IsShaded());

    ASSERT_TRUE(restored->GetMeshAsset());
    EXPECT_EQ(restored->GetMeshAsset()->vertices, m->GetMeshAsset()->vertices);
    EXPECT_EQ(restored->GetMeshAsset()->indices, m->GetMeshAsset()->indices);
}
