#include "o2/stdafx.h"
#include "MaterialAsset.h"

#include "o2/Assets/Assets.h"

namespace o2
{
    MaterialAsset::MaterialAsset():
        AssetWithDefaultMeta<MaterialAsset>(),
        Material()
    {}

    MaterialAsset::MaterialAsset(const MaterialAsset& asset):
        AssetWithDefaultMeta<MaterialAsset>(asset),
        Material(asset),
        mVertexShaderAsset(asset.mVertexShaderAsset),
        mFragmentShaderAsset(asset.mFragmentShaderAsset)
    {
        RebuildMaterial();
    }

    MaterialAsset& MaterialAsset::operator=(const MaterialAsset& asset)
    {
        Asset::operator=(asset);

        mVertexShaderAsset = asset.mVertexShaderAsset;
        mFragmentShaderAsset = asset.mFragmentShaderAsset;

        Material::SetVertexShader(asset.Material::GetVertexShader());
        Material::SetFragmentShader(asset.Material::GetFragmentShader());
        SetTexture(asset.GetTexture());
        Material::SetBlendMode(asset.Material::GetBlendMode());

        mParams.Clear();
        for (auto& param : asset.mParams)
            mParams.Add(param->CloneAsRef<IShaderParam>());
        mSamplers = asset.mSamplers;

        RebuildMaterial();

        return *this;
    }

    void MaterialAsset::SetVertexShader(const AssetRef<VertexShaderAsset>& shader)
    {
        mVertexShaderAsset = shader;
        RebuildMaterial();
    }

    const AssetRef<VertexShaderAsset>& MaterialAsset::GetVertexShader() const
    {
        return mVertexShaderAsset;
    }

    void MaterialAsset::SetFragmentShader(const AssetRef<FragmentShaderAsset>& shader)
    {
        mFragmentShaderAsset = shader;
        RebuildMaterial();
    }

    const AssetRef<FragmentShaderAsset>& MaterialAsset::GetFragmentShader() const
    {
        return mFragmentShaderAsset;
    }

    Vector<String> MaterialAsset::GetFileExtensions()
    {
        return { "mat" };
    }

    Ref<RefCounterable> MaterialAsset::CastToRefCounterable(const Ref<MaterialAsset>& ref)
    {
        return DynamicCast<Asset>(ref);
    }

    void MaterialAsset::RebuildMaterial()
    {
        if (mVertexShaderAsset && mVertexShaderAsset->GetShader())
            Material::SetVertexShader(mVertexShaderAsset->GetShader());
        else
            Material::SetVertexShader(Ref<Shader>());

        if (mFragmentShaderAsset && mFragmentShaderAsset->GetShader())
            Material::SetFragmentShader(mFragmentShaderAsset->GetShader());
        else
            Material::SetFragmentShader(Ref<Shader>());

        Build();
    }

	void MaterialAsset::OnDeserialized(const DataValue& node)
	{
		RebuildMaterial();
	}

	void MaterialAsset::OnDeserializedDelta(const DataValue& node, const IObject& origin)
	{
        OnDeserialized(node);
	}

}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::MaterialAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::MaterialAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::MaterialAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::MaterialAsset>>);
// --- META ---

DECLARE_CLASS(o2::MaterialAsset, o2__MaterialAsset);
// --- END META ---
