#pragma once

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

    public:
        // Default constructor
        SelectAction();

        // CUnstructor with new and previous selected objects
        SelectAction(const Vector<Ref<SceneEditableObject>>& selectedObjects, const Vector<Ref<SceneEditableObject>>& prevSelectedObjects);

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
}
END_META;
CLASS_METHODS_META(Editor::SelectAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
