#pragma once
#include "o2/Scene/Physics/IJoint.h"

namespace o2
{
    // ---------------------------------------------------------------
    // Weld 2D joint: rigidly welds two bodies together at this actor's
    // position (a soft weld when frequency > 0)
    // ---------------------------------------------------------------
    class WeldJoint: public IJoint
    {
    public:
        PROPERTIES(WeldJoint);
        PROPERTY(float, frequency, SetFrequency, GetFrequency);         // Softness frequency Hz (0 = rigid) property
        PROPERTY(float, dampingRatio, SetDampingRatio, GetDampingRatio); // Softness damping ratio property

    public:
        WeldJoint();
        WeldJoint(const WeldJoint& other);
        WeldJoint& operator=(const WeldJoint& other);

        void SetFrequency(float hz);
        float GetFrequency() const;

        void SetDampingRatio(float ratio);
        float GetDampingRatio() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(WeldJoint);
        CLONEABLE_REF(WeldJoint);

    private:
        float mFrequency = 0.0f;    // Softness frequency in Hz, 0 is rigid @SERIALIZABLE
        float mDampingRatio = 0.0f; // Softness damping ratio @SERIALIZABLE

    private:
        b2Joint* CreateJoint(b2Body* bodyA, b2Body* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::WeldJoint)
{
    BASE_CLASS(o2::IJoint);
}
END_META;
CLASS_FIELDS_META(o2::WeldJoint)
{
    FIELD().PUBLIC().NAME(frequency);
    FIELD().PUBLIC().NAME(dampingRatio);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFrequency);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mDampingRatio);
}
END_META;
CLASS_METHODS_META(o2::WeldJoint)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const WeldJoint&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFrequency, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetFrequency);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDampingRatio, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDampingRatio);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PRIVATE().SIGNATURE(b2Joint*, CreateJoint, b2Body*, b2Body*);
}
END_META;
// --- END META ---
