#include "o2/stdafx.h"
#include "VertexShaderAsset.h"

#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    VertexShaderAsset::VertexShaderAsset():
        ShaderAsset(mmake<Meta>())
    {}

    VertexShaderAsset::VertexShaderAsset(const VertexShaderAsset& asset):
        ShaderAsset(asset)
    {}

    VertexShaderAsset& VertexShaderAsset::operator=(const VertexShaderAsset& asset)
    {
        ShaderAsset::operator=(asset);
        return *this;
    }

    Ref<VertexShaderAsset::Meta> VertexShaderAsset::GetMeta() const
    {
        return DynamicCast<Meta>(mInfo.meta);
    }

    Vector<String> VertexShaderAsset::GetFileExtensions()
    {
        return { "vert" };
    }

    void VertexShaderAsset::LoadData(const String& path)
    {
		String source = o2FileSystem.ReadFile(path);

        mShader = mmake<Shader>();
        mShader->SetFileName(path);
        mShader->Compile(source, Shader::Type::Vertex);

#if IS_EDITOR
        mShader->SetFileEditDate(o2FileSystem.GetFileInfo(path).editDate);
#endif
    }
}

DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::VertexShaderAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::VertexShaderAsset>);
// --- META ---

DECLARE_CLASS(o2::VertexShaderAsset, o2__VertexShaderAsset);

DECLARE_CLASS(o2::VertexShaderAsset::Meta, o2__VertexShaderAsset__Meta);
// --- END META ---
