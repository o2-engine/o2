#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetInfo.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Meta.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/BinaryAsset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/UID.h"

using namespace o2;

namespace {

Ref<AssetMeta> CloneMeta(const Ref<AssetMeta>& src)
{
    return DynamicCast<AssetMeta>(src->CloneAsRef<AssetMeta>());
}

Ref<AssetInfo> MakeInfoWithRandomUID(const String& path)
{
    auto asset = mmake<BinaryAsset>();
    auto info = mmake<AssetInfo>(asset->GetMeta());
    info->path = path;
    return info;
}

} // namespace

// ===== AssetMeta =====

TEST(AssetMeta, DefaultIdIsEmpty) {
    auto meta = mmake<AssetMeta>();
    EXPECT_TRUE(meta->ID() == UID::empty);
}

TEST(AssetMeta, BaseAssetTypeIsAsset) {
    auto meta = mmake<AssetMeta>();
    EXPECT_EQ(meta->GetAssetType(), &TypeOf(Asset));
}

TEST(AssetMeta, DefaultAssetMetaReturnsConcreteType) {
    auto binMeta = mmake<DefaultAssetMeta<BinaryAsset>>();
    auto dataMeta = mmake<DefaultAssetMeta<DataAsset>>();
    EXPECT_EQ(binMeta->GetAssetType(), &TypeOf(BinaryAsset));
    EXPECT_EQ(dataMeta->GetAssetType(), &TypeOf(DataAsset));
}

TEST(AssetMeta, IsEqualClonePreservesIdAndType) {
    auto asset = mmake<BinaryAsset>();
    auto cloned = CloneMeta(asset->GetMeta());
    ASSERT_TRUE(cloned);
    EXPECT_TRUE(asset->GetMeta()->IsEqual(cloned.Get()));
}

TEST(AssetMeta, IsEqualDifferentInstancesNotEqual) {
    auto a1 = mmake<BinaryAsset>();
    auto a2 = mmake<BinaryAsset>();
    EXPECT_FALSE(a1->GetMeta()->IsEqual(a2->GetMeta().Get()));
}

TEST(AssetMeta, IsEqualDifferentTypesNotEqualEvenWithSameEmptyId) {
    auto baseMeta = mmake<AssetMeta>();
    auto binMeta = mmake<DefaultAssetMeta<BinaryAsset>>();
    EXPECT_TRUE(baseMeta->ID() == UID::empty);
    EXPECT_TRUE(binMeta->ID() == UID::empty);
    EXPECT_FALSE(baseMeta->IsEqual(binMeta.Get()));
}

// ===== AssetInfo =====

TEST(AssetInfo, BoolFalseWhenMetaMissing) {
    auto info = mmake<AssetInfo>();
    EXPECT_FALSE(info->IsValid());
    EXPECT_FALSE(static_cast<bool>(*info));
}

TEST(AssetInfo, BoolFalseWhenMetaHasEmptyUID) {
    auto info = mmake<AssetInfo>(mmake<AssetMeta>());
    EXPECT_FALSE(info->IsValid());
}

TEST(AssetInfo, BoolTrueWhenMetaHasNonEmptyUID) {
    auto asset = mmake<BinaryAsset>();
    auto info = mmake<AssetInfo>(asset->GetMeta());
    EXPECT_TRUE(info->IsValid());
}

TEST(AssetInfo, EqualityByMetaIdWhenBothHaveMeta) {
    auto asset = mmake<BinaryAsset>();
    auto info1 = mmake<AssetInfo>(asset->GetMeta());
    info1->path = "first.bin";
    auto info2 = mmake<AssetInfo>(asset->GetMeta());
    info2->path = "second.bin";
    EXPECT_TRUE(*info1 == *info2);
}

TEST(AssetInfo, EqualityFallsBackToPathWithoutMeta) {
    auto a = mmake<AssetInfo>();
    auto b = mmake<AssetInfo>();
    a->meta = nullptr;
    b->meta = nullptr;
    a->path = "same/path.bin";
    b->path = "same/path.bin";
    EXPECT_TRUE(*a == *b);

    b->path = "other.bin";
    EXPECT_FALSE(*a == *b);
}

TEST(AssetInfo, AddChildSetsParentAndAppearsInChildren) {
    auto parent = MakeInfoWithRandomUID("parent");
    auto child = MakeInfoWithRandomUID("parent/child");
    parent->AddChild(child);

    ASSERT_EQ(parent->GetChildren().Count(), 1);
    EXPECT_EQ(parent->GetChildren()[0], child);
    EXPECT_EQ(child->parent.Lock(), parent);
}

