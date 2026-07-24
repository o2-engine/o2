#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <filesystem>

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/BinaryAsset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/UID.h"

using namespace o2;

// ===== Asset =====

TEST(Asset, MmakeProducesUniqueUIDs) {
    auto a = mmake<BinaryAsset>();
    auto b = mmake<BinaryAsset>();
    EXPECT_FALSE(a->GetUID() == b->GetUID());
}

TEST(Asset, SetDirtyTogglesFlag) {
    auto a = mmake<BinaryAsset>();
    EXPECT_FALSE(a->IsDirty());
    a->SetDirty(true);
    EXPECT_TRUE(a->IsDirty());
    a->SetDirty(false);
    EXPECT_FALSE(a->IsDirty());
}

TEST(Asset, GetFullPathPrependsTreeAssetsPath) {
    auto a = mmake<BinaryAsset>();
    a->SetPath("standalone/file.bin");
    String prefix = o2Assets.GetAssetsTree().assetsPath;
    EXPECT_EQ(a->GetFullPath(), prefix + "standalone/file.bin");
}

TEST(Asset, SetPathRandomizesUIDAndUpdatesCache) {
    auto a = mmake<BinaryAsset>();
    auto oldUid = a->GetUID();

    a->SetPath("renamed/file.bin");
    auto newUid = a->GetUID();

    EXPECT_FALSE(oldUid == newUid);
    EXPECT_FALSE(o2Assets.GetAssetRef(oldUid).IsValid());
    auto cached = o2Assets.GetAssetRef(newUid);
    EXPECT_TRUE(cached.IsValid());
    EXPECT_EQ(cached.GetAssetBase(), a.Get());
}

TEST(Asset, PostRefConstructRegistersInAssetsCache) {
    auto a = mmake<BinaryAsset>();
    auto cached = o2Assets.GetAssetRef(a->GetUID());
    ASSERT_TRUE(cached.IsValid());
    EXPECT_EQ(cached.GetAssetBase(), a.Get());
}

TEST(Asset, SaveCreatesAssetAndMetaFilesWithExpectedContent) {
    namespace fs = std::filesystem;

    auto& tree = const_cast<AssetsTree&>(o2Assets.GetAssetsTree());
    String origAssetsPath = tree.assetsPath;

    UID dirUid;
    dirUid.Randomize();
    auto tempDir = fs::temp_directory_path() / ("o2test_save_" + std::string((String)dirUid));
    fs::create_directories(tempDir);

    String tempPrefix(tempDir.string().c_str());
    tempPrefix.ReplaceAll("\\", "/");
    if (!tempPrefix.EndsWith("/"))
        tempPrefix += "/";
    tree.assetsPath = tempPrefix;

    auto fullPath = tempDir / "data_test.json";
    auto metaPath = tempDir / "data_test.json.meta";

    {
        auto asset = mmake<DataAsset>();
        asset->data.Set(String("hello"));
        asset->SetPath("data_test.json");
        asset->Save();
    }

    EXPECT_TRUE(fs::exists(fullPath));
    EXPECT_TRUE(fs::exists(metaPath));

    if (fs::exists(fullPath))
    {
        DataDocument loaded;
        ASSERT_TRUE(loaded.LoadFromFile(String(fullPath.string().c_str())));
        String value;
        loaded.Get(value);
        EXPECT_EQ(value, String("hello"));
    }

    std::error_code ec;
    fs::remove_all(tempDir, ec);
    tree.assetsPath = origAssetsPath;
}

