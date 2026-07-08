#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"

namespace Editor
{
    FORWARD_CLASS_REF(TransformAction);

    // -------------------------
    // Scale objects editor tool
    // -------------------------
    class ScaleTool: public ITransformTool
    {
    public:
        float bothScaleSence = 0.01f;

    public:
        // Default constructor
        ScaleTool();

        // Destructor
        ~ScaleTool();

        IOBJECT(ScaleTool);

    protected:
        Ref<SceneDragHandle> mHorDragHandle;  // Horizontal scale drag handle
        Ref<SceneDragHandle> mVerDragHandle;  // Vertical scale drag handle
        Ref<SceneDragHandle> mBothDragHandle; // Bot axis scale drag handle

        Ref<SceneDragHandle3D> mXDragHandle3D; // Local X axis scale handle, 3D view mode only
        Ref<SceneDragHandle3D> mYDragHandle3D; // Local Y axis scale handle, 3D view mode only
        Ref<SceneDragHandle3D> mZDragHandle3D; // Local Z axis scale handle, 3D view mode only

        Ref<SceneDragHandle3D> mUniformHandle3D; // Center cube uniform scale handle, 3D view mode only

        Ref<SceneDragHandle3D> mXYPlaneHandle3D; // Local XY plane scale handle, 3D view mode only
        Ref<SceneDragHandle3D> mXZPlaneHandle3D; // Local XZ plane scale handle, 3D view mode only
        Ref<SceneDragHandle3D> mYZPlaneHandle3D; // Local YZ plane scale handle, 3D view mode only

        float mHandlesAngle = 0.0f;              // Handles angle in radians
        Vec2F mSceneHandlesPos;                  // Scene space handles position
        Vec2F mHandlesSize = Vec2F(100, 100); // Handles size in screen space

        Vec2F mLastHorHandlePos;  // Last horizontal handle position
        Vec2F mLastVerHandlePos;  // Last vertical handle position
        Vec2F mLastBothHandlePos; // Last both axis handle position

        bool  mToolEnabled = false;    // Is tool currently selected
        Vec3F mAnchor3D;               // Selection world center in 3D mode
        Vec3F mDragAxisDir3D;          // World direction of the dragged local axis at 3D drag start
        float mLastAxisParam3D = 1.0f; // Last axis line parameter under cursor while dragging a 3D axis handle

        Vec3F mDragPlaneNormal3D;           // World normal of the dragged plane at 3D plane drag start
        float mLastPlaneDistance3D = 1.0f;  // Last raycast distance from anchor while dragging a 3D plane handle
        float mLastUniformScreenX = 0.0f;   // Last screen cursor x while dragging the uniform scale cube

        Ref<TransformAction> mTransformAction; // Current transform action. Creates when transform started

    protected:
        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Returns shortcut keys for toggle
        ShortcutKeys GetShortcut() const override;

        // Updates tool
        void Update(float dt) override;

        // Draws tool scene pass: volumetric handles in 3D mode
        void DrawScene() override;

        // Draws screen
        void DrawScreen() override;

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

        // Returns 3D axis handle by axis index: 0 - X, 1 - Y, 2 - Z
        const Ref<SceneDragHandle3D>& GetAxisHandle3D(int axis) const;

        // Returns 3D plane handle by plane normal axis: 0 - YZ, 1 - XZ, 2 - XY
        const Ref<SceneDragHandle3D>& GetPlaneHandle3D(int normalAxis) const;

        // Returns world direction of the local scale axis: last selected actor rotation applied to the world axis
        Vec3F GetScaleAxisDirection3D(int axis) const;

        // Returns rotation of the scale handles frame: last selected actor rotation, identity with Ctrl
        Quat GetScaleFrameRotation3D() const;

        // Enables 2D handles in 2D mode and 3D axis handles in 3D mode
        void UpdateHandlesEnabledState();

        // Places 3D axis handles around the selection world center along projected local axes
        void UpdateHandles3D();

        // Called when 3D axis handle was pressed, stores the axis line and initial parameter under cursor
        void Axis3DHandlePressed(int axis);

        // Called when 3D axis handle was moved, maps cursor to relative parameter change along the axis
        void OnAxis3DHandleMoved(int axis);

        // Called when 3D plane handle was pressed, stores the plane and initial raycast distance from anchor
        void PlaneHandle3DPressed(int normalAxis);

        // Called when 3D plane handle was moved, scales both in-plane local axes by distance ratio
        void OnPlaneHandle3DMoved(int normalAxis);

