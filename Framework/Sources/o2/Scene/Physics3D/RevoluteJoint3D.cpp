#include "o2/stdafx.h"
#include "RevoluteJoint3D.h"

#include "o2/Physics/PhysicsWorld3D.h"
#include "o2/Utils/Math/Math.h"

namespace o2
{
    RevoluteJoint3D::RevoluteJoint3D()
    {}

    RevoluteJoint3D::RevoluteJoint3D(const RevoluteJoint3D& other):
        IJoint3D(other), mEnableLimit(other.mEnableLimit), mLowerAngle(other.mLowerAngle), mUpperAngle(other.mUpperAngle),
        mEnableMotor(other.mEnableMotor), mMotorSpeed(other.mMotorSpeed), mMaxMotorTorque(other.mMaxMotorTorque)
    {}

    RevoluteJoint3D& RevoluteJoint3D::operator=(const RevoluteJoint3D& other)
    {
        IJoint3D::operator=(other);
        mEnableLimit = other.mEnableLimit;
        mLowerAngle = other.mLowerAngle;
        mUpperAngle = other.mUpperAngle;
        mEnableMotor = other.mEnableMotor;
        mMotorSpeed = other.mMotorSpeed;
        mMaxMotorTorque = other.mMaxMotorTorque;
        return *this;
    }

    void RevoluteJoint3D::SetEnableLimit(bool enable) { mEnableLimit = enable; RebuildJoint(); }
    bool RevoluteJoint3D::IsLimitEnabled() const { return mEnableLimit; }
    void RevoluteJoint3D::SetLowerAngle(float degrees) { mLowerAngle = degrees; RebuildJoint(); }
    float RevoluteJoint3D::GetLowerAngle() const { return mLowerAngle; }
    void RevoluteJoint3D::SetUpperAngle(float degrees) { mUpperAngle = degrees; RebuildJoint(); }
    float RevoluteJoint3D::GetUpperAngle() const { return mUpperAngle; }
    void RevoluteJoint3D::SetEnableMotor(bool enable) { mEnableMotor = enable; RebuildJoint(); }
    bool RevoluteJoint3D::IsMotorEnabled() const { return mEnableMotor; }
    void RevoluteJoint3D::SetMotorSpeed(float degreesPerSecond) { mMotorSpeed = degreesPerSecond; RebuildJoint(); }
    float RevoluteJoint3D::GetMotorSpeed() const { return mMotorSpeed; }
    void RevoluteJoint3D::SetMaxMotorTorque(float torque) { mMaxMotorTorque = torque; RebuildJoint(); }
    float RevoluteJoint3D::GetMaxMotorTorque() const { return mMaxMotorTorque; }

    String RevoluteJoint3D::GetName() { return "Revolute joint 3D"; }
    String RevoluteJoint3D::GetCategory() { return "Physics 3D/Joints"; }
    bool RevoluteJoint3D::IsAvailableFromCreateMenu() { return true; }

    b3JointId RevoluteJoint3D::CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB)
    {
        b3RevoluteJointDef def = b3DefaultRevoluteJointDef();
        SetupBaseDef(def.base, bodyA, bodyB);

        def.enableLimit = mEnableLimit;
        def.lowerAngle = Math::Deg2rad(mLowerAngle);
        def.upperAngle = Math::Deg2rad(mUpperAngle);
        def.enableMotor = mEnableMotor;
        def.motorSpeed = Math::Deg2rad(mMotorSpeed);
        def.maxMotorTorque = mMaxMotorTorque;

        return b3CreateRevoluteJoint(o2Physics3D.GetWorldId(), &def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::RevoluteJoint3D>);
// --- META ---

DECLARE_CLASS(o2::RevoluteJoint3D, o2__RevoluteJoint3D);
// --- END META ---
