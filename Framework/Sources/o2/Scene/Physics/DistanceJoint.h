#pragma once
#include "o2/Scene/Physics/IJoint.h"

namespace o2
{
    // ----------------------------------------------------------------
    // Distance 2D joint: keeps two bodies at a fixed distance, or a
    // soft spring between them when frequency > 0
    // ----------------------------------------------------------------
    class DistanceJoint: public IJoint
    {
    public:
        PROPERTIES(DistanceJoint);
        PROPERTY(float, length, SetLength, GetLength);                // Rest length, world units (0 = current) property
        PROPERTY(float, frequency, SetFrequency, GetFrequency);       // Spring frequency Hz (0 = rigid) property
        PROPERTY(float, dampingRatio, SetDampingRatio, GetDampingRatio); // Spring damping ratio property

    public:
        DistanceJoint();
        DistanceJoint(const DistanceJoint& other);
        DistanceJoint& operator=(const DistanceJoint& other);

        void SetLength(float length);
        float GetLength() const;

        void SetFrequency(float hz);
        float GetFrequency() const;

        void SetDampingRatio(float ratio);
        float GetDampingRatio() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(DistanceJoint);
        CLONEABLE_REF(DistanceJoint);

    private:
        float mLength = 0.0f;       // Rest length in world units, 0 keeps the current distance @SERIALIZABLE
        float mFrequency = 0.0f;    // Spring frequency in Hz, 0 is rigid @SERIALIZABLE
        float mDampingRatio = 0.0f; // Spring damping ratio @SERIALIZABLE

    private:
        b2Joint* CreateJoint(b2Body* bodyA, b2Body* bodyB) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::DistanceJoint)
{
    BASE_CLASS(o2::IJoint);
}
END_META;
CLASS_FIELDS_META(o2::DistanceJoint)
{
    FIELD().PUBLIC().NAME(length);
    FIELD().PUBLIC().NAME(frequency);
    FIELD().PUBLIC().NAME(dampingRatio);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLength);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mFrequency);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mDampingRatio);
}
END_META;
CLASS_METHODS_META(o2::DistanceJoint)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const DistanceJoint&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLength, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLength);
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
