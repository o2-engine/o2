#include "o2Editor/stdafx.h"
#include "LayerCreate.h"

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"

namespace Editor
{
    LayerCreateAction::LayerCreateAction()
    {}

    LayerCreateAction::LayerCreateAction(const String& layerName):
        layerName(layerName)
    {}

    String LayerCreateAction::GetName() const
    {
        return "Create layer";
    }

    void LayerCreateAction::Redo()
    {
        o2Scene.AddLayer(layerName);
    }

    void LayerCreateAction::Undo()
    {
        if (o2Scene.HasLayer(layerName))
            o2Scene.RemoveLayer(layerName);
    }
}
// --- META ---

DECLARE_CLASS(Editor::LayerCreateAction, Editor__LayerCreateAction);
// --- END META ---
