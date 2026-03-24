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
    // Material asset. Inherits Material and adds asset serialization for
    // vertex/fragment shader references. The asset itself IS the material.
    // -----------------------------------------------------------------
    class MaterialAsset : public AssetWithDefaultMeta<MaterialAsset>, public Material
    {
    public:
        REF_COUNTERABLE_IMPL(Asset, Material);

        PROPERTIES(MaterialAsset);
		PROPERTY(AssetRef<VertexShaderAsset>, vertexShader, SetVertexShader, GetVertexShader);         // Vertex shader asset reference @EDITOR_PROPERTY
		PROPERTY(AssetRef<FragmentShaderAsset>, fragmentShader, SetFragmentShader, GetFragmentShader); // Fragment shader asset reference @EDITOR_PROPERTY

    public:
        // Default constructor
        MaterialAsset();

        // Copy-constructor
        MaterialAsset(const MaterialAsset& asset);

        // Assign operator
        MaterialAsset& operator=(const MaterialAsset& asset);

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

        // Resolves RefCounterable ambiguity for CloneRef (uses Asset base)
        static Ref<RefCounterable> CastToRefCounterable(const Ref<MaterialAsset>& ref);

        ASSET_TYPE(MaterialAsset, Meta);

    protected:
        AssetRef<VertexShaderAsset>   mVertexShaderAsset;   // Vertex shader asset reference @SERIALIZABLE
        AssetRef<FragmentShaderAsset> mFragmentShaderAsset; // Fragment shader asset reference @SERIALIZABLE

    protected:
        // Builds the material from current shader assets and parameters
        void RebuildMaterial();

        // Disambiguate On* callbacks from diamond ISerializable inheritance
        void OnSerialize(DataValue& node) const override {}
        void OnDeserialized(const DataValue& node) override;
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override {}
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

    public:
#if IS_SCRIPTING_SUPPORTED
        // Disambiguate GetScriptValue from diamond IObject inheritance
        ScriptValue GetScriptValue() const override { return Asset::GetScriptValue(); }
#endif

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::MaterialAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<MaterialAsset>);
    BASE_CLASS(o2::Material);
}
END_META;
CLASS_FIELDS_META(o2::MaterialAsset)
{
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().NAME(vertexShader);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().NAME(fragmentShader);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mVertexShaderAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mFragmentShaderAsset);
}
END_META;
CLASS_METHODS_META(o2::MaterialAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const MaterialAsset&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVertexShader, const AssetRef<VertexShaderAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<VertexShaderAsset>&, GetVertexShader);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFragmentShader, const AssetRef<FragmentShaderAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<FragmentShaderAsset>&, GetFragmentShader);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<MaterialAsset>&);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildMaterial);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
#if  IS_SCRIPTING_SUPPORTED
    FUNCTION().PUBLIC().SIGNATURE(ScriptValue, GetScriptValue);
#endif
}
END_META;
// --- END META ---
