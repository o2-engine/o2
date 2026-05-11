#pragma once

#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // -----------------------
    // Rename scene layer
    // -----------------------
    class LayerRenameAction: public IAction
    {
    public:
        String oldName; // Layer name before rename
        String newName; // Layer name after rename

    public:
        // Default constructor
        LayerRenameAction();

        // Constructor capturing old and new names
        LayerRenameAction(const String& oldName, const String& newName);

        // Returns name of action
        String GetName() const override;

        // Renames layer from oldName to newName
        void Redo() override;

        // Renames layer from newName back to oldName
        void Undo() override;

        SERIALIZABLE(LayerRenameAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::LayerRenameAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::LayerRenameAction)
{
    FIELD().PUBLIC().NAME(oldName);
    FIELD().PUBLIC().NAME(newName);
}
END_META;
CLASS_METHODS_META(Editor::LayerRenameAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
