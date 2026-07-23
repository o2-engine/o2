#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    class PhysicsConfig: public ISerializable
    {
    public:
        Vec2F gravity = Vec2F(0, -98.0f); // Gravity force @SERIALIZABLE

        float scale = 10.0f; // World metrics to physics metrics scale @SERIALIZABLE

        int velocityIterations = 8; // Number of velocity solver iterations @SERIALIZABLE
        int positionIterations = 3; // Number of position solver iterations @SERIALIZABLE

        float debugDrawAlpha = 0.5f; // Debug draw transparency @SERIALIZABLE

        bool operator==(const PhysicsConfig& other) const;

        SERIALIZABLE(PhysicsConfig);
    };
}
// --- META ---

CLASS_BASES_META(o2::PhysicsConfig)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::PhysicsConfig)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2F(0, -98.0f)).NAME(gravity);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10.0f).NAME(scale);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(8).NAME(velocityIterations);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(3).NAME(positionIterations);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.5f).NAME(debugDrawAlpha);
}
END_META;
CLASS_METHODS_META(o2::PhysicsConfig)
{
}
END_META;
// --- END META ---
