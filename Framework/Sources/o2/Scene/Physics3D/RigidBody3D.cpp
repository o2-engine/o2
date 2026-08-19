#include "o2/stdafx.h"
#include "RigidBody3D.h"

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/Box3DConvert.h"
#include "o2/Physics/PhysicsWorld3D.h"
#include "o2/Scene/Physics3D/ICollider3D.h"

namespace o2
{
    // Tolerances are relative and stay well above float epsilon (1.2e-7): a tighter test can't hold for
    // values that round-trip through the actor's transform, and would report every resting body as moved

    static bool ApproxEqual(const Vec3F& a, const Vec3F& b)
    {
        float toleranceSq = Math::Max(a.SqrLength(), b.SqrLength())*1e-12f;
        return (a - b).SqrLength() <= Math::Max(toleranceSq, 1e-8f);
    }

    static bool ApproxEqual(const Quat& a, const Quat& b)
    {
        float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;

        // Compared against the quaternion lengths, since the actor stores rotation as euler angles and
        // Quat::FromEuler returns a quaternion slightly off unit length
        float lengthsSq = (a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w)*(b.x*b.x + b.y*b.y + b.z*b.z + b.w*b.w);
        return dot*dot >= lengthsSq*(1.0f - 1e-6f); // quaternion double-cover safe
    }

    RigidBody3D::RigidBody3D(RefCounter* refCounter):
        Actor(refCounter), mBodyId{}
    {}

    RigidBody3D::RigidBody3D(RefCounter* refCounter, const RigidBody3D& other):
        Actor(refCounter, other), mBodyId{}, mBodyType(other.mBodyType), mLinearDamping(other.mLinearDamping),
        mAngularDamping(other.mAngularDamping), mGravityScale(other.mGravityScale), mIsBullet(other.mIsBullet),
        mIsFixedRotation(other.mIsFixedRotation)
    {}

    RigidBody3D::~RigidBody3D()
    {
        if (mHasBody)
            RemoveBody();
    }

    RigidBody3D& RigidBody3D::operator=(const RigidBody3D& other)
    {
        if (mHasBody)
            RemoveBody();

        mBodyType = other.mBodyType;
        mLinearDamping = other.mLinearDamping;
        mAngularDamping = other.mAngularDamping;
        mGravityScale = other.mGravityScale;
        mIsBullet = other.mIsBullet;
        mIsFixedRotation = other.mIsFixedRotation;

        if (IsOnScene())
            CreateBody();

        return *this;
    }

    b3BodyType RigidBody3D::GetBox3DBodyType(Type type)
    {
        return type == Type::Dynamic ? b3_dynamicBody : (type == Type::Kinematic ? b3_kinematicBody : b3_staticBody);
    }

    void RigidBody3D::SetBodyType(Type type)
    {
        mBodyType = type;

        if (mHasBody)
            b3Body_SetType(mBodyId, GetBox3DBodyType(type));
    }

    RigidBody3D::Type RigidBody3D::GetBodyType() const
    {
        return mBodyType;
    }

    void RigidBody3D::SetLinearVelocity(const Vec3F& velocity)
    {
        if (mHasBody)
            b3Body_SetLinearVelocity(mBodyId, ToBox3D(velocity/o2Config.physics3D.scale));
    }

    Vec3F RigidBody3D::GetLinearVelocity() const
    {
        if (mHasBody)
            return FromBox3D(b3Body_GetLinearVelocity(mBodyId))*o2Config.physics3D.scale;

        return Vec3F();
    }

    void RigidBody3D::SetAngularVelocity(const Vec3F& velocity)
    {
        if (mHasBody)
            b3Body_SetAngularVelocity(mBodyId, ToBox3D(velocity));
    }

    Vec3F RigidBody3D::GetAngularVelocity() const
    {
        if (mHasBody)
            return FromBox3D(b3Body_GetAngularVelocity(mBodyId));

        return Vec3F();
    }

    void RigidBody3D::SetLinearDamping(float damping)
    {
        mLinearDamping = damping;

        if (mHasBody)
            b3Body_SetLinearDamping(mBodyId, damping);
    }

    float RigidBody3D::GetLinearDamping() const
    {
        return mLinearDamping;
    }

    void RigidBody3D::SetAngularDamping(float damping)
    {
        mAngularDamping = damping;

        if (mHasBody)
            b3Body_SetAngularDamping(mBodyId, damping);
    }

    float RigidBody3D::GetAngularDamping() const
    {
        return mAngularDamping;
    }

    void RigidBody3D::SetGravityScale(float scale)
    {
        mGravityScale = scale;

        if (mHasBody)
            b3Body_SetGravityScale(mBodyId, scale);
    }

    float RigidBody3D::GetGravityScale() const
    {
        return mGravityScale;
    }

    void RigidBody3D::SetIsBullet(bool isBullet)
    {
        mIsBullet = isBullet;

        if (mHasBody)
            b3Body_SetBullet(mBodyId, isBullet);
    }

    bool RigidBody3D::IsBullet() const
    {
        return mIsBullet;
    }

    void RigidBody3D::SetIsSleeping(bool isSleeping)
    {
        if (mHasBody)
            b3Body_SetAwake(mBodyId, !isSleeping);
    }

