#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    // -------------------------------
    // Enable or disable scene objects
    // -------------------------------
    class EnableAction: public IAction
    {
    public:
        Vector<SceneUID> objectsIds; // Changed objects
        bool             enable;     // Enabled or disabled

    public:
        // Default constructor
        EnableAction();

        // COnstructor with enabled or disabled objects
        EnableAction(const Vector<Ref<SceneEditableObject>>& objects, bool enable);

        // Returns name of action
        String GetName() const override;

        // Enable or disable again
        void Redo() override;

        // Reverts objects to previous state
        void Undo() override;

        SERIALIZABLE(EnableAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::EnableAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::EnableAction)
{
    FIELD().PUBLIC().NAME(objectsIds);
    FIELD().PUBLIC().NAME(enable);
}
END_META;
CLASS_METHODS_META(Editor::EnableAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
