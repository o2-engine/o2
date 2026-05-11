#pragma once

#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------
    // Reorder scene layer in layer list
    // ----------------------------------
    class LayerReorderAction: public IAction
    {
    public:
        String layerName;   // Name of the layer
        int    fromIdx = 0; // Position before reorder
        int    toIdx = 0;   // Position after reorder

    public:
        // Default constructor
        LayerReorderAction();

        // Constructor capturing layer name and indexes
        LayerReorderAction(const String& layerName, int fromIdx, int toIdx);

        // Returns name of action
        String GetName() const override;

        // Moves layer to toIdx
        void Redo() override;

        // Moves layer back to fromIdx
        void Undo() override;

        SERIALIZABLE(LayerReorderAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::LayerReorderAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::LayerReorderAction)
{
    FIELD().PUBLIC().NAME(layerName);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(fromIdx);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(toIdx);
}
END_META;
CLASS_METHODS_META(Editor::LayerReorderAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, int, int);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
