#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Mesh.h"
#include "o2/Render/Mesh3DFill.h"
#include "o2/Utils/Math/Vertex.h"

using namespace o2;

TEST(Mesh3DFill, MeshBoundsWalkVertexBuffer)
{
    Mesh mesh(TextureRef::Null(), 3, 1);
    auto* vertices = mesh.GetVertices<Vertex>();
    vertices[0].SetPosition(-10.0f, 5.0f, 2.0f);
    vertices[1].SetPosition(30.0f, -20.0f, 0.0f);
    vertices[2].SetPosition(0.0f, 0.0f, 15.0f);
    mesh.vertexCount = 3;

    o2::AABB bounds;
    ASSERT_TRUE(Mesh3DPrimitives::GetMeshBounds(mesh, bounds));
    EXPECT_EQ(bounds.min, Vec3F(-10.0f, -20.0f, 0.0f));
    EXPECT_EQ(bounds.max, Vec3F(30.0f, 5.0f, 15.0f));

    Mesh empty(TextureRef::Null(), 4, 2);
    EXPECT_FALSE(Mesh3DPrimitives::GetMeshBounds(empty, bounds));
}

TEST(Mesh3DFill, FillMeshTransformsAndKeepsAlpha)
{
    auto data = Mesh3DPrimitives::BuildBox(Vec3F(2.0f, 2.0f, 2.0f));

    Mesh mesh;
    Mat4 transform = Basis3D::Build(Vec3F(10.0f, 0.0f, 0.0f), Vec3F(1, 1, 1), Quat::Identity()).ToMat4();
    Mesh3DPrimitives::FillMesh(mesh, data, transform, Color4(200, 100, 50, 120), TextureSource(), true, 0.5f);

    ASSERT_EQ(mesh.vertexCount, (UInt)data.positions.Count());
    ASSERT_EQ(mesh.polyCount, (UInt)(data.indices.Count()/3));

    o2::AABB bounds;
    ASSERT_TRUE(Mesh3DPrimitives::GetMeshBounds(mesh, bounds));
    EXPECT_NEAR(bounds.GetCenter().x, 10.0f, 1e-3f);

    // Alpha survives the shaded color bake
    auto* vertices = mesh.GetVertices<Vertex>();
    for (UInt i = 0; i < mesh.vertexCount; i++)
    {
        Color4 color;
        color.SetABGR(vertices[i].color);
        EXPECT_EQ(color.a, 120);
    }
}
