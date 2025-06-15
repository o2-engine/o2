#pragma once

#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Objects/Components/SkinnedMesh/SkinningMeshEditorLayer.h"
#include "o2Editor/Tools/MeshWeightsTool.h"

using namespace o2;

namespace o2
{
    class Spoiler;
    class Button;
}

namespace Editor
{
    FORWARD_CLASS_REF(SceneDragHandle);

    // ------------------------------
    // Skinning Mesh component viewer
    // ------------------------------
    class SkinningMeshBoneComponentViewer: public TObjectPropertiesViewer<SkinningMeshBoneComponent>
    {
    public:
        // Default constructor
        SkinningMeshBoneComponentViewer();

        // Destructor
        ~SkinningMeshBoneComponentViewer();

        // Copy operator
        SkinningMeshBoneComponentViewer& operator=(const SkinningMeshBoneComponentViewer& other);

        IOBJECT(SkinningMeshBoneComponentViewer);

    protected:
        Ref<MeshWeightsTool>         mWeightsTool;       // Mesh weights tool
        Ref<SkinningMeshEditorLayer> mFrameTetxureLayer; // Frame texture drawing layer

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SkinningMeshBoneComponentViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<SkinningMeshBoneComponent>);
}
END_META;
CLASS_FIELDS_META(Editor::SkinningMeshBoneComponentViewer)
{
    FIELD().PROTECTED().NAME(mWeightsTool);
    FIELD().PROTECTED().NAME(mFrameTetxureLayer);
}
END_META;
CLASS_METHODS_META(Editor::SkinningMeshBoneComponentViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
