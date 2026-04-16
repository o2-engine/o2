#include "o2/stdafx.h"
#include "RigidBody.h"

#include "Box2D/Dynamics/b2Body.h"
#include "o2/Physics/PhysicsWorld.h"
#include "o2/Scene/Physics/ICollider.h"

namespace o2
{
    RigidBody::RigidBody(RefCounter* refCounter):
        Actor(refCounter)
    {}

    RigidBody::RigidBody(RefCounter* refCounter, const RigidBody& other):
        Actor(refCounter, other), mBodyType(other.mBodyType), mMass(other.mMass), mInertia(other.mInertia),
        mLinearDamping(other.mLinearDamping), mAngularDamping(other.mAngularDamping), mGravityScale(other.mGravityScale),
        mIsBullet(other.mIsBullet), mIsFixedRotation(other.mIsFixedRotation)
    {}

    RigidBody::~RigidBody()
    {
        if (mBody)
            RemoveBody();
    }

    RigidBody& RigidBody::operator=(const RigidBody& other)
    {
        if (mBody)
            RemoveBody();

        mBodyType = other.mBodyType;
        mMass = other.mMass;
        mInertia = other.mInertia;
        mLinearDamping = other.mLinearDamping;
        mAngularDamping = other.mAngularDamping;
        mGravityScale = other.mGravityScale;
        mIsBullet = other.mIsBullet;
        mIsFixedRotation = other.mIsFixedRotation;

        if (IsOnScene())
            CreateBody();

        return *this;
    }

    void RigidBody::SetBodyType(Type type)
    {
        mBodyType = type;

        if (mBody)
            mBody->SetType(GetBodyType(type));
    }

    RigidBody::Type RigidBody::GetBodyType() const
    {
        return mBodyType;
    }

    b2BodyType RigidBody::GetBodyType(Type type)
    {
        return type == Type::Dynamic ? b2_dynamicBody : (type == Type::Kinematic ? b2_kinematicBody : b2_staticBody);
    }

    void RigidBody::SetMass(float mass)
    {
        mMass = mass;
        mMassData.mass = mMass;
        mMassData.I = mInertia;

        if (mBody)
            mBody->SetMassData(&mMassData);
    }

    float RigidBody::GetMass() const
    {
        return mMass;
    }

    void RigidBody::SetInertia(float inertia)
    {
        mInertia = inertia;
        mMassData.mass = mMass;
        mMassData.I = mInertia;

        if (mBody)
            mBody->SetMassData(&mMassData);
    }

    float RigidBody::GetInertia() const
    {
        return mInertia;
    }

    void RigidBody::SetLinearVelocity(const Vec2F& velocity)
    {
        if (mBody)
            mBody->SetLinearVelocity(velocity);
    }

    Vec2F RigidBody::GetLinearVelocity() const
    {
        if (mBody)
            return mBody->GetLinearVelocity();

        return Vec2F();
    }

    void RigidBody::SetAngularVelocity(float velocity)
    {
        if (mBody)
            mBody->SetAngularVelocity(velocity);
    }

    float RigidBody::GetAngularVelocity() const
    {
        if (mBody)
            return mBody->GetAngularVelocity();

        return 0.0f;
    }

    void RigidBody::SetLinearDamping(float damping)
    {
        mLinearDamping = damping;

        if (mBody)
            mBody->SetLinearDamping(damping);
    }

    float RigidBody::GetLinearDamping() const
    {
        return mLinearDamping;
    }

    void RigidBody::SetAngularDamping(float damping)
    {
        mAngularDamping = damping;

        if (mBody)
            mBody->SetAngularDamping(damping);
    }

    float RigidBody::GetAngularDamping() const
    {
        return mAngularDamping;
    }

    void RigidBody::SetGravityScale(float scale)
    {
        mGravityScale = scale;

        if (mBody)
            mBody->SetGravityScale(scale);
    }

    float RigidBody::GetGravityScale() const
    {
        return mGravityScale;
    }

    void RigidBody::SetIsBullet(bool isBullet)
    {
        mIsBullet = isBullet;

        if (mBody)
            mBody->SetBullet(isBullet);
    }

    bool RigidBody::IsBullet() const
    {
        return mIsBullet;
    }

    void RigidBody::SetIsSleeping(bool isSleeping)
    {
        if (mBody)
            mBody->SetAwake(!isSleeping);
    }

    bool RigidBody::IsSleeping() const
    {
        if (mBody)
            return !mBody->IsAwake();

        return false;
    }

    void RigidBody::SetIsFixedRotation(bool isFixedRotation)
    {
        mIsFixedRotation = isFixedRotation;

        if (mBody)
            mBody->SetFixedRotation(isFixedRotation);
    }

    bool RigidBody::IsFixedRotation() const
    {
        return mIsFixedRotation;
    }

    void RigidBody::OnEnabled()
    {
        Actor::OnEnabled();

        if (mBody)
            mBody->SetActive(true);
    }

    void RigidBody::OnDisabled()
    {
        Actor::OnDisabled();

        if (mBody)
            mBody->SetActive(false);
    }

    void RigidBody::OnAddToScene()
    {
        CreateBody();
        Actor::OnAddToScene();
    }

    void RigidBody::OnRemoveFromScene()
    {
        RemoveBody();
        Actor::OnRemoveFromScene();
    }

    void RigidBody::CreateBody()
    {
        b2BodyDef def;
        def.position = transform->GetWorldPosition();
        def.userData = this;
        def.active = mResEnabledInHierarchy;

        mMassData.mass = mMass;
        mMassData.I = mInertia;

        mBody = PhysicsWorld::Instance().mWorld.CreateBody(&def);
        mBody->SetMassData(&mMassData);
        mBody->SetType(mBodyType == Type::Dynamic ? b2_dynamicBody : (mBodyType == Type::Kinematic ? b2_kinematicBody : b2_staticBody));
        mBody->SetLinearDamping(mLinearDamping);
        mBody->SetAngularDamping(mAngularDamping);
        mBody->SetGravityScale(mGravityScale);
        mBody->SetBullet(mIsBullet);
        mBody->SetFixedRotation(mIsFixedRotation);
    }

    void RigidBody::RemoveBody()
    {
        if (mBody)
        {
            PhysicsWorld::Instance().mWorld.DestroyBody(mBody);
            mBody = nullptr;
        }
    }

    void RigidBody::AddCollider(ICollider* collider)
    {
        if (mColliders.Contains(Ref(collider)))
            return;

        if (mBody)
            collider->AddToRigidBody(this);

        mColliders.Add(Ref(collider));
    }

    void RigidBody::RemoveCollider(ICollider* collider)
    {
        if (mBody)
            collider->RemoveFromRigidBody();

        mColliders.RemoveFirst([&](auto& x) { return x == collider; });
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::RigidBody>);
// --- META ---

ENUM_META(o2::RigidBody::Type, o2__RigidBody__Type)
{
    ENUM_ENTRY(Dynamic);
    ENUM_ENTRY(Kinematic);
    ENUM_ENTRY(Static);
}
END_ENUM_META;

DECLARE_CLASS(o2::RigidBody, o2__RigidBody);
// --- END META ---
