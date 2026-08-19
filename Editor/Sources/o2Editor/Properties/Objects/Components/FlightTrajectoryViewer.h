#pragma once

#include "o2/Scene/Components/FlightTrajectoryComponent.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Tools/SplineTool.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------
    // Flight trajectory component viewer: properties plus spline editing with the scene
    // SplineTool (origin is the flight start point)
    // ---------------------------------------------------------------------------------
    class FlightTrajectoryViewer: public TObjectPropertiesViewer<FlightTrajectoryComponent>
    {
    public:
        // Default constructor
        FlightTrajectoryViewer();

        // Destructor, removes spline tool
        ~FlightTrajectoryViewer() override;

        IOBJECT(FlightTrajectoryViewer);

    protected:
        Ref<SplineTool>    mSplineTool;       // Spline tool
        WeakRef<IEditTool> mPrevSelectedTool; // Previous selected tool, for restore

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

CLASS_BASES_META(Editor::FlightTrajectoryViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<FlightTrajectoryComponent>);
}
END_META;
CLASS_FIELDS_META(Editor::FlightTrajectoryViewer)
{
    FIELD().PROTECTED().NAME(mSplineTool);
    FIELD().PROTECTED().NAME(mPrevSelectedTool);
}
END_META;
CLASS_METHODS_META(Editor::FlightTrajectoryViewer)
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
