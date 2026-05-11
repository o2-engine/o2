#include "o2Editor/stdafx.h"
#include "LayerDelete.h"

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"

namespace Editor
{
    LayerDeleteAction::LayerDeleteAction()
    {}

    LayerDeleteAction::LayerDeleteAction(const Ref<SceneLayer>& layer)
    {
        if (!layer)
            return;

        layerName    = layer->GetName();
        savedVisible = layer->visible;
        savedIdx     = o2Scene.GetLayers().IndexOf(layer);
    }

    String LayerDeleteAction::GetName() const
    {
        return "Delete layer";
    }

    void LayerDeleteAction::Redo()
    {
        if (o2Scene.HasLayer(layerName))
            o2Scene.RemoveLayer(layerName);
    }

    void LayerDeleteAction::Undo()
    {
        auto layer = o2Scene.AddLayer(layerName);
        if (!layer)
            return;

        layer->visible = savedVisible;
        if (savedIdx >= 0)
            o2Scene.SetLayerOrder(layer, savedIdx);
    }
}
// --- META ---

DECLARE_CLASS(Editor::LayerDeleteAction, Editor__LayerDeleteAction);
// --- END META ---
