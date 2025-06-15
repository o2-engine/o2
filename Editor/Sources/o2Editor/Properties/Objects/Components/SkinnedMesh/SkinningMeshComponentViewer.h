#pragma once

#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2/Scene/Components/SkinningMeshComponent.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Objects/Components/SkinnedMesh/SkinningMeshEditorLayer.h"
#include "o2Editor/Tools/CustomFrameTool.h"
#include "o2Editor/Tools/MeshTopologyTool.h"
#include "o2Editor/Tools/SplineTool.h"
#include "o2Editor/UI/SplineEditor/SplineEditor.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

using namespace o2;

namespace o2
{
    class Spoiler;
    class Button;
}

namespace Editor
{
    FORWARD_CLASS_REF(SceneDragHandle);

    struct SkeletonTool;
    FORWARD_REF(SkeletonTool);

    // ------------------------------
    // Skinning Mesh component viewer
    // ------------------------------
    class SkinningMeshComponentViewer: public TObjectPropertiesViewer<SkinningMeshComponent>
    {
    public:
        // Default constructor
        SkinningMeshComponentViewer();

        // Destructor
        ~SkinningMeshComponentViewer();

        // Copy operator
        SkinningMeshComponentViewer& operator=(const SkinningMeshComponentViewer& other);

        IOBJECT(SkinningMeshComponentViewer);

    protected:
        Ref<SplineTool>              mSplineTool;        // Spline tool
        Ref<CustomFrameTool>         mFrameTool;         // Mapping frame tool
        Ref<MeshTopologyTool>        mTopologyTool;      // Mesh topology tool
        Ref<SkeletonTool>            mSkeletonTool;      // Shared skeleton tool
        Ref<SkinningMeshEditorLayer> mFrameTetxureLayer; // Frame texture drawing layer

        Ref<Button> mFitAndCenterButton; // Fit and centerize button
        Ref<Button> mEditSkeletonButton; // Enable/disable skeleton editing button
        Ref<Button> mReskinButton;       // Resets mesh skinning and updates bones transforms

        bool mEditingSkeleton = false; // It editing skeleton enabled

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        // Called when button pressed
        void FitAndCenterize();

        // Called when edit skeleton pressed
        void OnEditSkeletonPressed();

        // Called when reskin pressed
        void OnReskinPressed();

        // Updates the caption on the skeleton edit button depending on whether it is enabled or not
        void UpdateEditSkeletonButtonCaption();
    };
}
// --- META ---

CLASS_BASES_META(Editor::SkinningMeshComponentViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<SkinningMeshComponent>);
}
END_META;
CLASS_FIELDS_META(Editor::SkinningMeshComponentViewer)
{
    FIELD().PROTECTED().NAME(mSplineTool);
    FIELD().PROTECTED().NAME(mFrameTool);
    FIELD().PROTECTED().NAME(mTopologyTool);
    FIELD().PROTECTED().NAME(mSkeletonTool);
    FIELD().PROTECTED().NAME(mFrameTetxureLayer);
    FIELD().PROTECTED().NAME(mFitAndCenterButton);
    FIELD().PROTECTED().NAME(mEditSkeletonButton);
    FIELD().PROTECTED().NAME(mReskinButton);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mEditingSkeleton);
}
END_META;
CLASS_METHODS_META(Editor::SkinningMeshComponentViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, FitAndCenterize);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditSkeletonPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnReskinPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateEditSkeletonButtonCaption);
}
END_META;
// --- END META ---
