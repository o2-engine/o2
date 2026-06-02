#include "o2Editor/stdafx.h"
#include "RemoveComponent.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"

namespace Editor
{
    RemoveComponentAction::RemoveComponentAction()
    {}

    RemoveComponentAction::RemoveComponentAction(const Vector<Ref<Component>>& components)
    {
        for (auto& component : components)
        {
            ComponentInfo info;
            auto actor = component->GetActor();
            info.actorId = actor ? actor->GetID() : 0;
            info.componentData = component;
            info.componentId = component->GetID();
            this->components.Add(info);
        }
    }

    String RemoveComponentAction::GetName() const
    {
        return "Remove component";
    }

    void RemoveComponentAction::Redo()
    {
        for (auto& info : components)
        {
            auto actor = o2Scene.GetActorByID(info.actorId);
            if (!actor)
                continue;

            if (auto component = actor->GetComponent(info.componentId))
                actor->RemoveComponent(component);
        }
    }

    void RemoveComponentAction::Undo()
    {
        for (auto& info : components)
        {
            auto actor = o2Scene.GetActorByID(info.actorId);
            if (!actor)
                continue;

            Ref<Component> component = info.componentData;
            actor->AddComponent(component);
            component->OnAddedFromEditor();

            info.componentId = component->GetID();
        }
    }

    bool RemoveComponentAction::ComponentInfo::operator==(const ComponentInfo& other) const
    {
        return actorId == other.actorId && componentData == other.componentData && componentId == other.componentId;
    }
}
// --- META ---

DECLARE_CLASS(Editor::RemoveComponentAction, Editor__RemoveComponentAction);

DECLARE_CLASS(Editor::RemoveComponentAction::ComponentInfo, Editor__RemoveComponentAction__ComponentInfo);
// --- END META ---
