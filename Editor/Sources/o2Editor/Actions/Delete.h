#pragma once

#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    // ---------------------
    // Delete objects action
    // ---------------------
    class DeleteAction: public IAction
    {
    public:
        class ObjectInfo: public ISerializable
        {
        public:
            DataDocument objectData;   // Serialized object @SERIALIZABLE
            SceneUID     parentId;       // Previous object parent @SERIALIZABLE
            SceneUID     prevObjectId; // Previous object sibling @SERIALIZABLE
            int          idx;          // Index in children @SERIALIZABLE

            bool operator==(const ObjectInfo& other) const;

            SERIALIZABLE(ObjectInfo);
        };

    public:
        Vector<ObjectInfo> objectsInfos; // Deleted objects infos

    public:
        // Default constructor
        DeleteAction();

        // Constructor with objects, that will be deleted
        DeleteAction(const Vector<Ref<SceneEditableObject>>& objects);

        // Returns name of action
        String GetName() const override;

        // Deletes objects again
        void Redo() override;

        // Reverting deleted objects
        void Undo() override;

        SERIALIZABLE(DeleteAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::DeleteAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::DeleteAction)
{
    FIELD().PUBLIC().NAME(objectsInfos);
}
END_META;
CLASS_METHODS_META(Editor::DeleteAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::DeleteAction::ObjectInfo)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::DeleteAction::ObjectInfo)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(objectData);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(parentId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(prevObjectId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(idx);
}
END_META;
CLASS_METHODS_META(Editor::DeleteAction::ObjectInfo)
{
}
END_META;
// --- END META ---
