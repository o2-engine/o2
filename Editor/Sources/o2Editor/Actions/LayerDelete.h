#pragma once

#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SceneLayer;
}

namespace Editor
{
    // -----------------------------------------------------
    // Delete scene layer; saves order/visibility for undo
    // -----------------------------------------------------
    class LayerDeleteAction: public IAction
    {
    public:
        String layerName;          // Name of the layer to delete
        bool   savedVisible = true; // Visibility flag captured at deletion time
        int    savedIdx = -1;       // Position in scene layer list captured at deletion time

    public:
        // Default constructor
        LayerDeleteAction();

        // Constructor capturing layer state for undo
        LayerDeleteAction(const Ref<SceneLayer>& layer);

        // Returns name of action
        String GetName() const override;

        // Removes layer
        void Redo() override;

        // Recreates layer at saved position with saved visibility
        void Undo() override;

        SERIALIZABLE(LayerDeleteAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::LayerDeleteAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::LayerDeleteAction)
{
    FIELD().PUBLIC().NAME(layerName);
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(savedVisible);
    FIELD().PUBLIC().DEFAULT_VALUE(-1).NAME(savedIdx);
}
END_META;
CLASS_METHODS_META(Editor::LayerDeleteAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<SceneLayer>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
