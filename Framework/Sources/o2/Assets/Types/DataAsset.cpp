#include "o2/stdafx.h"
#include "DataAsset.h"

#include "o2/Assets/Assets.h"

namespace o2
{
    DataAsset::DataAsset()
    {}

    DataAsset::DataAsset(const DataAsset& other):
        AssetWithDefaultMeta<DataAsset>(other), data(const_cast<DataDocument&>(other.data))
    {}

    DataAsset& DataAsset::operator=(const DataAsset& asset)
    {
        Asset::operator=(asset);
        data = const_cast<DataDocument&>(asset.data);

        return *this;
    }

    String DataAsset::GetJson() const
    {
        return data.SaveAsString(DataDocument::Format::JSON);
    }

    Vector<String> DataAsset::GetFileExtensions()
    {
        return { "json" };
    }

    void DataAsset::LoadData(const String& path)
    {
        data.Clear();
        data.LoadFromFile(path);
    }

    void DataAsset::SaveData(const String& path) const
    {
        data.SaveToFile(path);
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::DataAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::DataAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::DataAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::DataAsset>>);
// --- META ---

DECLARE_CLASS(o2::DataAsset, o2__DataAsset);
// --- END META ---
