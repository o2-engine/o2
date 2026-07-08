#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Render/ObjMeshFormat.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/Mesh3DComponent.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    const char* kQuadObj = R"(
# simple quad, full v/vt/vn refs
mtllib ignored.mtl
o Quad
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
vt 0 0
vt 1 0
vt 1 1
vt 0 1
vn 0 0 1
usemtl ignored
f 1/1/1 2/2/1 3/3/1 4/4/1
)";
}

TEST(ObjMeshFormat, QuadIsTriangulatedWithDedup)
{
    Mesh3DData data;
    String error;
    ASSERT_TRUE(ObjMeshFormat::Parse(kQuadObj, data, &error)) << error.Data();

    EXPECT_EQ(data.positions.Count(), 4);
    EXPECT_EQ(data.uvs.Count(), 4);
    EXPECT_EQ(data.normals.Count(), 4);
    EXPECT_EQ(data.indices.Count(), 6);

    EXPECT_NEAR(data.positions[2].x, 1.0f, kEps);
    EXPECT_NEAR(data.positions[2].y, 1.0f, kEps);
    EXPECT_NEAR(data.uvs[1].x, 1.0f, kEps);
    EXPECT_NEAR(data.normals[0].z, 1.0f, kEps);

    for (auto index : data.indices)
        EXPECT_LT((int)index, data.positions.Count());
}

TEST(ObjMeshFormat, NegativeAndPositionOnlyRefs)
{
    const char* obj = R"(
v 0 0 0
v 1 0 0
v 0 1 0
f -3 -2 -1
)";

    Mesh3DData data;
    ASSERT_TRUE(ObjMeshFormat::Parse(obj, data));
    EXPECT_EQ(data.positions.Count(), 3);
    EXPECT_EQ(data.indices.Count(), 3);
    EXPECT_EQ(data.uvs.Count(), 3);
}

TEST(ObjMeshFormat, MissingNormalsAreComputed)
{
    const char* obj = R"(
v 0 0 0
v 1 0 0
v 0 1 0
f 1 2 3
)";

    Mesh3DData data;
    ASSERT_TRUE(ObjMeshFormat::Parse(obj, data));
    ASSERT_EQ(data.normals.Count(), 3);

    for (auto& normal : data.normals)
    {
        EXPECT_NEAR(normal.Length(), 1.0f, kEps);
        EXPECT_NEAR(normal.z, 1.0f, kEps);
    }
}

TEST(ObjMeshFormat, VertexAndNormalWithoutUV)
{
    const char* obj = R"(
v 0 0 0
v 1 0 0
v 0 1 0
vn 0 0 1
f 1//1 2//1 3//1
)";

    Mesh3DData data;
    ASSERT_TRUE(ObjMeshFormat::Parse(obj, data));
    EXPECT_EQ(data.normals.Count(), 3);
    EXPECT_NEAR(data.normals[0].z, 1.0f, kEps);
}

TEST(ObjMeshFormat, InvalidFaceReferenceFails)
{
    const char* obj = R"(
v 0 0 0
f 1 2 3
)";

    Mesh3DData data;
    String error;
    EXPECT_FALSE(ObjMeshFormat::Parse(obj, data, &error));
    EXPECT_FALSE(error.IsEmpty());
}

TEST(ObjMeshFormat, WriteParseRoundTrip)
{
    Mesh3DData source = Mesh3DPrimitives::BuildBox(Vec3F(100, 50, 30));

    Mesh3DData restored;
    ASSERT_TRUE(ObjMeshFormat::Parse(ObjMeshFormat::Write(source), restored));

    ASSERT_EQ(restored.indices.Count(), source.indices.Count());
    ASSERT_EQ(restored.positions.Count(), source.positions.Count());

    for (int i = 0; i < source.positions.Count(); i++)
    {
        EXPECT_NEAR(source.positions[i].x, restored.positions[i].x, kEps);
        EXPECT_NEAR(source.positions[i].y, restored.positions[i].y, kEps);
        EXPECT_NEAR(source.positions[i].z, restored.positions[i].z, kEps);
    }
}

TEST(ObjMeshFormat, Mesh3DAssetLoadsFromObjText)
{
    AssetRef<Mesh3DAsset> asset;
    asset.CreateInstance();
    String error;
    ASSERT_TRUE(asset->LoadFromObj(kQuadObj, &error)) << error.Data();

    EXPECT_EQ(asset->vertices.Count(), 4);
    EXPECT_EQ(asset->indices.Count(), 6);
}

TEST(ObjMeshFormat, Mesh3DComponentDrawsObjGeometry)
{
    SceneCleanGuard guard;

    AssetRef<Mesh3DAsset> asset;
    asset.CreateInstance();
    ASSERT_TRUE(asset->LoadFromObj(kQuadObj));

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto component = actor->AddComponent<Mesh3DComponent>();
    component->SetMeshAsset(asset);
    TickFrame();

    const Mesh& mesh = component->GetMesh();
    EXPECT_EQ(mesh.vertexCount, 4u);
    EXPECT_EQ(mesh.polyCount, 2u);
}

TEST(ObjMeshFormat, LoadsRealCatAssetWhenPresent)
{
    namespace fs = std::filesystem;

    fs::path path;
    fs::path dir = fs::current_path();
    for (int i = 0; i < 8 && !dir.empty(); i++, dir = dir.parent_path())
    {
        auto candidate = dir/"Assets"/"12221_Cat_v1_l3.obj";
        if (fs::exists(candidate))
        {
            path = candidate;
            break;
        }
    }

    if (path.empty())
        GTEST_SKIP() << "project cat obj not found";

    std::ifstream file(path);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    Mesh3DData data;
    String error;
    ASSERT_TRUE(ObjMeshFormat::Parse(String(text.c_str()), data, &error)) << error.Data();

    EXPECT_GT(data.positions.Count(), 35000);
    EXPECT_EQ(data.indices.Count() % 3, 0);
    EXPECT_GE(data.indices.Count(), 35288*2*3);

    for (auto index : data.indices)
        ASSERT_LT((int)index, data.positions.Count());
}
