#include "o2/stdafx.h"
#include "FragmentShaderAsset.h"

#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    FragmentShaderAsset::FragmentShaderAsset():
        ShaderAsset(mmake<Meta>())
    {}

    FragmentShaderAsset::FragmentShaderAsset(const FragmentShaderAsset& asset):
        ShaderAsset(asset)
    {}

    FragmentShaderAsset& FragmentShaderAsset::operator=(const FragmentShaderAsset& asset)
    {
        ShaderAsset::operator=(asset);
        return *this;
    }

    Ref<FragmentShaderAsset::Meta> FragmentShaderAsset::GetMeta() const
    {
        return DynamicCast<Meta>(mInfo.meta);
    }

    Vector<String> FragmentShaderAsset::GetFileExtensions()
    {
        return { "fsh" };
    }

    void FragmentShaderAsset::LoadData(const String& path)
    {
		String source = o2FileSystem.ReadFile(path);

        mShader = mmake<Shader>();
        mShader->SetFileName(path);
        mShader->Compile(source, Shader::Type::Fragment);
    }
}

DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::FragmentShaderAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::FragmentShaderAsset>);
// --- META ---

DECLARE_CLASS(o2::FragmentShaderAsset, o2__FragmentShaderAsset);

DECLARE_CLASS(o2::FragmentShaderAsset::Meta, o2__FragmentShaderAsset__Meta);
// --- END META ---