// Hand-written data may reference an asset by path alone, without the id
TEST(AssetRef, DeserializeResolvesByPathWithoutId) {
    namespace fs = std::filesystem;

    auto& tree = const_cast<AssetsTree&>(o2Assets.GetAssetsTree());
    String origAssetsPath = tree.assetsPath;

    UID dirUid;
    dirUid.Randomize();
    auto tempDir = fs::temp_directory_path() / ("o2test_pathref_" + std::string((String)dirUid));
    fs::create_directories(tempDir);

    String tempPrefix(tempDir.string().c_str());
    tempPrefix.ReplaceAll("\\", "/");
    if (!tempPrefix.EndsWith("/"))
        tempPrefix += "/";
    tree.assetsPath = tempPrefix;

    {
        auto asset = mmake<DataAsset>();
        asset->data.Set(String("hello"));
        asset->SetPath("path_ref_test.json");
        asset->Save();
    }

    DataDocument node;
    node["path"] = String("path_ref_test.json");

    AssetRef<DataAsset> ref;
    ref.Deserialize(node);

    EXPECT_TRUE(ref.IsValid());
    if (ref)
        EXPECT_EQ(ref->GetPath(), String("path_ref_test.json"));

    std::error_code ec;
    fs::remove_all(tempDir, ec);
    tree.assetsPath = origAssetsPath;
}

// ===== AssetRef =====

TEST(AssetRef, DefaultIsInvalid) {
    AssetRef<BinaryAsset> ref;
    EXPECT_FALSE(ref.IsValid());
    EXPECT_FALSE(static_cast<bool>(ref));
}

TEST(AssetRef, ConstructFromRawPointer) {
    auto a = mmake<BinaryAsset>();
    AssetRef<BinaryAsset> ref(a.Get());
    EXPECT_TRUE(ref.IsValid());
    EXPECT_EQ(ref.Get(), a.Get());
    EXPECT_FALSE(ref.IsInstance());
}

TEST(AssetRef, EqualityRequiresSamePtrAndInstanceFlag) {
    auto a = mmake<BinaryAsset>();
    AssetRef<BinaryAsset> r1(a.Get());
    AssetRef<BinaryAsset> r2(a.Get());
    EXPECT_TRUE(r1 == r2);

    AssetRef<BinaryAsset> instance(a.Get());
    instance.SetInstance(a.Get());
    EXPECT_FALSE(r1 == instance);
}

TEST(AssetRef, GetAssetTypeReturnsTemplateType) {
    AssetRef<BinaryAsset> ref;
    EXPECT_EQ(&ref.GetAssetType(), &TypeOf(BinaryAsset));
    EXPECT_EQ(AssetRef<BinaryAsset>::GetAssetTypeStatic(), &TypeOf(BinaryAsset));

    AssetRef<DataAsset> dref;
    EXPECT_EQ(&dref.GetAssetType(), &TypeOf(DataAsset));
}

TEST(AssetRef, PolymorphicConversionDerivedToBase) {
    auto a = mmake<BinaryAsset>();
    AssetRef<BinaryAsset> derived(a.Get());
    derived.SetInstance(a.Get());

    AssetRef<Asset> base = derived;
    EXPECT_EQ(base.Get(), a.Get());
    EXPECT_TRUE(base.IsInstance());
}

TEST(AssetRef, BaseAssetRefCopyConstructorPreservesInstanceFlag) {
    auto a = mmake<BinaryAsset>();
    AssetRef<BinaryAsset> source(a.Get());
    source.SetInstance(a.Get());

    const BaseAssetRef& asBase = source;
    AssetRef<BinaryAsset> copy(asBase);
    EXPECT_EQ(copy.Get(), a.Get());
    EXPECT_TRUE(copy.IsInstance());
}

TEST(AssetRef, SetAssetBaseClearsInstanceFlag) {
    auto a = mmake<BinaryAsset>();
    AssetRef<BinaryAsset> ref;
    ref.SetInstance(a.Get());
    ASSERT_TRUE(ref.IsInstance());

    ref.SetAssetBase(a.Get());
    EXPECT_FALSE(ref.IsInstance());
    EXPECT_EQ(ref.Get(), a.Get());
}

TEST(AssetRef, SetInstanceSetsInstanceFlag) {
    auto a = mmake<BinaryAsset>();
    AssetRef<BinaryAsset> ref;
    ref.SetInstance(a.Get());
    EXPECT_TRUE(ref.IsInstance());
    EXPECT_EQ(ref.Get(), a.Get());
}

