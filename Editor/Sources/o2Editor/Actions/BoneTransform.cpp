#include "o2Editor/stdafx.h"
#include "BoneTransform.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorTransform.h"
#include "o2/Scene/Scene.h"

namespace Editor
{
    BoneTransformAction::BoneTransformAction()
    {}

    BoneTransformAction::BoneTransformAction(const Ref<Actor>& boneActor)
    {
        if (!boneActor)
            return;

        actorId = boneActor->GetID();
        beforeWorldBasis = boneActor->transform->worldBasis;
        beforeWorldPosition = boneActor->transform->GetWorldPosition2D();
    }

    void BoneTransformAction::Completed()
    {
        auto editable = o2Scene.GetEditableObjectByID(actorId);
        auto actor = DynamicCast<Actor>(editable);
        if (!actor)
            return;

        doneWorldBasis = actor->transform->worldBasis;
        doneWorldPosition = actor->transform->GetWorldPosition2D();
        doneCaptured = true;
    }

    String BoneTransformAction::GetName() const
    {
        return "Bone transform";
    }

    void BoneTransformAction::Redo()
    {
        auto editable = o2Scene.GetEditableObjectByID(actorId);
        auto actor = DynamicCast<Actor>(editable);
        if (!actor || !doneCaptured)
            return;

        actor->transform->worldBasis = doneWorldBasis;
        actor->transform->worldPosition2D = doneWorldPosition;
        actor->UpdateTransform();
    }

    void BoneTransformAction::Undo()
    {
        auto editable = o2Scene.GetEditableObjectByID(actorId);
        auto actor = DynamicCast<Actor>(editable);
        if (!actor)
            return;

        actor->transform->worldBasis = beforeWorldBasis;
        actor->transform->worldPosition2D = beforeWorldPosition;
        actor->UpdateTransform();
    }

    bool BoneTransformAction::TryMerge(const Ref<IAction>& other)
    {
        auto step = DynamicCast<BoneTransformAction>(other);
        if (!step || step->actorId != actorId || !step->doneCaptured)
            return false;

        doneWorldBasis = step->doneWorldBasis;
        doneWorldPosition = step->doneWorldPosition;
        doneCaptured = true;
        return true;
    }
}
// --- META ---

DECLARE_CLASS(Editor::BoneTransformAction, Editor__BoneTransformAction);
// --- END META ---
