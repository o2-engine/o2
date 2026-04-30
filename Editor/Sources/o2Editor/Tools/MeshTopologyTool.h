#pragma once
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Components/MeshComponent.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2Editor/Tools/IEditorTool.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"

namespace Editor
{
    FORWARD_CLASS_REF(MeshPointsAction);

    // ---------------------------------------------------------
    // Mesh component topology tool. Adds extra points into mesh-
    // --------------------------------------------------------
    struct MeshTopologyTool: public IEditTool, public SelectableDragHandlesGroup, public CursorAreaEventsListener, public KeyboardEventsListener
    {
        // ----------------------
        // Scene layer for editor
        // ----------------------
        struct SceneLayer: public SceneEditorLayer
        {
            WeakRef<MeshTopologyTool> tool; // Reference to tool

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

        bool isEnabled = false; // Is tool enabled now

        Function<void()> onChanged; // Called when frame changes     

    public:
        // Default constructor
        explicit MeshTopologyTool(RefCounter* refCounter);

        // Default constructor
        MeshTopologyTool(const MeshTopologyTool& other);

        // Destructor
        ~MeshTopologyTool();

        // Empty copy operator
        MeshTopologyTool& operator=(const MeshTopologyTool& other) { return *this; }

        // Setup editing callbacks
        void Setup(const Function<Vector<Vec2F>()>& getPoints, const Function<void(int, Vec2F)>& setPoint, 
                   const Function<Basis()>& getTransform, const Function<void(Vec2F)>& addPoint,
                   const Function<void(int)>& removePoint);

        // Resets callbacks
        void Reset();

        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Dynamic cast to RefCounterable via Widget
        static Ref<RefCounterable> CastToRefCounterable(const Ref<MeshTopologyTool>& ref);

        IOBJECT(MeshTopologyTool);

    private:
        Function<Vector<Vec2F>()>  mGetPoints;   // Gets editing points list
        Function<void(int, Vec2F)> mSetPoint;    // Sets editing point position by index
        Function<void(Vec2F)>      mAddPoint;    // Add new point
        Function<void(int)>        mRemovePoint; // Removes point by index

        Function<Basis()> mGetTransform; // Returns local space transformation

        DragHandle              mHandleSample; // Point handle sample
        Vector<Ref<DragHandle>> mHandles;      // List of all handles

        Sprite mSelectionSprite;       // Selection sprite
        Vec2F  mSelectingPressedPoint; // Point, where cursor was pressed, selection starts here, in local space

        Vector<Ref<DragHandle>> mSelectingHandlesBuf; // Potentially selecting handles while selecting

        FrameHandles mTransformFrame;                      // Keys transformation frame
        bool         mTransformFrameVisible = false;       // Is transform frame visible. it visible when 2 or more main handles was selected
        Basis        mTransformFrameBasis;                 // Basis of transform frame in screen space
        Vec2F        mTransformBasisOffet = Vec2F(10, 10); // Border between side points and frame

        Ref<MeshPointsAction> mPointsAction; // Current points-edit action

    private:
        // Initializes handles
        void InitializeHandles();

        // Clears all handles
        void ClearHandles();

        // Called when handle was moved
        void OnHandleMoved(int i, const Vec2F& pos);

        // Begins a points-edit action capturing the current points snapshot
        void BeginPointsAction();

        // Commits the current points-edit action to the undo stack
        void EndPointsAction();

        // Builds an incremental step with newPoints and Appends it to mPointsAction
        void AppendPointsStep(const Vector<Vec2F>& newPoints);

        // Converts world space point to local space
        Vec2F WorldToLocal(const Vec2F& point) const;

        // Converts local space point to world space
        Vec2F LocalToWorld(const Vec2F& point) const;

        // Draw selection sprite
        void DrawSelection();

        // Draws transformation frame
        void DrawTransformFrame();

        // Updates transformation frame by selected handles
        void UpdateTransformFrame();

        // Returns is transform frame visible. it will be visible when 2 or more main handles was selected
        bool IsTransformFrameVisible() const;

        // Called when transform frame was transformed
        void OnTransformFrameTransformed(const Basis& basis);

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor double clicked; creates new point
        void OnCursorDblClicked(const Input::Cursor& cursor) override;
        
        REF_COUNTERABLE_IMPL(IEditTool, SelectableDragHandlesGroup);
    };
}
// --- META ---

CLASS_BASES_META(Editor::MeshTopologyTool)
{
    BASE_CLASS(Editor::IEditTool);
    BASE_CLASS(o2::SelectableDragHandlesGroup);
    BASE_CLASS(o2::CursorAreaEventsListener);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::MeshTopologyTool)
{
    FIELD().PUBLIC().DEFAULT_VALUE(mmake<SceneLayer>()).NAME(sceneLayer);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(isEnabled);
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PRIVATE().NAME(mGetPoints);
    FIELD().PRIVATE().NAME(mSetPoint);
    FIELD().PRIVATE().NAME(mAddPoint);
    FIELD().PRIVATE().NAME(mRemovePoint);
    FIELD().PRIVATE().NAME(mGetTransform);
    FIELD().PRIVATE().NAME(mHandleSample);
    FIELD().PRIVATE().NAME(mHandles);
    FIELD().PRIVATE().NAME(mSelectionSprite);
    FIELD().PRIVATE().NAME(mSelectingPressedPoint);
    FIELD().PRIVATE().NAME(mSelectingHandlesBuf);
    FIELD().PRIVATE().NAME(mTransformFrame);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mTransformFrameVisible);
    FIELD().PRIVATE().NAME(mTransformFrameBasis);
    FIELD().PRIVATE().DEFAULT_VALUE(Vec2F(10, 10)).NAME(mTransformBasisOffet);
    FIELD().PRIVATE().NAME(mPointsAction);
}
END_META;
CLASS_METHODS_META(Editor::MeshTopologyTool)
{

    typedef const Function<void(int, Vec2F)>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const MeshTopologyTool&);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const Function<Vector<Vec2F>()>&, _tmp1, const Function<Basis()>&, const Function<void(Vec2F)>&, const Function<void(int)>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Reset);
    FUNCTION().PUBLIC().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, OnEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDisabled);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<MeshTopologyTool>&);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, ClearHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, OnHandleMoved, int, const Vec2F&);
    FUNCTION().PRIVATE().SIGNATURE(void, BeginPointsAction);
    FUNCTION().PRIVATE().SIGNATURE(void, EndPointsAction);
    FUNCTION().PRIVATE().SIGNATURE(void, AppendPointsStep, const Vector<Vec2F>&);
    FUNCTION().PRIVATE().SIGNATURE(Vec2F, WorldToLocal, const Vec2F&);
    FUNCTION().PRIVATE().SIGNATURE(Vec2F, LocalToWorld, const Vec2F&);
    FUNCTION().PRIVATE().SIGNATURE(void, DrawSelection);
    FUNCTION().PRIVATE().SIGNATURE(void, DrawTransformFrame);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateTransformFrame);
    FUNCTION().PRIVATE().SIGNATURE(bool, IsTransformFrameVisible);
    FUNCTION().PRIVATE().SIGNATURE(void, OnTransformFrameTransformed, const Basis&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
}
END_META;
// --- END META ---
