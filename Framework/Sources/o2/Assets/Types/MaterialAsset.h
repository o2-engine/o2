#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/VertexShaderAsset.h"
#include "o2/Assets/Types/FragmentShaderAsset.h"
#include "o2/Render/Material.h"
#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    // -----------------------------------------------------------------
    // Material asset. Wraps a material with vertex/fragment shaders,
    // optional texture, and configurable shader parameters.
    // Serializes shader asset references and parameter values.
    // -----------------------------------------------------------------
    class MaterialAsset : public AssetWithDefaultMeta<MaterialAsset>
    {
    public:
        PROPERTIES(MaterialAsset);
        GETTER(Ref<Material>, material, GetMaterial);                                                  // Underlying material render primitive
        PROPERTY(BlendMode, blendMode, SetBlendMode, GetBlendMode);                                    // Blend mode (applied to material) @EDITOR_PROPERTY
		PROPERTY(AssetRef<VertexShaderAsset>, vertexShader, SetVertexShader, GetVertexShader);         // Vertex shader asset reference @EDITOR_PROPERTY
		PROPERTY(AssetRef<FragmentShaderAsset>, fragmentShader, SetFragmentShader, GetFragmentShader); // Fragment shader asset reference @EDITOR_PROPERTY

    public:
        // Default constructor
        MaterialAsset();

        // Copy-constructor
        MaterialAsset(const MaterialAsset& asset);

        // Assign operator
        MaterialAsset& operator=(const MaterialAsset& asset);

        // Returns the underlying material render primitive
        Ref<Material> GetMaterial() const;

        // Sets blend mode (applied to built material)
        void SetBlendMode(BlendMode blendMode);
        BlendMode GetBlendMode() const;

        // Sets the vertex shader asset reference and rebuilds the material
        void SetVertexShader(const AssetRef<VertexShaderAsset>& shader);

        // Returns the vertex shader asset reference
        const AssetRef<VertexShaderAsset>& GetVertexShader() const;

        // Sets the fragment shader asset reference and rebuilds the material
        void SetFragmentShader(const AssetRef<FragmentShaderAsset>& shader);

        // Returns the fragment shader asset reference
        const AssetRef<FragmentShaderAsset>& GetFragmentShader() const;

        // Returns supported file extensions (.mat)
        static Vector<String> GetFileExtensions();

        // Returns editor icon for material asset
        static String GetEditorIcon() { return "ui/UI4_big_material_icon.png"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 89; }

        // Returns true because materials can be created from the editor
        static bool IsAvailableToCreateFromEditor() { return true; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return true; }

        ASSET_TYPE(MaterialAsset, Meta);

    protected:
        Ref<Material> mMaterial; // Compiled material render primitive
        BlendMode     mBlendMode = BlendMode::Normal; // Blend mode applied to material @SERIALIZABLE

        AssetRef<VertexShaderAsset>   mVertexShaderAsset;   // Vertex shader asset reference @SERIALIZABLE
        AssetRef<FragmentShaderAsset> mFragmentShaderAsset; // Fragment shader asset reference @SERIALIZABLE
        Vector<Ref<IShaderParam>>     mParams;              // Shader parameter values @SERIALIZABLE @EDITOR_PROPERTY @EXPANDED_BY_DEFAULT

    protected:
        // Loads material data from serialized file
        void LoadData(const String& path) override;

        // Saves material data to file
        void SaveData(const String& path) const override;

        // Builds the material from current shader assets and parameters
        void BuildMaterial();

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::MaterialAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<MaterialAsset>);
}
END_META;
CLASS_FIELDS_META(o2::MaterialAsset)
{
    FIELD().PUBLIC().NAME(material);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().NAME(blendMode);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().NAME(vertexShader);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().NAME(fragmentShader);
    FIELD().PROTECTED().NAME(mMaterial);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(BlendMode::Normal).NAME(mBlendMode);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mVertexShaderAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mFragmentShaderAsset);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().EXPANDED_BY_DEFAULT_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mParams);
}
END_META;
CLASS_METHODS_META(o2::MaterialAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const MaterialAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Material>, GetMaterial);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBlendMode, BlendMode);
    FUNCTION().PUBLIC().SIGNATURE(BlendMode, GetBlendMode);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVertexShader, const AssetRef<VertexShaderAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<VertexShaderAsset>&, GetVertexShader);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFragmentShader, const AssetRef<FragmentShaderAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<FragmentShaderAsset>&, GetFragmentShader);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildMaterial);
}
END_META;
// --- END META ---
