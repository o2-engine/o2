#include "o2/stdafx.h"
#include "BoxCollider.h"

#include "o2/Physics/PhysicsWorld.h"
#include "o2/Render/Gizmos.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    BoxCollider::BoxCollider()
    {}

    BoxCollider::BoxCollider(const BoxCollider& other):
        ICollider(other), mSize(other.mSize), mFitByActor(other.mFitByActor)
    {}

    void BoxCollider::SetSize(const Vec2F& size)
    {
        mSize = size;
        OnShapeChanged();
    }

    Vec2F BoxCollider::GetSize() const
    {
        return mSize;
    }

    void BoxCollider::SetFitByActor(bool fit)
    {
        mFitByActor = fit;

        if (fit)
        {
            mSize = mOwner.Lock()->transform->GetSize2D();
            OnShapeChanged();
        }
    }

    bool BoxCollider::IsFitByActor() const
    {
        return mFitByActor;
    }

    String BoxCollider::GetName()
    {
        return "Box collider";
    }

    String BoxCollider::GetCategory()
    {
        return "Physics";
    }

    bool BoxCollider::IsAvailableFromCreateMenu()
    {
        return true;
    }

    BoxCollider& BoxCollider::operator=(const BoxCollider& other)
    {
        ICollider::operator=(other);
        mSize = other.mSize;
        mFitByActor = other.mFitByActor;
        return *this;
    }

    b2Shape* BoxCollider::GetShape(const Basis& transform)
    {
        Vec2F halfSize = mSize*0.5f;

        if (halfSize.x < 0.001f)
            halfSize.x = 1.0f;

        if (halfSize.y < 0.001f)
            halfSize.y = 1.0f;

        b2Vec2 verticies[4];
        verticies[0] = Vec2F(-halfSize.x, -halfSize.y)*transform;
        verticies[1] = Vec2F(halfSize.x, -halfSize.y)*transform;
        verticies[2] = Vec2F(halfSize.x, halfSize.y)*transform;
        verticies[3] = Vec2F(-halfSize.x, halfSize.y)*transform;

        mShape.Set(verticies, 4);

        return &mShape;
    }

    void BoxCollider::OnTransformUpdated()
    {
#if IS_EDITOR
        if (o2Scene.IsEditor() && mFitByActor && !o2Scene.IsEditorPlaying())
            FitSize();
#endif

        ICollider::OnTransformUpdated();
    }

    void BoxCollider::OnAddToScene()
    {
        if (mFitByActor)
            FitSize();

        ICollider::OnAddToScene();
    }

    void BoxCollider::FitSize()
    {
        Vec2F prevSize = mSize;
        mSize = mOwner.Lock()->transform->GetSize2D();

        if (prevSize != mSize)
            OnShapeChanged();
    }

#if IS_EDITOR
    void BoxCollider::OnAddedFromEditor()
    {
        mSize = mOwner.Lock()->transform->GetSize2D();
        OnShapeChanged();
    }

    void BoxCollider::OnDrawGizmos()
    {
        auto owner = mOwner.Lock();
        if (!owner)
            return;

        Basis basis = owner->transform->GetWorldNonSizedBasis();
        float z = owner->transform->GetWorldPosition().z;
        Vec2F half = mSize*0.5f;

        o2Gizmos.SetColor(Gizmos::colliderColor);
        o2Gizmos.DrawPolyLine({ Vec3F(Vec2F(-half.x, -half.y)*basis, z), Vec3F(Vec2F(half.x, -half.y)*basis, z),
                                Vec3F(Vec2F(half.x, half.y)*basis, z), Vec3F(Vec2F(-half.x, half.y)*basis, z) },
                              true);
    }
#endif
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::BoxCollider>);
// --- META ---

DECLARE_CLASS(o2::BoxCollider, o2__BoxCollider);
// --- END META ---
