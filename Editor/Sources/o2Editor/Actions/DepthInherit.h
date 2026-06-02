#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class Actor;
}

namespace Editor
{
    // ----------------------------------------------------------
    // Change actors drawing-depth inheritance from parent action
    // ----------------------------------------------------------
    class DepthInheritAction: public IAction
    {
    public:
        Vector<SceneUID> objectsIds;     // Changed actors ids @SERIALIZABLE
        bool             inherit = false; // New inherit-from-parent value @SERIALIZABLE

    public:
        // Default constructor
        DepthInheritAction();

        // Constructor with actors and the new inherit value
        DepthInheritAction(const Vector<Ref<Actor>>& actors, bool inherit);

        // Returns name of action
        String GetName() const override;

        // Applies the inherit value again
        void Redo() override;

        // Reverts to the previous inherit value
        void Undo() override;

        SERIALIZABLE(DepthInheritAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::DepthInheritAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::DepthInheritAction)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(objectsIds);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(inherit);
}
END_META;
CLASS_METHODS_META(Editor::DepthInheritAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<Actor>>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
