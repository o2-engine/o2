#include "o2Editor/stdafx.h"
#include "MoveTool.h"

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
    }

    MoveTool::~MoveTool()
    {}

    void MoveTool::Update(float dt)
    {}

    void MoveTool::OnEnabled()
    {
        mHorDragHandle->enabled = true;
        mVerDragHandle->enabled = true;
        mBothDragHandle->enabled = true;
        UpdateHandlesPosition();
    }

    void MoveTool::OnDisabled()
    {
        mHorDragHandle->enabled = false;
        mVerDragHandle->enabled = false;
        mBothDragHandle->enabled = false;
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
    }

    void MoveTool::OnKeyPressed(const Input::Key& key)
    {
        if (!o2EditorSceneWindow.IsFocused())
            return;

        float delta = o2Input.IsKeyDown(VK_SHIFT) ? snapStep : 1.0f;

        bool isArrow = key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN;
        if (isArrow)
        {
            BeginKeyboardAction();

            if (key == VK_LEFT)  AppendKeyboardStep(Vec2F::Left()*delta);
            if (key == VK_RIGHT) AppendKeyboardStep(Vec2F::Right()*delta);
            if (key == VK_UP)    AppendKeyboardStep(Vec2F::Up()*delta);
            if (key == VK_DOWN)  AppendKeyboardStep(Vec2F::Down()*delta);
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
        if (!o2EditorSceneWindow.IsFocused())
            return;

        float delta = o2Input.IsKeyDown(VK_SHIFT) ? snapStep : 1.0f;

        if (key.pressedTime < 0.3f)
            return;

        if (key == VK_LEFT)  AppendKeyboardStep(Vec2F::Left()*delta);
        if (key == VK_RIGHT) AppendKeyboardStep(Vec2F::Right()*delta);
        if (key == VK_UP)    AppendKeyboardStep(Vec2F::Up()*delta);
        if (key == VK_DOWN)  AppendKeyboardStep(Vec2F::Down()*delta);
    }

    void MoveTool::OnKeyReleased(const Input::Key& key)
    {
        bool isArrow = key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN;
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
