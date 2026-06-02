#pragma once

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class Actor;
}

namespace Editor
{
    // ----------------------------------
    // Break actors prototype link action
    // ----------------------------------
    class BreakPrototypeAction: public IAction
    {
    public:
        class ActorInfo: public ISerializable
        {
        public:
            SceneUID             actorId = 0; // Actor id @SERIALIZABLE
            AssetRef<ActorAsset> prototype;   // Prototype asset to restore on undo @SERIALIZABLE

            bool operator==(const ActorInfo& other) const;

            SERIALIZABLE(ActorInfo);
        };

    public:
        Vector<ActorInfo> actors; // Actors with broken prototype links

    public:
        // Default constructor
        BreakPrototypeAction();

        // Constructor with actors whose prototype links will be broken
        BreakPrototypeAction(const Vector<Ref<Actor>>& actors);

        // Returns name of action
        String GetName() const override;

        // Breaks the prototype links again
        void Redo() override;

        // Restores the prototype links
        void Undo() override;

        SERIALIZABLE(BreakPrototypeAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::BreakPrototypeAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::BreakPrototypeAction)
{
    FIELD().PUBLIC().NAME(actors);
}
END_META;
CLASS_METHODS_META(Editor::BreakPrototypeAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<Actor>>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::BreakPrototypeAction::ActorInfo)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::BreakPrototypeAction::ActorInfo)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(actorId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(prototype);
}
END_META;
CLASS_METHODS_META(Editor::BreakPrototypeAction::ActorInfo)
{
}
END_META;
// --- END META ---
