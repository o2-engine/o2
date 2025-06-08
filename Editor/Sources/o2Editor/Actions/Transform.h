#pragma once

#include "o2/Scene/ActorTransform.h"
#include "o2/Utils/Math/Basis.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    // ---------------------------
    // Objects transforming action
    // ---------------------------
    class TransformAction: public IAction
    {
    public:
        struct Transform
        {
            Basis  transform;
            Layout layout;

            bool operator==(const Transform& other) const;
        };

        Vector<SceneUID>  objectsIds;       // Changed objects ids
        Vector<Transform> beforeTransforms; // Transforms before changing
        Vector<Transform> doneTransforms;   // Transforms after changing

    public:
        // Default constructor
        TransformAction();

        // Constructor with objects, stores theirs before changing transforms
        TransformAction(const Vector<Ref<SceneEditableObject>>& objects);

        // Called when transform completed, stores changed transforms
        void Completed();

        // Returns name of action
        String GetName() const override;

        // Sets new transforms again
        void Redo() override;

        // Sets transformations before transform
        void Undo() override;

        SERIALIZABLE(TransformAction);

    private:
        // Gets objects transforms and puts into transforms vector
        void GetTransforms(const Vector<SceneUID>& objectIds, Vector<Transform>& transforms);

        // Sets transformations and layouts to objects
        void SetTransforms(const Vector<SceneUID>& objectIds, Vector<Transform>& transforms);
    };
}
// --- META ---

CLASS_BASES_META(Editor::TransformAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::TransformAction)
{
    FIELD().PUBLIC().NAME(objectsIds);
    FIELD().PUBLIC().NAME(beforeTransforms);
    FIELD().PUBLIC().NAME(doneTransforms);
}
END_META;
CLASS_METHODS_META(Editor::TransformAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Completed);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PRIVATE().SIGNATURE(void, GetTransforms, const Vector<SceneUID>&, Vector<Transform>&);
    FUNCTION().PRIVATE().SIGNATURE(void, SetTransforms, const Vector<SceneUID>&, Vector<Transform>&);
}
END_META;
// --- END META ---
