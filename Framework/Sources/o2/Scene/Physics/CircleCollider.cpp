#include "o2/stdafx.h"
#include "CircleCollider.h"

#include "o2/Physics/PhysicsWorld.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    CircleCollider::CircleCollider()
    {}

    CircleCollider::CircleCollider(const CircleCollider& other):
        ICollider(other), mRadius(other.mRadius), mFitByActor(other.mFitByActor)
    {}

    void CircleCollider::SetRadius(float radius)
    {
        mRadius = radius;
        OnShapeChanged();
    }

    float CircleCollider::GetRadius() const
    {
        return mRadius;
    }

    void CircleCollider::SetFitByActor(bool fit)
    {
        mFitByActor = fit;

        if (fit)
        {
            mRadius = mOwner.Lock()->transform->GetSize().x*0.5f;
            OnShapeChanged();
        }
    }

    bool CircleCollider::IsFitByActor() const
    {
        return mFitByActor;
    }

    String CircleCollider::GetName()
    {
        return "Circle collider";
    }

    String CircleCollider::GetCategory()
    {
        return "Physics";
    }

    bool CircleCollider::IsAvailableFromCreateMenu()
    {
        return true;
    }

    CircleCollider& CircleCollider::operator=(const CircleCollider& other)
    {
        ICollider::operator=(other);
        mRadius = other.mRadius;
        mFitByActor = other.mFitByActor;
        return *this;
    }

    b2Shape* CircleCollider::GetShape(const Basis& transform)
    {
        float radius = mRadius;
        if (radius < FLT_EPSILON)
            radius = 1.0f;

        mShape.m_radius = radius*transform.xv.Length();
        mShape.m_p = transform.origin;

        return &mShape;
    }

    void CircleCollider::OnTransformUpdated()
    {
#if IS_EDITOR
        if (mFitByActor && !o2Scene.IsEditorPlaying())
        {
            float prevRadius = mRadius;
            mRadius = mOwner.Lock()->transform->GetSize().x*0.5f;
        }
#endif

        ICollider::OnTransformUpdated();
    }

#if IS_EDITOR
    void CircleCollider::OnAddedFromEditor()
    {
        mRadius = mOwner.Lock()->transform->GetSize().x*0.5f;
    }
#endif
}

DECLARE_TEMPLATE_CLASS(o2::ComponentRef<o2::CircleCollider>);
// --- META ---

DECLARE_CLASS(o2::CircleCollider, o2__CircleCollider);
// --- END META ---
