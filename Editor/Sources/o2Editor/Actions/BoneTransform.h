#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace o2
{
    class Actor;
}

namespace Editor
{
    // ---------------------------------------------------
    // Single-bone world transform change (Skeleton tool)
    // ---------------------------------------------------
    class BoneTransformAction: public IAction
    {
    public:
        SceneUID actorId = 0;

        Basis beforeWorldBasis;
        Vec2F beforeWorldPosition;

        Basis doneWorldBasis;
        Vec2F doneWorldPosition;

        bool doneCaptured = false;

    public:
        // Default constructor
        BoneTransformAction();

        // Constructor capturing the bone's current worldBasis and worldPosition as the before-snapshot
        explicit BoneTransformAction(const Ref<Actor>& boneActor);

        // Captures done-state from current scene
        void Completed();

        // Returns name of action
        String GetName() const override;

        // Sets done worldBasis and worldPosition
        void Redo() override;

        // Sets before worldBasis and worldPosition
        void Undo() override;

        // Merges a step over the same bone
        bool TryMerge(const Ref<IAction>& other) override;

        SERIALIZABLE(BoneTransformAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::BoneTransformAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::BoneTransformAction)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(actorId);
    FIELD().PUBLIC().NAME(beforeWorldBasis);
    FIELD().PUBLIC().NAME(beforeWorldPosition);
    FIELD().PUBLIC().NAME(doneWorldBasis);
    FIELD().PUBLIC().NAME(doneWorldPosition);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(doneCaptured);
}
END_META;
CLASS_METHODS_META(Editor::BoneTransformAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<Actor>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Completed);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PUBLIC().SIGNATURE(bool, TryMerge, const Ref<IAction>&);
}
END_META;
// --- END META ---
