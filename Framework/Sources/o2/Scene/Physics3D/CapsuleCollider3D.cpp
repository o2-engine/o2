#include "o2/stdafx.h"
#include "CapsuleCollider3D.h"

#include "o2/Physics/Box3DConvert.h"
#include "o2/Render/Gizmos.h"

namespace o2
{
    CapsuleCollider3D::CapsuleCollider3D()
    {}

    CapsuleCollider3D::CapsuleCollider3D(const CapsuleCollider3D& other):
        ICollider3D(other), mRadius(other.mRadius), mHeight(other.mHeight)
    {}

    CapsuleCollider3D& CapsuleCollider3D::operator=(const CapsuleCollider3D& other)
    {
        ICollider3D::operator=(other);
        mRadius = other.mRadius;
        mHeight = other.mHeight;
        return *this;
    }

    void CapsuleCollider3D::SetRadius(float radius)
    {
        mRadius = radius;
        OnShapeChanged();
    }

    float CapsuleCollider3D::GetRadius() const
    {
        return mRadius;
    }

    void CapsuleCollider3D::SetHeight(float height)
    {
        mHeight = height;
        OnShapeChanged();
    }

    float CapsuleCollider3D::GetHeight() const
    {
        return mHeight;
    }

    String CapsuleCollider3D::GetName()
    {
        return "Capsule collider 3D";
    }

    String CapsuleCollider3D::GetCategory()
    {
        return "Physics 3D";
    }

    bool CapsuleCollider3D::IsAvailableFromCreateMenu()
    {
        return true;
    }

    b3ShapeId CapsuleCollider3D::CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale)
    {
        Vec3F center = FromBox3D(relative.p);
        Quat  rot = FromBox3D(relative.q);
        Vec3F axis = rot*Vec3F(0, 1, 0); // capsule aligned with local Y

        float halfSeg = mHeight*0.5f*invScale;

        b3Capsule capsule;
        capsule.center1 = ToBox3D(center - axis*halfSeg);
        capsule.center2 = ToBox3D(center + axis*halfSeg);
        capsule.radius = Math::Max(mRadius*invScale, 0.01f);

        return b3CreateCapsuleShape(body, &def, &capsule);
    }

#if IS_EDITOR
    void CapsuleCollider3D::OnDrawGizmos()
    {
        auto owner = mOwner.Lock();
        if (!owner)
            return;

        Quat rot = owner->transform->GetWorldRotation();

        o2Gizmos.SetColor(Gizmos::colliderColor);
        o2Gizmos.DrawCapsule(owner->transform->GetWorldPosition(), rot*Vec3F(0, 1, 0), mRadius, mHeight);
    }
#endif
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::CapsuleCollider3D>);
// --- META ---

DECLARE_CLASS(o2::CapsuleCollider3D, o2__CapsuleCollider3D);
// --- END META ---
