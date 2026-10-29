#pragma once

#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class Label;
}

namespace Editor
{
    FORWARD_CLASS_REF(ModelView);

    // ------------------------------------------------------------------------------
    // Mesh asset viewer: vertices and triangles, the mesh in a 3D view, the preview
    // ------------------------------------------------------------------------------
    class Mesh3DAssetViewer: public DefaultObjectPropertiesViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(Mesh3DAssetViewer);

    protected:
        Ref<Label>     mInfoLabel; // Vertices and triangles
        Ref<ModelView> mModelView; // The mesh in 3D
        WeakRef<Asset> mAsset;     // Shown asset, for the preview window

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };

    // ---------------------------------------------------------------------------------------------
    // Skinned model asset viewer: vertices, joints, animation clips, the bind pose in a 3D view, the
    // preview playing the clips
    // ---------------------------------------------------------------------------------------------
    class SkinnedModelAssetViewer: public Mesh3DAssetViewer
    {
    public:
        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(SkinnedModelAssetViewer);

    protected:
        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::Mesh3DAssetViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::Mesh3DAssetViewer)
{
    FIELD().PROTECTED().NAME(mInfoLabel);
    FIELD().PROTECTED().NAME(mModelView);
    FIELD().PROTECTED().NAME(mAsset);
}
END_META;
CLASS_METHODS_META(Editor::Mesh3DAssetViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
}
END_META;

CLASS_BASES_META(Editor::SkinnedModelAssetViewer)
{
    BASE_CLASS(Editor::Mesh3DAssetViewer);
}
END_META;
CLASS_FIELDS_META(Editor::SkinnedModelAssetViewer)
{
}
END_META;
CLASS_METHODS_META(Editor::SkinnedModelAssetViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp1);
}
END_META;
// --- END META ---
