#pragma once

#include "o2/Utils/Types/String.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ---------------------------
    // Create scene layer by name
    // ---------------------------
    class LayerCreateAction: public IAction
    {
    public:
        String layerName; // Name of the layer to create

    public:
        // Default constructor
        LayerCreateAction();

        // Constructor capturing the new layer name
        LayerCreateAction(const String& layerName);

        // Returns name of action
        String GetName() const override;

        // Adds layer with stored name
        void Redo() override;

        // Removes layer with stored name
        void Undo() override;

        SERIALIZABLE(LayerCreateAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::LayerCreateAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::LayerCreateAction)
{
    FIELD().PUBLIC().NAME(layerName);
}
END_META;
CLASS_METHODS_META(Editor::LayerCreateAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
