#pragma once
#include "IEditorTool.h"

#include "o2/Render/Mesh.h"
#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

namespace Editor
{
    // -------------------------
    // Mesh weights editing tool
    // -------------------------
    struct MeshWeightsTool: public IEditTool
    {
        // ----------------------
        // Scene layer for editor
        // ----------------------
        struct SceneLayer: public SceneEditorLayer
        {
            WeakRef<MeshWeightsTool> tool; // Reference to tool

        public:
            // Draws editor over scene
            void DrawScene() override;

            // Draws mesh wire
            void DrawMeshWire(auto& mesh);

            // Updates editor
            void Update(float dt) override;

            // Updates brush
            void UpdateBrush(float dt);

            // Returns order of layer
            int GetOrder() const override;

            // Returns true if layer is enabled
            bool IsEnabled() const override;

            // Returns name of layer
            const String& GetName() const override;

            // Returns icon name of layer
            const String& GetIconName() const override;
        };

    public:
        Ref<SkinningMeshBoneComponent> boneComponent; // Reference to selected bone component

        Ref<SceneLayer> sceneLayer = mmake<SceneLayer>(); // Scene layer for drawing spline

        bool         isEnabled = false; // Is tool enabled now       

    public:
        // Default constructor
        explicit MeshWeightsTool(RefCounter* refCounter);

        // Empty copy operator
        MeshWeightsTool& operator=(const MeshWeightsTool& other) { return *this; }

        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        IOBJECT(MeshWeightsTool);

    private:
        Vec2F mCursosPos;            // Current cursor position, used for brush
        bool  mPressed = false;      // Is brush pressed
        float mBrushReadius = 50;    // Current brush radius
        float mBrushStrength = 1.0f; // Strength of brush

        Mesh mWeightsDebugMesh; // Mesh for drawing debug colors of weights

    private:
        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::MeshWeightsTool)
{
    BASE_CLASS(Editor::IEditTool);
}
END_META;
CLASS_FIELDS_META(Editor::MeshWeightsTool)
{
    FIELD().PUBLIC().NAME(boneComponent);
    FIELD().PUBLIC().DEFAULT_VALUE(mmake<SceneLayer>()).NAME(sceneLayer);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(isEnabled);
    FIELD().PRIVATE().NAME(mCursosPos);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mPressed);
    FIELD().PRIVATE().DEFAULT_VALUE(50).NAME(mBrushReadius);
    FIELD().PRIVATE().DEFAULT_VALUE(1.0f).NAME(mBrushStrength);
    FIELD().PRIVATE().NAME(mWeightsDebugMesh);
}
END_META;
CLASS_METHODS_META(Editor::MeshWeightsTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, OnEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDisabled);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
}
END_META;
// --- END META ---
