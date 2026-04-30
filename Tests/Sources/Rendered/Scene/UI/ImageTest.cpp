#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Image, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto img = mmake<Image>();
    ASSERT_TRUE(img);
}

TEST(Image, CopyPreservesImageRef)
{
    SceneCleanGuard guard;
    auto src = mmake<Image>();
    auto sprite = mmake<Sprite>();
    src->SetImage(sprite);
    auto copy = src->CloneAsRef<Image>();
    EXPECT_TRUE(copy->GetImage());
}

// ===== Image sprite =====

TEST(Image, SetImageRoundTrip)
{
    SceneCleanGuard guard;
    auto img = mmake<Image>();
    auto sprite = mmake<Sprite>();
    img->SetImage(sprite);
    EXPECT_EQ(img->GetImage(), sprite);
}

// ===== Image name =====

TEST(Image, SetImageNameRoundTripOrEmpty)
{
    SceneCleanGuard guard;
    auto img = mmake<Image>();
    img->SetImageName("non_existing_42");
    EXPECT_TRUE(img->GetImageName() == "non_existing_42" || img->GetImageName().IsEmpty());
}

// ===== Image asset =====

TEST(Image, SetEmptyImageAssetIsRetrievable)
{
    SceneCleanGuard guard;
    auto img = mmake<Image>();
    AssetRef<ImageAsset> empty;
    img->SetImageAsset(empty);
    EXPECT_FALSE(img->GetImageAsset());
}
