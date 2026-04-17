#include "o2/stdafx.h"
#include "SpineAtlasAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Render/Spine/SpineManager.h"

namespace o2
{
    SpineAtlasAsset::SpineAtlasAsset()
    {}

    SpineAtlasAsset::SpineAtlasAsset(const SpineAtlasAsset& other):
        AssetWithDefaultMeta<SpineAtlasAsset>(other)
    {}

    SpineAtlasAsset::~SpineAtlasAsset()
    {
        if (mAtlas)
        {
            delete mAtlas;
            mAtlas = nullptr;
        }
    }

    SpineAtlasAsset& SpineAtlasAsset::operator=(const SpineAtlasAsset& other)
    {
        Asset::operator=(other);
        return *this;
    }

    spine::Atlas* SpineAtlasAsset::GetSpineAtlas()
    {
        return mAtlas;
    }

    Vector<String> SpineAtlasAsset::GetFileExtensions()
    {
        return { "spine-atlas" };
    }

    void SpineAtlasAsset::LoadData(const String& path)
    {
#ifdef PLATFORM_ANDROID
        // spine::Atlas(path, ...) uses fopen — can't reach APK assets on Android.
        // Read bytes via our FileSystem (AAssetManager-aware), then use the
        // (data, length, dir) constructor.
        InFile file(path);
        if (!file.IsOpened())
        {
            o2Debug.LogError("Failed to open spine atlas file: %s", path.Data());
            mAtlas = nullptr;
            return;
        }
        UInt length = file.GetDataSize();
        char* buffer = mnew char[length];
        file.ReadData(buffer, length);
        String dir = FileSystem::GetParentPath(path);
        mAtlas = new spine::Atlas(buffer, (int)length, dir.Data(),
                                  &SpineManager::Instance().textureLoader);
        delete[] buffer;
#else
        mAtlas = new spine::Atlas(path.Data(), &SpineManager::Instance().textureLoader);
#endif

        if (mAtlas->getPages().size() == 0)
        {
            o2Debug.LogError("Failed to load spine atlas: %s", path.Data());
            delete mAtlas;
            mAtlas = nullptr;
        }
    }

    void SpineAtlasAsset::SaveData(const String& path) const
    {}
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::SpineAtlasAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::SpineAtlasAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::SpineAtlasAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::SpineAtlasAsset>>);
// --- META ---

DECLARE_CLASS(o2::SpineAtlasAsset, o2__SpineAtlasAsset);
// --- END META ---
