#include "o2/stdafx.h"
#include "SpineAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include "spine/SkeletonJson.h"

namespace o2
{
    SpineAsset::SpineAsset()
    {}

    SpineAsset::SpineAsset(const SpineAsset& other):
        AssetWithDefaultMeta<SpineAsset>(other)
    {}

    SpineAsset::~SpineAsset()
    {
        if (mSkeletonData)
        {
            delete mSkeletonData;
            mSkeletonData = nullptr;
        }

        if (mAnimationStateData)
        {
            delete mAnimationStateData;
            mAnimationStateData = nullptr;
        }
    }

    SpineAsset& SpineAsset::operator=(const SpineAsset& other)
    {
        Asset::operator=(other);
        return *this;
    }

    spine::SkeletonData* SpineAsset::GetSpineSkeletonData()
    {
        return mSkeletonData;
    }

    spine::AnimationStateData* SpineAsset::GetSpineAnimationStateData()
    {
        return mAnimationStateData;
    }

    Vector<String> SpineAsset::GetFileExtensions()
    {
        return { "spine-json" };
    }

    void SpineAsset::LoadData(const String& path)
    {
        auto atlasAssetPath = o2FileSystem.GetFileNameWithoutExtension(GetPath()) + ".spine-atlas";
        auto atlasAsset = o2Assets.GetAssetRefByType<SpineAtlasAsset>(atlasAssetPath);

        if (!atlasAsset)
        {
            o2Debug.LogError("Spine asset %s has no atlas", atlasAssetPath.Data());
            return;
        }

        auto spineAtlas = atlasAsset->GetSpineAtlas();
        if (spineAtlas == nullptr)
        {
            o2Debug.LogError("Failed to load spine atlas for spine asset: %s", path.Data());
            return;
        }

        spine::SkeletonJson json(spineAtlas);
#ifdef PLATFORM_ANDROID
        // spine::SkeletonJson::readSkeletonDataFile uses fopen — can't reach APK assets.
        InFile file(path);
        if (!file.IsOpened())
        {
            o2Debug.LogError("Failed to open spine json file: %s", path.Data());
            return;
        }
        String jsonText = file.ReadFullData();
        mSkeletonData = json.readSkeletonData(jsonText.Data());
#else
        mSkeletonData = json.readSkeletonDataFile(path.Data());
#endif

        if (!mSkeletonData)
        {
            o2Debug.LogError("Failed to load spine asset: %s", path.Data());
            return;
        }

        mAnimationStateData = new spine::AnimationStateData(mSkeletonData);
    }

    void SpineAsset::SaveData(const String& path) const
    {}
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::SpineAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::SpineAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::SpineAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::SpineAsset>>);
// --- META ---

DECLARE_CLASS(o2::SpineAsset, o2__SpineAsset);
// --- END META ---
