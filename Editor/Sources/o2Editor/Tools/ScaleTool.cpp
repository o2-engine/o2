#include "o2Editor/stdafx.h"
#include "ScaleTool.h"

#include "o2/Render/Render.h"
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
    {}

    void ScaleTool::DrawScreen()
    {
        SelectionTool::DrawScreen();

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
        mHorDragHandle->enabled = true;
        mVerDragHandle->enabled = true;
        mBothDragHandle->enabled = true;
        UpdateHandlesPosition();
    }

    void ScaleTool::OnDisabled()
    {
        mHorDragHandle->enabled = false;
        mVerDragHandle->enabled = false;
        mBothDragHandle->enabled = false;
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
        if (!o2EditorSceneWindow.IsFocused())
            return;

        if (key == VK_CONTROL)
            UpdateHandlesAngleAndPositions(0.0f);

        SelectionTool::OnKeyPressed(key);
    }

    void ScaleTool::OnKeyStayDown(const Input::Key& key)
    {}

    void ScaleTool::OnKeyReleased(const Input::Key& key)
    {
        if (!o2EditorSceneWindow.IsFocused())
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
        Basis transform =
            Basis::Translated(mSceneHandlesPos*-1.0f)*
            Basis::Rotated(-mHandlesAngle)*
            Basis::Scaled(scale)*
            Basis::Rotated(mHandlesAngle)*
            Basis::Translated(mSceneHandlesPos);

        for (auto& object : o2EditorSceneScreen.GetTopSelectedObjects())
        {
            object->SetTransform(object->GetTransform()*transform);
            object->UpdateTransform();
        }
    }

    void ScaleTool::HandlePressed()
    {
        mBeforeTransforms = o2EditorSceneScreen.GetTopSelectedObjects().Convert<Basis>(
            [](auto& x) { return x->GetTransform(); });

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
