#pragma once

#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class Actor;
}

namespace Editor
{
    // -------------------------------------
    // Revert actors to their prototype action
    // -------------------------------------
    class RevertToPrototypeAction: public IAction
    {
    public:
        class ActorInfo: public ISerializable
        {
        public:
            SceneUID     actorId = 0; // Actor id @SERIALIZABLE
            DataDocument snapshot;    // Actor state before revert, restored on undo @SERIALIZABLE

            bool operator==(const ActorInfo& other) const;

            SERIALIZABLE(ActorInfo);
        };

    public:
        Vector<ActorInfo> actors; // Reverted actors infos

    public:
        // Default constructor
        RevertToPrototypeAction();

        // Constructor with actors that will be reverted to their prototype
        RevertToPrototypeAction(const Vector<Ref<Actor>>& actors);

        // Returns name of action
        String GetName() const override;

        // Reverts the actors to their prototype again
        void Redo() override;

        // Restores the actors pre-revert state
        void Undo() override;

        SERIALIZABLE(RevertToPrototypeAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::RevertToPrototypeAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::RevertToPrototypeAction)
{
    FIELD().PUBLIC().NAME(actors);
}
END_META;
CLASS_METHODS_META(Editor::RevertToPrototypeAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<Actor>>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::RevertToPrototypeAction::ActorInfo)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::RevertToPrototypeAction::ActorInfo)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(actorId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(snapshot);
}
END_META;
CLASS_METHODS_META(Editor::RevertToPrototypeAction::ActorInfo)
{
}
END_META;
// --- END META ---
