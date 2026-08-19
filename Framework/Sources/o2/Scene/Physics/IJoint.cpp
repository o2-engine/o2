#include "o2/stdafx.h"
#include "IJoint.h"

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/PhysicsWorld.h"
#include "o2/Render/Gizmos.h"
#include "o2/Scene/Actor.h"

namespace o2
{
    IJoint::IJoint()
    {}

    IJoint::IJoint(const IJoint& other):
        Component(other), mBodyA(other.mBodyA), mBodyB(other.mBodyB), mCollideConnected(other.mCollideConnected)
    {}

    IJoint::~IJoint()
    {
        RemoveJoint();
    }

    IJoint& IJoint::operator=(const IJoint& other)
    {
        Component::operator=(other);

        mBodyA = other.mBodyA;
        mBodyB = other.mBodyB;
        mCollideConnected = other.mCollideConnected;

        RebuildJoint();
        return *this;
    }

    void IJoint::SetBodyA(const LinkRef<RigidBody>& body)
    {
        mBodyA = body;
        RebuildJoint();
    }

    const LinkRef<RigidBody>& IJoint::GetBodyA() const
    {
        return mBodyA;
    }

    void IJoint::SetBodyB(const LinkRef<RigidBody>& body)
    {
        mBodyB = body;
        RebuildJoint();
    }

    const LinkRef<RigidBody>& IJoint::GetBodyB() const
    {
        return mBodyB;
    }

    void IJoint::SetCollideConnected(bool value)
    {
        mCollideConnected = value;
        RebuildJoint();
    }

    bool IJoint::GetCollideConnected() const
    {
        return mCollideConnected;
    }

    bool IJoint::IsAvailableFromCreateMenu()
    {
        return false;
    }

    b2Joint* IJoint::CreateJoint(b2Body* bodyA, b2Body* bodyB)
    {
        return nullptr;
    }

    void IJoint::SetupBaseDef(b2JointDef& def, b2Body* bodyA, b2Body* bodyB)
    {
        def.bodyA = bodyA;
        def.bodyB = bodyB;
        def.collideConnected = mCollideConnected;
    }

    Vec2F IJoint::GetPhysicsAnchor() const
    {
        return mOwner.Lock()->transform->GetWorldPosition2D()/o2Config.physics.scale;
    }

    void IJoint::RebuildJoint()
    {
        RemoveJoint();

        auto bodyA = mBodyA ? mBodyA->GetBody() : nullptr;
        auto bodyB = mBodyB ? mBodyB->GetBody() : nullptr;
        if (!bodyA || !bodyB)
            return;

        mJoint = CreateJoint(bodyA, bodyB);
        mJointBodyA = bodyA;
        mJointBodyB = bodyB;
    }

    void IJoint::RemoveJoint()
    {
        if (!mJoint)
            return;

        auto bodyA = mBodyA ? mBodyA->GetBody() : nullptr;
        auto bodyB = mBodyB ? mBodyB->GetBody() : nullptr;

        // Only destroy when the exact bodies the joint was created with are still alive; otherwise
        // Box2D already destroyed the joint together with a dead body, so our handle is dangling.
        if (bodyA && bodyB && bodyA == mJointBodyA && bodyB == mJointBodyB)
            o2Physics.DestroyJoint(mJoint);

        mJoint = nullptr;
        mJointBodyA = nullptr;
        mJointBodyB = nullptr;
    }

    void IJoint::OnStart()
    {
        RebuildJoint();
    }

    void IJoint::OnUpdate(float dt)
    {
        if (!mJoint)
            RebuildJoint(); // retry until both referenced bodies have their Box2D body
    }

    void IJoint::OnRemoveFromScene()
    {
        RemoveJoint();
        Component::OnRemoveFromScene();
    }

#if IS_EDITOR
    void IJoint::OnDrawGizmos()
    {
        auto owner = mOwner.Lock();
        if (!owner)
            return;

        Vec3F anchor = owner->transform->GetWorldPosition();

        o2Gizmos.SetColor(Gizmos::jointColor);

        if (Ref<RigidBody> bodyA = mBodyA)
            o2Gizmos.DrawLine(anchor, bodyA->transform->GetWorldPosition());

        if (Ref<RigidBody> bodyB = mBodyB)
            o2Gizmos.DrawLine(anchor, bodyB->transform->GetWorldPosition());

        o2Gizmos.DrawCircle(anchor, 8.0f, 16);
    }
#endif
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::IJoint>);
// --- META ---

DECLARE_CLASS(o2::IJoint, o2__IJoint);
// --- END META ---
