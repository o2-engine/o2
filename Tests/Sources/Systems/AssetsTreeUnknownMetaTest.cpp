#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/AssetInfo.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Meta.h"
#include "o2/Assets/Types/BinaryAsset.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/UID.h"

using namespace o2;

// A meta written by a build that knows more asset types (the editor) must not break a build that does not
TEST(AssetsTreeUnknownMeta, FallsBackToBinaryAssetKeepingId)
{
    String dir = "./assets-tree-unknown-" + (String)(int)Math::Random(0, 1000000) + "/";
    o2FileSystem.FolderCreate(dir, true);

    UID id;
    id.Randomize();

    DataDocument asset;
    asset["graph"] = 1;
    asset.SaveToFile(dir + "thing.pipeline");

    DataDocument meta;
    meta["Type"] = "o2::DefaultAssetMeta<Nope::EditorOnlyAsset>";
    meta["Value"]["mId"] = id;
    meta.SaveToFile(dir + "thing.pipeline.meta");

    auto tree = mmake<AssetsTree>();
    tree->Build(dir);

    auto info = tree->Find("thing.pipeline");
    ASSERT_TRUE(info);
    ASSERT_TRUE(info->meta);
    EXPECT_EQ(info->meta->GetAssetType(), &TypeOf(BinaryAsset));
    EXPECT_EQ(info->meta->ID(), id);

    tree = nullptr;
    o2FileSystem.FolderRemove(dir, true);
}

// The built tree of another build may name asset types this one lacks; those entries stay usable as binary assets
TEST(AssetsTreeUnknownMeta, BuiltTreeEntryWithUnknownMetaReadsAsBinary)
{
    UID id;
    id.Randomize();

    String json = "{\"assetsPath\": \"x/\", \"builtAssetsPath\": \"y/\", \"rootAssets\": [ { \"Type\": \"o2::AssetInfo\", \"Value\": { "
        "\"path\": \"thing.pipeline\", \"meta\": { \"Type\": \"o2::DefaultAssetMeta<Nope::EditorOnlyAsset>\", \"Value\": { \"mId\": \"" +
        (String)id + "\" } } } } ] }";

    auto tree = mmake<AssetsTree>();
    tree->DeserializeFromString(json);

    auto info = tree->Find("thing.pipeline");
    ASSERT_TRUE(info);
    ASSERT_TRUE(info->meta);
    EXPECT_EQ(info->meta->GetAssetType(), &TypeOf(BinaryAsset));
    EXPECT_EQ(info->meta->ID(), id);
    EXPECT_TRUE(tree->allAssetsByUID.ContainsKey(id));
}
