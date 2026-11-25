#pragma once

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/Text/SdfGlyphGenerator.h"
#include "o2/Render/Text/VectorFont.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"

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

        // Returns true when the font is drawn by distance fields: by the meta, unless the override says otherwise
        bool IsDistanceField() const;

        // Overrides the meta of every vector font: 1 - distance fields, 0 - the bitmap atlas, -1 - by the meta. Takes
        // effect for fonts loaded after it; the environment variable O2_FONT_DISTANCE_FIELD sets it at start
        static void SetDistanceFieldOverride(int mode);

        // Returns the distance field override: 1, 0 or -1 for none
        static int GetDistanceFieldOverride();

        ASSET_TYPE(VectorFontAsset, Meta);

    public:
        // ----------------
        // Meta information
        // ----------------
        class Meta: public DefaultAssetMeta<VectorFontAsset>
        {
        public:
            bool    distanceField = false;      // Draws the texts by distance fields: any size, scale and style from one atlas, effects on GPU @SERIALIZABLE @EDITOR_PROPERTY
            int     fieldGlyphSize = 48;        // Distance field pixels per em @SERIALIZABLE @EDITOR_PROPERTY
            float   fieldDistanceRange = 16.0f; // Distance field pixels the distances are encoded within: the effects reach half of it, scaled to the text @SERIALIZABLE @EDITOR_PROPERTY
            bool    fieldMultiChannel = true;   // Multi-channel field (MTSDF): sharp corners @SERIALIZABLE @EDITOR_PROPERTY
            WString fieldCharacters;            // Characters baked into the atlas by the assets build; empty - ASCII, Cyrillic and typographic marks. Others are generated when shown @SERIALIZABLE @EDITOR_PROPERTY

        public:
            // Returns the distance field settings
            SdfFontSettings GetFieldSettings() const;

            // Returns the characters to bake
            WString GetFieldCharacters() const;

            // Returns true when other meta is equal to this, the distance field settings included
            bool IsEqual(AssetMeta* other) const override;

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
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDistanceField);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, SetDistanceFieldOverride, int);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetDistanceFieldOverride);
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
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(distanceField);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(48).NAME(fieldGlyphSize);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(16.0f).NAME(fieldDistanceRange);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(fieldMultiChannel);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(fieldCharacters);
}
END_META;
CLASS_METHODS_META(o2::VectorFontAsset::Meta)
{

    FUNCTION().PUBLIC().SIGNATURE(SdfFontSettings, GetFieldSettings);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetFieldCharacters);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, AssetMeta*);
}
END_META;
// --- END META ---
