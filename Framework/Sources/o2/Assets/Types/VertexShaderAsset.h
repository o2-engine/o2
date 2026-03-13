#pragma once

#include "o2/Assets/Types/ShaderAsset.h"

namespace o2
{
    // -----------------------------------------------------------------
    // Vertex shader asset. Loads .vsh files and compiles them as
    // vertex shader stage.
    // -----------------------------------------------------------------
    class VertexShaderAsset : public ShaderAsset
    {
    public:
        class Meta;

    public:
        PROPERTIES(VertexShaderAsset);
        GETTER(Ref<Meta>, meta, GetMeta); // Meta information getter

    public:
        // Default constructor
        VertexShaderAsset();

        // Copy-constructor
        VertexShaderAsset(const VertexShaderAsset& asset);

        // Assign operator
        VertexShaderAsset& operator=(const VertexShaderAsset& asset);

        // Returns meta information
        Ref<Meta> GetMeta() const;

        // Returns supported file extensions (.vsh)
        static Vector<String> GetFileExtensions();

        // Returns editor icon for vertex shader asset
        static String GetEditorIcon() { return "ui/UI4_big_vertex_shader_icon.png"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 91; }

        // Returns true because vertex shaders can be created from the editor
        static bool IsAvailableToCreateFromEditor() { return true; }

        ASSET_TYPE(VertexShaderAsset, Meta);

    public:
        // ----------------
        // Meta information
        // ----------------
        class Meta : public DefaultAssetMeta<VertexShaderAsset>
        {
        public:
            SERIALIZABLE(Meta);
            CLONEABLE_REF(Meta);
        };

    protected:
        // Loads vertex shader source from file and compiles it
        void LoadData(const String& path) override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::VertexShaderAsset)
{
    BASE_CLASS(o2::ShaderAsset);
}
END_META;
CLASS_FIELDS_META(o2::VertexShaderAsset)
{
    FIELD().PUBLIC().NAME(meta);
}
END_META;
CLASS_METHODS_META(o2::VertexShaderAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const VertexShaderAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Meta>, GetMeta);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
}
END_META;

CLASS_BASES_META(o2::VertexShaderAsset::Meta)
{
    BASE_CLASS(o2::DefaultAssetMeta<VertexShaderAsset>);
}
END_META;
CLASS_FIELDS_META(o2::VertexShaderAsset::Meta)
{
}
END_META;
CLASS_METHODS_META(o2::VertexShaderAsset::Meta)
{
}
END_META;
// --- END META ---
