#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------
    // Mesh points snapshot action (MeshTopology / similar tools)
    // ---------------------------------------------------------
    class MeshPointsAction: public IAction
    {
    public:
        Vector<Vec2F> beforePoints;
        Vector<Vec2F> donePoints;

        bool doneCaptured = false;

        Function<void(int, Vec2F)> setPoint; // Runtime-only setter; null on deserialization

    public:
        // Default constructor
        MeshPointsAction();

        // Constructor capturing the current points as before-snapshot
        MeshPointsAction(const Vector<Vec2F>& currentPoints, const Function<void(int, Vec2F)>& setPoint);

        // Captures done state from given current points
        void Completed(const Vector<Vec2F>& currentPoints);

        // Returns name of action
        String GetName() const override;

        // Replays done points via setPoint
        void Redo() override;

        // Restores before points via setPoint
        void Undo() override;

        // Merges a step over the same points set
        bool TryMerge(const Ref<IAction>& other) override;

        SERIALIZABLE(MeshPointsAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::MeshPointsAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::MeshPointsAction)
{
    FIELD().PUBLIC().NAME(beforePoints);
    FIELD().PUBLIC().NAME(donePoints);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(doneCaptured);
    FIELD().PUBLIC().NAME(setPoint);
}
END_META;
CLASS_METHODS_META(Editor::MeshPointsAction)
{

    typedef const Function<void(int, Vec2F)>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Vec2F>&, _tmp1);
    FUNCTION().PUBLIC().SIGNATURE(void, Completed, const Vector<Vec2F>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PUBLIC().SIGNATURE(bool, TryMerge, const Ref<IAction>&);
}
END_META;
// --- END META ---
