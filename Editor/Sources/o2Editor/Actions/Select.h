#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    // -----------------------------
    // Scene object selection action
    // -----------------------------
    class SelectAction: public IAction
    {
    public:
        Vector<SceneUID> selectedObjectsIds;     // Selected objects ids
        Vector<SceneUID> prevSelectedObjectsIds; // Selected objects ids before

        Function<void(const Vector<SceneUID>&)> applySelection; // Runtime-only selection setter; null on deserialization

    public:
        // Default constructor
        SelectAction();

        // Constructor with new and previous selected objects, default scene-bound callback
        SelectAction(const Vector<Ref<SceneEditableObject>>& selectedObjects, const Vector<Ref<SceneEditableObject>>& prevSelectedObjects);

        // Constructor with explicit selection callback (used by tests)
        SelectAction(const Vector<Ref<SceneEditableObject>>& selectedObjects, const Vector<Ref<SceneEditableObject>>& prevSelectedObjects,
                     const Function<void(const Vector<SceneUID>&)>& applySelection);

        // Returns name of action
        String GetName() const override;

        // Selects objects again
        void Redo() override;

        // Selects previous selected objects
        void Undo() override;

        SERIALIZABLE(SelectAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::SelectAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::SelectAction)
{
    FIELD().PUBLIC().NAME(selectedObjectsIds);
    FIELD().PUBLIC().NAME(prevSelectedObjectsIds);
    FIELD().PUBLIC().NAME(applySelection);
}
END_META;
CLASS_METHODS_META(Editor::SelectAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&, const Vector<Ref<SceneEditableObject>>&, const Function<void(const Vector<SceneUID>&)>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
