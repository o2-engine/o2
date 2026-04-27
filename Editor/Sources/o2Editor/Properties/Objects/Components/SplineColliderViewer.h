#pragma once

#include "o2/Scene/Physics/SplineCollider.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Tools/SplineTool.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------
    // Spline collider component editor viewer.
    // Drives the scene SplineTool against the
    // collider's spline so its curve can be
    // edited in the scene window.
    // ----------------------------------------
    class SplineColliderViewer: public TObjectPropertiesViewer<SplineCollider>
    {
    public:
        // Default constructor
        SplineColliderViewer();

        // Destructor
        ~SplineColliderViewer();

        // Copy operator
        SplineColliderViewer& operator=(const SplineColliderViewer& other);

        IOBJECT(SplineColliderViewer);

    protected:
        Ref<SplineTool>    mSplineTool;       // Spline tool
        WeakRef<IEditTool> mPrevSelectedTool; // Previous selected tool, for restore

    protected:
        // Called when the viewer is refreshed, builds properties via reflection
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

CLASS_BASES_META(Editor::SplineColliderViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<SplineCollider>);
}
END_META;
CLASS_FIELDS_META(Editor::SplineColliderViewer)
{
    FIELD().PROTECTED().NAME(mSplineTool);
    FIELD().PROTECTED().NAME(mPrevSelectedTool);
}
END_META;
CLASS_METHODS_META(Editor::SplineColliderViewer)
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
