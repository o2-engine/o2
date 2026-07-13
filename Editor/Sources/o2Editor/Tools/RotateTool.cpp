#include "o2Editor/stdafx.h"
#include "RotateTool.h"

#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2/Render/Mesh3DFill.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2Editor/Actions/Transform.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"
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

        mRingGeometry3D = Mesh3DPrimitives::BuildFlatRing(1.0f, mRing3DWidth, 64);
        for (int axis = 0; axis < 3; axis++)
            mRingMeshes3D.Add(mmake<Mesh>());

        mAngleSectorMesh3D = mmake<Mesh>();

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
    {
        bool is3DMode = SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode();
        mPivotDragHandle->enabled = mToolEnabled && !is3DMode;

        // Rings are drawn in the scene pass, so a hover change needs a redraw
        if (is3DMode && mToolEnabled && mPressedRing3D < 0 &&
            !o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
        {
            int hoveredRing = PickRing3D(o2Input.GetCursorPos());
            if (hoveredRing != mHoveredRing3D)
            {
                mHoveredRing3D = hoveredRing;
                o2EditorSceneScreen.NeedRedraw();
            }
        }
    }

    void RotateTool::DrawScene()
    {
        ITransformTool::DrawScene();

        if (!SceneEditScreen::IsSingletonInitialzed() || !o2EditorSceneScreen.IsView3DMode() ||
            o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
        {
            return;
        }

        UpdateRings3D();

        float radius = Math::Max(o2EditorSceneScreen.GetView3DState().distance, 1.0f)*0.15f;
        int hoveredRing = mPressedRing3D < 0 ? PickRing3D(o2Input.GetCursorPos()) : -1;

        // Flat annulus fills like the 2D tool ring band; crisp outlines are drawn in DrawScreen
        for (int axis = 0; axis < 3; axis++)
        {
            Color4 color = SceneDragHandle3D::GetAxisColor(axis);
            color.a = 110;

            if (mPressedRing3D >= 0)
            {
                if (mPressedRing3D == axis)
                    color = Color4(255, 220, 80, 160);
                else
                    color.a = 40;
            }
            else if (hoveredRing == axis)
                color.a = 180;

            Mat4 transform = Basis3D::Build(mPivot3D, Vec3F(radius, radius, radius),
                                            mRingFrame3D*Quat::FromToRotation(Vec3F::YAxis(), Vec3F::Axis(axis))).ToMat4();

            Mesh3DPrimitives::FillMesh(*mRingMeshes3D[axis], mRingGeometry3D, transform, color,
                                       TextureSource(), false);
            mRingMeshes3D[axis]->Draw();
        }
    }

    void RotateTool::DrawScreen()
    {
        bool is3DMode = SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode();

        if (o2EditorSceneScreen.GetSelectedObjects().Count() > 0)
        {
            if (is3DMode)
            {
                UpdateRings3D();

                for (int axis = 0; axis < 3; axis++)
                {
                    Color4 color = SceneDragHandle3D::GetAxisColor(axis);
                    if (mPressedRing3D >= 0 && mPressedRing3D != axis)
                        color.a = 60;

                    o2Render.DrawAALine(mRingPoints3D[axis], color);
                }

                DrawAngle3D();
            }
            else
            {
                UpdateMeshes();
                mRotateRingFillMesh->Draw();
                if (mRingPressed)
                    mAngleMesh->Draw();

                Vec2F screenPos = o2EditorSceneScreen.SceneToScreenPoint(mScenePivot);
                o2Render.DrawAACircle(screenPos, mRotateRingInsideRadius, mRotateRingsColor, mRotateRingSegs);
                o2Render.DrawAACircle(screenPos, mRotateRingOutsideRadius, mRotateRingsColor, mRotateRingSegs);
            }
        }

        SelectionTool::DrawScreen();
    }

    void RotateTool::OnEnabled()
    {
        mToolEnabled = true;
        CalcPivotByObjectsCenter();

        bool is3DMode = SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode();
        mPivotDragHandle->enabled = !is3DMode;
    }

    void RotateTool::OnDisabled()
    {
        mToolEnabled = false;
        mPivotDragHandle->enabled = false;
        mPressedRing3D = -1;
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

    void RotateTool::DrawAngle3D()
    {
        if (mPressedRing3D < 0)
            return;

        // The sector fills the disc inside the ring band, like the 2D tool fills its ring
        float radius = Math::Max(o2EditorSceneScreen.GetView3DState().distance, 1.0f)*0.15f*(1.0f - mRing3DWidth);

        // The sector lies in the press-time drag plane, matching the angle measurement
        Vec3F u = mDragRingU3D, v = mDragRingV3D;

        float sweep = mAccumulatedRingAngle3D;
        if (Math::Abs(sweep) > 0.001f)
        {
            Vec2F screenPivot = o2EditorSceneScreen.World3DToScreenPoint(mPivot3D);

            int steps = Math::Clamp(Math::CeilToInt(Math::Abs(sweep)/(2.0f*Math::PI())*64.0f), 2, 512);

            Color4 color = sweep > 0.0f ? mRotateMeshCClockwiseColor : mRotateMeshClockwiseColor;
            ULong colorUL = color.ABGR();

            mAngleSectorMesh3D->Resize(steps + 2, steps);
            Vertex* verts = mAngleSectorMesh3D->GetVertices<Vertex>();
            VertexIndex* indexes = mAngleSectorMesh3D->GetIndexes();

            verts[0] = Vertex(screenPivot, colorUL, 0.0f, 0.0f);
            for (int i = 0; i <= steps; i++)
            {
                float angle = mPressRingAngle3D + sweep*(float)i/(float)steps;
                Vec3F worldPoint = mPivot3D + (u*Math::Cos(angle) + v*Math::Sin(angle))*radius;
                verts[i + 1] = Vertex(o2EditorSceneScreen.World3DToScreenPoint(worldPoint), colorUL, 0.0f, 0.0f);
            }

            for (int i = 0; i < steps; i++)
            {
                indexes[i*3] = 0;
                indexes[i*3 + 1] = i + 1;
                indexes[i*3 + 2] = i + 2;
            }

            mAngleSectorMesh3D->vertexCount = steps + 2;
            mAngleSectorMesh3D->polyCount = steps;
            mAngleSectorMesh3D->Draw();
        }

        if (!mAngleText3D)
        {
            if (auto fontAsset = AssetRef<VectorFontAsset>("stdFont.ttf"))
            {
                mAngleText3D = mmake<Text>(fontAsset->GetFont());
                mAngleText3D->SetHeight(11);
                mAngleText3D->SetHorAlign(HorAlign::Left);
                mAngleText3D->SetVerAlign(VerAlign::Bottom);
                mAngleText3D->color = Color4(255, 255, 255, 255);
            }
        }

        if (mAngleText3D)
        {
            mAngleText3D->SetText(WString(String::Format("%.1f°", Math::Rad2deg(sweep))));
            mAngleText3D->SetPosition(o2Input.GetCursorPos() + Vec2F(15.0f, 15.0f));
            mAngleText3D->Draw();
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

    bool RotateTool::IsScreenPointInRing(const Vec2F& screenPivot, const Vec2F& screenPoint,
                                         float innerRadius, float outerRadius)
    {
        float dist = (screenPivot - screenPoint).Length();
        return dist > innerRadius && dist < outerRadius;
    }

    bool RotateTool::IsPointInRotateRing(const Vec2F& screenPoint) const
    {
        return IsScreenPointInRing(o2EditorSceneScreen.SceneToScreenPoint(mScenePivot), screenPoint,
                                   mRotateRingInsideRadius, mRotateRingOutsideRadius);
    }

    void RotateTool::UpdateRings3D()
    {
        // Rings follow the live selection center and orientation during drag too, like in Unity
        mPivot3D = ITransformTool::GetSelectionCenter3D(o2EditorSceneScreen.GetSelectedObjects());
        mScenePivot = mPivot3D.XY();
        mRingFrame3D = ITransformTool::GetSelectionFrameRotation3D(o2EditorSceneScreen.GetSelectedObjects());

        // Ring radius in world units derived from view distance, so the screen size stays roughly constant
        float radius = Math::Max(o2EditorSceneScreen.GetView3DState().distance, 1.0f)*0.15f;
        const int segments = 64;

        mRingPoints3D.Resize(3);
        for (int axis = 0; axis < 3; axis++)
        {
            Vec3F u, v;
            Geometry::AxisPlaneBasis(axis, u, v);

            Vec3F worldU = mRingFrame3D*u;
            Vec3F worldV = mRingFrame3D*v;

            auto& points = mRingPoints3D[axis];
            points.Clear();
            for (int i = 0; i <= segments; i++)
            {
                float angle = 2.0f*Math::PI()*(float)i/(float)segments;
                Vec3F worldPoint = mPivot3D + worldU*Math::Cos(angle)*radius + worldV*Math::Sin(angle)*radius;
                points.Add(o2EditorSceneScreen.World3DToScreenPoint(worldPoint));
            }
        }
    }

    bool RotateTool::IsRayHitInRingBand(const Vec3F& rayOrigin, const Vec3F& rayDirection,
                                        const Vec3F& pivot, const Vec3F& planeNormal,
                                        float innerRadius, float outerRadius, float tolerance,
                                        float& hitDistance)
    {
        // Same edge-on threshold as the ring drag guard
        float denominator = rayDirection.Dot(planeNormal);
        if (Math::Abs(denominator) < 0.02f)
            return false;

        float t = (pivot - rayOrigin).Dot(planeNormal)/denominator;
        if (t < 0.0f)
            return false;

        float hitRadius = (rayOrigin + rayDirection*t - pivot).Length();
        if (hitRadius < innerRadius - tolerance || hitRadius > outerRadius + tolerance)
            return false;

        hitDistance = t;
        return true;
    }

    int RotateTool::PickRing3D(const Vec2F& screenPoint) const
    {
        float outerRadius = Math::Max(o2EditorSceneScreen.GetView3DState().distance, 1.0f)*0.15f;
        float innerRadius = outerRadius*(1.0f - mRing3DWidth);

        // Picking matches the drawn flat annulus: ray vs ring plane, hit radius in the band
        int nearestAxis = -1;
        Vector<int> edgeOnAxes;

        Vec3F rayOrigin, rayDirection;
        if (o2EditorSceneScreen.ScreenToWorldRay(screenPoint, rayOrigin, rayDirection))
        {
            // A few pixels of tolerance converted to world units through the projected ring radius
            Vec3F viewRight = o2EditorSceneScreen.GetView3DState().GetRotation()*Vec3F(1.0f, 0.0f, 0.0f);
            Vec2F screenPivot = o2EditorSceneScreen.World3DToScreenPoint(mPivot3D);
            Vec2F screenAtRadius = o2EditorSceneScreen.World3DToScreenPoint(mPivot3D + viewRight*outerRadius);
            float screenRadius = Math::Max((screenAtRadius - screenPivot).Length(), 1.0f);
            float tolerance = 4.0f*outerRadius/screenRadius;

            Vector<int> hitAxes;
            for (int axis = 0; axis < 3; axis++)
            {
                Vec3F planeNormal = mRingFrame3D*Vec3F::Axis(axis);
                if (Math::Abs(rayDirection.Dot(planeNormal)) < 0.02f)
                {
                    edgeOnAxes.Add(axis);
                    continue;
                }

                float hitDistance;
                if (IsRayHitInRingBand(rayOrigin, rayDirection, mPivot3D, planeNormal,
                                       innerRadius, outerRadius, tolerance, hitDistance))
                {
                    hitAxes.Add(axis);
                }
            }

            if (hitAxes.Count() == 1)
                return hitAxes[0];

            // Overlapping bands: the ring whose visible circle is closest on screen wins
            if (hitAxes.Count() > 1)
            {
                float nearestCircleDistance = FLT_MAX;
                for (int axis : hitAxes)
                {
                    if (axis >= mRingPoints3D.Count())
                        continue;

                    float distance = Geometry::PointToPolylineDistance(mRingPoints3D[axis], screenPoint);
                    if (distance < nearestCircleDistance)
                    {
                        nearestCircleDistance = distance;
                        nearestAxis = axis;
                    }
                }

                if (nearestAxis >= 0)
                    return nearestAxis;
            }
        }
        else
        {
            for (int axis = 0; axis < 3; axis++)
                edgeOnAxes.Add(axis);
        }

        // Edge-on rings fall back to the projected polyline test
        const float pickThreshold = 8.0f;
        float nearestDistance = pickThreshold;
        for (int axis : edgeOnAxes)
        {
            if (axis >= mRingPoints3D.Count())
                continue;

            float distance = Geometry::PointToPolylineDistance(mRingPoints3D[axis], screenPoint);
            if (distance <= nearestDistance)
            {
                nearestDistance = distance;
                nearestAxis = axis;
            }
        }

        return nearestAxis;
    }

    bool RotateTool::GetCursorAngleOnPlane(const Vec3F& normal, const Vec3F& u, const Vec3F& v,
                                           const Vec2F& screenPoint, float& angle) const
    {
        Vec3F origin, direction;
        if (!o2EditorSceneScreen.ScreenToWorldRay(screenPoint, origin, direction))
            return false;

        // A ring seen nearly edge-on maps cursor movement to wild far-away plane hits, refuse the drag
        float denominator = direction.Dot(normal);
        if (Math::Abs(denominator) < 0.02f)
            return false;

        float t = (mPivot3D - origin).Dot(normal)/denominator;
        if (t < 0.0f)
            return false;

        Vec3F offset = origin + direction*t - mPivot3D;
        angle = Math::Atan2F(offset.Dot(v), offset.Dot(u));
        return true;
    }

    bool RotateTool::GetCursorRingAngle3D(int axis, const Vec2F& screenPoint, float& angle) const
    {
        Vec3F u, v;
        Geometry::AxisPlaneBasis(axis, u, v);

        return GetCursorAngleOnPlane(mRingFrame3D*Vec3F::Axis(axis),
                                     mRingFrame3D*u, mRingFrame3D*v, screenPoint, angle);
    }

    void RotateTool::OnCursorPressed(const Input::Cursor& cursor)
    {
        // Same screen-space source as UpdateMeshes — cursor.position can be in a different space.
        Vec2F screenCursor = o2Input.GetCursorPos();

        if (o2EditorSceneScreen.IsView3DMode())
        {
            if (!o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
            {
                UpdateRings3D();

                int axis = PickRing3D(screenCursor);
                float angle;
                if (axis >= 0 && GetCursorRingAngle3D(axis, screenCursor, angle))
                {
                    mPressedRing3D = axis;
                    mLastRingAngle3D = angle;
                    mPressRingAngle3D = angle;
                    mAccumulatedRingAngle3D = 0.0f;

                    // The rotation axis and the angle measuring plane are frozen at press,
                    // while the visible rings keep following the live orientation
                    Vec3F u, v;
                    Geometry::AxisPlaneBasis(axis, u, v);
                    mDragRingAxis3D = mRingFrame3D*Vec3F::Axis(axis);
                    mDragRingU3D = mRingFrame3D*u;
                    mDragRingV3D = mRingFrame3D*v;

                    mTransformAction = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());

                    onTransformBegin();
                    return;
                }
            }

            SelectionTool::OnCursorPressed(cursor);
            return;
        }

        if (IsPointInRotateRing(screenCursor))
        {
            mRingPressed = true;
            Vec2F cursorInScene = o2EditorSceneScreen.ScreenToScenePoint(screenCursor);
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
        if (mRingPressed || mPressedRing3D >= 0)
        {
            mRingPressed = false;
            mPressedRing3D = -1;

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
        if (mRingPressed || mPressedRing3D >= 0)
        {
            mRingPressed = false;
            mPressedRing3D = -1;

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
        if (SceneWindow::IsSingletonInitialzed() && !o2EditorSceneWindow.IsFocused())
            return;

        if (mPressedRing3D >= 0)
        {
            float angle;
            if (o2Input.GetCursorDelta() != Vec2F() &&
                GetCursorAngleOnPlane(mDragRingAxis3D, mDragRingU3D, mDragRingV3D, o2Input.GetCursorPos(), angle))
            {
                float delta = Math::WrapAngle(angle - mLastRingAngle3D);
                mLastRingAngle3D = angle;
                mAccumulatedRingAngle3D += delta;

                if (Math::Abs(delta) > FLT_EPSILON)
                    AppendRotateEulerStep(mTransformAction, mDragRingAxis3D, delta);
            }

            return;
        }

        if (mRingPressed)
        {
            Vec2F screenCursor = o2Input.GetCursorPos();
            Vec2F screenCursorDelta = o2Input.GetCursorDelta();
            if (screenCursorDelta != Vec2F())
            {
                Vec2F cursorInScene = o2EditorSceneScreen.ScreenToScenePoint(screenCursor);
                Vec2F lastCursorInScene = o2EditorSceneScreen.ScreenToScenePoint(screenCursor - screenCursorDelta);
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
        if (SceneWindow::IsSingletonInitialzed() && !o2EditorSceneWindow.IsFocused())
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

    void RotateTool::AppendRotateEulerStep(const Ref<TransformAction>& action, const Vec3F& worldAxis,
                                           float angleDelta)
    {
        if (!action)
            return;

        Quat worldRotation = Quat::FromAxisAngle(worldAxis, angleDelta);

        auto step = mmake<TransformAction>(o2EditorSceneScreen.GetTopSelectedObjects());
        step->doneTransforms = step->beforeTransforms;
        for (int i = 0; i < step->objectsIds.Count(); i++)
        {
            auto& t = step->doneTransforms[i];
            if (!t.has3D)
                continue;

            auto actor = DynamicCast<Actor>(o2Scene.GetEditableObjectByID(step->objectsIds[i]));
            if (!actor)
                continue;

            // Compose the world ring axis rotation over the current rotation: a plain euler component
            // increment rotates around a tilted axis once other components are non zero.
            // Explicit euler z is used: the basis projection loses it at degenerate orientations
            Quat current = Quat::FromEuler(Vec3F(t.eulerAnglesXY.x, t.eulerAnglesXY.y, t.eulerZ));
            Quat rotated = worldRotation*current;

            Vec3F rotatedEuler = rotated.ToEuler();
            t.eulerAnglesXY = rotatedEuler.XY();
            t.eulerZ = rotatedEuler.z;

            // Position orbits around the ring pivot
            Vec3F position = actor->transform->GetWorldPosition();
            Vec3F newPosition = mPivot3D + worldRotation*(position - mPivot3D);
            t.positionZ = newPosition.z;

            // Rebuild the projected basis consistently with the new rotation, keeping size and scale
            Vec2F sizeScale = actor->transform->GetSize2D()*actor->transform->GetScale2D();
            float shear = actor->transform->GetShear2D();
            float shearYY = Math::Sqrt(Math::Max(0.0f, 1.0f - shear*shear));

            Vec2F xv = (rotated*Vec3F(1.0f, 0.0f, 0.0f)).XY()*sizeScale.x;
            Vec2F yv = (rotated*Vec3F(shear, shearYY, 0.0f)).XY()*sizeScale.y;
            Vec2F origin = newPosition.XY() - xv*t.pivot.x - yv*t.pivot.y;

            t.transform = Basis(origin, xv, yv);
        }

        action->Append(step);
    }

}
// --- META ---

DECLARE_CLASS(Editor::RotateTool, Editor__RotateTool);
// --- END META ---
