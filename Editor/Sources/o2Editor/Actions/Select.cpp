#include "o2Editor/stdafx.h"
#include "Select.h"

#include "o2/Scene/Actor.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    SelectAction::SelectAction()
    {}

    SelectAction::SelectAction(const Vector<Ref<SceneEditableObject>>& selectedObjects,
                               const Vector<Ref<SceneEditableObject>>& prevSelectedObjects):
        SelectAction(selectedObjects, prevSelectedObjects,
                     [](const Vector<SceneUID>& ids) { o2EditorSceneScreen.SelectObjectsByIdsWithoutAction(ids); })
    {}

    SelectAction::SelectAction(const Vector<Ref<SceneEditableObject>>& selectedObjects,
                               const Vector<Ref<SceneEditableObject>>& prevSelectedObjects,
                               const Function<void(const Vector<SceneUID>&)>& applySelection):
        applySelection(applySelection)
    {
        selectedObjectsIds = selectedObjects.Convert<SceneUID>([](auto& x) { return x->GetID(); });
        prevSelectedObjectsIds = prevSelectedObjects.Convert<SceneUID>([](auto& x) { return x->GetID(); });
    }

    String SelectAction::GetName() const
    {
        return "Actors selection";
    }

    void SelectAction::Redo()
    {
        if (!applySelection.IsEmpty())
            applySelection(selectedObjectsIds);
    }

    void SelectAction::Undo()
    {
        if (!applySelection.IsEmpty())
            applySelection(prevSelectedObjectsIds);
    }

}
// --- META ---

DECLARE_CLASS(Editor::SelectAction, Editor__SelectAction);
// --- END META ---
