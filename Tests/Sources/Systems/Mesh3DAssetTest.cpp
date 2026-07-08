#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <filesystem>

#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Utils/Types/UID.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    // Redirects assets tree root to a temp folder for save/load and restores it back
    class TempAssetsPathGuard
    {
    public:
        TempAssetsPathGuard()
        {
            namespace fs = std::filesystem;

            auto& tree = const_cast<AssetsTree&>(o2Assets.GetAssetsTree());
            mOrigAssetsPath = tree.assetsPath;

            UID dirUid;
            dirUid.Randomize();
            mTempDir = fs::temp_directory_path()/("o2test_mesh3d_" + std::string((String)dirUid));
            fs::create_directories(mTempDir);

            String tempPrefix(mTempDir.string().c_str());
            tempPrefix.ReplaceAll("\\", "/");
            if (!tempPrefix.EndsWith("/"))
                tempPrefix += "/";

            tree.assetsPath = tempPrefix;
        }

        ~TempAssetsPathGuard()
        {
            auto& tree = const_cast<AssetsTree&>(o2Assets.GetAssetsTree());
            tree.assetsPath = mOrigAssetsPath;

            std::error_code ec;
            std::filesystem::remove_all(mTempDir, ec);
        }

        std::filesystem::path GetTempDir() const { return mTempDir; }

    private:
        String                mOrigAssetsPath;
        std::filesystem::path mTempDir;
    };
}

TEST(Mesh3DAsset, ExtensionIsRegistered)
{
    auto map = Assets::GetAssetsExtensionsTypes();
    ASSERT_TRUE(map.ContainsKey("mesh3d"));
    EXPECT_EQ(map["mesh3d"], &TypeOf(Mesh3DAsset));
}

TEST(Mesh3DAsset, SetFromPrimitiveCopiesGeometry)
{
    auto asset = mmake<Mesh3DAsset>();
    auto box = Mesh3DPrimitives::BuildBox(Vec3F(10, 20, 30));

    asset->SetFromPrimitive(box);

    EXPECT_EQ(asset->vertices, box.positions);
    EXPECT_EQ(asset->normals, box.normals);
    EXPECT_EQ(asset->uvs, box.uvs);
    EXPECT_EQ(asset->indices, box.indices);

    auto data = asset->GetMeshData();
    EXPECT_EQ(data.positions, box.positions);
    EXPECT_EQ(data.indices, box.indices);
}

TEST(Mesh3DAsset, SaveLoadRoundTripPreservesGeometry)
{
    TempAssetsPathGuard pathGuard;

    auto sphere = Mesh3DPrimitives::BuildSphere(25.0f, 8, 4);

    {
        auto asset = mmake<Mesh3DAsset>();
        asset->SetFromPrimitive(sphere);
        asset->SetPath("test_sphere.mesh3d");
        asset->Save();
    }

    auto fullPath = pathGuard.GetTempDir()/"test_sphere.mesh3d";
    ASSERT_TRUE(std::filesystem::exists(fullPath));
    ASSERT_TRUE(std::filesystem::exists(pathGuard.GetTempDir()/"test_sphere.mesh3d.meta"));

    // Load the saved file through the same serialization path as Asset::LoadData
    DataDocument loadedDoc;
    ASSERT_TRUE(loadedDoc.LoadFromFile(String(fullPath.string().c_str())));

    auto loaded = mmake<Mesh3DAsset>();
    loaded->Deserialize(loadedDoc);

    ASSERT_EQ(loaded->vertices.Count(), sphere.positions.Count());
    ASSERT_EQ(loaded->normals.Count(), sphere.normals.Count());
    ASSERT_EQ(loaded->uvs.Count(), sphere.uvs.Count());
    ASSERT_EQ(loaded->indices.Count(), sphere.indices.Count());

    for (int i = 0; i < sphere.positions.Count(); i++)
    {
        EXPECT_NEAR(loaded->vertices[i].x, sphere.positions[i].x, kEps);
        EXPECT_NEAR(loaded->vertices[i].y, sphere.positions[i].y, kEps);
        EXPECT_NEAR(loaded->vertices[i].z, sphere.positions[i].z, kEps);
    }

    EXPECT_EQ(loaded->indices, sphere.indices);
}

TEST(Mesh3DAsset, SerializationRoundTripInMemory)
{
    auto asset = mmake<Mesh3DAsset>();
    asset->SetFromPrimitive(Mesh3DPrimitives::BuildCylinder(15.0f, 40.0f, 6));

    DataDocument data;
    asset->Serialize(data);

    auto restored = mmake<Mesh3DAsset>();
    restored->Deserialize(data);

    EXPECT_EQ(restored->vertices, asset->vertices);
    EXPECT_EQ(restored->normals, asset->normals);
    EXPECT_EQ(restored->uvs, asset->uvs);
    EXPECT_EQ(restored->indices, asset->indices);
}
