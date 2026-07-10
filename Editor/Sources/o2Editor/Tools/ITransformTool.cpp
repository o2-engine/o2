#include "o2Editor/stdafx.h"
#include "ITransformTool.h"

#include "o2/Scene/Actor.h"
#include "o2/Utils/Editor/SceneEditableObject.h"

namespace Editor
{
    namespace
    {
        bool CollectObjectWorldCorners(const Ref<SceneEditableObject>& object, Vector<Vec3F>& corners)
        {
            auto actor = DynamicCast<Actor>(object);
            if (!actor)
            {
                Basis transform = object->GetTransform();
                corners.Add(Vec3F(Vec2F(0, 0)*transform, 0.0f));
                corners.Add(Vec3F(Vec2F(1, 0)*transform, 0.0f));
                corners.Add(Vec3F(Vec2F(1, 1)*transform, 0.0f));
                corners.Add(Vec3F(Vec2F(0, 1)*transform, 0.0f));
                return true;
            }

            // Material corners of the local drawable bounds: unlike the world AABB corners
            // they rotate with the object, so oriented frames built over them are tight and
            // keep their corners in place under corner-anchored scaling
            o2::AABB localBounds;
            bool hasLocalBounds = false;
            for (auto& component : actor->GetComponents())
            {
                o2::AABB componentBounds;
                if (component->Get3DDrawableLocalBounds(componentBounds))
                {
                    localBounds = hasLocalBounds ? localBounds.Expand(componentBounds) : componentBounds;
                    hasLocalBounds = true;
                }
            }

            if (hasLocalBounds)
            {
                Mat4 worldTransform = actor->transform->GetWorldTransform3D();
                for (int i = 0; i < 8; i++)
                {
                    Vec3F localCorner((i & 1) ? localBounds.max.x : localBounds.min.x,
                                      (i & 2) ? localBounds.max.y : localBounds.min.y,
                                      (i & 4) ? localBounds.max.z : localBounds.min.z);

                    corners.Add(worldTransform.TransformPoint(localCorner));
                }

                return true;
            }

            Mat4 worldTransform = actor->transform->GetWorldTransform3D();
            Vec2F size = actor->transform->GetSize2D();
            Vec2F pivot = actor->transform->GetPivot2D();

            static const Vec2F rectCorners[4] = { Vec2F(0, 0), Vec2F(1, 0), Vec2F(1, 1), Vec2F(0, 1) };

            for (int i = 0; i < 4; i++)
            {
                Vec2F local = (rectCorners[i] - pivot)*size;
                corners.Add(worldTransform.TransformPoint(Vec3F(local.x, local.y, 0.0f)));
            }

            return true;
        }
    }

    Vec3F ITransformTool::GetSelectionCenter3D(const Vector<Ref<SceneEditableObject>>& objects)
    {
        if (objects.IsEmpty())
            return Vec3F();

        Vec3F sum;
        for (auto& object : objects)
        {
            if (auto actor = DynamicCast<Actor>(object))
                sum += actor->transform->GetWorldPosition();
            else
                sum += Vec3F(object->GetPivot(), 0.0f);
        }

        return sum/(float)objects.Count();
    }

    Quat ITransformTool::GetSelectionFrameRotation3D(const Vector<Ref<SceneEditableObject>>& objects)
    {
        // Gizmos frame is local, like in Unity: last selected actor orientation, world axes with Ctrl
        if (!objects.IsEmpty() && !o2Input.IsKeyDown(VK_CONTROL))
        {
            if (auto actor = DynamicCast<Actor>(objects.Last()))
            {
                // World orientation: for nested actors the local rotation misses the parents
                Vec3F position, scale;
                Quat rotation;
                actor->transform->GetWorldTransform3D().Decompose(position, rotation, scale);
                return rotation;
            }
        }

        return Quat::Identity();
    }

    bool ITransformTool::GetObjectBounds3D(const Ref<SceneEditableObject>& object, o2::AABB& bounds)
    {
        return GetObjectBoundsInFrame3D(object, Quat::Identity(), bounds);
    }

    bool ITransformTool::GetObjectBoundsInFrame3D(const Ref<SceneEditableObject>& object, const Quat& frameRotation,
                                                  o2::AABB& bounds)
    {
        Vector<Vec3F> corners;
        if (!CollectObjectWorldCorners(object, corners) || corners.IsEmpty())
            return false;

        Quat inverted = frameRotation.Inverted();
        for (auto& corner : corners)
            corner = inverted*corner;

        bounds = o2::AABB::Bound(corners.Data(), corners.Count());
        return true;
    }

