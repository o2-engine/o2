#pragma once

#include "o2/Assets/Types/ShaderAsset.h"

namespace o2
{
    // -----------------------------------------------------------------
    // Fragment shader asset. Loads .fsh files and compiles them as
    // fragment (pixel) shader stage.
    // -----------------------------------------------------------------
    class FragmentShaderAsset : public ShaderAsset
    {
    public:
        class Meta;

    public:
        PROPERTIES(FragmentShaderAsset);
        GETTER(Ref<Meta>, meta, GetMeta); // Meta information getter

    public:
        // Default constructor
        FragmentShaderAsset();

        // Copy-constructor
        FragmentShaderAsset(const FragmentShaderAsset& asset);

        // Assign operator
        FragmentShaderAsset& operator=(const FragmentShaderAsset& asset);

        // Returns meta information
        Ref<Meta> GetMeta() const;

        // Returns supported file extensions (.fsh)
        static Vector<String> GetFileExtensions();

        // Returns editor icon for fragment shader asset
        static String GetEditorIcon() { return "ui/UI4_big_fragment_shader_icon.png"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 91; }

        // Returns true because fragment shaders can be created from the editor
        static bool IsAvailableToCreateFromEditor() { return true; }

        ASSET_TYPE(FragmentShaderAsset, Meta);

    public:
        // ----------------
        // Meta information
        // ----------------
        class Meta : public DefaultAssetMeta<FragmentShaderAsset>
        {
        public:
            SERIALIZABLE(Meta);
            CLONEABLE_REF(Meta);
        };

    protected:
        // Loads fragment shader source from file and compiles it
        void LoadData(const String& path) override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::FragmentShaderAsset)
{
    BASE_CLASS(o2::ShaderAsset);
}
END_META;
CLASS_FIELDS_META(o2::FragmentShaderAsset)
{
    FIELD().PUBLIC().NAME(meta);
}
END_META;
CLASS_METHODS_META(o2::FragmentShaderAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const FragmentShaderAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Meta>, GetMeta);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
}
END_META;

CLASS_BASES_META(o2::FragmentShaderAsset::Meta)
{
    BASE_CLASS(o2::DefaultAssetMeta<FragmentShaderAsset>);
}
END_META;
CLASS_FIELDS_META(o2::FragmentShaderAsset::Meta)
{
}
END_META;
CLASS_METHODS_META(o2::FragmentShaderAsset::Meta)
{
}
END_META;
// --- END META ---
