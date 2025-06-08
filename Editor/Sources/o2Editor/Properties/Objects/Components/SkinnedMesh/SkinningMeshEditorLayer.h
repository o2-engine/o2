#pragma once

#include "o2/Render/Sprite.h"
#include "o2/Scene/Components/SkinningMeshComponent.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------
    // Scene edit layer, used to draw mesh's texture frame and mesh wire
    // -----------------------------------------------------------------
    struct SkinningMeshEditorLayer : public SceneEditorLayer
    {
        bool drawMeshWire = true; // Is draw mesh wire

        Ref<SkinningMeshComponent> mesh;          // Reference to mesh component
        Sprite                     textureSprite; // Texture sprite

    public:
        // Draws editor over scene
        void DrawOverScene() override;

        // Updates editor
        void Update(float dt) override;

        // Returns order of layer
        int GetOrder() const override;

        // Returns true if layer is enabled
        bool IsEnabled() const override;

        // Returns name of layer
        const String& GetName() const override;

        // Returns icon name of layer
        const String& GetIconName() const override;

        // Draws mesh wire
        void DrawMeshWire();
    };
}
