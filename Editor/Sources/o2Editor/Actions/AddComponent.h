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
    // -------------------------------
    // Add components to actors action
    // -------------------------------
    class AddComponentAction: public IAction
    {
    public:
        class ComponentInfo: public ISerializable
        {
        public:
            SceneUID     actorId = 0;     // Owner actor id @SERIALIZABLE
            DataDocument componentData;   // Serialized added component @SERIALIZABLE
            SceneUID     componentId = 0; // Added component id @SERIALIZABLE

            bool operator==(const ComponentInfo& other) const;

            SERIALIZABLE(ComponentInfo);
        };

    public:
        Vector<ComponentInfo> components; // Added components infos

    public:
        // Default constructor
        AddComponentAction();

        // Constructor with actors and the component data to add to each of them
        AddComponentAction(const Vector<Ref<Actor>>& actors, const DataDocument& componentData);

        // Returns name of action
        String GetName() const override;

        // Adds the components again
        void Redo() override;

        // Removes the added components
        void Undo() override;

        SERIALIZABLE(AddComponentAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::AddComponentAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::AddComponentAction)
{
    FIELD().PUBLIC().NAME(components);
}
END_META;
CLASS_METHODS_META(Editor::AddComponentAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<Actor>>&, const DataDocument&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::AddComponentAction::ComponentInfo)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::AddComponentAction::ComponentInfo)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(actorId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(componentData);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(componentId);
}
END_META;
CLASS_METHODS_META(Editor::AddComponentAction::ComponentInfo)
{
}
END_META;
// --- END META ---
