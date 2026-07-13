#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    FORWARD_CLASS_REF(TransformAction); 

    // ------------------------
    // Move objects editor tool
    // ------------------------
    class MoveTool: public ITransformTool
    {
    public:
        float snapStep = 10.0f; // Moving snap step

    public:
        // Default constructor
        MoveTool();

        // Destructor
        ~MoveTool();

        IOBJECT(MoveTool);

    protected:
        Ref<SceneDragHandle> mHorDragHandle;  // Horizontal arrow handle
        Ref<SceneDragHandle> mVerDragHandle;  // Vertical arrow handle
        Ref<SceneDragHandle> mBothDragHandle; // Both arrow handle

        Ref<SceneDragHandle3D> mXDragHandle3D; // World X axis arrow handle, 3D view mode only
        Ref<SceneDragHandle3D> mYDragHandle3D; // World Y axis arrow handle, 3D view mode only
        Ref<SceneDragHandle3D> mZDragHandle3D; // World Z axis arrow handle, 3D view mode only

        Ref<SceneDragHandle3D> mXYPlaneHandle3D; // XY plane quad handle, 3D view mode only
        Ref<SceneDragHandle3D> mXZPlaneHandle3D; // XZ plane quad handle, 3D view mode only
        Ref<SceneDragHandle3D> mYZPlaneHandle3D; // YZ plane quad handle, 3D view mode only

        Vec2F mLastSceneHandlesPos; // Last scene handles position
        Vec2F mSnapPosition;        // Snapping handles position
        float mHandlesAngle = 0.0f; // Handles angle, in radians

        bool  mToolEnabled = false;    // Is tool currently selected
        Vec3F mDragAnchor3D;           // World anchor of the axis line at 3D axis drag start
        int   mDragAxis3D = 2;         // Axis of the current 3D drag: 0 - X, 1 - Y, 2 - Z
        Vec3F mDragAxisDir3D;          // World direction of the dragged frame axis, captured at press
        Vec3F mDragPlaneNormal3D;      // World normal of the dragged frame plane, captured at press
        float mLastAxisParam3D = 0.0f; // Last axis line parameter under cursor while dragging a 3D axis handle
        Vec3F mLastPlanePoint3D;       // Last plane raycast point under cursor while dragging a 3D plane handle

        Vector<Basis>        mBeforeTransforms;       // Before transformation transforms
        Ref<TransformAction> mTransformAction;        // Current drag transform action. Created on press, committed on release
        Ref<TransformAction> mKeyboardAction;         // Current keyboard nudge action. Lives while any arrow key is held
        int                  mPressedArrowsCount = 0; // Number of arrow keys currently held; transitions 0<->1 open/close mKeyboardAction

    protected:
        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Returns shortcut keys for toggle
        ShortcutKeys GetShortcut() const override;

        // Updates tool
        void Update(float dt) override;

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        // Called when scene objects was changed
        void OnSceneChanged(const Vector<Ref<SceneEditableObject>>& changedObjects) override;

        // Called when objects selection was changed
        void OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects) override;

        // Called when horizontal drag handle was moved
        void OnHorDragHandleMoved(const Vec2F& position);

        // Called when horizontal drag handle was moved
        void OnVerDragHandleMoved(const Vec2F& position);

        // Called when horizontal drag handle was moved
        void OnBothDragHandleMoved(const Vec2F& position);

        // Draws tool scene pass: volumetric handles in 3D mode
        void DrawScene() override;

        // Returns 3D axis handle by axis index: 0 - X, 1 - Y, 2 - Z
        const Ref<SceneDragHandle3D>& GetAxisHandle3D(int axis) const;

        // Returns 3D plane handle by plane normal axis: 0 - YZ, 1 - XZ, 2 - XY
        const Ref<SceneDragHandle3D>& GetPlaneHandle3D(int normalAxis) const;

        // Called when 3D axis handle was pressed, stores the axis line and initial parameter under cursor
        void Axis3DHandlePressed(int axis);

        // Called when 3D axis handle was moved, maps cursor to the axis line through selection
        void OnAxis3DHandleMoved(int axis);

        // Called when 3D plane handle was pressed, stores the plane and initial raycast point under cursor
        void PlaneHandle3DPressed(int normalAxis);

        // Called when 3D plane handle was moved, maps cursor to the world plane through selection
        void OnPlaneHandle3DMoved(int normalAxis);

        // Called when some handle was pressed, stores before transformations
        void HandlePressed();

        // Called when handle was released, completes transformation action
        void HandleReleased();

        // Handles moved
        void HandlesMoved(const Vec2F& delta, bool snapHor = false, bool spanVer = false);

        // Enables 2D handles in 2D mode and 3D axis handles in 3D mode
        void UpdateHandlesEnabledState();

        // Updates handles position
        void UpdateHandlesPosition();

        // Places 3D axis handles at the selection world center, aligned with projected world axes
        void UpdateAxis3DHandles();

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key stay down during frame
        void OnKeyStayDown(const Input::Key& key) override;

        // Called when key was pressed
        void OnKeyReleased(const Input::Key& key) override;

        // Builds a delta step (before = current scene, done = before + delta) and Appends it to action;
        // Append's TryMerge + Redo applies the move — scene is mutated only inside the action
        void AppendMoveStep(const Ref<TransformAction>& action, const Vec2F& delta);

        // Same as AppendMoveStep, but moves selection by a world 3D delta; actors also get the full
        // local position for restoring under 3D-rotated parents
        void AppendMoveStep3D(const Ref<TransformAction>& action, const Vec3F& delta);

        // Opens mKeyboardAction on the first held arrow
        void BeginKeyboardAction();

        // Routes a keyboard nudge through mKeyboardAction and refreshes handle positions
        void AppendKeyboardStep(const Vec2F& delta);

        // Routes a keyboard z nudge through mKeyboardAction, 3D view mode only
        void AppendKeyboardZStep(float deltaZ);

        // Closes and commits mKeyboardAction when the last arrow is released
        void EndKeyboardAction();
    };
}
// --- META ---

