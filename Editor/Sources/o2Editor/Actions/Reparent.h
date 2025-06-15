#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class Actor;
    class SceneEditableObject;
}

namespace Editor
{
    // ------------------------------
    // Change object's parents action
    // Storing old and new parent and 
    // index in parents
    // ------------------------------
    class ReparentAction: public IAction
    {
    public:
        struct ObjectInfo
        {
            SceneUID objectId;           // Object id
            SceneUID lastParentId;       // Previous parent id
            SceneUID lastPrevObjectId;   // Previous object id in children
            int      objectHierarchyIdx; // Object index in hierarchy
            Basis    transform;          // Object transform

            bool operator==(const ObjectInfo& other) const
            {
                return objectId == other.objectId;
            }
        };

        Vector<ObjectInfo> objectsInfos;    // Changed objects info
        SceneUID           newParentId;     // New parent id
        SceneUID           newPrevObjectId; // New object id in children

    public:
        // Default constructor
        ReparentAction();

        // Constructor
        ReparentAction(const Vector<Ref<SceneEditableObject>>& beginObjects);

        // Destructor
        ~ReparentAction();

        // Called when object are reparented, stores all required data to restore old objects' parents
        void ObjectsReparented(const Ref<SceneEditableObject>& newParent, const Ref<SceneEditableObject>& prevObject);

        // Returns name of action
        String GetName() const override;

        // Sets new parents again
        void Redo() override;

        // Sets previous stored parents and index in children
        void Undo() override;

        SERIALIZABLE(ReparentAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::ReparentAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::ReparentAction)
{
    FIELD().PUBLIC().NAME(objectsInfos);
    FIELD().PUBLIC().NAME(newParentId);
    FIELD().PUBLIC().NAME(newPrevObjectId);
}
END_META;
CLASS_METHODS_META(Editor::ReparentAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, ObjectsReparented, const Ref<SceneEditableObject>&, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
