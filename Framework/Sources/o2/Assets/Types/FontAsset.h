#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Render/Font.h"

namespace o2
{
    // ----------
    // Font asset
    // ----------
    class FontAsset: public Asset
    {
    public:
        PROPERTIES(FontAsset);
        GETTER(Ref<Font>, font, GetFont); // Font getter

    public:
        // Default constructor
        FontAsset();

        // Default constructor
        FontAsset(const Ref<AssetMeta>& meta);

        // Copy-constructor
        FontAsset(const FontAsset& asset);

        // Check equals operator
        FontAsset& operator=(const FontAsset& asset);

        // Returns font pointer
        virtual Ref<Font> GetFont() const;

        // Returns editor sorting weight
        static int GetEditorSorting() { return 93; }

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_font_file_icon.png"; }

        SERIALIZABLE(FontAsset);
        CLONEABLE_REF(FontAsset);

    protected:
        Ref<Font> mFont;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::FontAsset)
{
    BASE_CLASS(o2::Asset);
}
END_META;
CLASS_FIELDS_META(o2::FontAsset)
{
    FIELD().PUBLIC().NAME(font);
    FIELD().PROTECTED().NAME(mFont);
}
END_META;
CLASS_METHODS_META(o2::FontAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<AssetMeta>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const FontAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Font>, GetFont);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
}
END_META;
// --- END META ---
