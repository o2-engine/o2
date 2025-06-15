#pragma once
#include "IEditorTool.h"

#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"
#include "o2/Utils/Editor/FrameHandles.h"

namespace Editor
{
    // --------------------------------------------------------
    // Custom frame tool. Used for special frame editing things
    // --------------------------------------------------------
    struct CustomFrameTool: public IEditTool
    {
        // ----------------------
        // Scene layer for editor
        // ----------------------
        struct SceneLayer: public SceneEditorLayer
        {
            WeakRef<CustomFrameTool> tool; // Reference to tool

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
        };

    public:
        Ref<SceneLayer> sceneLayer = mmake<SceneLayer>(); // Scene layer for drawing spline

        Ref<FrameHandles> frameHandles;      // Frame handles 
        bool              isEnabled = false; // Is tool enabled now

        Function<void(const Basis&)> onChanged; // Called when frame changes
        Function<Vec2F()>            getOrigin; // Returns origin of frame         

    public:
        // Default constructor
        explicit CustomFrameTool(RefCounter* refCounter);

        // Empty copy operator
        CustomFrameTool& operator=(const CustomFrameTool& other) { return *this; }

        // Sets frame
        void SetFrame(const Basis& frame);

        // Returns current frame
        const Basis& GetFrame() const;

        // Resets callbacks
        void Reset();

        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        IOBJECT(CustomFrameTool);

    private:
        Basis mBasis; // Current editing basis
    };
}
// --- META ---

CLASS_BASES_META(Editor::CustomFrameTool)
{
    BASE_CLASS(Editor::IEditTool);
}
END_META;
CLASS_FIELDS_META(Editor::CustomFrameTool)
{
    FIELD().PUBLIC().DEFAULT_VALUE(mmake<SceneLayer>()).NAME(sceneLayer);
    FIELD().PUBLIC().NAME(frameHandles);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(isEnabled);
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PUBLIC().NAME(getOrigin);
    FIELD().PRIVATE().NAME(mBasis);
}
END_META;
CLASS_METHODS_META(Editor::CustomFrameTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFrame, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(const Basis&, GetFrame);
    FUNCTION().PUBLIC().SIGNATURE(void, Reset);
    FUNCTION().PUBLIC().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, OnEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDisabled);
}
END_META;
// --- END META ---
