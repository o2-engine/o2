#include "o2Editor/stdafx.h"
#include "RevertToPrototype.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"

namespace Editor
{
    RevertToPrototypeAction::RevertToPrototypeAction()
    {}

    RevertToPrototypeAction::RevertToPrototypeAction(const Vector<Ref<Actor>>& actors)
    {
        for (auto& actor : actors)
        {
            if (!actor->GetPrototypeDirectly())
                continue;

            ActorInfo info;
            info.actorId = actor->GetID();
            actor->Serialize(info.snapshot);
            this->actors.Add(info);
        }
    }

    String RevertToPrototypeAction::GetName() const
    {
        return "Revert to prototype";
    }

    void RevertToPrototypeAction::Redo()
    {
        for (auto& info : actors)
        {
            if (auto actor = o2Scene.GetActorByID(info.actorId))
                actor->RevertToPrototype();
        }
    }

    void RevertToPrototypeAction::Undo()
    {
        for (auto& info : actors)
        {
            if (auto actor = o2Scene.GetActorByID(info.actorId))
                actor->Deserialize(info.snapshot);
        }
    }

    bool RevertToPrototypeAction::ActorInfo::operator==(const ActorInfo& other) const
    {
        return actorId == other.actorId && snapshot == other.snapshot;
    }
}
// --- META ---

DECLARE_CLASS(Editor::RevertToPrototypeAction, Editor__RevertToPrototypeAction);

DECLARE_CLASS(Editor::RevertToPrototypeAction::ActorInfo, Editor__RevertToPrototypeAction__ActorInfo);
// --- END META ---
