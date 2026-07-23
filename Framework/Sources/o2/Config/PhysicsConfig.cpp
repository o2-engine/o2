#include "o2/stdafx.h"
#include "PhysicsConfig.h"

namespace o2
{
    bool PhysicsConfig::operator==(const PhysicsConfig& other) const
    {
        return gravity == other.gravity && Math::Equals(scale, other.scale) &&
            velocityIterations == other.velocityIterations && positionIterations == other.positionIterations &&
            Math::Equals(debugDrawAlpha, other.debugDrawAlpha);
    }
}
// --- META ---

DECLARE_CLASS(o2::PhysicsConfig, o2__PhysicsConfig);
// --- END META ---
