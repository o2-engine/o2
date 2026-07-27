#include "o2/stdafx.h"
#include "BoxCollider3D.h"

namespace o2
{
    BoxCollider3D::BoxCollider3D()
    {}

    BoxCollider3D::BoxCollider3D(const BoxCollider3D& other):
        ICollider3D(other), mSize(other.mSize)
    {}

    BoxCollider3D& BoxCollider3D::operator=(const BoxCollider3D& other)
    {
        ICollider3D::operator=(other);
        mSize = other.mSize;
        return *this;
    }

    void BoxCollider3D::SetSize(const Vec3F& size)
    {
        mSize = size;
        OnShapeChanged();
    }

    Vec3F BoxCollider3D::GetSize() const
    {
        return mSize;
    }

    String BoxCollider3D::GetName()
    {
        return "Box collider 3D";
    }

    String BoxCollider3D::GetCategory()
    {
        return "Physics 3D";
    }

    bool BoxCollider3D::IsAvailableFromCreateMenu()
    {
        return true;
    }

    b3ShapeId BoxCollider3D::CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale)
    {
        Vec3F half = mSize*0.5f*invScale;

        half.x = Math::Max(half.x, 0.01f);
        half.y = Math::Max(half.y, 0.01f);
        half.z = Math::Max(half.z, 0.01f);

        b3BoxHull hull = b3MakeTransformedBoxHull(half.x, half.y, half.z, relative);
        return b3CreateHullShape(body, &def, &hull.base);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::BoxCollider3D>);
// --- META ---

DECLARE_CLASS(o2::BoxCollider3D, o2__BoxCollider3D);
// --- END META ---