    bool ITransformTool::GetObjectWorldCorners3D(const Ref<SceneEditableObject>& object, Vector<Vec3F>& corners)
    {
        return CollectObjectWorldCorners(object, corners);
    }

    namespace
    {
        // Flat content gets a bit of thickness so the slab test doesn't degenerate
        void EnsureBoundsThickness(o2::AABB& bounds, float minThickness = 0.01f)
        {
            for (int axis = 0; axis < 3; axis++)
            {
                if (bounds.max[axis] - bounds.min[axis] < minThickness)
                {
                    bounds.min[axis] -= minThickness*0.5f;
                    bounds.max[axis] += minThickness*0.5f;
                }
            }
        }
    }

    bool ITransformTool::RayIntersectsObject3D(const Ref<SceneEditableObject>& object, const Vec3F& rayOrigin,
                                               const Vec3F& rayDirection, float& distance)
    {
        if (auto actor = DynamicCast<Actor>(object))
        {
            // Mesh content: exact oriented test against local geometry bounds through the world transform
            o2::AABB localBounds;
            bool hasLocalBounds = false;
            for (auto& component : actor->GetComponents())
            {
                o2::AABB componentBounds;
                if (component->Get3DDrawableLocalBounds(componentBounds))
                {
                    localBounds = hasLocalBounds ? localBounds.Expand(componentBounds) : componentBounds;
                    hasLocalBounds = true;
                }
            }

            if (hasLocalBounds)
            {
                Mat4 invertedWorld = actor->transform->GetWorldTransform3D().Inverted();
                Vec3F localOrigin = invertedWorld.TransformPoint(rayOrigin);
                Vec3F localDirection = invertedWorld.TransformPoint(rayOrigin + rayDirection) - localOrigin;

                float directionLength = localDirection.Length();
                if (directionLength < FLT_EPSILON)
                    return false;

                EnsureBoundsThickness(localBounds);

                float localDistance;
                if (!localBounds.IntersectsRay(localOrigin, localDirection/directionLength, localDistance))
                    return false;

                distance = localDistance/directionLength;
                return true;
            }
        }

        // Plane rect content: oriented bounds in the object rotation frame, the ray rotated into it
        Quat frameRotation = Quat::Identity();
        if (auto actor = DynamicCast<Actor>(object))
            frameRotation = actor->transform->GetRotation();

        o2::AABB frameBounds;
        if (!GetObjectBoundsInFrame3D(object, frameRotation, frameBounds))
            return false;

        EnsureBoundsThickness(frameBounds);

        Quat inverted = frameRotation.Inverted();
        return frameBounds.IntersectsRay(inverted*rayOrigin, inverted*rayDirection, distance);
    }

    bool ITransformTool::GetObjectScreenRect3D(const Ref<SceneEditableObject>& object,
                                               const Function<Vec2F(const Vec3F&)>& projector, RectF& rect)
    {
        Vector<Vec3F> corners;
        if (!CollectObjectWorldCorners(object, corners) || corners.IsEmpty())
            return false;

        Vec2F minPoint(FLT_MAX, FLT_MAX), maxPoint(-FLT_MAX, -FLT_MAX);
        for (auto& corner : corners)
        {
            Vec2F screen = projector(corner);
            minPoint.x = Math::Min(minPoint.x, screen.x);
            minPoint.y = Math::Min(minPoint.y, screen.y);
            maxPoint.x = Math::Max(maxPoint.x, screen.x);
            maxPoint.y = Math::Max(maxPoint.y, screen.y);
        }

        rect = RectF(minPoint, maxPoint);
        return true;
    }

    bool ITransformTool::GetSelectionBounds3D(const Vector<Ref<SceneEditableObject>>& objects, o2::AABB& bounds)
    {
        bool anyFound = false;
        for (auto& object : objects)
        {
            o2::AABB objectBounds;
            if (!GetObjectBounds3D(object, objectBounds))
                continue;

            if (anyFound)
                bounds.Include(objectBounds);
            else
                bounds = objectBounds;

            anyFound = true;
        }

        return anyFound;
    }
}
// --- META ---

DECLARE_CLASS(Editor::ITransformTool, Editor__ITransformTool);
// --- END META ---
