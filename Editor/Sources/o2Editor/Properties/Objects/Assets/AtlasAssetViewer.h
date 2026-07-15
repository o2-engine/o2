#pragma once

#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class Label;
}

namespace Editor
{
    FORWARD_CLASS_REF(TexturePreview);

    // ------------------------------------------------------------------
    // Atlas asset viewer: page texture preview on the checked background
    // ------------------------------------------------------------------
    class AtlasAssetViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(AtlasAssetViewer);

    private:
        Ref<Label>          mPagesLabel;     // Pages and images count label
        Ref<TexturePreview> mTexturePreview; // First page texture preview widget

    private:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::AtlasAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::AtlasAssetViewer)
{
    FIELD().PRIVATE().NAME(mPagesLabel);
    FIELD().PRIVATE().NAME(mTexturePreview);
}
END_META;
CLASS_METHODS_META(Editor::AtlasAssetViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PRIVATE().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PRIVATE().SIGNATURE(void, OnRefreshed, _tmp2);
}
END_META;
// --- END META ---
