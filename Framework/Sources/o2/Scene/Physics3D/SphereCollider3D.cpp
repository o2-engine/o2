#include "o2/stdafx.h"
#include "SphereCollider3D.h"

#include "o2/Physics/Box3DConvert.h"
#include "o2/Render/Gizmos.h"

namespace o2
{
    SphereCollider3D::SphereCollider3D()
    {}

    SphereCollider3D::SphereCollider3D(const SphereCollider3D& other):
        ICollider3D(other), mRadius(other.mRadius)
    {}

    SphereCollider3D& SphereCollider3D::operator=(const SphereCollider3D& other)
    {
        ICollider3D::operator=(other);
        mRadius = other.mRadius;
        return *this;
    }

    void SphereCollider3D::SetRadius(float radius)
    {
        mRadius = radius;
        OnShapeChanged();
    }

    float SphereCollider3D::GetRadius() const
    {
        return mRadius;
    }

    String SphereCollider3D::GetName()
    {
        return "Sphere collider 3D";
    }

    String SphereCollider3D::GetCategory()
    {
        return "Physics 3D";
    }

    bool SphereCollider3D::IsAvailableFromCreateMenu()
    {
        return true;
    }

    b3ShapeId SphereCollider3D::CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale)
    {
        b3Sphere sphere;
        sphere.center = relative.p; // rotation is irrelevant for a sphere
        sphere.radius = Math::Max(mRadius*invScale, 0.01f);

        return b3CreateSphereShape(body, &def, &sphere);
    }

#if IS_EDITOR
    void SphereCollider3D::OnDrawGizmos()
    {
        auto owner = mOwner.Lock();
        if (!owner)
            return;

        o2Gizmos.SetColor(Gizmos::colliderColor);
        o2Gizmos.DrawSphere(owner->transform->GetWorldPosition(), mRadius);
    }
#endif
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SphereCollider3D>);
// --- META ---

DECLARE_CLASS(o2::SphereCollider3D, o2__SphereCollider3D);
// --- END META ---
