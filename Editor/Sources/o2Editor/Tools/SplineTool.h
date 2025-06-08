#pragma once
#include "IEditorTool.h"

#include "o2/Utils/Math/Spline.h"
#include "o2Editor/UI/SplineEditor/SplineEditor.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

namespace Editor
{
    // ------------------------------------------------
    // Spline editing on scene tool
    // Contains scene editor layer and spline editor
    // Works around spline origin, provided from getter
    // ------------------------------------------------
    struct SplineTool: public IEditTool
    {
        // -------------------
        // Spline data wrapper
        // -------------------
        struct SplineWrapper: public SplineEditor::ISplineWrapper
        {
            WeakRef<SplineTool> tool;   // Spline tool
            Ref<Spline>         spline; // Spline

            Function<Vec2F()> getOrigin; // Origin point getter

        public:
            // Returns origin point
            Vec2F GetOrigin() const;

            // Converts world point to local space
            Vec2F WorldToLocal(const Vec2F& point) const override;

            // Converts local point to world space
            Vec2F LocalToWorld(const Vec2F& point) const override;

            // Returns count of points in spline
            int GetPointsCount() const override;

            // Returns true if spline is closed
            bool IsClosed() const override;

            // Adds new point to spline
            void AddPoint(int idx, const Vec2F& position, const Vec2F& prevSupport, const Vec2F& nextSupport) override;

            // Removes point from spline
            void RemovePoint(int idx) override;

            // Returns position of point
            Vec2F GetPointPos(int idx) const override;

            // Sets position of point
            void SetPointPos(int idx, const Vec2F& pos) override;

            // Sets point range value
            void SetPointRangeValue(int idx, float value) override;

            // Returns point range value
            float GetPointRangeValue(int idx) const override;

            // Returns previous support position of point
            Vec2F GetPointPrevSupportPos(int idx) const override;

            // Sets previous support position of point
            void SetPointPrevSupportPos(int idx, const Vec2F& pos) override;

            // Returns next support position of point
            Vec2F GetPointNextSupportPos(int idx) const override;

            // Sets next support position of point
            void SetPointNextSupportPos(int idx, const Vec2F& pos) override;

            // Returns approximation points
            const ApproximationVec2F* GetPointApproximationLeft(int idx) const override;

            // Returns approximation points
            const ApproximationVec2F* GetPointApproximationRight(int idx) const override;

            // Returns count of approximation points
            int GetPointApproximationCount(int idx) const override;

            // Called when spline was changed
            void OnChanged() override;
        };

        // ------------------------------------------------
        // Spline scene layer. Used to draw spline on scene
        // ------------------------------------------------
        struct SplineSceneLayer: public SceneEditorLayer
        {
            WeakRef<SplineTool> tool;

        public:
            // Draws spline over scene
            void DrawOverScene() override;

            // Updates spline editor from scene
            void Update(float dt) override;

            // Returns sorting order of layer
            int GetOrder() const override;

            // Returns true if layer is enabled
            bool IsEnabled() const override;

            // Returns name of layer
            const String& GetName() const override;

            // Returns icon name of layer
            const String& GetIconName() const override;
        };

    public:
        Function<void()> onChanged; // Called when spline was changed

    public:
        Ref<SplineEditor>     splineEditor;   // Animation spline editor
        Ref<SplineSceneLayer> sceneLayer; // Scene layer for drawing spline

        bool isEnabled = true; // Is tool enabled now

    public:
        // Default constructor
        explicit SplineTool(RefCounter* refCounter);

        // Empty copy operator
        SplineTool& operator=(const SplineTool& other) { return *this; }

        // Sets editing spline
        void SetSpline(const Ref<Spline>& spline, const Function<Vec2F()>& getOrigin);

        // Resets spline and callbacks
        void Reset();

        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        IOBJECT(SplineTool);
    };
}
// --- META ---

CLASS_BASES_META(Editor::SplineTool)
{
    BASE_CLASS(Editor::IEditTool);
}
END_META;
CLASS_FIELDS_META(Editor::SplineTool)
{
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PUBLIC().NAME(splineEditor);
    FIELD().PUBLIC().NAME(sceneLayer);
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(isEnabled);
}
END_META;
CLASS_METHODS_META(Editor::SplineTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpline, const Ref<Spline>&, const Function<Vec2F()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Reset);
    FUNCTION().PUBLIC().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, OnEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDisabled);
}
END_META;
// --- END META ---
