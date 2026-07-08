#include "o2Editor/stdafx.h"
#include "MoveTool.h"

#include "o2/Utils/Math/Geometry.h"

#include "o2/Render/Mesh3DFill.h"
#include "o2/Render/Sprite.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2Editor/Actions/Transform.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"

namespace Editor
{
    MoveTool::MoveTool()
    {
        mHorDragHandle = mmake<SceneDragHandle>(mmake<Sprite>("ui/UI2_right_move_arrow.png"),
                                         mmake<Sprite>("ui/UI2_right_move_arrow_select.png"),
                                         mmake<Sprite>("ui/UI2_right_move_arrow_pressed.png"));

        mVerDragHandle = mmake<SceneDragHandle>(mmake<Sprite>("ui/UI2_up_move_arrow.png"),
                                         mmake<Sprite>("ui/UI2_up_move_arrow_select.png"),
                                         mmake<Sprite>("ui/UI2_up_move_arrow_pressed.png"));

        mBothDragHandle = mmake<SceneDragHandle>(mmake<Sprite>("ui/UI2_move_tool_center.png"),
                                          mmake<Sprite>("ui/UI2_move_tool_center_select.png"),
                                          mmake<Sprite>("ui/UI2_move_tool_center_pressed.png"));

        mHorDragHandle->enabled = false;
        mVerDragHandle->enabled = false;
        mBothDragHandle->enabled = false;

        mHorDragHandle->onChangedPos = THIS_FUNC(OnHorDragHandleMoved);
        mVerDragHandle->onChangedPos = THIS_FUNC(OnVerDragHandleMoved);
        mBothDragHandle->onChangedPos = THIS_FUNC(OnBothDragHandleMoved);

        mHorDragHandle->onPressed = THIS_FUNC(HandlePressed);
        mVerDragHandle->onPressed = THIS_FUNC(HandlePressed);
        mBothDragHandle->onPressed = THIS_FUNC(HandlePressed);

        mHorDragHandle->onReleased = THIS_FUNC(HandleReleased);
        mVerDragHandle->onReleased = THIS_FUNC(HandleReleased);
        mBothDragHandle->onReleased = THIS_FUNC(HandleReleased);

        mHorDragHandle->GetRegularDrawable()->SetSizePivot(Vec2F(1, 5));
        mHorDragHandle->GetHoverDrawable()->SetSizePivot(Vec2F(1, 5));
        mHorDragHandle->GetPressedDrawable()->SetSizePivot(Vec2F(1, 5));

        mVerDragHandle->GetRegularDrawable()->SetSizePivot(Vec2F(5, 1));
        mVerDragHandle->GetHoverDrawable()->SetSizePivot(Vec2F(5, 1));
        mVerDragHandle->GetPressedDrawable()->SetSizePivot(Vec2F(5, 1));

        mBothDragHandle->GetRegularDrawable()->SetSizePivot(Vec2F(1, 1));
        mBothDragHandle->GetHoverDrawable()->SetSizePivot(Vec2F(1, 1));
        mBothDragHandle->GetPressedDrawable()->SetSizePivot(Vec2F(1, 1));

        for (int axis = 0; axis < 3; axis++)
        {
            auto handle = mmake<SceneDragHandle3D>();
            handle->SetGeometry(Mesh3DPrimitives::BuildArrowGeometry(1.0f, 0.01f, 0.18f, 0.045f, false));
            handle->SetColor(SceneDragHandle3D::GetAxisColor(axis));
            handle->SetScreenSizeFactor(0.25f);
            handle->SetPickPadding(0.05f);

            // Analytic picking: shaft cylinder and the cone head as a slightly wider cylinder segment
            handle->AddPickCylinder(Vec3F(), Vec3F(0.0f, 0.82f, 0.0f), 0.01f);
            handle->AddPickCylinder(Vec3F(0.0f, 0.82f, 0.0f), Vec3F(0.0f, 1.0f, 0.0f), 0.045f);
            handle->enabled = false;

            handle->onPressed = [this, axis]() { Axis3DHandlePressed(axis); };
            handle->onChangedPos = [this, axis](const Vec2F&) { OnAxis3DHandleMoved(axis); };
            handle->onReleased = THIS_FUNC(HandleReleased);

            (axis == 0 ? mXDragHandle3D : axis == 1 ? mYDragHandle3D : mZDragHandle3D) = handle;
        }

        for (int normalAxis = 0; normalAxis < 3; normalAxis++)
        {
            auto handle = mmake<SceneDragHandle3D>();
            handle->SetGeometry(Mesh3DPrimitives::BuildPlaneHandleGeometry(normalAxis, 0.06f, 0.28f,
                                                                       Mesh3DPrimitives::BakedLightDirection()));

            Color4 color = SceneDragHandle3D::GetAxisColor(normalAxis);
            Color4 regular = color; regular.a = 100;
            Color4 hover = color; hover.a = 180;
            handle->SetColors(regular, hover, Color4(255, 220, 80, 160));
            handle->SetScreenSizeFactor(0.25f);
            handle->SetPickPadding(0.02f);

            // Analytic picking: the actual quad, not its inflated bounds
            Vec3F u, v;
            Geometry::AxisPlaneBasis(normalAxis, u, v);
            handle->AddPickQuad((u + v)*0.06f, u*0.28f, v*0.28f);
            handle->enabled = false;

            handle->onPressed = [this, normalAxis]() { PlaneHandle3DPressed(normalAxis); };
            handle->onChangedPos = [this, normalAxis](const Vec2F&) { OnPlaneHandle3DMoved(normalAxis); };
            handle->onReleased = THIS_FUNC(HandleReleased);

            (normalAxis == 0 ? mYZPlaneHandle3D : normalAxis == 1 ? mXZPlaneHandle3D : mXYPlaneHandle3D) = handle;
        }
    }

