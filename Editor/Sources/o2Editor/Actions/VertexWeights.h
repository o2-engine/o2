#pragma once

#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Pair.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class SkinningMeshBoneComponent;
}

namespace Editor
{
    // -----------------------------------------------------
    // Bone vertex-weights snapshot action (MeshWeights tool)
    // -----------------------------------------------------
    class VertexWeightsAction: public IAction
    {
    public:
        SceneUID actorId = 0;

        Vector<Pair<int, float>> beforeWeights;
        Vector<Pair<int, float>> doneWeights;

        bool doneCaptured = false;

    public:
        // Default constructor
        VertexWeightsAction();

        // Constructor capturing the bone's current vertexWeights as before-snapshot
        explicit VertexWeightsAction(const Ref<SkinningMeshBoneComponent>& boneComponent);

        // Captures done state from current scene
        void Completed();

        // Returns name of action
        String GetName() const override;

        // Replays done weights
        void Redo() override;

        // Restores before weights
        void Undo() override;

        // Merges a step over the same bone
        bool TryMerge(const Ref<IAction>& other) override;

        SERIALIZABLE(VertexWeightsAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::VertexWeightsAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::VertexWeightsAction)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(actorId);
    FIELD().PUBLIC().NAME(beforeWeights);
    FIELD().PUBLIC().NAME(doneWeights);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(doneCaptured);
}
END_META;
CLASS_METHODS_META(Editor::VertexWeightsAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<SkinningMeshBoneComponent>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Completed);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PUBLIC().SIGNATURE(bool, TryMerge, const Ref<IAction>&);
}
END_META;
// --- END META ---
