#include "o2/stdafx.h"
#include "PrismaticJoint.h"

#include "Box2D/Dynamics/Joints/b2PrismaticJoint.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/PhysicsWorld.h"

namespace o2
{
    PrismaticJoint::PrismaticJoint()
    {}

    PrismaticJoint::PrismaticJoint(const PrismaticJoint& other):
        IJoint(other), mAxis(other.mAxis), mEnableLimit(other.mEnableLimit), mLowerTranslation(other.mLowerTranslation),
        mUpperTranslation(other.mUpperTranslation), mEnableMotor(other.mEnableMotor), mMotorSpeed(other.mMotorSpeed),
        mMaxMotorForce(other.mMaxMotorForce)
    {}

    PrismaticJoint& PrismaticJoint::operator=(const PrismaticJoint& other)
    {
        IJoint::operator=(other);
        mAxis = other.mAxis;
        mEnableLimit = other.mEnableLimit;
        mLowerTranslation = other.mLowerTranslation;
        mUpperTranslation = other.mUpperTranslation;
        mEnableMotor = other.mEnableMotor;
        mMotorSpeed = other.mMotorSpeed;
        mMaxMotorForce = other.mMaxMotorForce;
        return *this;
    }

    void PrismaticJoint::SetAxis(const Vec2F& axis) { mAxis = axis; RebuildJoint(); }
    Vec2F PrismaticJoint::GetAxis() const { return mAxis; }

    void PrismaticJoint::SetEnableLimit(bool enable) { mEnableLimit = enable; RebuildJoint(); }
    bool PrismaticJoint::IsLimitEnabled() const { return mEnableLimit; }

    void PrismaticJoint::SetLowerTranslation(float value) { mLowerTranslation = value; RebuildJoint(); }
    float PrismaticJoint::GetLowerTranslation() const { return mLowerTranslation; }

    void PrismaticJoint::SetUpperTranslation(float value) { mUpperTranslation = value; RebuildJoint(); }
    float PrismaticJoint::GetUpperTranslation() const { return mUpperTranslation; }

    void PrismaticJoint::SetEnableMotor(bool enable) { mEnableMotor = enable; RebuildJoint(); }
    bool PrismaticJoint::IsMotorEnabled() const { return mEnableMotor; }

    void PrismaticJoint::SetMotorSpeed(float value) { mMotorSpeed = value; RebuildJoint(); }
    float PrismaticJoint::GetMotorSpeed() const { return mMotorSpeed; }

    void PrismaticJoint::SetMaxMotorForce(float value) { mMaxMotorForce = value; RebuildJoint(); }
    float PrismaticJoint::GetMaxMotorForce() const { return mMaxMotorForce; }

    String PrismaticJoint::GetName() { return "Prismatic joint"; }
    String PrismaticJoint::GetCategory() { return "Physics/Joints"; }
    bool PrismaticJoint::IsAvailableFromCreateMenu() { return true; }

    b2Joint* PrismaticJoint::CreateJoint(b2Body* bodyA, b2Body* bodyB)
    {
        Vec2F axis = mAxis.Length() > 0.0001f ? mAxis.Normalized() : Vec2F(1, 0);

        b2PrismaticJointDef def;
        def.Initialize(bodyA, bodyB, GetPhysicsAnchor(), axis);

        float invScale = 1.0f/o2Config.physics.scale;
        def.enableLimit = mEnableLimit;
        def.lowerTranslation = mLowerTranslation*invScale;
        def.upperTranslation = mUpperTranslation*invScale;
        def.enableMotor = mEnableMotor;
        def.motorSpeed = mMotorSpeed*invScale;
        def.maxMotorForce = mMaxMotorForce;

        SetupBaseDef(def, bodyA, bodyB);
        return o2Physics.CreateJoint(def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::PrismaticJoint>);
// --- META ---

DECLARE_CLASS(o2::PrismaticJoint, o2__PrismaticJoint);
// --- END META ---