    MoveTool::~MoveTool()
    {}

    const Ref<SceneDragHandle3D>& MoveTool::GetAxisHandle3D(int axis) const
    {
        return axis == 0 ? mXDragHandle3D : axis == 1 ? mYDragHandle3D : mZDragHandle3D;
    }

    const Ref<SceneDragHandle3D>& MoveTool::GetPlaneHandle3D(int normalAxis) const
    {
        return normalAxis == 0 ? mYZPlaneHandle3D : normalAxis == 1 ? mXZPlaneHandle3D : mXYPlaneHandle3D;
    }

    void MoveTool::DrawScene()
    {
        ITransformTool::DrawScene();

        if (!SceneEditScreen::IsSingletonInitialzed() || !o2EditorSceneScreen.IsView3DMode())
            return;

        for (int axis = 0; axis < 3; axis++)
        {
            GetAxisHandle3D(axis)->DrawGeometry();
            GetPlaneHandle3D(axis)->DrawGeometry();
        }
    }

    void MoveTool::Update(float dt)
    {
        UpdateHandlesEnabledState();
    }

    void MoveTool::UpdateHandlesEnabledState()
    {
        bool is3DMode = SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode();

        bool enable2D = mToolEnabled && !is3DMode;
        bool enable3D = mToolEnabled && is3DMode && !o2EditorSceneScreen.GetSelectedObjects().IsEmpty();

        if (mHorDragHandle->IsEnabled() != enable2D)
        {
            mHorDragHandle->enabled = enable2D;
            mVerDragHandle->enabled = enable2D;
            mBothDragHandle->enabled = enable2D;

            if (enable2D)
                UpdateHandlesPosition();
        }

        if (enable3D)
        {
            // Placement depends on the camera, refresh every frame
            UpdateAxis3DHandles();
        }
        else
        {
            for (int axis = 0; axis < 3; axis++)
            {
                GetAxisHandle3D(axis)->enabled = false;
                GetPlaneHandle3D(axis)->enabled = false;
            }
        }
    }

    void MoveTool::OnEnabled()
    {
        mToolEnabled = true;
        UpdateHandlesEnabledState();
        UpdateHandlesPosition();
    }

    void MoveTool::OnDisabled()
    {
        mToolEnabled = false;
        UpdateHandlesEnabledState();
    }

    void MoveTool::OnSceneChanged(const Vector<Ref<SceneEditableObject>>& changedObjects)
    {
        UpdateHandlesPosition();
    }

