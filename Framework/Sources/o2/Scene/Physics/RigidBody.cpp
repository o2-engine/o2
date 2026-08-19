#include "o2/stdafx.h"
#include "RigidBody.h"

#include "Box2D/Dynamics/b2Body.h"
#include "o2/Config/ProjectConfig.h"
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
        ApplyMassData();
    }

    float RigidBody::GetMass() const
    {
        return mMass;
    }

    void RigidBody::SetInertia(float inertia)
    {
        mInertia = inertia;
        ApplyMassData();
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

    b2Body* RigidBody::GetBody() const
    {
        return mBody;
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
        def.position = transform->GetWorldPosition2D()/o2Config.physics.scale; // physics units, so joints read a correct pose at creation
        def.userData = this;
        def.active = mResEnabledInHierarchy;

        mBody = PhysicsWorld::Instance().mWorld.CreateBody(&def);
        mBody->SetType(GetBodyType(mBodyType));
        mBody->SetLinearDamping(mLinearDamping);
        mBody->SetAngularDamping(mAngularDamping);
        mBody->SetGravityScale(mGravityScale);
        mBody->SetBullet(mIsBullet);
        mBody->SetFixedRotation(mIsFixedRotation);

        ApplyMassData(); // after the type and fixed rotation flags, both reset mass data
    }

    void RigidBody::ApplyMassData()
    {
        if (!mBody)
            return;

        b2MassData massData;
        massData.center = mBody->GetLocalCenter();
        massData.mass = Math::Max(mMass, 0.0f);

        // Box2D expects inertia around the body origin and subtracts the center offset back,
        // so mInertia is kept as inertia around the center of mass and stays positive
        float bodyMass = massData.mass > 0.0f ? massData.mass : 1.0f;
        massData.I = mInertia > 0.0f ? mInertia + bodyMass*b2Dot(massData.center, massData.center) : 0.0f;

        mBody->SetMassData(&massData);
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
