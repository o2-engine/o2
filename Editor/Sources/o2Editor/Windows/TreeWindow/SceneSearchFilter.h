#pragma once

#include "o2/Utils/Editor/SceneEditableObject.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------
    // Recursive search of scene editable objects by name. Match is
    // substring and case-insensitive. No UI or scene dependencies
    // -----------------------------------------------------------------
    class SceneSearchFilter
    {
    public:
        // Returns all objects in the roots hierarchy whose name contains searchStr
        // (case-insensitive). Pre-order traversal: an object precedes its children.
        // Empty searchStr matches every object
        static Vector<Ref<SceneEditableObject>> Search(const Vector<Ref<SceneEditableObject>>& roots,
                                                       const String& searchStr);

    private:
        // Appends object and its matching descendants to result
        static void SearchRecursive(const Ref<SceneEditableObject>& object, const String& loweredSearchStr,
                                    Vector<Ref<SceneEditableObject>>& result);
    };
}