    void MoveTool::OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {
        UpdateHandlesPosition();
    }

    void MoveTool::OnHorDragHandleMoved(const Vec2F& position)
    {
        Vec2F axis = Vec2F::Rotated(mHandlesAngle);
        Vec2F delta = position - mLastSceneHandlesPos;
        Vec2F axisDelta = delta.Project(axis);

        HandlesMoved(axisDelta, o2Input.IsKeyDown(VK_SHIFT), false);
    }

    void MoveTool::OnVerDragHandleMoved(const Vec2F& position)
    {
        Vec2F axis = Vec2F::Rotated(mHandlesAngle + Math::PI()*0.5f);
        Vec2F delta = position - mLastSceneHandlesPos;
        Vec2F axisDelta = delta.Project(axis);

        HandlesMoved(axisDelta, false, o2Input.IsKeyDown(VK_SHIFT));
    }

    void MoveTool::OnBothDragHandleMoved(const Vec2F& position)
    {
        bool snap = o2Input.IsKeyDown(VK_SHIFT);
        HandlesMoved(position - mLastSceneHandlesPos, snap, snap);
    }

    void MoveTool::Axis3DHandlePressed(int axis)
    {
        HandlePressed();

        mDragAxis3D = axis;
        mDragAnchor3D = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());

        // World direction of the frame axis is captured at press and stays fixed for the drag
        mDragAxisDir3D = ITransformTool::GetSelectionFrameRotation3D(o2EditorSceneScreen.GetSelectedObjects())*
            Vec3F::Axis(axis);