TEST(AssetRef, CreateInstanceWithEmptyRefCreatesEmptyOfTemplateType) {
    AssetRef<BinaryAsset> ref;
    ref.CreateInstance();

    ASSERT_TRUE(ref.IsValid());
    EXPECT_TRUE(ref.IsInstance());
    EXPECT_EQ(&ref->GetType(), &TypeOf(BinaryAsset));
}

TEST(AssetRef, CreateInstanceClonesExistingAssetWithDifferentUID) {
    auto a = mmake<DataAsset>();
    a->data.Set(String("source"));
    AssetRef<DataAsset> ref(a.Get());
    auto sourceUid = a->GetUID();

    ref.CreateInstance();

    ASSERT_TRUE(ref.IsValid());
    EXPECT_TRUE(ref.IsInstance());
    EXPECT_NE(ref.Get(), a.Get());
    EXPECT_FALSE(ref->GetUID() == sourceUid);
}

TEST(AssetRef, RemoveInstanceNullifiesPtrAndFlag) {
    AssetRef<BinaryAsset> ref;
    ref.CreateInstance();
    ASSERT_TRUE(ref.IsInstance());

    ref.RemoveInstance();
    EXPECT_FALSE(ref.IsValid());
    EXPECT_FALSE(ref.IsInstance());
}

TEST(AssetRef, RemoveInstanceIsNoOpForNonInstance) {
    auto a = mmake<BinaryAsset>();
    AssetRef<BinaryAsset> ref(a.Get());
    ASSERT_FALSE(ref.IsInstance());

    ref.RemoveInstance();
    EXPECT_TRUE(ref.IsValid());
    EXPECT_EQ(ref.Get(), a.Get());
}

TEST(AssetRef, SerializeNonInstanceWritesIdAndPath) {
    auto a = mmake<DataAsset>();
    a->SetPath("ref/test.json");
    AssetRef<DataAsset> ref(a.Get());

    DataDocument doc;
    ref.Serialize(doc);

    EXPECT_TRUE(doc.FindMember("id") != nullptr);
    EXPECT_TRUE(doc.FindMember("path") != nullptr);
    EXPECT_TRUE(doc.FindMember("instance") == nullptr);
}

TEST(AssetRef, SerializeInstanceWritesInstanceAndMeta) {
    auto a = mmake<DataAsset>();
    AssetRef<DataAsset> ref;
    ref.SetInstance(a.Get());

    DataDocument doc;
    ref.Serialize(doc);

    EXPECT_TRUE(doc.FindMember("instance") != nullptr);
    EXPECT_TRUE(doc.FindMember("meta") != nullptr);
    EXPECT_TRUE(doc.FindMember("id") == nullptr);
    EXPECT_TRUE(doc.FindMember("path") == nullptr);
}

TEST(AssetRef, RoundtripInstancePreservesInstanceFlag) {
    auto a = mmake<DataAsset>();
    a->data.Set(String("payload"));
    AssetRef<DataAsset> ref;
    ref.SetInstance(a.Get());

    DataDocument doc;
    ref.Serialize(doc);

    AssetRef<DataAsset> restored;
    restored.Deserialize(doc);

    ASSERT_TRUE(restored.IsValid());
    EXPECT_TRUE(restored.IsInstance());
    EXPECT_NE(restored.Get(), a.Get());
}

TEST(AssetRef, RoundtripPathReferenceFindsCachedAsset) {
    auto a = mmake<DataAsset>();
    a->SetPath("ref/cached_lookup.json");
    AssetRef<DataAsset> ref(a.Get());

    DataDocument doc;
    ref.Serialize(doc);

    AssetRef<DataAsset> restored;
    restored.Deserialize(doc);

    ASSERT_TRUE(restored.IsValid());
    EXPECT_EQ(restored.Get(), a.Get());
    EXPECT_FALSE(restored.IsInstance());
}
