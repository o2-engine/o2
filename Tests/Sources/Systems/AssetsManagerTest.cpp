#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetInfo.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Meta.h"
#include "o2/Assets/Types/BinaryAsset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/UID.h"

using namespace o2;

namespace {

Ref<AssetInfo> MakeInfoFor(const String& path, const Ref<AssetMeta>& meta)
{
    auto info = mmake<AssetInfo>(meta);
    info->path = path;
    return info;
}

Ref<AssetMeta> MakeBinaryMeta()
{
    auto a = mmake<BinaryAsset>();
    return a->GetMeta();
}

Ref<AssetMeta> MakeFolderMeta()
{
    auto a = mmake<FolderAsset>();
    return a->GetMeta();
}

} // namespace

// ===== AssetsTree =====

TEST(AssetsTree, AddAssetWithSimplePathGoesToRoot) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto info = MakeInfoFor("foo.bin", MakeBinaryMeta());
    tree->AddAsset(info);

    EXPECT_TRUE(tree->rootAssets.Contains(info));
    EXPECT_TRUE(tree->allAssetsByPath.ContainsKey("foo.bin"));
}

TEST(AssetsTree, AddAssetNestedAttachesToParentByPathPrefix) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto parent = MakeInfoFor("dir", MakeFolderMeta());
    auto child = MakeInfoFor("dir/inner.bin", MakeBinaryMeta());

    tree->AddAsset(parent);
    tree->AddAsset(child);

    ASSERT_EQ(parent->GetChildren().Count(), 1);
    EXPECT_EQ(parent->GetChildren()[0], child);
    EXPECT_EQ(child->parent.Lock(), parent);
}

TEST(AssetsTree, FindByPathReturnsRegisteredAsset) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto info = MakeInfoFor("findable.bin", MakeBinaryMeta());
    tree->AddAsset(info);

    EXPECT_EQ(tree->Find("findable.bin"), info);
}

TEST(AssetsTree, FindByUIDReturnsRegisteredAsset) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto meta = MakeBinaryMeta();
    auto uid = meta->ID();
    auto info = MakeInfoFor("uid.bin", meta);
    tree->AddAsset(info);

    EXPECT_EQ(tree->Find(uid), info);
}

TEST(AssetsTree, FindReturnsNullForUnknownPathOrUID) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    EXPECT_FALSE(tree->Find(String("absent.bin")));
    EXPECT_FALSE(tree->Find(UID::empty));
}

TEST(AssetsTree, RemoveAssetCleansAllMaps) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto meta = MakeBinaryMeta();
    auto uid = meta->ID();
    auto info = MakeInfoFor("temp.bin", meta);
    tree->AddAsset(info);

    tree->RemoveAsset(info);

    EXPECT_FALSE(tree->allAssetsByPath.ContainsKey("temp.bin"));
    EXPECT_FALSE(tree->allAssetsByUID.ContainsKey(uid));
    EXPECT_FALSE(tree->rootAssets.Contains(info));
}

TEST(AssetsTree, RemoveFolderCascadesToChildren) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto folder = MakeInfoFor("dir", MakeFolderMeta());
    auto childMeta = MakeBinaryMeta();
    auto childUid = childMeta->ID();
    auto child = MakeInfoFor("dir/inside.bin", childMeta);

    tree->AddAsset(folder);
    tree->AddAsset(child);

    tree->RemoveAsset(folder);

    EXPECT_FALSE(tree->allAssetsByPath.ContainsKey("dir"));
    EXPECT_FALSE(tree->allAssetsByPath.ContainsKey("dir/inside.bin"));
    EXPECT_FALSE(tree->allAssetsByUID.ContainsKey(childUid));
}

TEST(AssetsTree, SortAssetsOrdersByPathLength) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto folder = MakeInfoFor("d", MakeFolderMeta());
    auto longer = MakeInfoFor("d/longer.bin", MakeBinaryMeta());
    auto shorter = MakeInfoFor("s.bin", MakeBinaryMeta());

    tree->AddAsset(folder);
    tree->AddAsset(longer);
    tree->AddAsset(shorter);

    tree->SortAssets();

    ASSERT_FALSE(tree->allAssets.IsEmpty());
    int prevLen = -1;
    for (auto& weak : tree->allAssets)
    {
        auto info = weak.Lock();
        ASSERT_TRUE(info);
        int len = info->path.Length();
        EXPECT_LE(prevLen, len);
        prevLen = len;
    }
}

TEST(AssetsTree, ClearEmptiesAllCollections) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    tree->AddAsset(MakeInfoFor("a.bin", MakeBinaryMeta()));
    tree->AddAsset(MakeInfoFor("b.bin", MakeBinaryMeta()));

    tree->Clear();

    EXPECT_TRUE(tree->rootAssets.IsEmpty());
    EXPECT_TRUE(tree->allAssets.IsEmpty());
    EXPECT_TRUE(tree->allAssetsByPath.IsEmpty());
    EXPECT_TRUE(tree->allAssetsByUID.IsEmpty());
}

