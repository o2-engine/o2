#include "o2Editor/stdafx.h"
#include "SplineKeys.h"

namespace Editor
{
    SplineKeysAction::SplineKeysAction()
    {}

    SplineKeysAction::SplineKeysAction(const Ref<Spline>& s):
        spline(s)
    {
        if (s)
            beforeKeys = s->GetKeys();
    }

    void SplineKeysAction::Completed()
    {
        auto s = spline.Lock();
        if (!s)
            return;

        doneKeys = s->GetKeys();
        doneCaptured = true;
    }

    String SplineKeysAction::GetName() const
    {
        return "Spline keys";
    }

    void SplineKeysAction::Redo()
    {
        auto s = spline.Lock();
        if (!s || !doneCaptured)
            return;

        s->SetKeys(doneKeys);
    }

    void SplineKeysAction::Undo()
    {
        auto s = spline.Lock();
        if (!s)
            return;

        s->SetKeys(beforeKeys);
    }

    bool SplineKeysAction::TryMerge(const Ref<IAction>& other)
    {
        auto step = DynamicCast<SplineKeysAction>(other);
        if (!step || !step->doneCaptured)
            return false;

        auto myS = spline.Lock();
        auto stepS = step->spline.Lock();
        if (!myS || myS != stepS)
            return false;

        doneKeys = step->doneKeys;
        doneCaptured = true;
        return true;
    }
}
// --- META ---

DECLARE_CLASS(Editor::SplineKeysAction, Editor__SplineKeysAction);
// --- END META ---
