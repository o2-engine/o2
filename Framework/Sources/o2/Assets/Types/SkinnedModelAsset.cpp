#include "o2/stdafx.h"
#include "SkinnedModelAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    SkinnedModelAsset::SkinnedModelAsset()
    {}

    SkinnedModelAsset::SkinnedModelAsset(const SkinnedModelAsset& other):
        AssetWithDefaultMeta<SkinnedModelAsset>(other), mModelData(other.mModelData), mRawData(other.mRawData)
    {}

    SkinnedModelAsset& SkinnedModelAsset::operator=(const SkinnedModelAsset& other)
    {
        Asset::operator=(other);

        mModelData = other.mModelData;
        mRawData = other.mRawData;

        return *this;
    }

    const SkinnedModelData& SkinnedModelAsset::GetModelData() const
    {
        return mModelData;
    }

    void SkinnedModelAsset::SetModelData(const SkinnedModelData& data)
    {
        mModelData = data;
        mRawData.Clear();
    }

    bool SkinnedModelAsset::LoadFromGlb(const UInt8* data, UInt size, String* errorMessage /*= nullptr*/)
    {
        if (!GlbModelFormat::Parse(data, size, mModelData, errorMessage))
            return false;

        mRawData.Clear();
        mRawData.Resize((int)size);
        memcpy(mRawData.Data(), data, size);

        return true;
    }

    void SkinnedModelAsset::LoadData(const String& path)
    {
        if (FileSystem::GetFileExtension(path).ToLowerCase() == "glb")
        {
            InFile file(path);
            if (!file.IsOpened())
            {
                o2Debug.LogError("Failed to open skinned model file " + path);
                return;
            }

            Vector<UInt8> data;
            data.Resize((int)file.GetDataSize());
            file.ReadData(data.Data(), (UInt)data.Count());

            String error;
            if (!GlbModelFormat::Parse(data.Data(), (UInt)data.Count(), mModelData, &error))
                o2Debug.LogError("Failed to load skinned model " + path + ": " + error);
            else
                mRawData = data;
        }
        else
            Asset::LoadData(path);
    }

    void SkinnedModelAsset::SaveData(const String& path) const
    {
        if (FileSystem::GetFileExtension(path).ToLowerCase() == "glb")
        {
            if (!mRawData.IsEmpty())
            {
                OutFile file(path);
                file.WriteData(const_cast<Vector<UInt8>&>(mRawData).Data(), (UInt)mRawData.Count());
            }
        }
        else
            Asset::SaveData(path);
    }

    Vector<String> SkinnedModelAsset::GetFileExtensions()
    {
        return { "glb" };
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::SkinnedModelAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::SkinnedModelAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::SkinnedModelAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::SkinnedModelAsset>>);
// --- META ---

DECLARE_CLASS(o2::SkinnedModelAsset, o2__SkinnedModelAsset);
// --- END META ---
