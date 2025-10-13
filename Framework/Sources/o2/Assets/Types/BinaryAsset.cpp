#include "o2/stdafx.h"
#include "BinaryAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    BinaryAsset::BinaryAsset()
    {}

    BinaryAsset::BinaryAsset(const BinaryAsset& other):
        AssetWithDefaultMeta<BinaryAsset>(other)
    {
        if (other.mDataSize > 0)
        {
            mDataSize = other.mDataSize;
            mData = mnew char[mDataSize];
            memcpy(mData, other.mData, mDataSize);
        }
        else
        {
            mDataSize = 0;
            mData = nullptr;
        }
    }

    BinaryAsset::~BinaryAsset()
    {
        if (mData)
            delete[] mData;
    }

    BinaryAsset& BinaryAsset::operator=(const BinaryAsset& other)
    {
        Asset::operator=(other);

        if (mData)
            delete[] mData;

        if (other.mDataSize > 0)
        {
            mDataSize = other.mDataSize;
            mData = mnew char[mDataSize];
            memcpy(mData, other.mData, mDataSize);
        }
        else
        {
            mDataSize = 0;
            mData = nullptr;
        }

        return *this;
    }

    char* BinaryAsset::GetData() const
    {
        return mData;
    }

    UInt BinaryAsset::GetDataSize() const
    {
        return mDataSize;
    }

    void BinaryAsset::SetData(char* data, UInt size)
    {
        if (mData)
            delete[] mData;

        if (size > 0)
        {
            mDataSize = size;
            mData = mnew char[mDataSize];
            memcpy(mData, data, mDataSize);
        }
        else
        {
            mDataSize = 0;
            mData = nullptr;
        }
    }

    Vector<String> BinaryAsset::GetFileExtensions()
    {
        return { "bin" };
    }

    void BinaryAsset::LoadData(const String& path)
    {
        InFile file(path);
        if (!file.IsOpened())
            GetAssetsLogStream()->Error("Failed to load binary asset data: can't open file " + path);

        mDataSize = file.GetDataSize();
        mData = mnew char[mDataSize];
        file.ReadFullData(mData);
    }

    void BinaryAsset::SaveData(const String& path) const
    {
        OutFile file(path);
        if (mDataSize > 0 && mData)
            file.WriteData(mData, mDataSize);
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::BinaryAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::BinaryAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::BinaryAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::BinaryAsset>>);
// --- META ---

DECLARE_CLASS(o2::BinaryAsset, o2__BinaryAsset);
// --- END META ---
