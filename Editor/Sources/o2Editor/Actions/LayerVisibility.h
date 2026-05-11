#pragma once

#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------
    // Toggle scene layer visibility flag
    // ---------------------------------
    class LayerVisibilityAction: public IAction
    {
    public:
        String layerName;       // Name of the layer
        bool   visible = true;  // Desired visibility

    public:
        // Default constructor
        LayerVisibilityAction();

        // Constructor capturing layer name and desired visibility
        LayerVisibilityAction(const String& layerName, bool visible);

        // Returns name of action
        String GetName() const override;

        // Sets stored visibility on the layer
        void Redo() override;

        // Restores opposite visibility on the layer
        void Undo() override;

        SERIALIZABLE(LayerVisibilityAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::LayerVisibilityAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::LayerVisibilityAction)
{
    FIELD().PUBLIC().NAME(layerName);
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(visible);
}
END_META;
CLASS_METHODS_META(Editor::LayerVisibilityAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
