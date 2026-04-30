#include "o2Editor/stdafx.h"
#include "MeshPoints.h"

namespace Editor
{
    MeshPointsAction::MeshPointsAction()
    {}

    MeshPointsAction::MeshPointsAction(const Vector<Vec2F>& currentPoints,
                                       const Function<void(int, Vec2F)>& setPointFn):
        beforePoints(currentPoints), setPoint(setPointFn)
    {}

    void MeshPointsAction::Completed(const Vector<Vec2F>& currentPoints)
    {
        donePoints = currentPoints;
        doneCaptured = true;
    }

    String MeshPointsAction::GetName() const
    {
        return "Mesh points";
    }

    void MeshPointsAction::Redo()
    {
        if (setPoint.IsEmpty() || !doneCaptured)
            return;

        for (int i = 0; i < donePoints.Count(); i++)
            setPoint(i, donePoints[i]);
    }

    void MeshPointsAction::Undo()
    {
        if (setPoint.IsEmpty())
            return;

        for (int i = 0; i < beforePoints.Count(); i++)
            setPoint(i, beforePoints[i]);
    }

    bool MeshPointsAction::TryMerge(const Ref<IAction>& other)
    {
        auto step = DynamicCast<MeshPointsAction>(other);
        if (!step || !step->doneCaptured || step->donePoints.Count() != beforePoints.Count())
            return false;

        donePoints = step->donePoints;
        doneCaptured = true;
        return true;
    }
}
// --- META ---

DECLARE_CLASS(Editor::MeshPointsAction, Editor__MeshPointsAction);
// --- END META ---
