#include "o2/stdafx.h"
#include "SplineCollider.h"

#include "Box2D/Collision/Shapes/b2ChainShape.h"

namespace o2
{
    SplineCollider::SplineCollider()
    {
        InitSpline();
    }

    SplineCollider::SplineCollider(const SplineCollider& other):
        ICollider(other), spline(other.spline), mIsLoop(other.mIsLoop)
    {
        spline->onKeysChanged = THIS_FUNC(OnSplineChanged);
        spline->SetClosed(mIsLoop);
    }

    SplineCollider::~SplineCollider()
    {
        delete mShape;
    }

    SplineCollider& SplineCollider::operator=(const SplineCollider& other)
    {
        ICollider::operator=(other);
        spline = other.spline;
        mIsLoop = other.mIsLoop;
        spline->onKeysChanged = THIS_FUNC(OnSplineChanged);
        spline->SetClosed(mIsLoop);
        OnShapeChanged();
        return *this;
    }

    Ref<Spline> SplineCollider::GetSpline() const
    {
        return spline;
    }

    void SplineCollider::SetIsLoop(bool loop)
    {
        if (mIsLoop == loop)
            return;

        mIsLoop = loop;
        spline->SetClosed(loop);
        OnShapeChanged();
    }

    bool SplineCollider::IsLoop() const
    {
        return mIsLoop;
    }

    String SplineCollider::GetName()
    {
        return "Spline collider";
    }

    String SplineCollider::GetCategory()
    {
        return "Physics";
    }

    bool SplineCollider::IsAvailableFromCreateMenu()
    {
        return true;
    }

#if IS_EDITOR
    void SplineCollider::OnAddedFromEditor()
    {
        InitSpline();
        OnShapeChanged();
    }
#endif

    void SplineCollider::InitSpline()
    {
        if (spline->GetKeys().IsEmpty())
        {
            spline->BeginKeysBatchChange();
            spline->AppendKey(Vec2F(-100.0f, 0.0f));
            spline->AppendKey(Vec2F(100.0f, 0.0f));

            for (int i = 0; i < 2; i++)
            {
                auto key = spline->GetKey(i);
                key.supportsType = Spline::Key::Type::Broken;
                key.prevSupport = Vec2F();
                key.nextSupport = Vec2F();
                spline->SetKey(key, i);
            }
            spline->CompleteKeysBatchingChange();
        }

        spline->onKeysChanged = THIS_FUNC(OnSplineChanged);
        spline->SetClosed(mIsLoop);
    }

    void SplineCollider::OnSplineChanged()
    {
        OnShapeChanged();
    }

    void SplineCollider::OnAddToScene()
    {
        spline->onKeysChanged = THIS_FUNC(OnSplineChanged);
        spline->SetClosed(mIsLoop);
        ICollider::OnAddToScene();
    }

    b2Shape* SplineCollider::GetShape(const Basis& transform)
    {
        if (!spline)
            return nullptr;

        const auto& keys = spline->GetKeys();
        if (keys.Count() < 2)
            return nullptr;

        // mLeftApproxValues[i] of key K stores the bezier segment from key K-1 to key K
        // (precomputed at 20 points). Index 0 is the position at key K-1, last is at key K.
        // Skip index 0 on subsequent segments to avoid duplicate vertices at shared endpoints.
        Vector<b2Vec2> verts;
        for (int i = 1; i < keys.Count(); i++)
        {
            const ApproximationVec2F* approx = keys[i].GetApproximatedPointsLeft();
            int count = keys[i].GetApproximatedPointsCount();
            int start = (i == 1) ? 0 : 1;
            for (int j = start; j < count; j++)
            {
                Vec2F p = approx[j].value * transform;
                verts.Add(b2Vec2(p.x, p.y));
            }
        }

        if (mIsLoop && verts.Count() > 1)
        {
            const b2Vec2& first = verts[0];
            const b2Vec2& last = verts.Last();
            if (Math::Equals(first.x, last.x) && Math::Equals(first.y, last.y))
                verts.PopBack();
        }

        if (verts.Count() < 2)
            return nullptr;

        delete mShape;
        mShape = mnew b2ChainShape();
        if (mIsLoop)
            mShape->CreateLoop(verts.Data(), verts.Count());
        else
            mShape->CreateChain(verts.Data(), verts.Count());

        return mShape;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SplineCollider>);
// --- META ---

DECLARE_CLASS(o2::SplineCollider, o2__SplineCollider);
// --- END META ---
