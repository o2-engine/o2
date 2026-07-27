#pragma once
#include "o2/Scene/Physics3D/IJoint3D.h"

namespace o2
{
    // Revolute 3D joint: hinge around this actor's frame, with optional angle limits and a motor
    class RevoluteJoint3D: public IJoint3D
    {
    public:
        PROPERTIES(RevoluteJoint3D);
        PROPERTY(bool, enableLimit, SetEnableLimit, IsLimitEnabled);           // Enable angle limits property
        PROPERTY(float, lowerAngle, SetLowerAngle, GetLowerAngle);             // Lower angle limit, degrees property
        PROPERTY(float, upperAngle, SetUpperAngle, GetUpperAngle);             // Upper angle limit, degrees property
        PROPERTY(bool, enableMotor, SetEnableMotor, IsMotorEnabled);           // Enable motor property
        PROPERTY(float, motorSpeed, SetMotorSpeed, GetMotorSpeed);             // Motor speed, degrees per second property
        PROPERTY(float, maxMotorTorque, SetMaxMotorTorque, GetMaxMotorTorque); // Max motor torque property

    public:
        RevoluteJoint3D();
        RevoluteJoint3D(const RevoluteJoint3D& other);
        RevoluteJoint3D& operator=(const RevoluteJoint3D& other);

        void SetEnableLimit(bool enable);
        bool IsLimitEnabled() const;
        void SetLowerAngle(float degrees);
        float GetLowerAngle() const;
        void SetUpperAngle(float degrees);
        float GetUpperAngle() const;
        void SetEnableMotor(bool enable);
        bool IsMotorEnabled() const;
        void SetMotorSpeed(float degreesPerSecond);
        float GetMotorSpeed() const;
        void SetMaxMotorTorque(float torque);
        float GetMaxMotorTorque() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(RevoluteJoint3D);
        CLONEABLE_REF(RevoluteJoint3D);

    private:
        bool  mEnableLimit = false;   // @SERIALIZABLE
        float mLowerAngle = 0.0f;     // Lower angle limit, degrees @SERIALIZABLE
        float mUpperAngle = 0.0f;     // Upper angle limit, degrees @SERIALIZABLE
        bool  mEnableMotor = false;   // @SERIALIZABLE
        float mMotorSpeed = 0.0f;     // Motor speed, degrees per second @SERIALIZABLE
        float mMaxMotorTorque = 0.0f; // @SERIALIZABLE

    private:
        b3JointId CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::RevoluteJoint3D)
{
    BASE_CLASS(o2::IJoint3D);
}
END_META;
CLASS_FIELDS_META(o2::RevoluteJoint3D)
{
    FIELD().PUBLIC().NAME(enableLimit);
    FIELD().PUBLIC().NAME(lowerAngle);
    FIELD().PUBLIC().NAME(upperAngle);
    FIELD().PUBLIC().NAME(enableMotor);
    FIELD().PUBLIC().NAME(motorSpeed);
    FIELD().PUBLIC().NAME(maxMotorTorque);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableLimit);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLowerAngle);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mUpperAngle);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableMotor);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMotorSpeed);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMaxMotorTorque);
}
END_META;
CLASS_METHODS_META(o2::RevoluteJoint3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const RevoluteJoint3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableLimit, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLimitEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLowerAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLowerAngle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUpperAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetUpperAngle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableMotor, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMotorEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMotorSpeed, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMotorSpeed);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxMotorTorque, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMaxMotorTorque);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PRIVATE().SIGNATURE(b3JointId, CreateJoint, RigidBody3D*, RigidBody3D*);
}
END_META;
// --- END META ---
