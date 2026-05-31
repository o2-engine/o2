#include "o2Editor/stdafx.h"
#include "SceneSearchFilter.h"

namespace Editor
{
    Vector<Ref<SceneEditableObject>> SceneSearchFilter::Search(const Vector<Ref<SceneEditableObject>>& roots,
                                                               const String& searchStr)
    {
        String lowered = searchStr.ToLowerCase();

        Vector<Ref<SceneEditableObject>> result;
        for (auto& object : roots)
            SearchRecursive(object, lowered, result);

        return result;
    }

    void SceneSearchFilter::SearchRecursive(const Ref<SceneEditableObject>& object, const String& loweredSearchStr,
                                            Vector<Ref<SceneEditableObject>>& result)
    {
        if (object->GetName().ToLowerCase().Contains(loweredSearchStr))
            result.Add(object);

        for (auto& child : object->GetEditableChildren())
            SearchRecursive(child, loweredSearchStr, result);
    }
}
