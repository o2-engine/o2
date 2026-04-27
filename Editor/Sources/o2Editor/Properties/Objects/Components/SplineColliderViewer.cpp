#include "o2Editor/stdafx.h"
#include "SplineColliderViewer.h"

#include "o2/Scene/Actor.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    SplineColliderViewer::SplineColliderViewer()
    {
        mSplineTool = mmake<SplineTool>();
    }

    SplineColliderViewer::~SplineColliderViewer()
    {
        mSplineTool->Reset();
        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }

    SplineColliderViewer& SplineColliderViewer::operator=(const SplineColliderViewer& other)
    {
        TObjectPropertiesViewer<SplineCollider>::operator=(other);
        return *this;
    }

    void SplineColliderViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(SplineCollider), mPropertiesContext, "",
                                                 mOnPropertyChangeCompleted, mOnPropertyChanged);
    }

    void SplineColliderViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevTargetObjects = mTypeTargetObjects;

        TObjectPropertiesViewer<SplineCollider>::OnRefreshed(targetObjets);

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

    void SplineColliderViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddTool(mSplineTool);

        mPrevSelectedTool = o2EditorSceneScreen.GetSelectedTool();
        o2EditorSceneScreen.SelectTool<SplineTool>();
    }

    void SplineColliderViewer::OnPropertiesDisabled()
    {
        auto selectedTool = o2EditorSceneScreen.GetSelectedTool();
        if (selectedTool == mSplineTool)
            o2EditorSceneScreen.SelectTool(mPrevSelectedTool.Lock());

        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::SplineCollider>);
// --- META ---

DECLARE_CLASS(Editor::SplineColliderViewer, Editor__SplineColliderViewer);
// --- END META ---
