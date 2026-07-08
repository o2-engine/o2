#include "o2Editor/stdafx.h"
#include "SceneDragHandle3D.h"

#include "o2/Render/Mesh3DFill.h"
#include "o2/Render/Mesh.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    namespace
    {
        Vector<SceneDragHandle3D*> gAllHandles3D;
    }

    SceneDragHandle3D::SceneDragHandle3D(RefCounter* refCounter):
        SceneDragHandle(refCounter)
    {
        mMesh = mmake<Mesh>();
        gAllHandles3D.Add(this);
    }

    SceneDragHandle3D::SceneDragHandle3D(RefCounter* refCounter, const SceneDragHandle3D& other):
        SceneDragHandle(refCounter, other)
    {
        mMesh = mmake<Mesh>();
        mGeometry = other.mGeometry;
        mLocalBounds = other.mLocalBounds;
        mPosition3D = other.mPosition3D;
        mRotation3D = other.mRotation3D;
        mPickShapes = other.mPickShapes;
        mScreenSizeFactor = other.mScreenSizeFactor;
        mShaded = other.mShaded;
        mPickPadding = other.mPickPadding;
        mRegularColor = other.mRegularColor;
        mHoverColor = other.mHoverColor;
        mPressedColor = other.mPressedColor;

        gAllHandles3D.Add(this);
    }

    SceneDragHandle3D::~SceneDragHandle3D()
    {
        gAllHandles3D.Remove(this);
    }

    void SceneDragHandle3D::SetGeometry(const Mesh3DData& data)
    {
        mGeometry = data;

        mLocalBounds = o2::AABB();
        mGeometry.GetBounds(mLocalBounds);
    }

    void SceneDragHandle3D::SetPose(const Vec3F& position, const Quat& rotation)
    {
        mPosition3D = position;
        mRotation3D = rotation;
        mPositionZ = position.z;
        SetPosition(position.XY());
    }

    Vec3F SceneDragHandle3D::GetPosition3D() const
    {
        return mPosition3D;
    }

    Quat SceneDragHandle3D::GetRotation3D() const
    {
        return mRotation3D;
    }

    void SceneDragHandle3D::SetColor(const Color4& color)
    {
        mRegularColor = color;
        mHoverColor = Math::Lerp(color, Color4(255, 255, 255, color.a), 0.35f);
        mPressedColor = Color4(255, 220, 80, color.a);
    }

    void SceneDragHandle3D::SetColors(const Color4& regular, const Color4& hover, const Color4& pressed)
    {
        mRegularColor = regular;
        mHoverColor = hover;
        mPressedColor = pressed;
    }

    void SceneDragHandle3D::SetScreenSizeFactor(float factor)
    {
        mScreenSizeFactor = factor;
    }

    float SceneDragHandle3D::GetWorldScale() const
    {
        if (!SceneEditScreen::IsSingletonInitialzed() || !o2EditorSceneScreen.IsView3DMode())
            return mScreenSizeFactor;

        float distance = (o2EditorSceneScreen.GetView3DState().GetCameraPosition() - mPosition3D).Length();
        return mScreenSizeFactor*Math::Max(distance, 0.001f);
    }

    void SceneDragHandle3D::SetShaded(bool shaded)
    {
        mShaded = shaded;
    }

    void SceneDragHandle3D::SetPickPadding(float padding)
    {
        mPickPadding = padding;
    }

    void SceneDragHandle3D::ClearPickShapes()
    {
        mPickShapes.Clear();
    }

    void SceneDragHandle3D::AddPickCylinder(const Vec3F& start, const Vec3F& end, float radius)
    {
        PickShape shape;
        shape.type = PickShape::Type::Cylinder;
        shape.pointA = start;
        shape.pointB = end;
        shape.radius = radius;
        mPickShapes.Add(shape);
    }

    void SceneDragHandle3D::AddPickQuad(const Vec3F& corner, const Vec3F& edgeU, const Vec3F& edgeV)
    {
        PickShape shape;
        shape.type = PickShape::Type::Quad;
        shape.pointA = corner;
        shape.pointB = edgeU;
        shape.pointC = edgeV;
        mPickShapes.Add(shape);
    }

    void SceneDragHandle3D::AddPickBox(const o2::AABB& box)
    {
        PickShape shape;
        shape.type = PickShape::Type::Box;
        shape.pointA = box.min;
        shape.pointB = box.max;
        mPickShapes.Add(shape);
    }

    void SceneDragHandle3D::Draw()
    {
        if (!mEnabled)
            return;

        if (mLastScreenPosUpdateFrame != o2Time.GetCurrentFrame())
        {
            UpdateScreenPosition();
            mLastScreenPosUpdateFrame = o2Time.GetCurrentFrame();
        }

        // Geometry is drawn in the scene pass by DrawGeometry, here only cursor events are registered
        CursorAreaEventsListener::OnDrawn();
        IDrawable::OnDrawn();
    }

    void SceneDragHandle3D::DrawGeometry()
    {
        if (!mEnabled || mGeometry.positions.IsEmpty())
            return;

        float scale = GetWorldScale();
        Mat4 transform = Basis3D::Build(mPosition3D, Vec3F(scale, scale, scale), mRotation3D).ToMat4();

        // High ambient keeps gizmos in uniform saturated colors with only a light shape hint
        Mesh3DPrimitives::FillMesh(*mMesh, mGeometry, transform, GetCurrentColor(), TextureSource(), mShaded, 0.85f);
        mMesh->Draw();
    }

    o2::AABB SceneDragHandle3D::GetWorldBounds() const
    {
        float scale = GetWorldScale();
        return mLocalBounds.Transformed(Basis3D::Build(mPosition3D, Vec3F(scale, scale, scale), mRotation3D));
    }

    bool SceneDragHandle3D::GetRayHitDistance(const Vec2F& screenPoint, float& distance) const
    {
        if (mGeometry.positions.IsEmpty() || !SceneEditScreen::IsSingletonInitialzed())
            return false;

        Vec3F origin, direction;
        if (!o2EditorSceneScreen.ScreenToWorldRay(screenPoint, origin, direction))
            return false;

        // Ray in the handle's rotated frame: rotation preserves distances, scale is applied to the shapes
        float scale = GetWorldScale();
        Quat invertedRotation = mRotation3D.Inverted();
        Vec3F localOrigin = invertedRotation*(origin - mPosition3D);
        Vec3F localDirection = invertedRotation*direction;

        float padding = mPickPadding*scale;

        if (mPickShapes.IsEmpty())
        {
            o2::AABB paddedBounds(mLocalBounds.min*scale - Vec3F(padding, padding, padding),
                                  mLocalBounds.max*scale + Vec3F(padding, padding, padding));

            return paddedBounds.IntersectsRay(localOrigin, localDirection, distance);
        }

        float bestDistance = FLT_MAX;
        for (auto& shape : mPickShapes)
        {
            float hitDistance;
            bool hit = false;

            switch (shape.type)
            {
                case PickShape::Type::Cylinder:
                {
                    hit = Geometry::RayIntersectsCylinder(localOrigin, localDirection, shape.pointA*scale,
                                                          shape.pointB*scale, shape.radius*scale + padding,
                                                          hitDistance);
                    break;
                }

                case PickShape::Type::Quad:
                {
                    // Padding expands the parallelogram outwards along its edges
                    Vec3F edgeU = shape.pointB*scale;
                    Vec3F edgeV = shape.pointC*scale;
                    Vec3F expandU = edgeU.Normalized()*padding;
                    Vec3F expandV = edgeV.Normalized()*padding;

                    hit = Geometry::RayIntersectsQuad(localOrigin, localDirection,
                                                      shape.pointA*scale - expandU - expandV,
                                                      edgeU + expandU*2.0f, edgeV + expandV*2.0f, hitDistance);
                    break;
                }

                case PickShape::Type::Box:
                {
                    o2::AABB box(shape.pointA*scale - Vec3F(padding, padding, padding),
                                 shape.pointB*scale + Vec3F(padding, padding, padding));
                    hit = box.IntersectsRay(localOrigin, localDirection, hitDistance);
                    break;
                }
            }

            if (hit)
                bestDistance = Math::Min(bestDistance, hitDistance);
        }

        if (bestDistance == FLT_MAX)
            return false;

        distance = bestDistance;
        return true;
    }

    bool SceneDragHandle3D::IsUnderPoint(const Vec2F& point)
    {
        if (!mEnabled || !SceneEditScreen::IsSingletonInitialzed() || !o2EditorSceneScreen.IsView3DMode())
            return false;

        if (!mDrawingScissorRect.IsInside(point))
            return false;

        float distance;
        if (!GetRayHitDistance(point, distance))
            return false;

        // The nearest hit handle wins between overlapping handles
        for (auto other : gAllHandles3D)
        {
            if (other == this || !other->IsEnabled())
                continue;

            float otherDistance;
            if (other->GetRayHitDistance(point, otherDistance) && otherDistance < distance)
                return false;
        }

        return true;
    }

    void SceneDragHandle3D::OnCursorEnter(const Input::Cursor& cursor)
    {
        SceneDragHandle::OnCursorEnter(cursor);

        if (SceneEditScreen::IsSingletonInitialzed())
            o2EditorSceneScreen.NeedRedraw();
    }

    void SceneDragHandle3D::OnCursorExit(const Input::Cursor& cursor)
    {
        SceneDragHandle::OnCursorExit(cursor);

        if (SceneEditScreen::IsSingletonInitialzed())
            o2EditorSceneScreen.NeedRedraw();
    }

    Color4 SceneDragHandle3D::GetAxisColor(int axis)
    {
        switch (axis)
        {
            case 0: return Color4(219, 75, 60, 255);
            case 1: return Color4(156, 203, 59, 255);
            default: return Color4(59, 126, 219, 255);
        }
    }

    Color4 SceneDragHandle3D::GetCurrentColor() const
    {
        if (mIsPressed)
            return mPressedColor;

        if (mIsHovered)
            return mHoverColor;

        return mRegularColor;
    }
}
// --- META ---

ENUM_META(Editor::SceneDragHandle3D::PickShape::Type, Editor__SceneDragHandle3D__PickShape__Type)
{
    ENUM_ENTRY(Box);
    ENUM_ENTRY(Cylinder);
    ENUM_ENTRY(Quad);
}
END_ENUM_META;

DECLARE_CLASS(Editor::SceneDragHandle3D, Editor__SceneDragHandle3D);
// --- END META ---
