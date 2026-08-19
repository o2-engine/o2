#include "o2/stdafx.h"
#include "RevoluteJoint.h"

#include "Box2D/Dynamics/Joints/b2RevoluteJoint.h"
#include "o2/Physics/PhysicsWorld.h"
#include "o2/Utils/Math/Math.h"

namespace o2
{
    RevoluteJoint::RevoluteJoint()
    {}

    RevoluteJoint::RevoluteJoint(const RevoluteJoint& other):
        IJoint(other), mEnableLimit(other.mEnableLimit), mLowerAngle(other.mLowerAngle), mUpperAngle(other.mUpperAngle),
        mEnableMotor(other.mEnableMotor), mMotorSpeed(other.mMotorSpeed), mMaxMotorTorque(other.mMaxMotorTorque)
    {}

    RevoluteJoint& RevoluteJoint::operator=(const RevoluteJoint& other)
    {
        IJoint::operator=(other);
        mEnableLimit = other.mEnableLimit;
        mLowerAngle = other.mLowerAngle;
        mUpperAngle = other.mUpperAngle;
        mEnableMotor = other.mEnableMotor;
        mMotorSpeed = other.mMotorSpeed;
        mMaxMotorTorque = other.mMaxMotorTorque;
        return *this;
    }

    void RevoluteJoint::SetEnableLimit(bool enable) { mEnableLimit = enable; RebuildJoint(); }
    bool RevoluteJoint::IsLimitEnabled() const { return mEnableLimit; }

    void RevoluteJoint::SetLowerAngle(float degrees) { mLowerAngle = degrees; RebuildJoint(); }
    float RevoluteJoint::GetLowerAngle() const { return mLowerAngle; }

    void RevoluteJoint::SetUpperAngle(float degrees) { mUpperAngle = degrees; RebuildJoint(); }
    float RevoluteJoint::GetUpperAngle() const { return mUpperAngle; }

    void RevoluteJoint::SetEnableMotor(bool enable) { mEnableMotor = enable; RebuildJoint(); }
    bool RevoluteJoint::IsMotorEnabled() const { return mEnableMotor; }

    void RevoluteJoint::SetMotorSpeed(float degreesPerSecond) { mMotorSpeed = degreesPerSecond; RebuildJoint(); }
    float RevoluteJoint::GetMotorSpeed() const { return mMotorSpeed; }

    void RevoluteJoint::SetMaxMotorTorque(float torque) { mMaxMotorTorque = torque; RebuildJoint(); }
    float RevoluteJoint::GetMaxMotorTorque() const { return mMaxMotorTorque; }

    String RevoluteJoint::GetName() { return "Revolute joint"; }
    String RevoluteJoint::GetCategory() { return "Physics/Joints"; }
    bool RevoluteJoint::IsAvailableFromCreateMenu() { return true; }

    b2Joint* RevoluteJoint::CreateJoint(b2Body* bodyA, b2Body* bodyB)
    {
        b2RevoluteJointDef def;
        def.Initialize(bodyA, bodyB, GetPhysicsAnchor());

        def.enableLimit = mEnableLimit;
        def.lowerAngle = Math::Deg2rad(mLowerAngle);
        def.upperAngle = Math::Deg2rad(mUpperAngle);
        def.enableMotor = mEnableMotor;
        def.motorSpeed = Math::Deg2rad(mMotorSpeed);
        def.maxMotorTorque = mMaxMotorTorque;

        SetupBaseDef(def, bodyA, bodyB);
        return o2Physics.CreateJoint(def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::RevoluteJoint>);
// --- META ---

DECLARE_CLASS(o2::RevoluteJoint, o2__RevoluteJoint);
// --- END META ---
