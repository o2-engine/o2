#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Mesh.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Utils/Math/Matrix4.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;
}

TEST(MeshPrimitiveComponent, Defaults)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshPrimitiveComponent>();

    EXPECT_EQ(m->GetPrimitiveType(), PrimitiveType3D::Box);
    EXPECT_EQ(m->GetSize(), Vec3F(100, 100, 100));
    EXPECT_EQ(m->GetSegments(), 24);
    EXPECT_EQ(m->GetColor(), Color4::White());
    EXPECT_FALSE(m->GetTexture());
    EXPECT_TRUE(m->IsShaded());
}

TEST(MeshPrimitiveComponent, ParamsRoundTrip)
{
    SceneCleanGuard guard;
    auto m = mmake<MeshPrimitiveComponent>();

    m->SetPrimitiveType(PrimitiveType3D::Cylinder);
    m->SetSize(Vec3F(50, 80, 50));
    m->SetSegments(12);
    m->SetColor(Color4(10, 20, 30, 40));
    m->SetShaded(false);

    EXPECT_EQ(m->GetPrimitiveType(), PrimitiveType3D::Cylinder);
    EXPECT_EQ(m->GetSize(), Vec3F(50, 80, 50));
    EXPECT_EQ(m->GetSegments(), 12);
    EXPECT_EQ(m->GetColor(), Color4(10, 20, 30, 40));
    EXPECT_FALSE(m->IsShaded());
}

TEST(MeshPrimitiveComponent, AttachesToActorAndBuildsMesh)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();
    TickFrame();

    EXPECT_EQ(a->GetComponent<MeshPrimitiveComponent>(), m);
    EXPECT_EQ(m->GetMesh().vertexCount, 24u);
    EXPECT_EQ(m->GetMesh().polyCount, 12u);
}

TEST(MeshPrimitiveComponent, SerializationRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();

    m->SetPrimitiveType(PrimitiveType3D::Sphere);
    m->SetSize(Vec3F(60, 70, 80));
    m->SetSegments(10);
    m->SetColor(Color4(1, 2, 3, 4));
    m->SetShaded(false);

    DataDocument data;
    m->Serialize(data);

    auto restored = mmake<MeshPrimitiveComponent>();
    restored->Deserialize(data);

    EXPECT_EQ(restored->GetPrimitiveType(), PrimitiveType3D::Sphere);
    EXPECT_EQ(restored->GetSize(), Vec3F(60, 70, 80));
    EXPECT_EQ(restored->GetSegments(), 10);
    EXPECT_EQ(restored->GetColor(), Color4(1, 2, 3, 4));
    EXPECT_FALSE(restored->IsShaded());
}

TEST(MeshPrimitiveComponent, WorldTransformAppliedToDrawnMesh)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();
    m->SetSize(Vec3F(100, 100, 100));
    m->SetShaded(false);

    Vec3F position(10, 20, 30);
    Vec3F euler(0.3f, 0.5f, 0.2f);
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

        Vec3F expectedNormal = expected.TransformDirection(localData.normals[i]).Normalized();
        EXPECT_NEAR(verts[i].nx, expectedNormal.x, kEps);
        EXPECT_NEAR(verts[i].ny, expectedNormal.y, kEps);
        EXPECT_NEAR(verts[i].nz, expectedNormal.z, kEps);
    }
}

TEST(MeshPrimitiveComponent, UnshadedVertexColorMatchesParam)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();
    m->SetColor(Color4(10, 20, 30, 255));
    m->SetShaded(false);
    TickFrame();

    const Mesh& mesh = m->GetMesh();
    ASSERT_GT(mesh.vertexCount, 0u);

    const Vertex* verts = mesh.GetVertices<Vertex>();
    EXPECT_EQ(verts[0].color, Color4(10, 20, 30, 255).ABGR());
}

TEST(MeshPrimitiveComponent, ShadedBakesDifferentColorsPerFace)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();
    m->SetColor(Color4::White());
    m->SetShaded(true);
    TickFrame();

    const Mesh& mesh = m->GetMesh();
    ASSERT_EQ(mesh.vertexCount, 24u);

    const Vertex* verts = mesh.GetVertices<Vertex>();
    bool anyDifferent = false;
    for (UInt i = 1; i < mesh.vertexCount; i++)
    {
        if (verts[i].color != verts[0].color)
            anyDifferent = true;
    }

    EXPECT_TRUE(anyDifferent);
}

TEST(MeshPrimitiveComponent, MeshFollowsTransformWithoutBeingDrawn)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();
    m->SetSize(Vec3F(100, 100, 100));
    TickFrame();

    a->transform->SetPosition(Vec3F(500, 0, 0));
    TickFrame();

    // Filling is deferred to the first use, so it must still be up to date for anyone asking
    const Mesh& mesh = m->GetMesh();
    ASSERT_EQ(mesh.vertexCount, 24u);

    const Vertex* verts = mesh.GetVertices<Vertex>();
    for (UInt i = 0; i < mesh.vertexCount; i++)
        EXPECT_NEAR(verts[i].x, 500.0f, 50.0f + kEps);
}

TEST(MeshPrimitiveComponent, WorldBoundsFollowTransform)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();
    m->SetSize(Vec3F(100, 100, 100));
    a->transform->SetPosition(Vec3F(10, 20, 30));
    TickFrame();

    AABB bounds;
    ASSERT_TRUE(m->Get3DDrawableBounds(bounds));
    EXPECT_NEAR(bounds.GetCenter().x, 10.0f, kEps);
    EXPECT_NEAR(bounds.GetCenter().y, 20.0f, kEps);
    EXPECT_NEAR(bounds.GetCenter().z, 30.0f, kEps);
    EXPECT_NEAR(bounds.GetSize().x, 100.0f, kEps);

    a->transform->SetPosition(Vec3F(-40, 20, 30));
    TickFrame();

    ASSERT_TRUE(m->Get3DDrawableBounds(bounds));
    EXPECT_NEAR(bounds.GetCenter().x, -40.0f, kEps);
}

TEST(MeshPrimitiveComponent, LocalBoundsFollowSize)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto m = a->AddComponent<MeshPrimitiveComponent>();
    m->SetSize(Vec3F(20, 40, 60));
    TickFrame();

    AABB bounds;
    ASSERT_TRUE(m->Get3DDrawableLocalBounds(bounds));
    EXPECT_NEAR(bounds.GetSize().x, 20.0f, kEps);
    EXPECT_NEAR(bounds.GetSize().y, 40.0f, kEps);
    EXPECT_NEAR(bounds.GetSize().z, 60.0f, kEps);

    m->SetSize(Vec3F(200, 40, 60));
    ASSERT_TRUE(m->Get3DDrawableLocalBounds(bounds));
    EXPECT_NEAR(bounds.GetSize().x, 200.0f, kEps);
}
