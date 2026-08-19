#pragma once
#include "o2/Scene/Physics3D/IJoint3D.h"

namespace o2
{
    // Weld 3D joint: rigidly fixes two bodies relative to this actor's frame (soft when hertz > 0)
    class WeldJoint3D: public IJoint3D
    {
    public:
        PROPERTIES(WeldJoint3D);
        PROPERTY(float, linearHertz, SetLinearHertz, GetLinearHertz);                     // Linear softness Hz property
        PROPERTY(float, angularHertz, SetAngularHertz, GetAngularHertz);                  // Angular softness Hz property
        PROPERTY(float, linearDampingRatio, SetLinearDampingRatio, GetLinearDampingRatio);   // Linear damping ratio property
        PROPERTY(float, angularDampingRatio, SetAngularDampingRatio, GetAngularDampingRatio); // Angular damping ratio property

    public:
        WeldJoint3D();
        WeldJoint3D(const WeldJoint3D& other);
        WeldJoint3D& operator=(const WeldJoint3D& other);

        void SetLinearHertz(float hz); float GetLinearHertz() const;
        void SetAngularHertz(float hz); float GetAngularHertz() const;
        void SetLinearDampingRatio(float ratio); float GetLinearDampingRatio() const;
        void SetAngularDampingRatio(float ratio); float GetAngularDampingRatio() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(WeldJoint3D);
        CLONEABLE_REF(WeldJoint3D);

    private:
        float mLinearHertz = 0.0f;         // @SERIALIZABLE
        float mAngularHertz = 0.0f;        // @SERIALIZABLE
        float mLinearDampingRatio = 0.0f;  // @SERIALIZABLE
        float mAngularDampingRatio = 0.0f; // @SERIALIZABLE

    private:
        b3JointId CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::WeldJoint3D)
{
    BASE_CLASS(o2::IJoint3D);
}
END_META;
CLASS_FIELDS_META(o2::WeldJoint3D)
{
    FIELD().PUBLIC().NAME(linearHertz);
    FIELD().PUBLIC().NAME(angularHertz);
    FIELD().PUBLIC().NAME(linearDampingRatio);
    FIELD().PUBLIC().NAME(angularDampingRatio);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLinearHertz);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mAngularHertz);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLinearDampingRatio);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mAngularDampingRatio);
}
END_META;
CLASS_METHODS_META(o2::WeldJoint3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const WeldJoint3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinearHertz, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLinearHertz);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngularHertz, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngularHertz);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinearDampingRatio, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLinearDampingRatio);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngularDampingRatio, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngularDampingRatio);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PRIVATE().SIGNATURE(b3JointId, CreateJoint, RigidBody3D*, RigidBody3D*);
}
END_META;
// --- END META ---
