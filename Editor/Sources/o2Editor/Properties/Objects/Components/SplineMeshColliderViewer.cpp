#include "o2Editor/stdafx.h"
#include "SplineMeshColliderViewer.h"

#include "o2/Scene/Actor.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    SplineMeshColliderViewer::SplineMeshColliderViewer()
    {
        mSplineTool = mmake<SplineTool>();
    }

    SplineMeshColliderViewer::~SplineMeshColliderViewer()
    {
        mSplineTool->Reset();
        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }

    SplineMeshColliderViewer& SplineMeshColliderViewer::operator=(const SplineMeshColliderViewer& other)
    {
        TObjectPropertiesViewer<SplineMeshCollider>::operator=(other);
        return *this;
    }

    void SplineMeshColliderViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(SplineMeshCollider), mPropertiesContext, "",
                                                 mOnPropertyChangeCompleted, mOnPropertyChanged);
    }

    void SplineMeshColliderViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevTargetObjects = mTypeTargetObjects;

        TObjectPropertiesViewer<SplineMeshCollider>::OnRefreshed(targetObjets);

        if (mTypeTargetObjects.IsEmpty())
        {
            if (!prevTargetObjects.IsEmpty())
                mSplineTool->Reset();

            return;
        }

        if (prevTargetObjects != mTypeTargetObjects)
        {
            // Capture mTypeTargetObjects by reference so the lambdas always read live
            // viewer state. Capturing a raw target* by value goes dangling the moment
            // the component is removed from the actor while this tool stays on screen.
            Function<Vec2F()> getOrigin = [&]() {
                return mTypeTargetObjects[0].first->GetActor()->transform->GetWorldNonSizedBasis().origin;
            };

            mSplineTool->SetSpline(mTypeTargetObjects[0].first->GetSpline(), getOrigin);
            mSplineTool->onChanged = [&]() { mTypeTargetObjects[0].first->GetActor()->OnChanged(); };
        }
    }

    void SplineMeshColliderViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddTool(mSplineTool);

        mPrevSelectedTool = o2EditorSceneScreen.GetSelectedTool();
        o2EditorSceneScreen.SelectTool<SplineTool>();
    }

    void SplineMeshColliderViewer::OnPropertiesDisabled()
    {
        auto selectedTool = o2EditorSceneScreen.GetSelectedTool();
        if (selectedTool == mSplineTool)
            o2EditorSceneScreen.SelectTool(mPrevSelectedTool.Lock());

        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::SplineMeshCollider>);
// --- META ---

DECLARE_CLASS(Editor::SplineMeshColliderViewer, Editor__SplineMeshColliderViewer);
// --- END META ---
