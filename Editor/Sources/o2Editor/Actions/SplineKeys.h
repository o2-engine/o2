#pragma once

#include "o2/Utils/Math/Spline.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IAction.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------
    // Spline keys snapshot action
    // -----------------------------------
    class SplineKeysAction: public IAction
    {
    public:
        WeakRef<Spline> spline;

        Vector<Spline::Key> beforeKeys;
        Vector<Spline::Key> doneKeys;

        bool doneCaptured = false;

    public:
        // Default constructor
        SplineKeysAction();

        // Constructor capturing the current spline keys as before-snapshot
        explicit SplineKeysAction(const Ref<Spline>& spline);

        // Captures done state from the spline current keys
        void Completed();

        // Returns name of action
        String GetName() const override;

        // Replays done keys
        void Redo() override;

        // Restores before keys
        void Undo() override;

        // Merges a step over the same spline
        bool TryMerge(const Ref<IAction>& other) override;

        SERIALIZABLE(SplineKeysAction);
    };
}
// --- META ---

CLASS_BASES_META(Editor::SplineKeysAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::SplineKeysAction)
{
    FIELD().PUBLIC().NAME(spline);
    FIELD().PUBLIC().NAME(beforeKeys);
    FIELD().PUBLIC().NAME(doneKeys);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(doneCaptured);
}
END_META;
CLASS_METHODS_META(Editor::SplineKeysAction)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<Spline>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Completed);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
    FUNCTION().PUBLIC().SIGNATURE(bool, TryMerge, const Ref<IAction>&);
}
END_META;
// --- END META ---
