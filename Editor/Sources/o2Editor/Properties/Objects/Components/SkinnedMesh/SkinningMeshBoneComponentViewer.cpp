#include "o2Editor/stdafx.h"
#include "SkinningMeshBoneComponentViewer.h"

#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    SkinningMeshBoneComponentViewer::SkinningMeshBoneComponentViewer()
    {
        mWeightsTool = mmake<MeshWeightsTool>();
        mFrameTetxureLayer = mmake<SkinningMeshEditorLayer>();
    }

    SkinningMeshBoneComponentViewer::~SkinningMeshBoneComponentViewer()
    {
        o2EditorSceneScreen.RemoveTool(mWeightsTool);
    }

    SkinningMeshBoneComponentViewer& SkinningMeshBoneComponentViewer::operator=(const SkinningMeshBoneComponentViewer& other)
    {
        TObjectPropertiesViewer<SkinningMeshBoneComponent>::operator=(other);
        return *this;
    }

    void SkinningMeshBoneComponentViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(SkinningMeshBoneComponent), mPropertiesContext, "",
                                                 mOnChildFieldChangeCompleted, onChanged);
    }

    void SkinningMeshBoneComponentViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevTargetObjects = mTypeTargetObjects;

        TObjectPropertiesViewer<SkinningMeshBoneComponent>::OnRefreshed(targetObjets);

        if (!mTypeTargetObjects.IsEmpty() && prevTargetObjects != mTypeTargetObjects)
        {
            mWeightsTool->boneComponent = Ref(mTypeTargetObjects[0].first);

            mFrameTetxureLayer->mesh = mTypeTargetObjects[0].first->FindSkinningMesh();
        }
    }

    void SkinningMeshBoneComponentViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddTool(mWeightsTool);
        o2EditorSceneScreen.SelectTool<MeshWeightsTool>();

        o2EditorSceneScreen.AddEditorLayer(mFrameTetxureLayer);
    }

    void SkinningMeshBoneComponentViewer::OnPropertiesDisabled()
    {
        o2EditorSceneScreen.RemoveTool(mWeightsTool);
        o2EditorSceneScreen.RemoveEditorLayer(mFrameTetxureLayer);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<SkinningMeshBoneComponent>);
// --- META ---

DECLARE_CLASS(Editor::SkinningMeshBoneComponentViewer, Editor__SkinningMeshBoneComponentViewer);
// --- END META ---
