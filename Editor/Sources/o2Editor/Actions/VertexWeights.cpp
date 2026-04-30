#include "o2Editor/stdafx.h"
#include "VertexWeights.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2/Scene/Scene.h"

namespace Editor
{
    namespace
    {
        Ref<SkinningMeshBoneComponent> ResolveBone(SceneUID id)
        {
            auto editable = o2Scene.GetEditableObjectByID(id);
            auto actor = DynamicCast<Actor>(editable);
            if (!actor)
                return nullptr;
            return actor->GetComponent<SkinningMeshBoneComponent>();
        }
    }

    VertexWeightsAction::VertexWeightsAction()
    {}

    VertexWeightsAction::VertexWeightsAction(const Ref<SkinningMeshBoneComponent>& boneComponent)
    {
        if (!boneComponent)
            return;

        auto actor = boneComponent->GetActor();
        if (!actor)
            return;

        actorId = actor->GetID();
        beforeWeights = boneComponent->vertexWeights;
    }

    void VertexWeightsAction::Completed()
    {
        auto bone = ResolveBone(actorId);
        if (!bone)
            return;

        doneWeights = bone->vertexWeights;
        doneCaptured = true;
    }

    String VertexWeightsAction::GetName() const
    {
        return "Vertex weights";
    }

    void VertexWeightsAction::Redo()
    {
        auto bone = ResolveBone(actorId);
        if (!bone || !doneCaptured)
            return;

        bone->vertexWeights = doneWeights;
    }

    void VertexWeightsAction::Undo()
    {
        auto bone = ResolveBone(actorId);
        if (!bone)
            return;

        bone->vertexWeights = beforeWeights;
    }

    bool VertexWeightsAction::TryMerge(const Ref<IAction>& other)
    {
        auto step = DynamicCast<VertexWeightsAction>(other);
        if (!step || step->actorId != actorId || !step->doneCaptured)
            return false;

        doneWeights = step->doneWeights;
        doneCaptured = true;
        return true;
    }
}
// --- META ---

DECLARE_CLASS(Editor::VertexWeightsAction, Editor__VertexWeightsAction);
// --- END META ---
