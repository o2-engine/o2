#include "o2Editor/stdafx.h"
#include "LayerReorder.h"

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"

namespace Editor
{
    LayerReorderAction::LayerReorderAction()
    {}

    LayerReorderAction::LayerReorderAction(const String& layerName, int fromIdx, int toIdx):
        layerName(layerName), fromIdx(fromIdx), toIdx(toIdx)
    {}

    String LayerReorderAction::GetName() const
    {
        return "Reorder layer";
    }

    void LayerReorderAction::Redo()
    {
        if (!o2Scene.HasLayer(layerName))
            return;
        o2Scene.SetLayerOrder(o2Scene.GetLayer(layerName), toIdx);
    }

    void LayerReorderAction::Undo()
    {
        if (!o2Scene.HasLayer(layerName))
            return;
        o2Scene.SetLayerOrder(o2Scene.GetLayer(layerName), fromIdx);
    }
}
// --- META ---

DECLARE_CLASS(Editor::LayerReorderAction, Editor__LayerReorderAction);
// --- END META ---
