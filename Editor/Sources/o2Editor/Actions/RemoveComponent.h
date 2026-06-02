#pragma once

#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class Component;
}

namespace Editor
{
    // ------------------------------------
    // Remove components from actors action
    // ------------------------------------
    class RemoveComponentAction: public IAction
    {
    public:
        class ComponentInfo: public ISerializable
        {
        public:
            SceneUID     actorId = 0;     // Owner actor id @SERIALIZABLE
            DataDocument componentData;   // Serialized removed component @SERIALIZABLE
            SceneUID     componentId = 0; // Removed component id @SERIALIZABLE

            bool operator==(const ComponentInfo& other) const;

            SERIALIZABLE(ComponentInfo);
        };

    public:
        Vector<ComponentInfo> components; // Removed components infos

    public:
        // Default constructor
        RemoveComponentAction();

        // Constructor with components that will be removed
        RemoveComponentAction(const Vector<Ref<Component>>& components);

        // Returns name of action
        String GetName() const override;

        // Removes the components again
        void Redo() override;

        // Restores the removed components
        void Undo() override;

        SERIALIZABLE(RemoveComponentAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::RemoveComponentAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::RemoveComponentAction)
{
    FIELD().PUBLIC().NAME(components);
}
END_META;
CLASS_METHODS_META(Editor::RemoveComponentAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<Component>>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::RemoveComponentAction::ComponentInfo)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::RemoveComponentAction::ComponentInfo)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(actorId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(componentData);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(componentId);
}
END_META;
CLASS_METHODS_META(Editor::RemoveComponentAction::ComponentInfo)
{
}
END_META;
// --- END META ---
