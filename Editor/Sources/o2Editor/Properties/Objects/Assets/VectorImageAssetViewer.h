#pragma once

#include "o2/Scene/UI/Widgets/Label.h"
#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(BorderIProperty);
    FORWARD_CLASS_REF(ImageSlicesEditorWidget);

    // --------------------------------------------------------------------------------------
    // Vector image asset viewer: the image drawn over a checker with its slices, size and mesh
    // --------------------------------------------------------------------------------------
    class VectorImageAssetViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(VectorImageAssetViewer);

    private:
        Ref<Label> mSizeLabel;  // Size label
        Ref<Label> mMeshLabel;  // Shapes, triangles and how the image is drawn
        Ref<Label> mErrorLabel; // Parsing error and warnings

        Ref<ImageSlicesEditorWidget> mSlicesEditor; // Preview with the slices editor

        WeakRef<Asset> mAsset; // Shown asset, for the preview window

    private:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::VectorImageAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::VectorImageAssetViewer)
{
    FIELD().PRIVATE().NAME(mSizeLabel);
    FIELD().PRIVATE().NAME(mMeshLabel);
    FIELD().PRIVATE().NAME(mErrorLabel);
    FIELD().PRIVATE().NAME(mSlicesEditor);
    FIELD().PRIVATE().NAME(mAsset);
}
END_META;
CLASS_METHODS_META(Editor::VectorImageAssetViewer)
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
