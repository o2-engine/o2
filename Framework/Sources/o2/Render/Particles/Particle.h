#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Math/Color.h"

namespace o2
{
    class Particle
    {
    public:
        int index = 0; // Index of particle in container

        Vec3F position; // Position of particle center; z is zero in 2D emitters
        Vec3F velocity; // Particle velocity

        float angle = 0;      // Particle roll angle around facing axis in radians
        float angleSpeed = 0; // Angle speed in radians/sec

        Vec2F size; // Size of particle

        Color4 color; // Particle's color

        float timeLeft = 0; // Estimate life time
        float lifetime = 0; // Total life time

        bool alive = false; // Is particle alive

    public:
        bool operator==(const Particle& other) const
        {
            return position == other.position && velocity == other.velocity && Math::Equals(angle, other.angle) &&
                Math::Equals(angleSpeed, other.angleSpeed) && Math::Equals(timeLeft, other.timeLeft) && size == other.size &&
                color == other.color && alive == other.alive;
        }
    };
}
