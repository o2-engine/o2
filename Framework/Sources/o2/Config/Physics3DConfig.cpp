#include "o2/stdafx.h"
#include "Physics3DConfig.h"

namespace o2
{
    bool Physics3DConfig::operator==(const Physics3DConfig& other) const
    {
        return gravity == other.gravity && Math::Equals(scale, other.scale) &&
            subStepCount == other.subStepCount && Math::Equals(debugDrawAlpha, other.debugDrawAlpha);
    }
}
// --- META ---

DECLARE_CLASS(o2::Physics3DConfig, o2__Physics3DConfig);
// --- END META ---
