#pragma once

#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(FontSampleView);

    // ----------------------------------------------------------------------------------------
    // Font asset viewer: lines of text of a few sizes, the preview window with all of them and
    // with the atlas of the glyphs
    // ----------------------------------------------------------------------------------------
    class FontAssetViewer: public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(FontAssetViewer);

    protected:
        Ref<FontSampleView> mSample; // Lines of text
        WeakRef<Asset>      mAsset;  // Shown asset, for the preview window

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::FontAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::FontAssetViewer)
{
    FIELD().PROTECTED().NAME(mSample);
    FIELD().PROTECTED().NAME(mAsset);
}
END_META;
CLASS_METHODS_META(Editor::FontAssetViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
}
END_META;
// --- END META ---
