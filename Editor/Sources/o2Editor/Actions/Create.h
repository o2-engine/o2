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
    // Scene objects creation action
    // -----------------------------
    class CreateAction: public IAction
    {
    public:
        DataDocument     objectsData;        // Serialized created objects
        Vector<SceneUID> objectsIds;         // Created objects ids
        SceneUID         insertParentId;     // Parent id
        SceneUID         insertPrevObjectId; // Previous object id

    public:
        // Default constructor
        CreateAction();

        // Constructor wit created actors and their places in their parents
        CreateAction(const Vector<Ref<SceneEditableObject>>& objects, const Ref<SceneEditableObject>& parent, const Ref<SceneEditableObject>& prevObject);

        // Returns name of action
        String GetName() const override;

        // Creates all objects again
        void Redo() override;

        // Removes created objects
        void Undo() override;

        SERIALIZABLE(CreateAction);
    };

}
// --- META ---

CLASS_BASES_META(Editor::CreateAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::CreateAction)
{
    FIELD().PUBLIC().NAME(objectsData);
    FIELD().PUBLIC().NAME(objectsIds);
    FIELD().PUBLIC().NAME(insertParentId);
    FIELD().PUBLIC().NAME(insertPrevObjectId);
}
END_META;
CLASS_METHODS_META(Editor::CreateAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&, const Ref<SceneEditableObject>&, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
