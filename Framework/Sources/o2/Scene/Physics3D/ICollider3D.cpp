#include "o2/stdafx.h"
#include "ICollider3D.h"

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/Box3DConvert.h"
#include "o2/Physics/PhysicsWorld3D.h"
#include "o2/Scene/Physics3D/RigidBody3D.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    ICollider3D::ICollider3D():
        mShapeId{}
    {}

    ICollider3D::ICollider3D(const ICollider3D& other):
        Component(other), mShapeId{}, mFriction(other.mFriction), mDensity(other.mDensity),
        mRestitution(other.mRestitution), mIsSensor(other.mIsSensor)
    {}

    ICollider3D::~ICollider3D()
    {
        RemoveFromRigidBody();
    }

    ICollider3D& ICollider3D::operator=(const ICollider3D& other)
    {
        Component::operator=(other);

        mFriction = other.mFriction;
        mDensity = other.mDensity;
        mRestitution = other.mRestitution;
        mIsSensor = other.mIsSensor;

        OnShapeChanged();

        return *this;
    }

    void ICollider3D::SetFriction(float value)
    {
        mFriction = value;

        if (mHasShape)
            b3Shape_SetFriction(mShapeId, value);
    }

    float ICollider3D::GetFriction() const
    {
        return mFriction;
    }

    void ICollider3D::SetDensity(float value)
    {
        mDensity = value;

        if (mHasShape)
            b3Shape_SetDensity(mShapeId, value, true);
    }

    float ICollider3D::GetDensity() const
    {
        return mDensity;
    }

    void ICollider3D::SetRestitution(float value)
    {
        mRestitution = value;

        if (mHasShape)
            b3Shape_SetRestitution(mShapeId, value);
    }

    float ICollider3D::GetRestitution() const
    {
        return mRestitution;
    }

    void ICollider3D::SetIsSensor(bool value)
    {
        mIsSensor = value;
        OnShapeChanged(); // sensor flag is fixed at shape creation, so rebuild the shape
    }

    bool ICollider3D::IsSensor() const
    {
        return mIsSensor;
    }

    bool ICollider3D::IsAvailableFromCreateMenu()
    {
        return false;
    }

    b3ShapeId ICollider3D::CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale)
    {
        return b3ShapeId{};
    }

    b3ShapeDef ICollider3D::MakeShapeDef()
    {
        b3ShapeDef def = b3DefaultShapeDef();
        def.density = mDensity;
        def.baseMaterial.friction = mFriction;
        def.baseMaterial.restitution = mRestitution;
        def.userData = this;
        def.isSensor = mIsSensor;
        return def;
    }

    b3Transform ICollider3D::GetRelativeTransform(RigidBody3D* body, float invScale) const
    {
        auto thisTransform = mOwner.Lock()->transform;
        auto bodyTransform = body->transform;

        Basis3D thisBasis = thisTransform->GetWorldNonSizedBasis3D();
        Basis3D bodyBasis = bodyTransform->GetWorldNonSizedBasis3D();

        Basis3D relative = thisBasis*bodyBasis.Inverted();

        Vec3F pos; Quat rot; Vec3F scale;
        relative.Decompose(&pos, &rot, &scale);

        return b3Transform{ ToBox3D(pos*invScale), ToBox3D(rot) };
    }

    void ICollider3D::AddToRigidBody(RigidBody3D* body)
    {
        if (!body->mHasBody)
            return;

        float invScale = 1.0f/o2Config.physics3D.scale;
        b3Transform relative = GetRelativeTransform(body, invScale);
        b3ShapeDef def = MakeShapeDef();

        mShapeId = CreateShape(body->mBodyId, def, relative, invScale);
        mHasShape = b3Shape_IsValid(mShapeId);
        mRigidBodyComp = body;
    }

    void ICollider3D::RemoveFromRigidBody()
    {
        if (mHasShape && b3Shape_IsValid(mShapeId))
            b3DestroyShape(mShapeId, true);

        mShapeId = b3ShapeId{};
        mHasShape = false;
        mRigidBodyComp = nullptr;
    }

    void ICollider3D::OnBodyDestroyed()
    {
        mShapeId = b3ShapeId{};
        mHasShape = false;
        mRigidBodyComp = nullptr;
    }

    Ref<RigidBody3D> ICollider3D::FindRigidBody() const
    {
        auto itActor = mOwner.Lock();
        while (itActor)
        {
            if (auto body = DynamicCast<RigidBody3D>(itActor))
                return body;

            itActor = itActor->GetParent().Lock();
        }

        return nullptr;
    }

    void ICollider3D::OnShapeChanged()
    {
        if (auto rigidBody = FindRigidBody())
        {
            rigidBody->RemoveCollider(this);
            rigidBody->AddCollider(this);
        }
    }

    void ICollider3D::OnTransformUpdated()
    {
#if IS_EDITOR
        // Runtime reshape on transform update exists only to support edit-time manipulation in the
        // editor. In a standalone runner this would fire every physics step, destroying and recreating
        // the shape and wiping Box3D's contact cache.
        if (!o2Scene.IsEditor())
            return;

        if (o2Scene.IsEditorPlaying())
            return;

        OnShapeChanged();
#endif
    }

    void ICollider3D::OnAddToScene()
    {
        if (auto rigidBody = FindRigidBody())
            rigidBody->AddCollider(this);

        Component::OnAddToScene();
    }

    void ICollider3D::OnRemoveFromScene()
    {
        RemoveFromRigidBody();
        Component::OnRemoveFromScene();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::ICollider3D>);
// --- META ---

DECLARE_CLASS(o2::ICollider3D, o2__ICollider3D);
// --- END META ---
