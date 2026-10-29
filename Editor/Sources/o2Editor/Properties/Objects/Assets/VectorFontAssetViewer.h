#pragma once

#include "o2Editor/Properties/Objects/Assets/FontAssetViewer.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------
    // Vector font asset viewer: the lines of text and the atlas of the font
    // ---------------------------------------------------------------------
    class VectorFontAssetViewer : public FontAssetViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(VectorFontAssetViewer);
    };
}
// --- META ---

CLASS_BASES_META(Editor::VectorFontAssetViewer)
{
    BASE_CLASS(Editor::FontAssetViewer);
}
END_META;
CLASS_FIELDS_META(Editor::VectorFontAssetViewer)
{
}
END_META;
CLASS_METHODS_META(Editor::VectorFontAssetViewer)
{

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
}
END_META;
// --- END META ---