// ===== Assets singleton =====

TEST(Assets, IsAssetExistFalseForUnknownPath) {
    EXPECT_FALSE(o2Assets.IsAssetExist(String("definitely/not/here.bin")));
}

TEST(Assets, GetStdAssetTypeIsBinaryAsset) {
    EXPECT_EQ(Assets::GetStdAssetType(), &TypeOf(BinaryAsset));
}

TEST(Assets, GetAssetTypeByExtensionForUnknownReturnsStd) {
    auto type = Assets::GetAssetTypeByExtension("totally_unknown_extension_xyz");
    EXPECT_EQ(type, Assets::GetStdAssetType());
}

TEST(Assets, GetAssetsExtensionsTypesContainsRegisteredTypes) {
    auto map = Assets::GetAssetsExtensionsTypes();
    EXPECT_FALSE(map.IsEmpty());
    EXPECT_TRUE(map.ContainsKey("bin"));
    EXPECT_TRUE(map.ContainsKey("json"));
    EXPECT_EQ(map["bin"], &TypeOf(BinaryAsset));
    EXPECT_EQ(map["json"], &TypeOf(DataAsset));
}

TEST(Assets, CreateAssetTRegistersInCache) {
    auto created = o2Assets.CreateAsset<DataAsset>();
    ASSERT_TRUE(created.IsValid());
    auto cached = o2Assets.GetAssetRef(created->GetUID());
    EXPECT_TRUE(cached.IsValid());
    EXPECT_EQ(cached.GetAssetBase(), created.GetAssetBase());
}

TEST(Assets, MakeUniqueAssetNameReturnsSameForFreePath) {
    String unusedPath = "totally_unused_path_for_test.json";
    EXPECT_EQ(o2Assets.MakeUniqueAssetName(unusedPath), unusedPath);
}

#if IS_EDITOR

// RefreshCachedAssetsInfo must copy the tree's current children list into the cached
// asset's mInfo. Without it, mInfo.mChildren stays empty after the first refresh.
TEST(Assets, RefreshCachedAssetsInfoPopulatesFolderChildrenFromTree) {
    auto folder = mmake<FolderAsset>(); // cached automatically via PostRefConstruct
    auto folderMeta = folder->GetMeta();

    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto folderInfo = mmake<AssetInfo>(folderMeta);
    folderInfo->path = "refreshtest_folder_a";

    auto childInfo = MakeInfoFor("refreshtest_folder_a/child.bin", MakeBinaryMeta());

    tree->AddAsset(folderInfo);
    tree->AddAsset(childInfo);

    ASSERT_EQ(folder->GetInfo().GetChildren().Count(), 0);

    o2Assets.RefreshCachedAssetsInfo(tree);

    ASSERT_EQ(folder->GetInfo().GetChildren().Count(), 1);
    EXPECT_EQ(folder->GetInfo().GetChildren()[0]->path, "refreshtest_folder_a/child.bin");
}

// The actual bug: cached folder asset is loaded with N children, then a file is removed
// and the tree is reloaded. RefreshCachedAssetsInfo must drop the stale child from the
// cached mInfo. Without the refresh, mInfo.mChildren keeps the deleted entry.
TEST(Assets, RefreshCachedAssetsInfoDropsStaleChildrenAfterFileRemoval) {
    auto folder = mmake<FolderAsset>();
    auto folderMeta = folder->GetMeta();

    // First "load": folder has one child.
    {
        auto tree = mmake<AssetsTree>();
        tree->assetsPath = "Local/";

        auto folderInfo = mmake<AssetInfo>(folderMeta);
        folderInfo->path = "refreshtest_folder_b";

        auto childInfo = MakeInfoFor("refreshtest_folder_b/will_be_deleted.bin", MakeBinaryMeta());

        tree->AddAsset(folderInfo);
        tree->AddAsset(childInfo);

        o2Assets.RefreshCachedAssetsInfo(tree);
    }

    ASSERT_EQ(folder->GetInfo().GetChildren().Count(), 1);

    // Second "load": the child file is gone. Tree has the folder with no children.
    {
        auto newTree = mmake<AssetsTree>();
        newTree->assetsPath = "Local/";

        auto folderInfo = mmake<AssetInfo>(folderMeta);
        folderInfo->path = "refreshtest_folder_b";

        newTree->AddAsset(folderInfo);

        o2Assets.RefreshCachedAssetsInfo(newTree);
    }

    EXPECT_EQ(folder->GetInfo().GetChildren().Count(), 0);
}

#endif
