#include "o2Editor/stdafx.h"
#include "BreakPrototype.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"

namespace Editor
{
    BreakPrototypeAction::BreakPrototypeAction()
    {}

    BreakPrototypeAction::BreakPrototypeAction(const Vector<Ref<Actor>>& actors)
    {
        for (auto& actor : actors)
        {
            auto prototype = actor->GetPrototypeDirectly();
            if (!prototype)
                continue;

            ActorInfo info;
            info.actorId = actor->GetID();
            info.prototype = prototype;
            this->actors.Add(info);
        }
    }

    String BreakPrototypeAction::GetName() const
    {
        return "Break prototype link";
    }

    void BreakPrototypeAction::Redo()
    {
        for (auto& info : actors)
        {
            if (auto actor = o2Scene.GetActorByID(info.actorId))
                actor->BreakPrototypeLink();
        }
    }

    void BreakPrototypeAction::Undo()
    {
        for (auto& info : actors)
        {
            if (auto actor = o2Scene.GetActorByID(info.actorId))
                actor->SetPrototype(info.prototype);
        }
    }

    bool BreakPrototypeAction::ActorInfo::operator==(const ActorInfo& other) const
    {
        return actorId == other.actorId && prototype == other.prototype;
    }
}
// --- META ---

DECLARE_CLASS(Editor::BreakPrototypeAction, Editor__BreakPrototypeAction);

DECLARE_CLASS(Editor::BreakPrototypeAction::ActorInfo, Editor__BreakPrototypeAction__ActorInfo);
// --- END META ---
