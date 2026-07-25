#pragma once
#include "o2/Scene/Physics/IJoint.h"

namespace o2
{
    // ------------------------------------------------------------------
    // Prismatic 2D joint: constrains two bodies to slide along a shared
    // axis (no relative rotation), with optional limits and a motor
    // ------------------------------------------------------------------
    class PrismaticJoint: public IJoint
    {
    public:
        PROPERTIES(PrismaticJoint);
        PROPERTY(Vec2F, axis, SetAxis, GetAxis);                             // Slide axis in body A local space property
        PROPERTY(bool, enableLimit, SetEnableLimit, IsLimitEnabled);         // Enable translation limits property
        PROPERTY(float, lowerTranslation, SetLowerTranslation, GetLowerTranslation); // Lower translation, world units property
        PROPERTY(float, upperTranslation, SetUpperTranslation, GetUpperTranslation); // Upper translation, world units property
        PROPERTY(bool, enableMotor, SetEnableMotor, IsMotorEnabled);         // Enable motor property
        PROPERTY(float, motorSpeed, SetMotorSpeed, GetMotorSpeed);           // Motor speed, world units per second property
        PROPERTY(float, maxMotorForce, SetMaxMotorForce, GetMaxMotorForce);  // Max motor force property

    public:
        PrismaticJoint();
        PrismaticJoint(const PrismaticJoint& other);
        PrismaticJoint& operator=(const PrismaticJoint& other);

        void SetAxis(const Vec2F& axis);
        Vec2F GetAxis() const;

        void SetEnableLimit(bool enable);
        bool IsLimitEnabled() const;

        void SetLowerTranslation(float value);
        float GetLowerTranslation() const;

        void SetUpperTranslation(float value);
        float GetUpperTranslation() const;

        void SetEnableMotor(bool enable);
        bool IsMotorEnabled() const;

        void SetMotorSpeed(float value);
        float GetMotorSpeed() const;

        void SetMaxMotorForce(float value);
        float GetMaxMotorForce() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(PrismaticJoint);
        CLONEABLE_REF(PrismaticJoint);

    private:
        Vec2F mAxis = Vec2F(1, 0);      // Slide axis @SERIALIZABLE
        bool  mEnableLimit = false;     // Enable translation limits @SERIALIZABLE
        float mLowerTranslation = 0.0f; // Lower translation, world units @SERIALIZABLE
        float mUpperTranslation = 0.0f; // Upper translation, world units @SERIALIZABLE
        bool  mEnableMotor = false;     // Enable motor @SERIALIZABLE
        float mMotorSpeed = 0.0f;       // Motor speed, world units per second @SERIALIZABLE
        float mMaxMotorForce = 0.0f;    // Max motor force @SERIALIZABLE

    private:
        b2Joint* CreateJoint(b2Body* bodyA, b2Body* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::PrismaticJoint)
{
    BASE_CLASS(o2::IJoint);
}
END_META;
CLASS_FIELDS_META(o2::PrismaticJoint)
{
    FIELD().PUBLIC().NAME(axis);
    FIELD().PUBLIC().NAME(enableLimit);
    FIELD().PUBLIC().NAME(lowerTranslation);
    FIELD().PUBLIC().NAME(upperTranslation);
    FIELD().PUBLIC().NAME(enableMotor);
    FIELD().PUBLIC().NAME(motorSpeed);
    FIELD().PUBLIC().NAME(maxMotorForce);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2F(1, 0)).NAME(mAxis);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableLimit);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLowerTranslation);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mUpperTranslation);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableMotor);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMotorSpeed);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMaxMotorForce);
}
END_META;
CLASS_METHODS_META(o2::PrismaticJoint)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PrismaticJoint&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAxis, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetAxis);
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
    FUNCTION().PRIVATE().SIGNATURE(b2Joint*, CreateJoint, b2Body*, b2Body*);
}
END_META;
// --- END META ---
