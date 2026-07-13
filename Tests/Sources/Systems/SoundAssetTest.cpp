#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <filesystem>

#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Utils/Types/UID.h"
#include "Sound/SoundTestHelpers.h"

using namespace o2;

namespace
{
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
            mTempDir = fs::temp_directory_path()/("o2test_sound_" + std::string((String)dirUid));
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

TEST(SoundAsset, ExtensionsAreRegistered)
{
    auto map = Assets::GetAssetsExtensionsTypes();
    for (auto& extension : SoundAsset::GetFileExtensions())
    {
        ASSERT_TRUE(map.ContainsKey(extension));
        EXPECT_EQ(map[extension], &TypeOf(SoundAsset));
    }
}

TEST(SoundAsset, SetDataParsesFormatInfo)
{
    auto asset = MakeTestSoundAsset(0.5f, 44100);

    EXPECT_NEAR(asset->GetDuration(), 0.5f, 0.01f);
    EXPECT_EQ(asset->GetChannelsCount(), 1);
    EXPECT_EQ(asset->GetSampleRate(), 44100);
}

TEST(SoundAsset, EmptyAssetHasZeroDuration)
{
    auto asset = mmake<SoundAsset>();

    EXPECT_EQ(asset->GetDuration(), 0.0f);
    EXPECT_EQ(asset->GetChannelsCount(), 0);
    EXPECT_EQ(asset->GetDataSize(), 0u);
}

TEST(SoundAsset, InvalidDataGivesZeroDuration)
{
    std::vector<char> junk(128, 'x');

    auto asset = mmake<SoundAsset>();
    asset->SetData(junk.data(), (UInt)junk.size());

    EXPECT_EQ(asset->GetDuration(), 0.0f);
}

TEST(SoundAsset, CopyPreservesData)
{
    auto asset = MakeTestSoundAsset(0.25f);

    auto copy = asset->CloneAsRef<SoundAsset>();

    ASSERT_EQ(copy->GetDataSize(), asset->GetDataSize());
    EXPECT_EQ(memcmp(copy->GetData(), asset->GetData(), asset->GetDataSize()), 0);
    EXPECT_NEAR(copy->GetDuration(), 0.25f, 0.01f);
}

TEST(SoundAsset, SaveWritesRawFileAndMeta)
{
    TempAssetsPathGuard pathGuard;

    auto wav = BuildTestWav(0.3f);

    auto asset = mmake<SoundAsset>();
    asset->SetData(wav.data(), (UInt)wav.size());
    asset->SetPath("test_sound.wav");
    asset->Save();

    auto fullPath = pathGuard.GetTempDir()/"test_sound.wav";
    ASSERT_TRUE(std::filesystem::exists(fullPath));
    ASSERT_TRUE(std::filesystem::exists(pathGuard.GetTempDir()/"test_sound.wav.meta"));

    ASSERT_EQ(std::filesystem::file_size(fullPath), wav.size());

    std::ifstream file(fullPath, std::ios::binary);
    std::vector<char> loaded((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_EQ(memcmp(loaded.data(), wav.data(), wav.size()), 0);
}
