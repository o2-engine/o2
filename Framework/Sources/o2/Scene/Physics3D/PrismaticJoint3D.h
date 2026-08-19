#pragma once
#include "o2/Scene/Physics3D/IJoint3D.h"

namespace o2
{
    // Prismatic 3D joint: slides along this actor's frame axis (no relative rotation), with limits/motor.
    // Orient the joint actor to choose the slide axis.
    class PrismaticJoint3D: public IJoint3D
    {
    public:
        PROPERTIES(PrismaticJoint3D);
        PROPERTY(bool, enableLimit, SetEnableLimit, IsLimitEnabled);         // Enable translation limits property
        PROPERTY(float, lowerTranslation, SetLowerTranslation, GetLowerTranslation); // Lower translation, world units property
        PROPERTY(float, upperTranslation, SetUpperTranslation, GetUpperTranslation); // Upper translation, world units property
        PROPERTY(bool, enableMotor, SetEnableMotor, IsMotorEnabled);         // Enable motor property
        PROPERTY(float, motorSpeed, SetMotorSpeed, GetMotorSpeed);           // Motor speed, world units per second property
        PROPERTY(float, maxMotorForce, SetMaxMotorForce, GetMaxMotorForce);  // Max motor force property

    public:
        PrismaticJoint3D();
        PrismaticJoint3D(const PrismaticJoint3D& other);
        PrismaticJoint3D& operator=(const PrismaticJoint3D& other);

        void SetEnableLimit(bool enable); bool IsLimitEnabled() const;
        void SetLowerTranslation(float value); float GetLowerTranslation() const;
        void SetUpperTranslation(float value); float GetUpperTranslation() const;
        void SetEnableMotor(bool enable); bool IsMotorEnabled() const;
        void SetMotorSpeed(float value); float GetMotorSpeed() const;
        void SetMaxMotorForce(float value); float GetMaxMotorForce() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(PrismaticJoint3D);
        CLONEABLE_REF(PrismaticJoint3D);

    private:
        bool  mEnableLimit = false;     // @SERIALIZABLE
        float mLowerTranslation = 0.0f; // Lower translation, world units @SERIALIZABLE
        float mUpperTranslation = 0.0f; // Upper translation, world units @SERIALIZABLE
        bool  mEnableMotor = false;     // @SERIALIZABLE
        float mMotorSpeed = 0.0f;       // Motor speed, world units per second @SERIALIZABLE
        float mMaxMotorForce = 0.0f;    // @SERIALIZABLE

    private:
        b3JointId CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::PrismaticJoint3D)
{
    BASE_CLASS(o2::IJoint3D);
}
END_META;
CLASS_FIELDS_META(o2::PrismaticJoint3D)
{
    FIELD().PUBLIC().NAME(enableLimit);
    FIELD().PUBLIC().NAME(lowerTranslation);
    FIELD().PUBLIC().NAME(upperTranslation);
    FIELD().PUBLIC().NAME(enableMotor);
    FIELD().PUBLIC().NAME(motorSpeed);
    FIELD().PUBLIC().NAME(maxMotorForce);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableLimit);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLowerTranslation);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mUpperTranslation);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableMotor);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMotorSpeed);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMaxMotorForce);
}
END_META;
CLASS_METHODS_META(o2::PrismaticJoint3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PrismaticJoint3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableLimit, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLimitEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLowerTranslation, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLowerTranslation);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUpperTranslation, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetUpperTranslation);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableMotor, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMotorEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMotorSpeed, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMotorSpeed);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxMotorForce, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMaxMotorForce);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PRIVATE().SIGNATURE(b3JointId, CreateJoint, RigidBody3D*, RigidBody3D*);
}
END_META;
// --- END META ---
