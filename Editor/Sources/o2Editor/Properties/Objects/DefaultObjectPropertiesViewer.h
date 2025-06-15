#pragma once

#include "o2Editor/Properties/IObjectPropertiesViewer.h"

namespace Editor
{
    // ------------------------------------------------------------
    // Default object properties viewer. Builds field by reflection
    // ------------------------------------------------------------
    class DefaultObjectPropertiesViewer: public IObjectPropertiesViewer
    {
    public:
        IOBJECT(DefaultObjectPropertiesViewer);

    protected:
        const Type* mBuiltObjectType = &TypeOf(IObject); // Type of built object properties
        bool        mBuiltWithHiddenProperties = false;  // Is properties was built with hidden properties section

    protected:
        // Checks if properties need to be rebuilt, rebuilds if necessary
        bool CheckBuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::DefaultObjectPropertiesViewer)
{
    BASE_CLASS(Editor::IObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::DefaultObjectPropertiesViewer)
{
    FIELD().PROTECTED().DEFAULT_VALUE(&TypeOf(IObject)).NAME(mBuiltObjectType);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBuiltWithHiddenProperties);
}
END_META;
CLASS_METHODS_META(Editor::DefaultObjectPropertiesViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PROTECTED().SIGNATURE(bool, CheckBuildProperties, _tmp1);
}
END_META;
// --- END META ---