    bool RigidBody3D::IsSleeping() const
    {
        if (mHasBody)
            return !b3Body_IsAwake(mBodyId);

        return false;
    }

    void RigidBody3D::SetIsFixedRotation(bool isFixedRotation)
    {
        mIsFixedRotation = isFixedRotation;

        if (mHasBody)
        {
            b3MotionLocks locks = { false, false, false, isFixedRotation, isFixedRotation, isFixedRotation };
            b3Body_SetMotionLocks(mBodyId, locks);
        }
    }

    bool RigidBody3D::IsFixedRotation() const
    {
        return mIsFixedRotation;
    }

    b3BodyId RigidBody3D::GetBodyId() const
    {
        return mBodyId;
    }

    void RigidBody3D::OnEnabled()
    {
        Actor::OnEnabled();

        if (mHasBody)
            b3Body_Enable(mBodyId);
    }

    void RigidBody3D::OnDisabled()
    {
        Actor::OnDisabled();

        if (mHasBody)
            b3Body_Disable(mBodyId);
    }

    void RigidBody3D::OnAddToScene()
    {
        CreateBody();
        Actor::OnAddToScene();
    }

    void RigidBody3D::OnRemoveFromScene()
    {
        RemoveBody();
        Actor::OnRemoveFromScene();
    }

    void RigidBody3D::CreateBody()
    {
        if (mHasBody)
            RemoveBody();

        float invScale = 1.0f/o2Config.physics3D.scale;

        b3BodyDef def = b3DefaultBodyDef();
        def.type = GetBox3DBodyType(mBodyType);
        def.position = ToBox3D(transform->GetWorldPosition()*invScale);
        def.rotation = ToBox3D(transform->GetWorldRotation());
        def.linearDamping = mLinearDamping;
        def.angularDamping = mAngularDamping;
        def.gravityScale = mGravityScale;
        def.isBullet = mIsBullet;
        def.isEnabled = mResEnabledInHierarchy;
        def.userData = this;

        if (mIsFixedRotation)
            def.motionLocks = b3MotionLocks{ false, false, false, true, true, true };

        mBodyId = b3CreateBody(o2Physics3D.GetWorldId(), &def);
        mHasBody = true;

        mLastSyncPos = transform->GetWorldPosition();
        mLastSyncRot = transform->GetWorldRotation();

        o2Physics3D.Register(this);

        // Re-attach colliders that registered before the body existed (e.g. after a body recreation)
        for (auto& c : mColliders)
        {
            if (auto collider = c.Lock())
                collider->AddToRigidBody(this);
        }
    }

    void RigidBody3D::RemoveBody()
    {
        if (!mHasBody)
            return;

        o2Physics3D.Unregister(this);

        // Box3D destroys the body's shapes with it; clear the colliders' handles so they don't double-free
        for (auto& c : mColliders)
        {
            if (auto collider = c.Lock())
                collider->OnBodyDestroyed();
        }

        b3DestroyBody(mBodyId);
        mBodyId = b3BodyId{};
        mHasBody = false;
    }

    void RigidBody3D::AddCollider(ICollider3D* collider)
    {
        if (mColliders.Contains(Ref(collider)))
            return;

        if (mHasBody)
            collider->AddToRigidBody(this);

        mColliders.Add(Ref(collider));
    }

    void RigidBody3D::RemoveCollider(ICollider3D* collider)
    {
        if (mHasBody)
            collider->RemoveFromRigidBody();

        mColliders.RemoveFirst([&](auto& x) { return x == collider; });
    }

    void RigidBody3D::SyncActorToBody(float invScale)
    {
        if (!mHasBody)
            return;

        Vec3F wp = transform->GetWorldPosition();
        Quat  wr = transform->GetWorldRotation();

        // Only teleport when the actor was moved by something other than the physics step,
        // so a freely simulating body keeps its authoritative Box3D pose (no euler round-trip).
        if (ApproxEqual(wp, mLastSyncPos) && ApproxEqual(wr, mLastSyncRot))
            return;

        b3Body_SetTransform(mBodyId, ToBox3D(wp*invScale), ToBox3D(wr));
        b3Body_SetAwake(mBodyId, true);

        mLastSyncPos = wp;
        mLastSyncRot = wr;
    }

    void RigidBody3D::SyncBodyToActor(float scale)
    {
        if (!mHasBody)
            return;

        if (mBodyType == Type::Static)
            return;

        b3Transform xf = b3Body_GetTransform(mBodyId);

        transform->SetWorldPosition(FromBox3D(xf.p)*scale);
        transform->SetWorldRotation(FromBox3D(xf.q));

        // Remember what the actor now reports (rotation passes through the actor's euler storage),
        // so the next PreUpdate only re-teleports the body on a genuine external move.
        mLastSyncPos = transform->GetWorldPosition();
        mLastSyncRot = transform->GetWorldRotation();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::RigidBody3D>);
// --- META ---

ENUM_META(o2::RigidBody3D::Type, o2__RigidBody3D__Type)
{
    ENUM_ENTRY(Dynamic);
    ENUM_ENTRY(Kinematic);
    ENUM_ENTRY(Static);
}
END_ENUM_META;

DECLARE_CLASS(o2::RigidBody3D, o2__RigidBody3D);
// --- END META ---
