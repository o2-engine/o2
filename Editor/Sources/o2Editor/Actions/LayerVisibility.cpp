#include "o2Editor/stdafx.h"
#include "LayerVisibility.h"

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"

namespace Editor
{
    LayerVisibilityAction::LayerVisibilityAction()
    {}

    LayerVisibilityAction::LayerVisibilityAction(const String& layerName, bool visible):
        layerName(layerName), visible(visible)
    {}

    String LayerVisibilityAction::GetName() const
    {
        return visible ? "Show layer" : "Hide layer";
    }

    void LayerVisibilityAction::Redo()
    {
        if (!o2Scene.HasLayer(layerName))
            return;
        o2Scene.GetLayer(layerName)->SetVisible(visible);
    }

    void LayerVisibilityAction::Undo()
    {
        if (!o2Scene.HasLayer(layerName))
            return;
        o2Scene.GetLayer(layerName)->SetVisible(!visible);
    }
}
// --- META ---

DECLARE_CLASS(Editor::LayerVisibilityAction, Editor__LayerVisibilityAction);
// --- END META ---
