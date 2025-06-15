#pragma once
#include "o2Editor/Properties/Objects/DefaultObjectPropertiesViewer.h"
#include "../../Tools/SplineTool.h"

using namespace o2;

namespace o2
{
    class Button;
}

namespace Editor
{
    // ------------------------------
    // Particles spline effect viewer
    // ------------------------------
    class ParticlesSplineEffectViewer : public DefaultObjectPropertiesViewer
    {
    public:
        // Default constructor
        ParticlesSplineEffectViewer();

        // Default constructor, removes spline tool
        ~ParticlesSplineEffectViewer() override;

        // Returns viewing objects type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(ParticlesSplineEffectViewer);

    private:
        Ref<SplineTool>    mSplineTool;       // Spline tool
        WeakRef<IEditTool> mPrevSelectedTool; // Previous selected tool, for restore

        Ref<ParticlesSplineEffect> mSplineEffect; // Current effect

    private:
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

CLASS_BASES_META(Editor::ParticlesSplineEffectViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::ParticlesSplineEffectViewer)
{
    FIELD().PRIVATE().NAME(mSplineTool);
    FIELD().PRIVATE().NAME(mPrevSelectedTool);
    FIELD().PRIVATE().NAME(mSplineEffect);
}
END_META;
CLASS_METHODS_META(Editor::ParticlesSplineEffectViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PRIVATE().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PRIVATE().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PRIVATE().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PRIVATE().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
