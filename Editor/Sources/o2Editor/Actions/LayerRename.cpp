#include "o2Editor/stdafx.h"
#include "LayerRename.h"

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"

namespace Editor
{
    LayerRenameAction::LayerRenameAction()
    {}

    LayerRenameAction::LayerRenameAction(const String& oldName, const String& newName):
        oldName(oldName), newName(newName)
    {}

    String LayerRenameAction::GetName() const
    {
        return "Rename layer";
    }

    void LayerRenameAction::Redo()
    {
        if (!o2Scene.HasLayer(oldName))
            return;
        o2Scene.GetLayer(oldName)->SetName(newName);
    }

    void LayerRenameAction::Undo()
    {
        if (!o2Scene.HasLayer(newName))
            return;
        o2Scene.GetLayer(newName)->SetName(oldName);
    }
}
// --- META ---

DECLARE_CLASS(Editor::LayerRenameAction, Editor__LayerRenameAction);
// --- END META ---
