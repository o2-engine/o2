#pragma once

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/VectorFont.h"

namespace o2
{
    // -----------------
    // Vector font asset
    // -----------------
    class VectorFontAsset: public FontAsset
    {
    public:
        class Meta;

    public:
        PROPERTIES(VectorFontAsset);
        GETTER(Ref<Meta>, meta, GetMeta); // Meta information getter

    public:
        // Default constructor
        VectorFontAsset();

        // Copy-constructor
        VectorFontAsset(const VectorFontAsset& asset);

        // Check equals operator
        VectorFontAsset& operator=(const VectorFontAsset& asset);

        // Returns meta information
        Ref<Meta> GetMeta() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor sorting weight
        static int GetEditorSorting() { return 92; }

        ASSET_TYPE(VectorFontAsset, Meta);

    public:
        // ----------------
        // Meta information
        // ----------------
        class Meta: public DefaultAssetMeta<VectorFontAsset>
        {
        public:
            SERIALIZABLE(Meta);
            CLONEABLE_REF(Meta);
        };

    protected:
        // Loads data
        void LoadData(const String& path) override;

        // Saves asset data, using DataValue and serialization
        void SaveData(const String& path) const override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::VectorFontAsset)
{
    BASE_CLASS(o2::FontAsset);
}
END_META;
CLASS_FIELDS_META(o2::VectorFontAsset)
{
    FIELD().PUBLIC().NAME(meta);
}
END_META;
CLASS_METHODS_META(o2::VectorFontAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const VectorFontAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Meta>, GetMeta);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
}
END_META;

CLASS_BASES_META(o2::VectorFontAsset::Meta)
{
    BASE_CLASS(o2::DefaultAssetMeta<VectorFontAsset>);
}
END_META;
CLASS_FIELDS_META(o2::VectorFontAsset::Meta)
{
}
END_META;
CLASS_METHODS_META(o2::VectorFontAsset::Meta)
{
}
END_META;
// --- END META ---
