#include "o2Editor/stdafx.h"
#include "AddComponent.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"

namespace Editor
{
    AddComponentAction::AddComponentAction()
    {}

    AddComponentAction::AddComponentAction(const Vector<Ref<Actor>>& actors, const DataDocument& componentData)
    {
        for (auto& actor : actors)
        {
            ComponentInfo info;
            info.actorId = actor->GetID();
            info.componentData = componentData;
            components.Add(info);
        }
    }

    String AddComponentAction::GetName() const
    {
        return "Add component";
    }

    void AddComponentAction::Redo()
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

    void AddComponentAction::Undo()
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

    bool AddComponentAction::ComponentInfo::operator==(const ComponentInfo& other) const
    {
        return actorId == other.actorId && componentData == other.componentData && componentId == other.componentId;
    }
}
// --- META ---

DECLARE_CLASS(Editor::AddComponentAction, Editor__AddComponentAction);

DECLARE_CLASS(Editor::AddComponentAction::ComponentInfo, Editor__AddComponentAction__ComponentInfo);
// --- END META ---
