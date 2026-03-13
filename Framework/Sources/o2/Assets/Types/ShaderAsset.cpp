#include "o2/stdafx.h"
#include "ShaderAsset.h"

namespace o2
{
    ShaderAsset::ShaderAsset():
        Asset()
    {}

    ShaderAsset::ShaderAsset(const Ref<AssetMeta>& meta):
        Asset(meta)
    {}

    ShaderAsset::ShaderAsset(const ShaderAsset& asset):
        Asset(asset),
        mShader(asset.mShader)
    {}

    ShaderAsset& ShaderAsset::operator=(const ShaderAsset& asset)
    {
        Asset::operator=(asset);
        mShader = asset.mShader;
        return *this;
    }

    Ref<Shader> ShaderAsset::GetShader() const
    {
        return mShader;
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::ShaderAsset>);
// --- META ---

DECLARE_CLASS(o2::ShaderAsset, o2__ShaderAsset);
// --- END META ---
