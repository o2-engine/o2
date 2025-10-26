#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"

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
                         
        Vec2F mLastSceneHandlesPos; // Last scene handles position 
        Vec2F mSnapPosition;        // Snapping handles position
        float mHandlesAngle = 0.0f; // Handles angle, in radians
                         
        Vector<Basis>       mBeforeTransforms; // Before transformation transforms
        Ref<TransformAction> mTransformAction; // Current transform action. Creates when transform started

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

        // Called when some handle was pressed, stores before transformations
        void HandlePressed();

        // Called when handle was released, completes transformation action
        void HandleReleased();

        // Handles moved
        void HandlesMoved(const Vec2F& delta, bool snapHor = false, bool spanVer = false);

        // Updates handles position
        void UpdateHandlesPosition();

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key stay down during frame
        void OnKeyStayDown(const Input::Key& key) override;

        // Called when key was pressed
        void OnKeyReleased(const Input::Key& key) override;

        // Moves selected objects on delta
        void MoveSelectedObjects(const Vec2F& delta);

        // Moves selected objects on delta
        void MoveSelectedObjectsWithAction(const Vec2F& delta);
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
    FIELD().PROTECTED().NAME(mLastSceneHandlesPos);
    FIELD().PROTECTED().NAME(mSnapPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mHandlesAngle);
    FIELD().PROTECTED().NAME(mBeforeTransforms);
    FIELD().PROTECTED().NAME(mTransformAction);
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
    FUNCTION().PROTECTED().SIGNATURE(void, HandlePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, HandleReleased);
    FUNCTION().PROTECTED().SIGNATURE(void, HandlesMoved, const Vec2F&, bool, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesPosition);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveSelectedObjects, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveSelectedObjectsWithAction, const Vec2F&);
}
END_META;
// --- END META ---
