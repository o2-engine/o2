#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    class Physics3DConfig: public ISerializable
    {
    public:
        Vec3F gravity = Vec3F(0, -9.81f, 0); // Gravity force @SERIALIZABLE

        float scale = 1.0f; // World metrics to physics metrics scale @SERIALIZABLE

        int subStepCount = 4; // Number of solver sub-steps @SERIALIZABLE

        float debugDrawAlpha = 0.5f; // Debug draw transparency @SERIALIZABLE

        bool operator==(const Physics3DConfig& other) const;

        SERIALIZABLE(Physics3DConfig);
    };
}
// --- META ---

CLASS_BASES_META(o2::Physics3DConfig)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::Physics3DConfig)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec3F(0, -9.81f, 0)).NAME(gravity);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(scale);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(4).NAME(subStepCount);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.5f).NAME(debugDrawAlpha);
}
END_META;
CLASS_METHODS_META(o2::Physics3DConfig)
{
}
END_META;
// --- END META ---
