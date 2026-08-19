#pragma once
#include "o2/Scene/Physics3D/IJoint3D.h"

namespace o2
{
    // Distance 3D joint: keeps two bodies' frames at a fixed distance, or a spring/limited range
    class DistanceJoint3D: public IJoint3D
    {
    public:
        PROPERTIES(DistanceJoint3D);
        PROPERTY(float, length, SetLength, GetLength);                    // Rest length, world units property
        PROPERTY(bool, enableSpring, SetEnableSpring, IsSpringEnabled);   // Enable spring property
        PROPERTY(float, hertz, SetHertz, GetHertz);                       // Spring frequency Hz property
        PROPERTY(float, dampingRatio, SetDampingRatio, GetDampingRatio);  // Spring damping ratio property
        PROPERTY(bool, enableLimit, SetEnableLimit, IsLimitEnabled);      // Enable length limits property
        PROPERTY(float, minLength, SetMinLength, GetMinLength);           // Min length, world units property
        PROPERTY(float, maxLength, SetMaxLength, GetMaxLength);           // Max length, world units property

    public:
        DistanceJoint3D();
        DistanceJoint3D(const DistanceJoint3D& other);
        DistanceJoint3D& operator=(const DistanceJoint3D& other);

        void SetLength(float length); float GetLength() const;
        void SetEnableSpring(bool enable); bool IsSpringEnabled() const;
        void SetHertz(float hz); float GetHertz() const;
        void SetDampingRatio(float ratio); float GetDampingRatio() const;
        void SetEnableLimit(bool enable); bool IsLimitEnabled() const;
        void SetMinLength(float value); float GetMinLength() const;
        void SetMaxLength(float value); float GetMaxLength() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(DistanceJoint3D);
        CLONEABLE_REF(DistanceJoint3D);

    private:
        float mLength = 100.0f;      // Rest length, world units @SERIALIZABLE
        bool  mEnableSpring = false; // @SERIALIZABLE
        float mHertz = 0.0f;         // @SERIALIZABLE
        float mDampingRatio = 0.0f;  // @SERIALIZABLE
        bool  mEnableLimit = false;  // @SERIALIZABLE
        float mMinLength = 0.0f;     // Min length, world units @SERIALIZABLE
        float mMaxLength = 100.0f;   // Max length, world units @SERIALIZABLE

    private:
        b3JointId CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::DistanceJoint3D)
{
    BASE_CLASS(o2::IJoint3D);
}
END_META;
CLASS_FIELDS_META(o2::DistanceJoint3D)
{
    FIELD().PUBLIC().NAME(length);
    FIELD().PUBLIC().NAME(enableSpring);
    FIELD().PUBLIC().NAME(hertz);
    FIELD().PUBLIC().NAME(dampingRatio);
    FIELD().PUBLIC().NAME(enableLimit);
    FIELD().PUBLIC().NAME(minLength);
    FIELD().PUBLIC().NAME(maxLength);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100.0f).NAME(mLength);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableSpring);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mHertz);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mDampingRatio);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEnableLimit);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMinLength);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100.0f).NAME(mMaxLength);
}
END_META;
CLASS_METHODS_META(o2::DistanceJoint3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const DistanceJoint3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLength, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLength);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableSpring, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSpringEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHertz, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHertz);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDampingRatio, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDampingRatio);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableLimit, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLimitEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMinLength, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMinLength);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxLength, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMaxLength);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PRIVATE().SIGNATURE(b3JointId, CreateJoint, RigidBody3D*, RigidBody3D*);
}
END_META;
// --- END META ---
