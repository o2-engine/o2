#pragma once

#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    // -----------------------------------------------------
    // Scene object property change action.
    // Storing path to value, values before and after change
    // -----------------------------------------------------
    class PropertyChangeAction: public IAction
    {
    public:
        Vector<SceneUID>     objectsIds;   // Changed objects
        String               propertyPath; // Path to property
        Vector<DataDocument> beforeValues; // Serialized values before change
        Vector<DataDocument> afterValues;  // Serialized values after change

    public:
        // Default constructor
        PropertyChangeAction();

        // Constructor with all data
        PropertyChangeAction(const Vector<Ref<SceneEditableObject>>& objects,
                             const String& propertyPath,
                             const Vector<DataDocument>& beforeValues,
                             const Vector<DataDocument>& afterValues);

        // Returns name of action
        String GetName() const override;

        // Sets object's properties value as after change
        void Redo() override;

        // Sets object's properties value as before change
        void Undo() override;

        SERIALIZABLE(PropertyChangeAction);

    protected:
        // Sets object's properties values
        void SetProperties(Vector<DataDocument>& value);
    };
}
// --- META ---

CLASS_BASES_META(Editor::PropertyChangeAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::PropertyChangeAction)
{
    FIELD().PUBLIC().NAME(objectsIds);
    FIELD().PUBLIC().NAME(propertyPath);
    FIELD().PUBLIC().NAME(beforeValues);
    FIELD().PUBLIC().NAME(afterValues);
}
END_META;
CLASS_METHODS_META(Editor::PropertyChangeAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PROTECTED().SIGNATURE(void, SetProperties, Vector<DataDocument>&);
}
END_META;
// --- END META ---
