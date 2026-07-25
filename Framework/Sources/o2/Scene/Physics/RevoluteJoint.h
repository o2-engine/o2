#pragma once
#include "o2/Scene/Physics/IJoint.h"

namespace o2
{
    // -----------------------------------------------------------------
    // Revolute 2D joint: pins two bodies at this actor's position and
    // lets them rotate around it, with optional angle limits and a motor
    // -----------------------------------------------------------------
    class RevoluteJoint: public IJoint
    {
    public:
        PROPERTIES(RevoluteJoint);
        PROPERTY(bool, enableLimit, SetEnableLimit, IsLimitEnabled);       // Enable angle limits property
        PROPERTY(float, lowerAngle, SetLowerAngle, GetLowerAngle);         // Lower angle limit, degrees property
        PROPERTY(float, upperAngle, SetUpperAngle, GetUpperAngle);         // Upper angle limit, degrees property
        PROPERTY(bool, enableMotor, SetEnableMotor, IsMotorEnabled);       // Enable motor property
        PROPERTY(float, motorSpeed, SetMotorSpeed, GetMotorSpeed);         // Motor speed, degrees per second property
        PROPERTY(float, maxMotorTorque, SetMaxMotorTorque, GetMaxMotorTorque); // Max motor torque property

    public:
        RevoluteJoint();
        RevoluteJoint(const RevoluteJoint& other);
        RevoluteJoint& operator=(const RevoluteJoint& other);

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

        SERIALIZABLE(RevoluteJoint);
        CLONEABLE_REF(RevoluteJoint);

    private:
        bool  mEnableLimit = false;    // Enable angle limits @SERIALIZABLE
        float mLowerAngle = 0.0f;      // Lower angle limit, degrees @SERIALIZABLE
        float mUpperAngle = 0.0f;      // Upper angle limit, degrees @SERIALIZABLE
        bool  mEnableMotor = false;    // Enable motor @SERIALIZABLE
        float mMotorSpeed = 0.0f;      // Motor speed, degrees per second @SERIALIZABLE
        float mMaxMotorTorque = 0.0f;  // Max motor torque @SERIALIZABLE

    private:
        b2Joint* CreateJoint(b2Body* bodyA, b2Body* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::RevoluteJoint)
{
    BASE_CLASS(o2::IJoint);
}
END_META;
CLASS_FIELDS_META(o2::RevoluteJoint)
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
CLASS_METHODS_META(o2::RevoluteJoint)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const RevoluteJoint&);
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
    FUNCTION().PRIVATE().SIGNATURE(b2Joint*, CreateJoint, b2Body*, b2Body*);
}
END_META;
// --- END META ---