CLASS_BASES_META(Editor::MoveTool)
{
    BASE_CLASS(Editor::ITransformTool);
}
END_META;
CLASS_FIELDS_META(Editor::MoveTool)
{
    FIELD().PUBLIC().DEFAULT_VALUE(10.0f).NAME(snapStep);
    FIELD().PROTECTED().NAME(mHorDragHandle);
    FIELD().PROTECTED().NAME(mVerDragHandle);
    FIELD().PROTECTED().NAME(mBothDragHandle);
    FIELD().PROTECTED().NAME(mXDragHandle3D);
    FIELD().PROTECTED().NAME(mYDragHandle3D);
    FIELD().PROTECTED().NAME(mZDragHandle3D);
    FIELD().PROTECTED().NAME(mXYPlaneHandle3D);
    FIELD().PROTECTED().NAME(mXZPlaneHandle3D);
    FIELD().PROTECTED().NAME(mYZPlaneHandle3D);
    FIELD().PROTECTED().NAME(mLastSceneHandlesPos);
    FIELD().PROTECTED().NAME(mSnapPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mHandlesAngle);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mToolEnabled);
    FIELD().PROTECTED().NAME(mDragAnchor3D);
    FIELD().PROTECTED().DEFAULT_VALUE(2).NAME(mDragAxis3D);
    FIELD().PROTECTED().NAME(mDragAxisDir3D);
    FIELD().PROTECTED().NAME(mDragPlaneNormal3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mLastAxisParam3D);
    FIELD().PROTECTED().NAME(mLastPlanePoint3D);
    FIELD().PROTECTED().NAME(mBeforeTransforms);
    FIELD().PROTECTED().NAME(mTransformAction);
    FIELD().PROTECTED().NAME(mKeyboardAction);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPressedArrowsCount);
}
END_META;
CLASS_METHODS_META(Editor::MoveTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PROTECTED().SIGNATURE(ShortcutKeys, GetShortcut);
    FUNCTION().PROTECTED().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSceneChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectsSelectionChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHorDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnVerDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBothDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScene);
    FUNCTION().PROTECTED().SIGNATURE(const Ref<SceneDragHandle3D>&, GetAxisHandle3D, int);
    FUNCTION().PROTECTED().SIGNATURE(const Ref<SceneDragHandle3D>&, GetPlaneHandle3D, int);
    FUNCTION().PROTECTED().SIGNATURE(void, Axis3DHandlePressed, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAxis3DHandleMoved, int);
    FUNCTION().PROTECTED().SIGNATURE(void, PlaneHandle3DPressed, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlaneHandle3DMoved, int);
    FUNCTION().PROTECTED().SIGNATURE(void, HandlePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, HandleReleased);
    FUNCTION().PROTECTED().SIGNATURE(void, HandlesMoved, const Vec2F&, bool, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesEnabledState);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesPosition);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateAxis3DHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendMoveStep, const Ref<TransformAction>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendMoveStep3D, const Ref<TransformAction>&, const Vec3F&);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginKeyboardAction);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendKeyboardStep, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendKeyboardZStep, float);
    FUNCTION().PROTECTED().SIGNATURE(void, EndKeyboardAction);
}
END_META;
// --- END META ---