        o2EditorSceneScreen.ScreenToWorldAxisParam(o2Input.GetCursorPos(), mDragAnchor3D,
                                                   mDragAxisDir3D, mLastAxisParam3D);
    }

    void MoveTool::OnAxis3DHandleMoved(int axis)
    {
        float param;
        if (!o2EditorSceneScreen.ScreenToWorldAxisParam(o2Input.GetCursorPos(), mDragAnchor3D, mDragAxisDir3D, param))
            return;

        float delta = param - mLastAxisParam3D;
        mLastAxisParam3D = param;

        if (Math::Abs(delta) > FLT_EPSILON)
            AppendMoveStep3D(mTransformAction, mDragAxisDir3D*delta);

        UpdateHandlesPosition();
    }

    void MoveTool::PlaneHandle3DPressed(int normalAxis)
    {
        HandlePressed();

        mDragAnchor3D = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());
        mDragPlaneNormal3D = ITransformTool::GetSelectionFrameRotation3D(o2EditorSceneScreen.GetSelectedObjects())*
            Vec3F::Axis(normalAxis);

        if (!o2EditorSceneScreen.ScreenToWorldPlanePoint(o2Input.GetCursorPos(), mDragAnchor3D,
                                                         mDragPlaneNormal3D, mLastPlanePoint3D))
        {
            mLastPlanePoint3D = mDragAnchor3D;
        }
    }

    void MoveTool::OnPlaneHandle3DMoved(int normalAxis)
    {
        Vec3F hit;
        if (!o2EditorSceneScreen.ScreenToWorldPlanePoint(o2Input.GetCursorPos(), mDragAnchor3D,
                                                         mDragPlaneNormal3D, hit))
        {
            return;
        }

        Vec3F delta = hit - mLastPlanePoint3D;
        mLastPlanePoint3D = hit;

        if (delta.Length() > FLT_EPSILON)
            AppendMoveStep3D(mTransformAction, delta);

        UpdateHandlesPosition();
    }

    void MoveTool::HandlePressed()
    {
        mBeforeTransforms = o2EditorSceneScreen.GetTopSelectedObjects().Convert<Basis>(
            [](auto& x) { return x->GetTransform(); });

        mTransformAction = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());

        onTransformBegin();
    }

    void MoveTool::HandleReleased()
    {
        mTransformAction->Completed();
        o2EditorSceneWindow.DoneAction(mTransformAction);
        mTransformAction = nullptr;

        onTransformEnd();
    }

    void MoveTool::HandlesMoved(const Vec2F& delta, bool snapHor /*= false*/, bool spanVer /*= false*/)
    {
        if (spanVer || snapHor)
        {
            mSnapPosition = mLastSceneHandlesPos + delta;
            Vec2F roundedSnap(snapHor ? Math::Round(mSnapPosition.x / snapStep)*snapStep : mSnapPosition.x,
                              spanVer ? Math::Round(mSnapPosition.y / snapStep)*snapStep : mSnapPosition.y);

            Vec2F roundDelta = roundedSnap - mLastSceneHandlesPos;
            if (roundDelta.Length() > FLT_EPSILON)
                AppendMoveStep(mTransformAction, roundDelta);
        }
        else
        {
            AppendMoveStep(mTransformAction, delta);
        }

        UpdateHandlesPosition();
    }

    void MoveTool::UpdateHandlesPosition()
    {
        auto selectedObjects = o2EditorSceneScreen.GetSelectedObjects();
        mLastSceneHandlesPos =
            selectedObjects.Sum<Vec2F>([](auto x) { return x->GetPivot(); }) /
            (float)selectedObjects.Count();

        mVerDragHandle->position = mLastSceneHandlesPos;
        mHorDragHandle->position = mLastSceneHandlesPos;
        mBothDragHandle->position = mLastSceneHandlesPos;

        if (selectedObjects.Count() > 0 && !o2Input.IsKeyDown(VK_CONTROL))
        {
            auto lastSelectedObject = selectedObjects.Last();
            mHandlesAngle = -lastSelectedObject->GetTransform().xv.Normalized().Angle(Vec2F::Right());

            mVerDragHandle->angle = mHandlesAngle;
            mHorDragHandle->angle = mHandlesAngle;
            mBothDragHandle->angle = mHandlesAngle;
        }
        else
        {
            mHandlesAngle = 0.0f;
            mVerDragHandle->angle = mHandlesAngle;
            mHorDragHandle->angle = mHandlesAngle;
            mBothDragHandle->angle = mHandlesAngle;
        }

        UpdateAxis3DHandles();
    }

    void MoveTool::UpdateAxis3DHandles()
    {
        if (!SceneEditScreen::IsSingletonInitialzed() || !o2EditorSceneScreen.IsView3DMode() ||
            o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
        {
            return;
        }

        Vec3F anchor = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());
        Quat frameRotation = ITransformTool::GetSelectionFrameRotation3D(o2EditorSceneScreen.GetSelectedObjects());

        for (int axis = 0; axis < 3; axis++)
        {
            auto& handle = GetAxisHandle3D(axis);
            handle->SetPose(anchor, frameRotation*Quat::FromToRotation(Vec3F::YAxis(), Vec3F::Axis(axis)));
            handle->enabled = mToolEnabled;

            auto& planeHandle = GetPlaneHandle3D(axis);
            planeHandle->SetPose(anchor, frameRotation);
            planeHandle->enabled = mToolEnabled;
        }
    }

    void MoveTool::OnKeyPressed(const Input::Key& key)
    {
        if (SceneWindow::IsSingletonInitialzed() && !o2EditorSceneWindow.IsFocused())
            return;

        float delta = o2Input.IsKeyDown(VK_SHIFT) ? snapStep : 1.0f;

        bool is3DMode = o2EditorSceneScreen.IsView3DMode();
        bool isArrow = key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN ||
            (is3DMode && (key == VK_PRIOR || key == VK_NEXT));

        if (isArrow)
        {
            BeginKeyboardAction();

            if (key == VK_LEFT)  AppendKeyboardStep(Vec2F::Left()*delta);
            if (key == VK_RIGHT) AppendKeyboardStep(Vec2F::Right()*delta);
            if (key == VK_UP)    AppendKeyboardStep(Vec2F::Up()*delta);
            if (key == VK_DOWN)  AppendKeyboardStep(Vec2F::Down()*delta);
            if (key == VK_PRIOR) AppendKeyboardZStep(delta);
            if (key == VK_NEXT)  AppendKeyboardZStep(-delta);
        }

        if (key == VK_CONTROL)
        {
            mHandlesAngle = 0.0f;
            mVerDragHandle->angle = mHandlesAngle;
            mHorDragHandle->angle = mHandlesAngle;
            mBothDragHandle->angle = mHandlesAngle;
        }

        SelectionTool::OnKeyPressed(key);
    }

    void MoveTool::OnKeyStayDown(const Input::Key& key)
    {
        if (SceneWindow::IsSingletonInitialzed() && !o2EditorSceneWindow.IsFocused())
            return;

        float delta = o2Input.IsKeyDown(VK_SHIFT) ? snapStep : 1.0f;

        if (key.pressedTime < 0.3f)
            return;

        if (key == VK_LEFT)  AppendKeyboardStep(Vec2F::Left()*delta);
        if (key == VK_RIGHT) AppendKeyboardStep(Vec2F::Right()*delta);
        if (key == VK_UP)    AppendKeyboardStep(Vec2F::Up()*delta);
        if (key == VK_DOWN)  AppendKeyboardStep(Vec2F::Down()*delta);

        if (mKeyboardAction && o2EditorSceneScreen.IsView3DMode())
        {
            if (key == VK_PRIOR) AppendKeyboardZStep(delta);
            if (key == VK_NEXT)  AppendKeyboardZStep(-delta);
        }
    }

    void MoveTool::OnKeyReleased(const Input::Key& key)
    {
        bool isArrow = key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN ||
            ((key == VK_PRIOR || key == VK_NEXT) && mKeyboardAction);

        if (isArrow)
            EndKeyboardAction();

        if (key == VK_CONTROL)
        {
            auto selectedObjects = o2EditorSceneScreen.GetSelectedObjects();
            if (selectedObjects.Count() > 0)
            {
                auto lastSelectedObject = selectedObjects.Last();
                mHandlesAngle = -lastSelectedObject->GetTransform().xv.Normalized().Angle(Vec2F::Right());

                mVerDragHandle->angle = mHandlesAngle;
                mHorDragHandle->angle = mHandlesAngle;
                mBothDragHandle->angle = mHandlesAngle;
            }
        }
    }

    void MoveTool::AppendMoveStep(const Ref<TransformAction>& action, const Vec2F& delta)
    {
        if (!action)
            return;

        auto step = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        step->doneTransforms = step->beforeTransforms;
        for (auto& t : step->doneTransforms)
            t.transform.origin += delta;

        action->Append(step);
    }

    void MoveTool::AppendMoveStep3D(const Ref<TransformAction>& action, const Vec3F& delta)
    {
        if (!action)
            return;

        auto step = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        step->doneTransforms = step->beforeTransforms;
        for (auto& t : step->doneTransforms)
        {
            t.transform.origin += delta.XY();

            if (t.has3D)
                t.positionZ += delta.z;
        }

        action->Append(step);
    }

    void MoveTool::AppendKeyboardZStep(float deltaZ)
    {
        AppendMoveStep3D(mKeyboardAction, Vec3F(0.0f, 0.0f, deltaZ));
        UpdateHandlesPosition();
    }

    void MoveTool::BeginKeyboardAction()
    {
        if (mPressedArrowsCount == 0)
            mKeyboardAction = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());

        ++mPressedArrowsCount;
    }

    void MoveTool::AppendKeyboardStep(const Vec2F& delta)
    {
        AppendMoveStep(mKeyboardAction, delta);
        UpdateHandlesPosition();
    }

    void MoveTool::EndKeyboardAction()
    {
        if (mPressedArrowsCount > 0)
            --mPressedArrowsCount;

        if (mPressedArrowsCount == 0 && mKeyboardAction)
        {
            mKeyboardAction->Completed();
            o2EditorSceneWindow.DoneAction(mKeyboardAction);
            mKeyboardAction = nullptr;
        }
    }

    String MoveTool::GetPanelIcon() const
    {
        return "ui/UI4_move_tool.png";
    }

    ShortcutKeys MoveTool::GetShortcut() const
    {
        return ShortcutKeys({ VK_W });
    }

}
// --- META ---

DECLARE_CLASS(Editor::MoveTool, Editor__MoveTool);
// --- END META ---
