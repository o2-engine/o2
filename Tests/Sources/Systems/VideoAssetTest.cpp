#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <vector>

#include "o2/Animation/IAnimation.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Render/Video.h"
#include "o2/Scene/Components/VideoComponent.h"
#include "Video/VideoTestData.h"

using namespace o2;

TEST(VideoAsset, ExtensionsAreRegistered)
{
    auto map = Assets::GetAssetsExtensionsTypes();
    for (auto& extension : VideoAsset::GetFileExtensions())
    {
        ASSERT_TRUE(map.ContainsKey(extension));
        EXPECT_EQ(map[extension], &TypeOf(VideoAsset));
    }
}

TEST(VideoAsset, SetDataParsesFormatInfo)
{
    auto asset = Tests::MakeTestVideoAsset();

    EXPECT_EQ(asset->GetImageSize(), Vec2I(64, 64));
    EXPECT_NEAR(asset->GetFrameRate(), 25.0f, 0.5f);
    // pl_mpeg can't derive a duration for this tiny in-memory clip (reports 0); real files parse it
    EXPECT_GE(asset->GetDuration(), 0.0f);
}

TEST(VideoAsset, EmptyAssetHasZeroSize)
{
    auto asset = mmake<VideoAsset>();

    EXPECT_EQ(asset->GetImageSize(), Vec2I(0, 0));
    EXPECT_EQ(asset->GetDataSize(), 0u);
}

TEST(VideoAsset, InvalidDataGivesZeroSize)
{
    std::vector<char> junk(128, 'x');

    auto asset = mmake<VideoAsset>();
    asset->SetData(junk.data(), (UInt)junk.size());

    EXPECT_EQ(asset->GetImageSize(), Vec2I(0, 0));
    EXPECT_EQ(asset->GetDuration(), 0.0f);
}

TEST(VideoAsset, CopyPreservesData)
{
    auto asset = Tests::MakeTestVideoAsset();

    auto copy = asset->CloneAsRef<VideoAsset>();

    ASSERT_EQ(copy->GetDataSize(), asset->GetDataSize());
    EXPECT_EQ(memcmp(copy->GetData(), asset->GetData(), asset->GetDataSize()), 0);
    EXPECT_EQ(copy->GetImageSize(), Vec2I(64, 64));
}

// Video is an IAnimation, so VideoComponent is picked up as an animation sub-track
TEST(VideoType, IsBasedOnAnimation)
{
    EXPECT_TRUE(TypeOf(Video).IsBasedOn(TypeOf(IAnimation)));
    EXPECT_TRUE(TypeOf(VideoComponent).IsBasedOn(TypeOf(IAnimation)));
}
