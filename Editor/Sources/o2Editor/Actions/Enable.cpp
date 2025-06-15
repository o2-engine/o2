#include "o2Editor/stdafx.h"
#include "Enable.h"

#include "o2/Scene/Actor.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    EnableAction::EnableAction()
    {}

    EnableAction::EnableAction(const Vector<Ref<SceneEditableObject>>& objects, bool enable):
        enable(enable)
    {
        objectsIds = objects.Convert<SceneUID>([](auto& x) { return x->GetID(); });
    }

    String EnableAction::GetName() const
    {
        return enable ? "Enable actors" : "Disable actors";
    }

    void EnableAction::Redo()
    {
        for (auto& id : objectsIds)
        {
            auto object = o2Scene.GetEditableObjectByID(id);
            if (object)
                object->SetEnabled(enable);
        }
    }

    void EnableAction::Undo()
    {
        for (auto& id : objectsIds)
        {
            auto object = o2Scene.GetEditableObjectByID(id);
            if (object)
                object->SetEnabled(!enable);
        }
    }

}
// --- META ---

DECLARE_CLASS(Editor::EnableAction, Editor__EnableAction);
// --- END META ---
