#include "o2Editor/stdafx.h"
#include "ScaleTool.h"

#include "o2/Utils/Math/Geometry.h"

#include "o2/Render/Render.h"
#include "o2/Render/Mesh3DFill.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
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
    ScaleTool::ScaleTool()
    {
        mHorDragHandle = mmake<SceneDragHandle>(mmake<Sprite>("ui/UI2_right_scale_arrow.png"),
                                                mmake<Sprite>("ui/UI2_right_scale_arrow_select.png"),
                                                mmake<Sprite>("ui/UI2_right_scale_arrow_pressed.png"));

        mVerDragHandle = mmake<SceneDragHandle>(mmake<Sprite>("ui/UI2_up_scale_arrow.png"),
                                                mmake<Sprite>("ui/UI2_up_scale_arrow_select.png"),
                                                mmake<Sprite>("ui/UI2_up_scale_arrow_pressed.png"));

        mBothDragHandle = mmake<SceneDragHandle>(mmake<Sprite>("ui/UI2_scale_both.png"),
                                                 mmake<Sprite>("ui/UI2_scale_both_select.png"),
                                                 mmake<Sprite>("ui/UI2_scale_both_pressed.png"));

        mHorDragHandle->enabled = false;
        mVerDragHandle->enabled = false;
        mBothDragHandle->enabled = false;

        mHorDragHandle->onChangedPos = THIS_FUNC(OnHorDragHandleMoved);
        mVerDragHandle->onChangedPos = THIS_FUNC(OnVerDragHandleMoved);
        mBothDragHandle->onChangedPos = THIS_FUNC(OnBothDragHandleMoved);

        mHorDragHandle->onPressed = THIS_FUNC(HandlePressed);
        mVerDragHandle->onPressed = THIS_FUNC(HandlePressed);
        mBothDragHandle->onPressed = THIS_FUNC(HandlePressed);

        mHorDragHandle->onReleased = THIS_FUNC(UpdateHandlesPosition);
        mVerDragHandle->onReleased = THIS_FUNC(UpdateHandlesPosition);

        mHorDragHandle->onReleased += THIS_FUNC(HandleReleased);
        mVerDragHandle->onReleased += THIS_FUNC(HandleReleased);
        mBothDragHandle->onReleased += THIS_FUNC(HandleReleased);

        for (int axis = 0; axis < 3; axis++)
        {
            auto handle = mmake<SceneDragHandle3D>();
            handle->SetGeometry(Mesh3DPrimitives::BuildArrowGeometry(1.0f, 0.01f, 0.12f, 0.06f, true));
            handle->SetColor(SceneDragHandle3D::GetAxisColor(axis));
            handle->SetScreenSizeFactor(0.2f);
            handle->SetPickPadding(0.05f);

            // Analytic picking: shaft cylinder and the cube head as a box on its own segment
            handle->AddPickCylinder(Vec3F(), Vec3F(0.0f, 0.88f, 0.0f), 0.01f);
            handle->AddPickBox(o2::AABB(Vec3F(-0.06f, 0.88f, -0.06f), Vec3F(0.06f, 1.0f, 0.06f)));
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
            handle->SetScreenSizeFactor(0.2f);
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

        mUniformHandle3D = mmake<SceneDragHandle3D>();
        mUniformHandle3D->SetGeometry(Mesh3DPrimitives::BuildBox(Vec3F(0.15f, 0.15f, 0.15f)));
        mUniformHandle3D->SetColor(Color4(230, 230, 230, 255));
        mUniformHandle3D->SetScreenSizeFactor(0.2f);
        mUniformHandle3D->enabled = false;

        mUniformHandle3D->onPressed = THIS_FUNC(UniformHandle3DPressed);
        mUniformHandle3D->onChangedPos = [this](const Vec2F&) { OnUniformHandle3DMoved(); };
        mUniformHandle3D->onReleased = THIS_FUNC(HandleReleased);
    }

    ScaleTool::~ScaleTool()
    {}

    String ScaleTool::GetPanelIcon() const
    {
        return "ui/UI4_scale_tool.png";
    }

    ShortcutKeys ScaleTool::GetShortcut() const
    {
        return ShortcutKeys({VK_R});
    }

    void ScaleTool::Update(float dt)
    {
        UpdateHandlesEnabledState();
    }

    const Ref<SceneDragHandle3D>& ScaleTool::GetAxisHandle3D(int axis) const
    {
        return axis == 0 ? mXDragHandle3D : axis == 1 ? mYDragHandle3D : mZDragHandle3D;
    }

    const Ref<SceneDragHandle3D>& ScaleTool::GetPlaneHandle3D(int normalAxis) const
    {
        return normalAxis == 0 ? mYZPlaneHandle3D : normalAxis == 1 ? mXZPlaneHandle3D : mXYPlaneHandle3D;
    }

    Quat ScaleTool::GetScaleFrameRotation3D() const
    {
        return ITransformTool::GetSelectionFrameRotation3D(o2EditorSceneScreen.GetSelectedObjects());
    }

    Vec3F ScaleTool::GetScaleAxisDirection3D(int axis) const
    {
        return GetScaleFrameRotation3D()*Vec3F::Axis(axis);
    }

    void ScaleTool::UpdateHandlesEnabledState()
    {
        bool is3DMode = SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode();

        bool enable2D = mToolEnabled && !is3DMode;
        bool enable3D = mToolEnabled && is3DMode && !o2EditorSceneScreen.GetSelectedObjects().IsEmpty();

        if (mHorDragHandle->IsEnabled() != enable2D)
        {
            mHorDragHandle->enabled = enable2D;
            mVerDragHandle->enabled = enable2D;

            if (enable2D)
                UpdateHandlesPosition();
        }

        if (mBothDragHandle->IsEnabled() != enable2D)
            mBothDragHandle->enabled = enable2D;

        if (enable3D)
        {
            // Placement depends on the camera and selection orientation, refresh every frame
            bool anyPressed = mXDragHandle3D->IsPressed() || mYDragHandle3D->IsPressed() ||
                mZDragHandle3D->IsPressed() || mUniformHandle3D->IsPressed() ||
                mXYPlaneHandle3D->IsPressed() || mXZPlaneHandle3D->IsPressed() || mYZPlaneHandle3D->IsPressed();

            if (!anyPressed)
                UpdateHandles3D();
        }
        else
        {
            for (int axis = 0; axis < 3; axis++)
            {
                GetAxisHandle3D(axis)->enabled = false;
                GetPlaneHandle3D(axis)->enabled = false;
            }

            mUniformHandle3D->enabled = false;
        }
    }

    void ScaleTool::UpdateHandles3D()
    {
        mAnchor3D = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());
        mSceneHandlesPos = mAnchor3D.XY();

        Quat frameRotation = GetScaleFrameRotation3D();

        for (int axis = 0; axis < 3; axis++)
        {
            auto& handle = GetAxisHandle3D(axis);
            handle->SetPose(mAnchor3D, frameRotation*Quat::FromToRotation(Vec3F::YAxis(), Vec3F::Axis(axis)));
            handle->enabled = mToolEnabled;

            auto& planeHandle = GetPlaneHandle3D(axis);
            planeHandle->SetPose(mAnchor3D, frameRotation);
            planeHandle->enabled = mToolEnabled;
        }

        mUniformHandle3D->SetPose(mAnchor3D, frameRotation);
        mUniformHandle3D->enabled = mToolEnabled;
    }

    void ScaleTool::Axis3DHandlePressed(int axis)
    {
        HandlePressed();

        mAnchor3D = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());
        mDragAxisDir3D = GetScaleAxisDirection3D(axis);

        if (!o2EditorSceneScreen.ScreenToWorldAxisParam(o2Input.GetCursorPos(), mAnchor3D, mDragAxisDir3D,
                                                        mLastAxisParam3D))
        {
            mLastAxisParam3D = 1.0f;
        }
    }

    void ScaleTool::OnAxis3DHandleMoved(int axis)
    {
        float param;
        if (!o2EditorSceneScreen.ScreenToWorldAxisParam(o2Input.GetCursorPos(), mAnchor3D, mDragAxisDir3D, param))
            return;

        if (Math::Abs(mLastAxisParam3D) < 0.001f)
            return;

        // Mirror the 2D delta-to-scale formula: ratio of distances from the anchor along the axis
        float scale = Math::Abs(param)/Math::Abs(mLastAxisParam3D);
        mLastAxisParam3D = param;

        Vec3F scale3(axis == 0 ? scale : 1.0f, axis == 1 ? scale : 1.0f, axis == 2 ? scale : 1.0f);
        AppendScaleStep3D(mTransformAction, scale3);
    }

    void ScaleTool::PlaneHandle3DPressed(int normalAxis)
    {
        HandlePressed();

        mAnchor3D = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());
        mDragPlaneNormal3D = GetScaleAxisDirection3D(normalAxis);

        Vec3F hit;
        if (o2EditorSceneScreen.ScreenToWorldPlanePoint(o2Input.GetCursorPos(), mAnchor3D, mDragPlaneNormal3D, hit))
            mLastPlaneDistance3D = Math::Max((hit - mAnchor3D).Length(), 0.001f);
        else
            mLastPlaneDistance3D = 1.0f;
    }

    void ScaleTool::OnPlaneHandle3DMoved(int normalAxis)
    {
        Vec3F hit;
        if (!o2EditorSceneScreen.ScreenToWorldPlanePoint(o2Input.GetCursorPos(), mAnchor3D, mDragPlaneNormal3D, hit))
            return;

        float distance = Math::Max((hit - mAnchor3D).Length(), 0.001f);
        float scale = distance/mLastPlaneDistance3D;
        mLastPlaneDistance3D = distance;

        Vec3F scale3(normalAxis == 0 ? 1.0f : scale, normalAxis == 1 ? 1.0f : scale, normalAxis == 2 ? 1.0f : scale);
        AppendScaleStep3D(mTransformAction, scale3);
    }

    void ScaleTool::UniformHandle3DPressed()
    {
        HandlePressed();

        mAnchor3D = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());
        mLastUniformScreenX = o2Input.GetCursorPos().x;
    }

    void ScaleTool::OnUniformHandle3DMoved()
    {
        float screenX = o2Input.GetCursorPos().x;
        float scale = 1.0f + (screenX - mLastUniformScreenX)*bothScaleSence;
        mLastUniformScreenX = screenX;

        AppendScaleStep3D(mTransformAction, Vec3F(scale, scale, scale));
    }

    void ScaleTool::DrawScene()
    {
        ITransformTool::DrawScene();

        if (!SceneEditScreen::IsSingletonInitialzed() || !o2EditorSceneScreen.IsView3DMode())
            return;

        for (int axis = 0; axis < 3; axis++)
        {
            GetAxisHandle3D(axis)->DrawGeometry();
            GetPlaneHandle3D(axis)->DrawGeometry();
        }

        mUniformHandle3D->DrawGeometry();
    }

    void ScaleTool::DrawScreen()
    {
        SelectionTool::DrawScreen();

        if (SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode())
            return;

        if (!mHorDragHandle->IsPressed() && !mVerDragHandle->IsPressed() && !mBothDragHandle->IsPressed())
            UpdateHandlesPositions();

        Vec2F screenHandlesPos = o2EditorSceneScreen.SceneToScreenPoint(mSceneHandlesPos);
        Vec2F screenHorHandlePos = o2EditorSceneScreen.SceneToScreenPoint(mHorDragHandle->GetPosition());
        Vec2F screenVerHandlePos = o2EditorSceneScreen.SceneToScreenPoint(mVerDragHandle->GetPosition());
        o2Render.DrawAALine(screenHandlesPos, screenHorHandlePos, Color4::Green());
        o2Render.DrawAALine(screenHandlesPos, screenVerHandlePos, Color4::Red());
    }

    void ScaleTool::OnEnabled()
    {
        mToolEnabled = true;
        UpdateHandlesEnabledState();

        if (!o2EditorSceneScreen.IsView3DMode())
            UpdateHandlesPosition();
    }

    void ScaleTool::OnDisabled()
    {
        mToolEnabled = false;
        UpdateHandlesEnabledState();
    }

    void ScaleTool::OnSceneChanged(const Vector<Ref<SceneEditableObject>>& changedObjects)
    {}

    void ScaleTool::OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {
        UpdateHandlesPosition();
    }

    void ScaleTool::OnHorDragHandleMoved(const Vec2F& position)
    {
        Vec2F axis = Vec2F::Rotated(mHandlesAngle);
        Vec2F handlePos = (position - mSceneHandlesPos).Project(axis) + mSceneHandlesPos;
        float scale = (handlePos - mSceneHandlesPos).Length() / (mLastHorHandlePos - mSceneHandlesPos).Length();

        mLastHorHandlePos = handlePos;
        mHorDragHandle->position = handlePos;

        ScaleSelectedObjects(Vec2F(scale, 1.0f));
    }

    void ScaleTool::OnVerDragHandleMoved(const Vec2F& position)
    {
        Vec2F axis = Vec2F::Rotated(mHandlesAngle).Perpendicular();
        Vec2F handlePos = (position - mSceneHandlesPos).Project(axis) + mSceneHandlesPos;
        float scale = (handlePos - mSceneHandlesPos).Length() / (mLastVerHandlePos - mSceneHandlesPos).Length();

        mLastVerHandlePos = handlePos;
        mVerDragHandle->position = handlePos;

        ScaleSelectedObjects(Vec2F(1.0f, scale));
    }

    void ScaleTool::OnBothDragHandleMoved(const Vec2F& position)
    {
        float delta = o2EditorSceneScreen.SceneToScreenPoint(position).x -
            o2EditorSceneScreen.SceneToScreenPoint(mLastBothHandlePos).x;

        float scale = 1.0f + delta*bothScaleSence;
        mLastBothHandlePos = position;

        mBothDragHandle->position = mSceneHandlesPos;

        ScaleSelectedObjects(Vec2F(scale, scale));
    }

    void ScaleTool::UpdateHandlesPosition()
    {
        auto selectedObjects = o2EditorSceneScreen.GetSelectedObjects();
        mSceneHandlesPos =
            selectedObjects.Sum<Vec2F>([](auto x) { return x->GetTransform().origin; }) /
            (float)selectedObjects.Count();

        if (selectedObjects.Count() > 0 && !o2Input.IsKeyDown(VK_CONTROL))
        {
            auto lastSelectedObject = selectedObjects.Last();
            UpdateHandlesAngleAndPositions(-lastSelectedObject->GetTransform().xv.Normalized().Angle(Vec2F::Right()));
        }
        else
            UpdateHandlesAngleAndPositions(0.0f);
    }

    void ScaleTool::UpdateHandlesAngleAndPositions(float angle)
    {
        mHandlesAngle = angle;

        mVerDragHandle->angle = mHandlesAngle;
        mHorDragHandle->angle = mHandlesAngle;
        mBothDragHandle->angle = mHandlesAngle;

        UpdateHandlesPositions();
    }

    void ScaleTool::UpdateHandlesPositions()
    {
        Vec2F handlesAxis = Vec2F::Rotated(mHandlesAngle);
        Vec2F handlesSceneSize = o2EditorSceneScreen.ScreenToScenePoint(mHandlesSize) -
            o2EditorSceneScreen.ScreenToScenePoint(Vec2F());

        mVerDragHandle->position = mSceneHandlesPos + handlesAxis.Perpendicular()*handlesSceneSize.y;
        mHorDragHandle->position = mSceneHandlesPos + handlesAxis*handlesSceneSize.x;
        mBothDragHandle->position = mSceneHandlesPos;

        mLastVerHandlePos = mVerDragHandle->position;
        mLastHorHandlePos = mHorDragHandle->position;
        mLastBothHandlePos = mBothDragHandle->position;
    }

    void ScaleTool::OnKeyPressed(const Input::Key& key)
    {
        if (SceneWindow::IsSingletonInitialzed() && !o2EditorSceneWindow.IsFocused())
            return;

        if (key == VK_CONTROL)
            UpdateHandlesAngleAndPositions(0.0f);

        SelectionTool::OnKeyPressed(key);
    }

    void ScaleTool::OnKeyStayDown(const Input::Key& key)
    {}

    void ScaleTool::OnKeyReleased(const Input::Key& key)
    {
        if (SceneWindow::IsSingletonInitialzed() && !o2EditorSceneWindow.IsFocused())
            return;

        if (key == VK_CONTROL)
        {
            auto selectedObjects = o2EditorSceneScreen.GetSelectedObjects();
            if (selectedObjects.Count() > 0)
            {
                auto lastSelectedObject = selectedObjects.Last();
                UpdateHandlesAngleAndPositions(-lastSelectedObject->GetTransform().xv.Normalized().Angle(Vec2F::Right()));
            }
        }
    }

    void ScaleTool::ScaleSelectedObjects(const Vec2F& scale)
    {
        AppendScaleStep(mTransformAction, scale);
    }

    void ScaleTool::AppendScaleStep(const Ref<TransformAction>& action, const Vec2F& scale)
    {
        if (!action)
            return;

        Basis transform =
            Basis::Translated(mSceneHandlesPos * -1.0f) *
            Basis::Rotated(-mHandlesAngle) *
            Basis::Scaled(scale) *
            Basis::Rotated(mHandlesAngle) *
            Basis::Translated(mSceneHandlesPos);

        auto step = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        step->doneTransforms = step->beforeTransforms;
        for (auto& t : step->doneTransforms)
            t.transform = t.transform * transform;

        action->Append(step);
    }

    void ScaleTool::AppendScaleStep3D(const Ref<TransformAction>& action, const Vec3F& scale)
    {
        if (!action)
            return;

        auto step = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        step->doneTransforms = step->beforeTransforms;
        for (auto& t : step->doneTransforms)
        {
            // Scale around the object's own pivot so its position stays fixed, like in Unity
            Vec2F worldPivot = t.transform.origin + t.transform.xv*t.pivot.x + t.transform.yv*t.pivot.y;

            t.transform.xv *= scale.x;
            t.transform.yv *= scale.y;
            t.transform.origin = worldPivot - t.transform.xv*t.pivot.x - t.transform.yv*t.pivot.y;

            if (t.has3D)
            {
                // The scaled basis together with the scaled x/y keeps the decoded size intact,
                // so the change lands in the actor scale (what 3D meshes actually use)
                t.scaleXY *= Vec2F(scale.x, scale.y);
                t.scaleZ *= scale.z;
            }
        }

        action->Append(step);
    }

    void ScaleTool::HandlePressed()
    {
        mTransformAction = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());

        onTransformBegin();
    }

    void ScaleTool::HandleReleased()
    {
        mTransformAction->Completed();
        o2EditorSceneWindow.DoneAction(mTransformAction);
        mTransformAction = nullptr;

        onTransformEnd();
    }

}
// --- META ---

DECLARE_CLASS(Editor::ScaleTool, Editor__ScaleTool);
// --- END META ---