TEST(AssetInfo, AddChildReparentsFromOldParent) {
    auto parentA = MakeInfoWithRandomUID("a");
    auto parentB = MakeInfoWithRandomUID("b");
    auto child = MakeInfoWithRandomUID("child");

    parentA->AddChild(child);
    ASSERT_EQ(parentA->GetChildren().Count(), 1);

    parentB->AddChild(child);
    EXPECT_EQ(parentA->GetChildren().Count(), 0);
    ASSERT_EQ(parentB->GetChildren().Count(), 1);
    EXPECT_EQ(child->parent.Lock(), parentB);
}

TEST(AssetInfo, RemoveChildClearsParent) {
    auto parent = MakeInfoWithRandomUID("p");
    auto child = MakeInfoWithRandomUID("p/c");
    parent->AddChild(child);
    parent->RemoveChild(child);

    EXPECT_EQ(parent->GetChildren().Count(), 0);
    EXPECT_FALSE(child->parent.Lock());
}

TEST(AssetInfo, RemoveAllChildrenEmptiesList) {
    auto parent = MakeInfoWithRandomUID("p");
    auto c1 = MakeInfoWithRandomUID("p/c1");
    auto c2 = MakeInfoWithRandomUID("p/c2");
    parent->AddChild(c1);
    parent->AddChild(c2);

    parent->RemoveAllChildren();
    EXPECT_EQ(parent->GetChildren().Count(), 0);
    // Children themselves remain alive via our Ref<>
    EXPECT_TRUE(c1.IsValid());
    EXPECT_TRUE(c2.IsValid());
}

TEST(AssetInfo, SetTreeRegistersInAllMaps) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto info = MakeInfoWithRandomUID("foo.bin");
    auto uid = info->meta->ID();
    info->SetTree(tree);

    EXPECT_EQ(tree->allAssets.Count(), 1);
    EXPECT_TRUE(tree->allAssetsByPath.ContainsKey("foo.bin"));
    EXPECT_TRUE(tree->allAssetsByUID.ContainsKey(uid));
}

TEST(AssetInfo, SetTreeRecursesIntoChildren) {
    auto tree = mmake<AssetsTree>();
    tree->assetsPath = "Local/";

    auto parent = MakeInfoWithRandomUID("dir");
    auto child = MakeInfoWithRandomUID("dir/file.bin");
    parent->AddChild(child);

    parent->SetTree(tree);

    EXPECT_TRUE(tree->allAssetsByPath.ContainsKey("dir"));
    EXPECT_TRUE(tree->allAssetsByPath.ContainsKey("dir/file.bin"));
}

// ---------- AtlasAsset::Meta platform overrides ----------

TEST(AtlasAssetMeta, WebAssemblyPlatformOverrideIsApplied)
{
    auto meta = mmake<AtlasAsset::Meta>();
    meta->common.compression = TextureCompression::ASTC4x4;

    // without an override every platform falls back to the common meta
    EXPECT_EQ(meta->GetResultPlatformMeta(Platform::WebAssembly).compression, TextureCompression::ASTC4x4);

    auto web = mmake<AtlasAsset::PlatformMeta>();
    web->compression = TextureCompression::None;
    meta->webAssembly = web;

    EXPECT_EQ(meta->GetResultPlatformMeta(Platform::WebAssembly).compression, TextureCompression::None);
    EXPECT_EQ(meta->GetResultPlatformMeta(Platform::Mac).compression, TextureCompression::ASTC4x4);
}

TEST(AtlasAssetMeta, WebAssemblyOverrideParticipatesInEquality)
{
    auto makeMeta = [](TextureCompression webCompression) {
        auto meta = mmake<AtlasAsset::Meta>();
        meta->webAssembly = mmake<AtlasAsset::PlatformMeta>();
        meta->webAssembly->compression = webCompression;
        return meta;
    };

    auto a = makeMeta(TextureCompression::None);
    EXPECT_TRUE(a->IsEqual(makeMeta(TextureCompression::None).Get()));
    EXPECT_FALSE(a->IsEqual(makeMeta(TextureCompression::ASTC4x4).Get()));
    EXPECT_FALSE(a->IsEqual(mmake<AtlasAsset::Meta>().Get())); // override vs none differ
}

TEST(AtlasAssetMeta, WebAssemblyOverrideSurvivesSerialization)
{
    auto meta = mmake<AtlasAsset::Meta>();
    meta->webAssembly = mmake<AtlasAsset::PlatformMeta>();
    meta->webAssembly->compression = TextureCompression::None;

    DataDocument doc;
    meta->Serialize(doc);
    printf("serialized meta: %s\n", ((String)doc.SaveAsString()).Data());

    auto loaded = mmake<AtlasAsset::Meta>();
    loaded->Deserialize(doc);
    ASSERT_TRUE(loaded->webAssembly);
    EXPECT_EQ(loaded->webAssembly->compression, TextureCompression::None);
}

TEST(AtlasAssetMeta, ImagesScaleParticipatesInEquality)
{
    AtlasAsset::PlatformMeta a, b;
    EXPECT_TRUE(a == b);

    b.imagesScale = 0.5f;
    EXPECT_FALSE(a == b);
}
