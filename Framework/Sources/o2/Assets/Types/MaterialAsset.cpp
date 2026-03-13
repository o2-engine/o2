#include "o2/stdafx.h"
#include "MaterialAsset.h"

#include "o2/Assets/Assets.h"

namespace o2
{
    MaterialAsset::MaterialAsset():
        AssetWithDefaultMeta<MaterialAsset>()
    {}

    MaterialAsset::MaterialAsset(const MaterialAsset& asset):
        AssetWithDefaultMeta<MaterialAsset>(asset),
        mBlendMode(asset.mBlendMode),
        mVertexShaderAsset(asset.mVertexShaderAsset),
        mFragmentShaderAsset(asset.mFragmentShaderAsset)
    {
        for (auto& param : asset.mParams)
            mParams.Add(param->CloneAsRef<IShaderParam>());

        BuildMaterial();
    }

    MaterialAsset& MaterialAsset::operator=(const MaterialAsset& asset)
    {
        Asset::operator=(asset);
        mBlendMode = asset.mBlendMode;
        mVertexShaderAsset = asset.mVertexShaderAsset;
        mFragmentShaderAsset = asset.mFragmentShaderAsset;

        mParams.Clear();
        for (auto& param : asset.mParams)
            mParams.Add(param->CloneAsRef<IShaderParam>());

        BuildMaterial();
        return *this;
    }

    Ref<Material> MaterialAsset::GetMaterial() const
    {
        return mMaterial;
    }

    void MaterialAsset::SetBlendMode(BlendMode blendMode)
    {
        mBlendMode = blendMode;
        if (mMaterial)
            mMaterial->SetBlendMode(mBlendMode);
    }

    BlendMode MaterialAsset::GetBlendMode() const
    {
        return mBlendMode;
    }

    void MaterialAsset::SetVertexShader(const AssetRef<VertexShaderAsset>& shader)
    {
        mVertexShaderAsset = shader;
        BuildMaterial();
    }

    const AssetRef<VertexShaderAsset>& MaterialAsset::GetVertexShader() const
    {
        return mVertexShaderAsset;
    }

    void MaterialAsset::SetFragmentShader(const AssetRef<FragmentShaderAsset>& shader)
    {
        mFragmentShaderAsset = shader;
        BuildMaterial();
    }

    const AssetRef<FragmentShaderAsset>& MaterialAsset::GetFragmentShader() const
    {
        return mFragmentShaderAsset;
    }

    Vector<String> MaterialAsset::GetFileExtensions()
    {
        return { "mat" };
    }

    void MaterialAsset::LoadData(const String& path)
    {
        Asset::LoadData(path);
        BuildMaterial();
    }

    void MaterialAsset::SaveData(const String& path) const
    {
        Asset::SaveData(path);
    }

    void MaterialAsset::BuildMaterial()
    {
        mMaterial = mmake<Material>();

        if (mVertexShaderAsset && mVertexShaderAsset->GetShader())
            mMaterial->SetVertexShader(mVertexShaderAsset->GetShader());

        if (mFragmentShaderAsset && mFragmentShaderAsset->GetShader())
            mMaterial->SetFragmentShader(mFragmentShaderAsset->GetShader());

        mMaterial->SetParams(mParams);
        mMaterial->Build();
        mMaterial->SetBlendMode(mBlendMode);
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::MaterialAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::MaterialAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::MaterialAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::MaterialAsset>>);
// --- META ---

DECLARE_CLASS(o2::MaterialAsset, o2__MaterialAsset);
// --- END META ---
