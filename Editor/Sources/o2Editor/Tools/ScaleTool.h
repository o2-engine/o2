#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"

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

        float mHandlesAngle = 0.0f;              // Handles angle in radians
        Vec2F mSceneHandlesPos;                  // Scene space handles position
        Vec2F mHandlesSize = Vec2F(100, 100); // Handles size in screen space

        Vec2F mLastHorHandlePos;  // Last horizontal handle position
        Vec2F mLastVerHandlePos;  // Last vertical handle position
        Vec2F mLastBothHandlePos; // Last both axis handle position

        Ref<TransformAction> mTransformAction; // Current transform action. Creates when transform started

    protected:
        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Returns shortcut keys for toggle
        ShortcutKeys GetShortcut() const override;

        // Updates tool
        void Update(float dt) override;

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
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mHandlesAngle);
    FIELD().PROTECTED().NAME(mSceneHandlesPos);
    FIELD().PROTECTED().DEFAULT_VALUE(Vec2F(100, 100)).NAME(mHandlesSize);
    FIELD().PROTECTED().NAME(mLastHorHandlePos);
    FIELD().PROTECTED().NAME(mLastVerHandlePos);
    FIELD().PROTECTED().NAME(mLastBothHandlePos);
    FIELD().PROTECTED().NAME(mTransformAction);
}
END_META;
CLASS_METHODS_META(Editor::ScaleTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PROTECTED().SIGNATURE(ShortcutKeys, GetShortcut);
    FUNCTION().PROTECTED().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScreen);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSceneChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectsSelectionChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHorDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnVerDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBothDragHandleMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesPosition);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesAngleAndPositions, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesPositions);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, ScaleSelectedObjects, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendScaleStep, const Ref<TransformAction>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, HandlePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, HandleReleased);
}
END_META;
// --- END META ---
