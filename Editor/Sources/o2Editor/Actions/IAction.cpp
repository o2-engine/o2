#include "o2Editor/stdafx.h"
#include "IAction.h"

#include "o2/Utils/Debug/Assert.h"

namespace Editor
{
    void IAction::Append(const Ref<IAction>& other)
    {
        bool merged = TryMerge(other);
        Assert(merged, "IAction::Append: TryMerge rejected appended action");
        if (merged)
            other->Redo();
    }
}
// --- META ---

DECLARE_CLASS(Editor::IAction, Editor__IAction);
// --- END META ---
