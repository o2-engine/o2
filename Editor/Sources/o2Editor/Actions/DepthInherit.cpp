#include "o2Editor/stdafx.h"
#include "DepthInherit.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"

namespace Editor
{
    DepthInheritAction::DepthInheritAction()
    {}

    DepthInheritAction::DepthInheritAction(const Vector<Ref<Actor>>& actors, bool inherit):
        inherit(inherit)
    {
        objectsIds = actors.Convert<SceneUID>([](auto& x) { return x->GetID(); });
    }

    String DepthInheritAction::GetName() const
    {
        return "Change depth inheritance";
    }

    void DepthInheritAction::Redo()
    {
        for (auto& id : objectsIds)
        {
            if (auto actor = o2Scene.GetActorByID(id))
                actor->SetDrawingDepthInheritFromParent(inherit);
        }
    }

    void DepthInheritAction::Undo()
    {
        for (auto& id : objectsIds)
        {
            if (auto actor = o2Scene.GetActorByID(id))
                actor->SetDrawingDepthInheritFromParent(!inherit);
        }
    }
}
// --- META ---

DECLARE_CLASS(Editor::DepthInheritAction, Editor__DepthInheritAction);
// --- END META ---
