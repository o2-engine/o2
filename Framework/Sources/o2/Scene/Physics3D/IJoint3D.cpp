#include "o2/stdafx.h"
#include "IJoint3D.h"

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/Box3DConvert.h"
#include "o2/Physics/PhysicsWorld3D.h"
#include "o2/Scene/Actor.h"

namespace o2
{
    IJoint3D::IJoint3D():
        mJointId{}
    {}

    IJoint3D::IJoint3D(const IJoint3D& other):
        Component(other), mJointId{}, mBodyA(other.mBodyA), mBodyB(other.mBodyB),
        mCollideConnected(other.mCollideConnected)
    {}

    IJoint3D::~IJoint3D()
    {
        RemoveJoint();
    }

    IJoint3D& IJoint3D::operator=(const IJoint3D& other)
    {
        Component::operator=(other);

        mBodyA = other.mBodyA;
        mBodyB = other.mBodyB;
        mCollideConnected = other.mCollideConnected;

        RebuildJoint();
        return *this;
    }

    void IJoint3D::SetBodyA(const LinkRef<RigidBody3D>& body) { mBodyA = body; RebuildJoint(); }
    const LinkRef<RigidBody3D>& IJoint3D::GetBodyA() const { return mBodyA; }

    void IJoint3D::SetBodyB(const LinkRef<RigidBody3D>& body) { mBodyB = body; RebuildJoint(); }
    const LinkRef<RigidBody3D>& IJoint3D::GetBodyB() const { return mBodyB; }

    void IJoint3D::SetCollideConnected(bool value) { mCollideConnected = value; RebuildJoint(); }
    bool IJoint3D::GetCollideConnected() const { return mCollideConnected; }

    bool IJoint3D::IsAvailableFromCreateMenu() { return false; }

    b3JointId IJoint3D::CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB) { return b3JointId{}; }

    b3Transform IJoint3D::ComputeLocalFrame(RigidBody3D* body) const
    {
        float invScale = 1.0f/o2Config.physics3D.scale;

        Vec3F jointPos = mOwner.Lock()->transform->GetWorldPosition()*invScale;
        Quat  jointRot = mOwner.Lock()->transform->GetWorldRotation();

        Vec3F bodyPos = body->transform->GetWorldPosition()*invScale;
        Quat  bodyRotInv = body->transform->GetWorldRotation().Inverted();

        Vec3F localPos = bodyRotInv*(jointPos - bodyPos);
        Quat  localRot = bodyRotInv*jointRot;

        return b3Transform{ ToBox3D(localPos), ToBox3D(localRot) };
    }

    void IJoint3D::SetupBaseDef(b3JointDef& def, RigidBody3D* bodyA, RigidBody3D* bodyB)
    {
        def.bodyIdA = bodyA->GetBodyId();
        def.bodyIdB = bodyB->GetBodyId();
        def.localFrameA = ComputeLocalFrame(bodyA);
        def.localFrameB = ComputeLocalFrame(bodyB);
        def.collideConnected = mCollideConnected;
    }

    void IJoint3D::RebuildJoint()
    {
        RemoveJoint();

        Ref<RigidBody3D> a = mBodyA;
        Ref<RigidBody3D> b = mBodyB;
        if (!a || !b)
            return;

        if (!b3Body_IsValid(a->GetBodyId()) || !b3Body_IsValid(b->GetBodyId()))
            return;

        mJointId = CreateJoint(a.Get(), b.Get());
        mHasJoint = b3Joint_IsValid(mJointId);
    }

    void IJoint3D::RemoveJoint()
    {
        if (mHasJoint && b3Joint_IsValid(mJointId))
            b3DestroyJoint(mJointId, false);

        mJointId = b3JointId{};
        mHasJoint = false;
    }

    void IJoint3D::OnStart()
    {
        RebuildJoint();
    }

    void IJoint3D::OnUpdate(float dt)
    {
        if (!mHasJoint)
            RebuildJoint(); // retry until both referenced bodies have their Box3D body
    }

    void IJoint3D::OnRemoveFromScene()
    {
        RemoveJoint();
        Component::OnRemoveFromScene();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::IJoint3D>);
// --- META ---

DECLARE_CLASS(o2::IJoint3D, o2__IJoint3D);
// --- END META ---
