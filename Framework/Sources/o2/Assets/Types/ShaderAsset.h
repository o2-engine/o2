#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Render/Shader.h"

namespace o2
{
    // -----------------------------------------------------------------
    // Base shader asset. Wraps a compiled shader render primitive.
    // Derived types (VertexShaderAsset, FragmentShaderAsset) specify
    // the shader stage and file extension.
    // -----------------------------------------------------------------
    class ShaderAsset : public Asset
    {
    public:
        PROPERTIES(ShaderAsset);
        GETTER(Ref<Shader>, shader, GetShader); // Underlying shader render primitive

    public:
        // Default constructor
        ShaderAsset();

        // Constructor with meta
        ShaderAsset(const Ref<AssetMeta>& meta);

        // Copy-constructor
        ShaderAsset(const ShaderAsset& asset);

        // Assign operator
        ShaderAsset& operator=(const ShaderAsset& asset);

        // Returns the underlying compiled shader render primitive
        virtual Ref<Shader> GetShader() const;

        // Returns editor sorting weight
        static int GetEditorSorting() { return 90; }

        SERIALIZABLE(ShaderAsset);
        CLONEABLE_REF(ShaderAsset);

    protected:
        Ref<Shader> mShader; // Compiled shader render primitive

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::ShaderAsset)
{
    BASE_CLASS(o2::Asset);
}
END_META;
CLASS_FIELDS_META(o2::ShaderAsset)
{
    FIELD().PUBLIC().NAME(shader);
    FIELD().PROTECTED().NAME(mShader);
}
END_META;
CLASS_METHODS_META(o2::ShaderAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<AssetMeta>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const ShaderAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Shader>, GetShader);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
}
END_META;
// --- END META ---
