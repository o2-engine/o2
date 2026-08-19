#include "o2/stdafx.h"
#include "DistanceJoint.h"

#include "Box2D/Dynamics/Joints/b2DistanceJoint.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/PhysicsWorld.h"

namespace o2
{
    DistanceJoint::DistanceJoint()
    {}

    DistanceJoint::DistanceJoint(const DistanceJoint& other):
        IJoint(other), mLength(other.mLength), mFrequency(other.mFrequency), mDampingRatio(other.mDampingRatio)
    {}

    DistanceJoint& DistanceJoint::operator=(const DistanceJoint& other)
    {
        IJoint::operator=(other);
        mLength = other.mLength;
        mFrequency = other.mFrequency;
        mDampingRatio = other.mDampingRatio;
        return *this;
    }

    void DistanceJoint::SetLength(float length) { mLength = length; RebuildJoint(); }
    float DistanceJoint::GetLength() const { return mLength; }

    void DistanceJoint::SetFrequency(float hz) { mFrequency = hz; RebuildJoint(); }
    float DistanceJoint::GetFrequency() const { return mFrequency; }

    void DistanceJoint::SetDampingRatio(float ratio) { mDampingRatio = ratio; RebuildJoint(); }
    float DistanceJoint::GetDampingRatio() const { return mDampingRatio; }

    String DistanceJoint::GetName() { return "Distance joint"; }
    String DistanceJoint::GetCategory() { return "Physics/Joints"; }
    bool DistanceJoint::IsAvailableFromCreateMenu() { return true; }

    b2Joint* DistanceJoint::CreateJoint(b2Body* bodyA, b2Body* bodyB)
    {
        float invScale = 1.0f/o2Config.physics.scale;
        Vec2F anchorA = mBodyA->transform->GetWorldPosition2D()*invScale;
        Vec2F anchorB = mBodyB->transform->GetWorldPosition2D()*invScale;

        b2DistanceJointDef def;
        def.Initialize(bodyA, bodyB, anchorA, anchorB);

        if (mLength > 0.0f)
            def.length = mLength*invScale;

        def.frequencyHz = mFrequency;
        def.dampingRatio = mDampingRatio;

        SetupBaseDef(def, bodyA, bodyB);
        return o2Physics.CreateJoint(def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::DistanceJoint>);
// --- META ---

DECLARE_CLASS(o2::DistanceJoint, o2__DistanceJoint);
// --- END META ---