        // Called when the uniform scale cube was pressed, stores initial screen cursor x
        void UniformHandle3DPressed();

        // Called when the uniform scale cube was moved, scales all axes by screen x delta
        void OnUniformHandle3DMoved();

        // Updates handles position
        void UpdateHandlesPosition();

        // Updates handles angle and position
        void UpdateHandlesAngleAndPositions(float angle);

        // Updates handles position
        void UpdateHandlesPositions();

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key stay down during frame
        void OnKeyStayDown(const Input::Key& key) override;

        // Called when key was pressed
        void OnKeyReleased(const Input::Key& key) override;

        // Scales selected objects
        void ScaleSelectedObjects(const Vec2F& scale);

        // Builds a scale step and Appends it to action
        void AppendScaleStep(const Ref<TransformAction>& action, const Vec2F& scale);

        // Builds a per-object local scale step: x/y through the basis axes, z through scaleZ
        void AppendScaleStep3D(const Ref<TransformAction>& action, const Vec3F& scale);

        // Called when some handle was pressed, stores before transformations
        void HandlePressed();

        // Called when handle was released, completes transformation action
        void HandleReleased();
    };
}
// --- META ---

CLASS_BASES_META(Editor::ScaleTool)
{
    BASE_CLASS(Editor::ITransformTool);
}
END_META;
CLASS_FIELDS_META(Editor::ScaleTool)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0.01f).NAME(bothScaleSence);
    FIELD().PROTECTED().NAME(mHorDragHandle);
    FIELD().PROTECTED().NAME(mVerDragHandle);
    FIELD().PROTECTED().NAME(mBothDragHandle);
    FIELD().PROTECTED().NAME(mXDragHandle3D);
    FIELD().PROTECTED().NAME(mYDragHandle3D);
    FIELD().PROTECTED().NAME(mZDragHandle3D);
    FIELD().PROTECTED().NAME(mUniformHandle3D);
    FIELD().PROTECTED().NAME(mXYPlaneHandle3D);
    FIELD().PROTECTED().NAME(mXZPlaneHandle3D);
    FIELD().PROTECTED().NAME(mYZPlaneHandle3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mHandlesAngle);
    FIELD().PROTECTED().NAME(mSceneHandlesPos);
    FIELD().PROTECTED().DEFAULT_VALUE(Vec2F(100, 100)).NAME(mHandlesSize);
    FIELD().PROTECTED().NAME(mLastHorHandlePos);
    FIELD().PROTECTED().NAME(mLastVerHandlePos);
    FIELD().PROTECTED().NAME(mLastBothHandlePos);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mToolEnabled);
    FIELD().PROTECTED().NAME(mAnchor3D);
    FIELD().PROTECTED().NAME(mDragAxisDir3D);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mLastAxisParam3D);
    FIELD().PROTECTED().NAME(mDragPlaneNormal3D);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mLastPlaneDistance3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mLastUniformScreenX);
    FIELD().PROTECTED().NAME(mTransformAction);
}
END_META;
CLASS_METHODS_META(Editor::ScaleTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PROTECTED().SIGNATURE(ShortcutKeys, GetShortcut);
    FUNCTION().PROTECTED().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScene);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScreen);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSceneChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectsSelectionChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHorDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnVerDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBothDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(const Ref<SceneDragHandle3D>&, GetAxisHandle3D, int);
    FUNCTION().PROTECTED().SIGNATURE(const Ref<SceneDragHandle3D>&, GetPlaneHandle3D, int);
    FUNCTION().PROTECTED().SIGNATURE(Vec3F, GetScaleAxisDirection3D, int);
    FUNCTION().PROTECTED().SIGNATURE(Quat, GetScaleFrameRotation3D);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesEnabledState);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandles3D);
    FUNCTION().PROTECTED().SIGNATURE(void, Axis3DHandlePressed, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAxis3DHandleMoved, int);
    FUNCTION().PROTECTED().SIGNATURE(void, PlaneHandle3DPressed, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlaneHandle3DMoved, int);
    FUNCTION().PROTECTED().SIGNATURE(void, UniformHandle3DPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUniformHandle3DMoved);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesPosition);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesAngleAndPositions, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesPositions);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, ScaleSelectedObjects, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendScaleStep, const Ref<TransformAction>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendScaleStep3D, const Ref<TransformAction>&, const Vec3F&);
    FUNCTION().PROTECTED().SIGNATURE(void, HandlePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, HandleReleased);
}
END_META;
// --- END META ---
