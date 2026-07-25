#pragma once
#include "o2/Scene/Physics3D/IJoint3D.h"

namespace o2
{
    // Spherical (ball) 3D joint: a point constraint with optional cone and twist limits
    class SphericalJoint3D: public IJoint3D
    {
    public:
        PROPERTIES(SphericalJoint3D);
        PROPERTY(bool, enableConeLimit, SetEnableConeLimit, IsConeLimitEnabled);    // Enable cone limit property
        PROPERTY(float, coneAngle, SetConeAngle, GetConeAngle);                     // Cone half-angle, degrees property
        PROPERTY(bool, enableTwistLimit, SetEnableTwistLimit, IsTwistLimitEnabled); // Enable twist limit property
        PROPERTY(float, lowerTwistAngle, SetLowerTwistAngle, GetLowerTwistAngle);   // Lower twist angle, degrees property
        PROPERTY(float, upperTwistAngle, SetUpperTwistAngle, GetUpperTwistAngle);   // Upper twist angle, degrees property

    public:
        SphericalJoint3D();
        SphericalJoint3D(const SphericalJoint3D& other);
        SphericalJoint3D& operator=(const SphericalJoint3D& other);

        void SetEnableConeLimit(bool enable); bool IsConeLimitEnabled() const;
        void SetConeAngle(float degrees); float GetConeAngle() const;
        void SetEnableTwistLimit(bool enable); bool IsTwistLimitEnabled() const;
        void SetLowerTwistAngle(float degrees); float GetLowerTwistAngle() const;
        void SetUpperTwistAngle(float degrees); float GetUpperTwistAngle() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(SphericalJoint3D);
        CLONEABLE_REF(SphericalJoint3D);

    private:
        bool  mEnableConeLimit = false;  // @SERIALIZABLE
        float mConeAngle = 45.0f;        // Cone half-angle, degrees @SERIALIZABLE
        bool  mEnableTwistLimit = false; // @SERIALIZABLE
        float mLowerTwistAngle = 0.0f;   // Lower twist angle, degrees @SERIALIZABLE
        float mUpperTwistAngle = 0.0f;   // Upper twist angle, degrees @SERIALIZABLE

    private:
        b3JointId CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::SphericalJoint3D)
{
    BASE_CLASS(o2::IJoint3D);
}
END_META;
CLASS_FIELDS_META(o2::SphericalJoint3D)
{
    FIELD().PUBLIC().NAME(enableConeLimit);
    FIELD().PUBLIC().NAME(coneAngle);
    FIELD().PUBLIC().NAME(enableTwistLimit);
    FIELD().PUBLIC().NAME(lowerTwistAngle);
    FIELD().PUBLIC().NAME(upperTwistAngle);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableConeLimit);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(45.0f).NAME(mConeAngle);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableTwistLimit);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLowerTwistAngle);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mUpperTwistAngle);
}
END_META;
CLASS_METHODS_META(o2::SphericalJoint3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SphericalJoint3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableConeLimit, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsConeLimitEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetConeAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetConeAngle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableTwistLimit, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsTwistLimitEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLowerTwistAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLowerTwistAngle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUpperTwistAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetUpperTwistAngle);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PRIVATE().SIGNATURE(b3JointId, CreateJoint, RigidBody3D*, RigidBody3D*);
}
END_META;
// --- END META ---
