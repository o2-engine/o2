#pragma once

#include "o2/Utils/Math/Layout.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    // ------------------------------------------------
    // Changes layout (anchors/offsets) of widget layers
    // ------------------------------------------------
    class WidgetLayerLayoutAction: public IAction
    {
    public:
        Vector<SceneUID> objectsIds;    // Changed objects ids
        Vector<Layout>   beforeLayouts; // Layouts before changing
        Vector<Layout>   doneLayouts;   // Layouts after changing

    public:
        // Default constructor
        WidgetLayerLayoutAction();

        // Constructor with objects (stores their current layouts as before) and the target layouts
        WidgetLayerLayoutAction(const Vector<Ref<SceneEditableObject>>& objects,
                                const Vector<Layout>& doneLayouts);

        // Returns name of action
        String GetName() const override;

        // Sets the target layouts again
        void Redo() override;

        // Restores the layouts before changing
        void Undo() override;

        SERIALIZABLE(WidgetLayerLayoutAction);

    private:
        // Applies layouts to objects and recomputes their transforms
        void SetLayouts(const Vector<Layout>& layouts);
    };
}
// --- META ---

CLASS_BASES_META(Editor::WidgetLayerLayoutAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::WidgetLayerLayoutAction)
{
    FIELD().PUBLIC().NAME(objectsIds);
    FIELD().PUBLIC().NAME(beforeLayouts);
    FIELD().PUBLIC().NAME(doneLayouts);
}
END_META;
CLASS_METHODS_META(Editor::WidgetLayerLayoutAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&, const Vector<Layout>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PRIVATE().SIGNATURE(void, SetLayouts, const Vector<Layout>&);
}
END_META;
// --- END META ---
