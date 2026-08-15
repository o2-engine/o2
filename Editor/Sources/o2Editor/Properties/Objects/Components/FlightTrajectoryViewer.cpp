#include "o2Editor/stdafx.h"
#include "FlightTrajectoryViewer.h"

#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    FlightTrajectoryViewer::FlightTrajectoryViewer()
    {
        mSplineTool = mmake<SplineTool>();
    }

    FlightTrajectoryViewer::~FlightTrajectoryViewer()
    {
        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }

    void FlightTrajectoryViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(FlightTrajectoryComponent), mPropertiesContext, "",
                                                 mOnPropertyChangeCompleted, mOnPropertyChanged);
    }

    void FlightTrajectoryViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevTargetObjects = mTypeTargetObjects;

        TObjectPropertiesViewer<FlightTrajectoryComponent>::OnRefreshed(targetObjets);

        if (!mTypeTargetObjects.IsEmpty() && prevTargetObjects != mTypeTargetObjects)
        {
            auto trajectory = mTypeTargetObjects[0].first;
            if (!trajectory->spline)
                trajectory->spline = mmake<Spline>();

            // the spline is edited in its own space anchored at the flight start point
            Function<Vec2F()> getOrigin = [=]() { return trajectory->startPoint; };

            mSplineTool->SetSpline(trajectory->spline, getOrigin);
            mSplineTool->onChanged = [=]() { trajectory->GetActor()->OnChanged(); };
        }
    }

    void FlightTrajectoryViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddTool(mSplineTool);

        mPrevSelectedTool = o2EditorSceneScreen.GetSelectedTool();
        o2EditorSceneScreen.SelectTool<SplineTool>();
    }

    void FlightTrajectoryViewer::OnPropertiesDisabled()
    {
        auto selectedTool = o2EditorSceneScreen.GetSelectedTool();
        if (selectedTool == mSplineTool)
            o2EditorSceneScreen.SelectTool(mPrevSelectedTool.Lock());

        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<FlightTrajectoryComponent>);
// --- META ---

DECLARE_CLASS(Editor::FlightTrajectoryViewer, Editor__FlightTrajectoryViewer);
// --- END META ---
