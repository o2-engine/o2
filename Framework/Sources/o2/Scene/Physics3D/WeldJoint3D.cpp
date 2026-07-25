#include "o2/stdafx.h"
#include "WeldJoint3D.h"

#include "o2/Physics/PhysicsWorld3D.h"

namespace o2
{
    WeldJoint3D::WeldJoint3D()
    {}

    WeldJoint3D::WeldJoint3D(const WeldJoint3D& other):
        IJoint3D(other), mLinearHertz(other.mLinearHertz), mAngularHertz(other.mAngularHertz),
        mLinearDampingRatio(other.mLinearDampingRatio), mAngularDampingRatio(other.mAngularDampingRatio)
    {}

    WeldJoint3D& WeldJoint3D::operator=(const WeldJoint3D& other)
    {
        IJoint3D::operator=(other);
        mLinearHertz = other.mLinearHertz;
        mAngularHertz = other.mAngularHertz;
        mLinearDampingRatio = other.mLinearDampingRatio;
        mAngularDampingRatio = other.mAngularDampingRatio;
        return *this;
    }

    void WeldJoint3D::SetLinearHertz(float hz) { mLinearHertz = hz; RebuildJoint(); }
    float WeldJoint3D::GetLinearHertz() const { return mLinearHertz; }
    void WeldJoint3D::SetAngularHertz(float hz) { mAngularHertz = hz; RebuildJoint(); }
    float WeldJoint3D::GetAngularHertz() const { return mAngularHertz; }
    void WeldJoint3D::SetLinearDampingRatio(float ratio) { mLinearDampingRatio = ratio; RebuildJoint(); }
    float WeldJoint3D::GetLinearDampingRatio() const { return mLinearDampingRatio; }
    void WeldJoint3D::SetAngularDampingRatio(float ratio) { mAngularDampingRatio = ratio; RebuildJoint(); }
    float WeldJoint3D::GetAngularDampingRatio() const { return mAngularDampingRatio; }

    String WeldJoint3D::GetName() { return "Weld joint 3D"; }
    String WeldJoint3D::GetCategory() { return "Physics 3D/Joints"; }
    bool WeldJoint3D::IsAvailableFromCreateMenu() { return true; }

    b3JointId WeldJoint3D::CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB)
    {
        b3WeldJointDef def = b3DefaultWeldJointDef();
        SetupBaseDef(def.base, bodyA, bodyB);

        def.linearHertz = mLinearHertz;
        def.angularHertz = mAngularHertz;
        def.linearDampingRatio = mLinearDampingRatio;
        def.angularDampingRatio = mAngularDampingRatio;

        return b3CreateWeldJoint(o2Physics3D.GetWorldId(), &def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::WeldJoint3D>);
// --- META ---

DECLARE_CLASS(o2::WeldJoint3D, o2__WeldJoint3D);
// --- END META ---
