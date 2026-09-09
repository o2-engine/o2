#include "o2/stdafx.h"
#include "PipelineAsset.h"

#include "o2/Assets/Assets.h"

namespace o2
{
    PipelineAsset::PipelineAsset()
    {}

    PipelineAsset::PipelineAsset(const PipelineAsset& other):
        AssetWithDefaultMeta<PipelineAsset>(other), document(const_cast<DataDocument&>(other.document))
    {}

    PipelineAsset& PipelineAsset::operator=(const PipelineAsset& other)
    {
        Asset::operator=(other);
        document = const_cast<DataDocument&>(other.document);
        return *this;
    }

    Vector<String> PipelineAsset::GetFileExtensions()
    {
        return { "pipeline" };
    }

    void PipelineAsset::LoadData(const String& path)
    {
        document.Clear();
        document.LoadFromFile(path);
    }

    void PipelineAsset::SaveData(const String& path) const
    {
        document.SaveToFile(path);
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::PipelineAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::PipelineAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::PipelineAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::PipelineAsset>>);
// --- META ---

DECLARE_CLASS(o2::PipelineAsset, o2__PipelineAsset);
// --- END META ---
