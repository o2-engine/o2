#include "o2/stdafx.h"
#include "PrismaticJoint3D.h"

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/PhysicsWorld3D.h"

namespace o2
{
    PrismaticJoint3D::PrismaticJoint3D()
    {}

    PrismaticJoint3D::PrismaticJoint3D(const PrismaticJoint3D& other):
        IJoint3D(other), mEnableLimit(other.mEnableLimit), mLowerTranslation(other.mLowerTranslation),
        mUpperTranslation(other.mUpperTranslation), mEnableMotor(other.mEnableMotor), mMotorSpeed(other.mMotorSpeed),
        mMaxMotorForce(other.mMaxMotorForce)
    {}

    PrismaticJoint3D& PrismaticJoint3D::operator=(const PrismaticJoint3D& other)
    {
        IJoint3D::operator=(other);
        mEnableLimit = other.mEnableLimit;
        mLowerTranslation = other.mLowerTranslation;
        mUpperTranslation = other.mUpperTranslation;
        mEnableMotor = other.mEnableMotor;
        mMotorSpeed = other.mMotorSpeed;
        mMaxMotorForce = other.mMaxMotorForce;
        return *this;
    }

    void PrismaticJoint3D::SetEnableLimit(bool enable) { mEnableLimit = enable; RebuildJoint(); }
    bool PrismaticJoint3D::IsLimitEnabled() const { return mEnableLimit; }
    void PrismaticJoint3D::SetLowerTranslation(float value) { mLowerTranslation = value; RebuildJoint(); }
    float PrismaticJoint3D::GetLowerTranslation() const { return mLowerTranslation; }
    void PrismaticJoint3D::SetUpperTranslation(float value) { mUpperTranslation = value; RebuildJoint(); }
    float PrismaticJoint3D::GetUpperTranslation() const { return mUpperTranslation; }
    void PrismaticJoint3D::SetEnableMotor(bool enable) { mEnableMotor = enable; RebuildJoint(); }
    bool PrismaticJoint3D::IsMotorEnabled() const { return mEnableMotor; }
    void PrismaticJoint3D::SetMotorSpeed(float value) { mMotorSpeed = value; RebuildJoint(); }
    float PrismaticJoint3D::GetMotorSpeed() const { return mMotorSpeed; }
    void PrismaticJoint3D::SetMaxMotorForce(float value) { mMaxMotorForce = value; RebuildJoint(); }
    float PrismaticJoint3D::GetMaxMotorForce() const { return mMaxMotorForce; }

    String PrismaticJoint3D::GetName() { return "Prismatic joint 3D"; }
    String PrismaticJoint3D::GetCategory() { return "Physics 3D/Joints"; }
    bool PrismaticJoint3D::IsAvailableFromCreateMenu() { return true; }

    b3JointId PrismaticJoint3D::CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB)
    {
        float invScale = 1.0f/o2Config.physics3D.scale;

        b3PrismaticJointDef def = b3DefaultPrismaticJointDef();
        SetupBaseDef(def.base, bodyA, bodyB);

        def.enableLimit = mEnableLimit;
        def.lowerTranslation = mLowerTranslation*invScale;
        def.upperTranslation = mUpperTranslation*invScale;
        def.enableMotor = mEnableMotor;
        def.motorSpeed = mMotorSpeed*invScale;
        def.maxMotorForce = mMaxMotorForce;

        return b3CreatePrismaticJoint(o2Physics3D.GetWorldId(), &def);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::PrismaticJoint3D>);
// --- META ---

DECLARE_CLASS(o2::PrismaticJoint3D, o2__PrismaticJoint3D);
// --- END META ---
