#include "o2Editor/stdafx.h"
#include "Transform.h"

#include "o2/Scene/Actor.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    TransformAction::TransformAction()
    {}

    TransformAction::TransformAction(const Vector<Ref<SceneEditableObject>>& actors)
    {
        objectsIds = actors.Convert<UInt64>([](auto& x) { return x->GetID(); });
        GetTransforms(objectsIds, beforeTransforms);
    }

    void TransformAction::Completed()
    {
        GetTransforms(objectsIds, doneTransforms);
    }

    String TransformAction::GetName() const
    {
        return "Actors transformation";
    }

    void TransformAction::Redo()
    {
        SetTransforms(objectsIds, doneTransforms);
    }

    void TransformAction::Undo()
    {
        SetTransforms(objectsIds, beforeTransforms);
    }

    bool TransformAction::TryMerge(const Ref<IAction>& other)
    {
        auto step = DynamicCast<TransformAction>(other);
        if (!step || step->objectsIds != objectsIds)
            return false;

        doneTransforms = step->doneTransforms;
        return true;
    }

    namespace
    {
        // Solve worldPivot = basis.origin + basis.xv*a + basis.yv*b for relative (a, b).
        // Stored relative so a basis-only edit recovers the same relative pivot
        // (world pivot derives from the new basis), while a pivot-only edit
        // changes the relative coords.
        bool TryWorldPivotToRelative(const Basis& basis, const Vec2F& worldPivot, Vec2F& outRel)
        {
            float det = basis.xv.x*basis.yv.y - basis.yv.x*basis.xv.y;
            if (Math::Abs(det) < FLT_EPSILON)
                return false;

            Vec2F p = worldPivot - basis.origin;
            outRel.x = (p.x*basis.yv.y - p.y*basis.yv.x) / det;
            outRel.y = (basis.xv.x*p.y - basis.xv.y*p.x) / det;
            return true;
        }

        Vec2F RelativePivotToWorld(const Basis& basis, const Vec2F& rel)
        {
            return basis.origin + basis.xv*rel.x + basis.yv*rel.y;
        }
    }

    void TransformAction::GetTransforms(const Vector<SceneUID>& objectIds, Vector<Transform>& transforms)
    {
        transforms = objectIds.Convert<Transform>([=](SceneUID id)
        {
            auto object = o2Scene.GetEditableObjectByID(id);
            if (object)
            {
                Transform res;
                res.transform = object->GetTransform();
                res.layout = object->GetLayout();
                if (object->IsSupportsPivot())
                    TryWorldPivotToRelative(res.transform, object->GetPivot(), res.pivot);

                if (auto actor = DynamicCast<Actor>(object))
                {
                    res.has3D = true;
                    res.positionZ = actor->transform->GetPositionZ();
                    res.eulerAnglesXY = actor->transform->GetEulerAngles().XY();
                    res.eulerZ = actor->transform->GetEulerAngles().z;
                    res.scaleXY = actor->transform->GetScale().XY();
                    res.scaleZ = actor->transform->GetScaleZ();
                    res.sizeZ = actor->transform->GetSizeZ();
                }

                return res;
            }

            return Transform();
        });
    }

    void TransformAction::SetTransforms(const Vector<SceneUID>& objectIds, Vector<Transform>& transforms)
    {
        for (int i = 0; i < objectsIds.Count(); i++)
        {
            auto object = o2Scene.GetEditableObjectByID(objectsIds[i]);
            if (object)
            {
                // Euler x/y go BEFORE SetTransform: the basis was captured under them and
                // SetBasis decodes size from the projection using the current euler
                if (transforms[i].has3D)
                {
                    if (auto actor = DynamicCast<Actor>(object))
                    {
                        // Full euler goes first: SetBasis re-decodes z from the healthy basis
                        // projection, and keeps this explicit z when the projection is degenerate
                        actor->transform->SetEulerAngles(Vec3F(transforms[i].eulerAnglesXY.x,
                                                               transforms[i].eulerAnglesXY.y,
                                                               transforms[i].eulerZ));

                        // Scale goes BEFORE SetTransform: SetBasis decodes size from the
                        // basis projection divided by the current scale
                        actor->transform->SetScale(Vec3F(transforms[i].scaleXY.x, transforms[i].scaleXY.y,
                                                         transforms[i].scaleZ));
                    }
                }

                // SetLayout BEFORE SetTransform: for Widgets, SetTransform shifts
                // offsets via WidgetLayout::UpdateOffsetsByCurrentTransform, which
                // would be clobbered if SetLayout ran afterwards (it resets all
                // four anchor/offset fields to the captured values, undoing the
                // shift). With this order SetLayout restores the captured anchors
                // first, then SetTransform shifts offsets relative to them.
                object->SetLayout(transforms[i].layout);
                object->SetTransform(transforms[i].transform);

                if (transforms[i].has3D)
                {
                    if (auto actor = DynamicCast<Actor>(object))
                    {
                        actor->transform->SetPositionZ(transforms[i].positionZ);
                        actor->transform->SetSizeZ(transforms[i].sizeZ);
                    }
                }

                if (object->IsSupportsPivot())
                {
                    Basis applied = transforms[i].transform;
                    if (applied.xv.SqrLength() > FLT_EPSILON && applied.yv.SqrLength() > FLT_EPSILON)
                    {
                        // UpdateTransform first — SetWorldPivot reads the cached worldTransform,
                        // which is still stale right after SetTransform.
                        object->UpdateTransform();
                        object->SetPivot(RelativePivotToWorld(applied, transforms[i].pivot));
                    }
                }
                object->UpdateTransform();
            }
        }
    }

    bool TransformAction::Transform::operator==(const Transform& other) const
    {
        return transform == other.transform && layout == other.layout && pivot == other.pivot &&
            has3D == other.has3D && Math::Equals(positionZ, other.positionZ) &&
            eulerAnglesXY == other.eulerAnglesXY && Math::Equals(eulerZ, other.eulerZ) &&
            scaleXY == other.scaleXY &&
            Math::Equals(scaleZ, other.scaleZ) &&
            Math::Equals(sizeZ, other.sizeZ);
    }

}
// --- META ---

DECLARE_CLASS(Editor::TransformAction, Editor__TransformAction);
// --- END META ---
