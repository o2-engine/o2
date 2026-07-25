#include "o2/stdafx.h"
#include "WeldJoint.h"

#include "Box2D/Dynamics/Joints/b2WeldJoint.h"
#include "o2/Physics/PhysicsWorld.h"

namespace o2
{
    WeldJoint::WeldJoint()
    {}

    WeldJoint::WeldJoint(const WeldJoint& other):
        IJoint(other), mFrequency(other.mFrequency), mDampingRatio(other.mDampingRatio)
    {}

    WeldJoint& WeldJoint::operator=(const WeldJoint& other)
    {
        IJoint::operator=(other);
        mFrequency = other.mFrequency;
        mDampingRatio = other.mDampingRatio;
        return *this;
    }

    void WeldJoint::SetFrequency(float hz) { mFrequency = hz; RebuildJoint(); }
    float WeldJoint::GetFrequency() const { return mFrequency; }

    void WeldJoint::SetDampingRatio(float ratio) { mDampingRatio = ratio; RebuildJoint(); }
    float WeldJoint::GetDampingRatio() const { return mDampingRatio; }

    String WeldJoint::GetName() { return "Weld joint"; }
    String WeldJoint::GetCategory() { return "Physics/Joints"; }
    bool WeldJoint::IsAvailableFromCreateMenu() { return true; }

    b2Joint* WeldJoint::CreateJoint(b2Body* bodyA, b2Body* bodyB)
    {
        b2WeldJointDef def;
        def.Initialize(bodyA, bodyB, GetPhysicsAnchor());

        def.frequencyHz = mFrequency;
        def.dampingRatio = mDampingRatio;

        SetupBaseDef(def, bodyA, bodyB);
        return o2Physics.CreateJoint(def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::WeldJoint>);
// --- META ---

DECLARE_CLASS(o2::WeldJoint, o2__WeldJoint);
// --- END META ---
