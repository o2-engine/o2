#include "o2Editor/stdafx.h"
#include "RotateTool.h"

#include "o2/Render/Mesh.h"
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
    RotateTool::RotateTool()
    {
        mPivotDragHandle = mmake<SceneDragHandle>(mmake<Sprite>("ui/UI2_pivot.png"),
                                                  mmake<Sprite>("ui/UI2_pivot_select.png"),
                                                  mmake<Sprite>("ui/UI2_pivot_pressed.png"));

        mRotateRingFillMesh = mmake<Mesh>(TextureRef::Null(), mRotateRingSegs * 4, mRotateRingSegs * 2);
        mAngleMesh = mmake<Mesh>(TextureRef::Null(), mRotateRingSegs * 4, mRotateRingSegs * 2);

        mPivotDragHandle->onChangedPos += THIS_FUNC(OnPivotDragHandleMoved);
        mPivotDragHandle->enabled = false;
    }

    RotateTool::~RotateTool()
    {}

    String RotateTool::GetPanelIcon() const
    {
        return "ui/UI4_rotate_tool.png";
    }

    ShortcutKeys RotateTool::GetShortcut() const
    {
        return ShortcutKeys({ VK_E });
    }

    void RotateTool::Update(float dt)
    {}

    void RotateTool::DrawScreen()
    {
        if (o2EditorSceneScreen.GetSelectedObjects().Count() > 0)
        {
            UpdateMeshes();
            mRotateRingFillMesh->Draw();
            if (mRingPressed)
                mAngleMesh->Draw();

            Vec2F screenPos = o2EditorSceneScreen.SceneToScreenPoint(mScenePivot);
            o2Render.DrawAACircle(screenPos, mRotateRingInsideRadius, mRotateRingsColor, mRotateRingSegs);
            o2Render.DrawAACircle(screenPos, mRotateRingOutsideRadius, mRotateRingsColor, mRotateRingSegs);
        }

        SelectionTool::DrawScreen();
    }

    void RotateTool::OnEnabled()
    {
        CalcPivotByObjectsCenter();
        mPivotDragHandle->enabled = true;
    }

    void RotateTool::OnDisabled()
    {
        mPivotDragHandle->enabled = false;
    }

    void RotateTool::OnSceneChanged(const Vector<Ref<SceneEditableObject>>& changedObjects)
    {}

    void RotateTool::OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {
        CalcPivotByObjectsCenter();
    }

    void RotateTool::UpdateMeshes()
    {
        Vec2F screenPos = o2EditorSceneScreen.SceneToScreenPoint(mScenePivot);

        Color4 fillColor = mRotateRingsFillColor;
        Color4 fillColor2 = mRotateRingsFillColor2;

        const float selectionAlphaCoef = 1.2f;
        const float pressingAlphaCoef = 0.5f;

        if (mRingPressed)
        {
            fillColor.a = (int)((float)fillColor.a*pressingAlphaCoef);
            fillColor2.a = (int)((float)fillColor2.a*pressingAlphaCoef);
        }
        else if (IsPointInRotateRing(o2Input.GetCursorPos()))
        {
            fillColor.a = (int)((float)fillColor.a*selectionAlphaCoef);
            fillColor2.a = (int)((float)fillColor2.a*selectionAlphaCoef);
        }

        ULong fillColorUL = fillColor.ARGB();
        ULong fillColorUL2 = fillColor2.ARGB();
        mRotateRingFillMesh->vertexCount = mRotateRingSegs * 4;
        mRotateRingFillMesh->polyCount = mRotateRingSegs * 2;
        float segAngle = 2.0f*Math::PI() / (float)mRotateRingSegs;

        Vertex* fillVerts = mRotateRingFillMesh->GetVertices<Vertex>();
        VertexIndex* fillIdx = mRotateRingFillMesh->GetIndexes();
        float angle = 0.0f;
        int i = 0;
        while (angle < 2.0f*Math::PI())
        {
            float angleNext = angle + segAngle;

            Vec2F pinside = Vec2F::Rotated(angle)*mRotateRingInsideRadius + screenPos;
            Vec2F poutside = Vec2F::Rotated(angle)*mRotateRingOutsideRadius + screenPos;
            Vec2F pinsideNext = Vec2F::Rotated(angleNext)*mRotateRingInsideRadius + screenPos;
            Vec2F poutsideNext = Vec2F::Rotated(angleNext)*mRotateRingOutsideRadius + screenPos;

            int vi = i * 4;
            int pi = i * 6;
            i++;

            ULong currFillColor = fillColorUL;
            if (angle < Math::PI()*0.5f || (angle >= Math::PI() - FLT_EPSILON && angle <= Math::PI()*1.5f + FLT_EPSILON))
                currFillColor = fillColorUL2;

            fillVerts[vi] = Vertex(pinside, currFillColor, 0.0f, 0.0f);
            fillVerts[vi + 1] = Vertex(poutside, currFillColor, 0.0f, 0.0f);
            fillVerts[vi + 2] = Vertex(pinsideNext, currFillColor, 0.0f, 0.0f);
            fillVerts[vi + 3] = Vertex(poutsideNext, currFillColor, 0.0f, 0.0f);

            fillIdx[pi] = vi;
            fillIdx[pi + 1] = vi + 1;
            fillIdx[pi + 2] = vi + 3;

            fillIdx[pi + 3] = vi;
            fillIdx[pi + 4] = vi + 3;
            fillIdx[pi + 5] = vi + 2;

            angle = angleNext;
        }

        mAngleMesh->vertexCount = 0;
        mAngleMesh->polyCount = 0;

        if (!mRingPressed)
            return;

        angle = mPressAngle;
        i = 0;
        float direction = Math::Sign(mCurrentRotateAngle - mPressAngle);
        ULong angleRingColor = direction > 0.0f ? mRotateMeshClockwiseColor.ABGR() : mRotateMeshCClockwiseColor.ABGR();
        int reqAngleMeshSegs = Math::CeilToInt(Math::Abs(mCurrentRotateAngle - mPressAngle) / segAngle) + 1;
        mAngleMesh->Resize(reqAngleMeshSegs * 4, reqAngleMeshSegs * 2);
        Vertex* angleVerts = mAngleMesh->GetVertices<Vertex>();
        VertexIndex* angleIdx = mAngleMesh->GetIndexes();
        while (direction > 0.0f ? angle < mCurrentRotateAngle : angle > mCurrentRotateAngle)
        {
            float angleNext = angle + segAngle*direction;
            if (direction > 0.0f ? angleNext > mCurrentRotateAngle : angleNext < mCurrentRotateAngle)
                angleNext = mCurrentRotateAngle;

            Vec2F pinside = Vec2F::Rotated(-angle)*mRotateRingInsideRadius + screenPos;
            Vec2F poutside = Vec2F::Rotated(-angle)*mRotateRingOutsideRadius + screenPos;
            Vec2F pinsideNext = Vec2F::Rotated(-angleNext)*mRotateRingInsideRadius + screenPos;
            Vec2F poutsideNext = Vec2F::Rotated(-angleNext)*mRotateRingOutsideRadius + screenPos;

            int vi = i * 4;
            int pi = i * 6;
            i++;

            angleVerts[vi] = Vertex(pinside, angleRingColor, 0.0f, 0.0f);
            angleVerts[vi + 1] = Vertex(poutside, angleRingColor, 0.0f, 0.0f);
            angleVerts[vi + 2] = Vertex(pinsideNext, angleRingColor, 0.0f, 0.0f);
            angleVerts[vi + 3] = Vertex(poutsideNext, angleRingColor, 0.0f, 0.0f);

            angleIdx[pi] = vi;
            angleIdx[pi + 1] = vi + 1;
            angleIdx[pi + 2] = vi + 3;

            angleIdx[pi + 3] = vi;
            angleIdx[pi + 4] = vi + 3;
            angleIdx[pi + 5] = vi + 2;

            mAngleMesh->vertexCount = i * 4;
            mAngleMesh->polyCount = i * 2;

            angle = angleNext;
        }
    }

    void RotateTool::CalcPivotByObjectsCenter()
    {
        auto selectedObjects = o2EditorSceneScreen.GetSelectedObjects();
        mScenePivot =
            selectedObjects.Sum<Vec2F>([](auto x) { return x->GetPivot(); }) /
            (float)selectedObjects.Count();

        mPivotDragHandle->position = mScenePivot;
    }

    void RotateTool::OnPivotDragHandleMoved(const Vec2F& position)
    {
        mScenePivot = position;
    }

    bool RotateTool::IsPointInRotateRing(const Vec2F& point) const
    {
        float pivotDist = (o2EditorSceneScreen.SceneToScreenPoint(mScenePivot) - point).Length();

        return pivotDist > mRotateRingInsideRadius && pivotDist < mRotateRingOutsideRadius;
    }

    void RotateTool::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (IsPointInRotateRing(cursor.position))
        {
            mRingPressed = true;
            Vec2F cursorInScene = o2EditorSceneScreen.ScreenToScenePoint(cursor.position);
            mPressAngle = Vec2F::Angle(cursorInScene - mScenePivot, Vec2F::Right());
            mCurrentRotateAngle = mPressAngle;
            mSnapAngleAccumulated = 0.0f;

            mTransformAction = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());

            onTransformBegin();
        }
        else
            SelectionTool::OnCursorPressed(cursor);
    }

    void RotateTool::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mRingPressed)
        {
            mRingPressed = false;

            mTransformAction->Completed();
            o2EditorSceneWindow.DoneAction(mTransformAction);
            mTransformAction = nullptr;

            onTransformEnd();
        }
        else 
            SelectionTool::OnCursorReleased(cursor);
    }

    void RotateTool::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        if (mRingPressed)
        {
            mRingPressed = false;

            mTransformAction->Completed();
            o2EditorSceneWindow.DoneAction(mTransformAction);
            mTransformAction = nullptr;

            onTransformEnd();
        }
        else 
            SelectionTool::OnCursorPressBreak(cursor);
    }

    void RotateTool::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!o2EditorSceneWindow.IsFocused())
            return;

        if (mRingPressed)
        {
            if (cursor.delta != Vec2F())
            {
                Vec2F cursorInScene = o2EditorSceneScreen.ScreenToScenePoint(cursor.position);
                Vec2F lastCursorInScene = o2EditorSceneScreen.ScreenToScenePoint(cursor.position - cursor.delta);
                float angleDelta = Vec2F::SignedAngle(cursorInScene - mScenePivot, lastCursorInScene - mScenePivot);

                if (o2Input.IsKeyDown(VK_SHIFT))
                {
                    float angleStepRad = Math::Deg2rad(angleSnapStep);
                    mSnapAngleAccumulated += angleDelta;
                    float dir = Math::Sign(mSnapAngleAccumulated);

                    while (mSnapAngleAccumulated*dir > angleStepRad)
                    {
                        mSnapAngleAccumulated -= dir*angleStepRad;

                        if (o2Input.IsKeyDown(VK_CONTROL)) 
                            RotateObjectsSeparated(angleStepRad*dir);
                        else 
                            RotateObjects(angleStepRad*dir);
                    }
                }
                else
                {
                    if (o2Input.IsKeyDown(VK_CONTROL)) 
                        RotateObjectsSeparated(angleDelta);
                    else 
                        RotateObjects(angleDelta);
                }

                mCurrentRotateAngle += angleDelta;
            }
        }
        else 
            SelectionTool::OnCursorStillDown(cursor);
    }

    void RotateTool::OnKeyPressed(const Input::Key& key)
    {
        if (!o2EditorSceneWindow.IsFocused())
            return;

        float angle = o2Input.IsKeyDown(VK_SHIFT) ? angleSnapStep : 1.0f;

        if (key == VK_LEFT || key == VK_DOWN)
        {
            if (o2Input.IsKeyDown(VK_CONTROL)) 
                RotateObjectsSeparatedWithAction(Math::Deg2rad(-angle));
            else 
                RotateObjectsWithAction(Math::Deg2rad(-angle));
        }

        if (key == VK_RIGHT || key == VK_UP)
        {
            if (o2Input.IsKeyDown(VK_CONTROL)) 
                RotateObjectsSeparatedWithAction(Math::Deg2rad(-angle));
            else 
                RotateObjectsWithAction(Math::Deg2rad(angle));
        }

        SelectionTool::OnKeyPressed(key);
    }

    void RotateTool::OnKeyStayDown(const Input::Key& key)
    {
        if (key.pressedTime < 0.3f)
            return;

        float angle = o2Input.IsKeyDown(VK_SHIFT) ? angleSnapStep : 1.0f;

        if (key == VK_LEFT || key == VK_DOWN)
        {
            if (o2Input.IsKeyDown(VK_CONTROL)) 
                RotateObjectsSeparatedWithAction(Math::Deg2rad(-angle));
            else 
                RotateObjectsWithAction(Math::Deg2rad(-angle));
        }

        if (key == VK_RIGHT || key == VK_UP)
        {
            if (o2Input.IsKeyDown(VK_CONTROL)) 
                RotateObjectsSeparatedWithAction(Math::Deg2rad(-angle));
            else 
                RotateObjectsWithAction(Math::Deg2rad(angle));
        }
    }

    void RotateTool::RotateObjects(float angleDelta)
    {
        AppendRotateStep(mTransformAction, angleDelta, false);
    }

    void RotateTool::RotateObjectsSeparated(float angleDelta)
    {
        AppendRotateStep(mTransformAction, angleDelta, true);
    }

    void RotateTool::RotateObjectsWithAction(float angleDelta)
    {
        auto action = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        AppendRotateStep(action, angleDelta, false);
        action->Completed();
        o2EditorSceneWindow.DoneAction(action);
    }

    void RotateTool::RotateObjectsSeparatedWithAction(float angleDelta)
    {
        auto action = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        AppendRotateStep(action, angleDelta, true);
        action->Completed();
        o2EditorSceneWindow.DoneAction(action);
    }

    void RotateTool::AppendRotateStep(const Ref<TransformAction>& action, float angleDelta, bool separated)
    {
        if (!action)
            return;

        Basis rotation = separated
            ? Basis::Rotated(-angleDelta)
            : Basis::Translated(mScenePivot * -1.0f) *
              Basis::Rotated(-angleDelta) *
              Basis::Translated(mScenePivot);

        auto step = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        step->doneTransforms = step->beforeTransforms;
        for (auto& t : step->doneTransforms)
            t.transform = t.transform * rotation;

        action->Append(step);
    }

}
// --- META ---

DECLARE_CLASS(Editor::RotateTool, Editor__RotateTool);
// --- END META ---
