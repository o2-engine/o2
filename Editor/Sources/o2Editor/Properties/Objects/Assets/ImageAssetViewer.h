#pragma once

#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    FORWARD_CLASS_REF(Button);
    FORWARD_CLASS_REF(WidgetDragHandle);
}

namespace Editor
{
    FORWARD_CLASS_REF(AssetProperty);
    FORWARD_CLASS_REF(BorderIProperty);
    FORWARD_CLASS_REF(ImageSlicesEditorWidget);

    // ------------------
    // Image asset viewer
    // ------------------
    class ImageAssetViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(ImageAssetViewer);

    private:
        Ref<ImageSlicesEditorWidget> mSlicesEditor; // Slices editor widget

        Vector<Ref<IAbstractValueProxy>> mAtlasProxies;  // Atlas proxies for property, converts UID to AssetRef<AtlasAsset> and back
        Ref<AssetProperty>               mAtlasProperty; // Atlas property

    private:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::ImageAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::ImageAssetViewer)
{
    FIELD().PRIVATE().NAME(mSlicesEditor);
    FIELD().PRIVATE().NAME(mAtlasProxies);
    FIELD().PRIVATE().NAME(mAtlasProperty);
}
END_META;
CLASS_METHODS_META(Editor::ImageAssetViewer)
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
