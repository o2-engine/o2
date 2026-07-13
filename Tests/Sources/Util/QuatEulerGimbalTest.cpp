#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Quaternion.h"

using namespace o2;

namespace
{
    float QuatDot(const Quat& a, const Quat& b)
    {
        return Math::Abs(a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w);
    }
}

// FromEuler(ToEuler(q)) must reproduce the rotation at the gimbal lock (pitch +-90),
// where the naive conversion collapses roll and yaw and corrupts the orientation
TEST(QuatEuler, RoundTripAtGimbalLock)
{
    // Rotation composed over a yaw-90 orientation, like ring drags on upright Z-up objects
    Quat current = Quat::FromEuler(Vec3F(0.0f, Math::Deg2rad(90.0f), 0.5f));
    Quat rotated = Quat::FromAxisAngle(current*Vec3F(1, 0, 0), 0.35f)*current;

    Quat back = Quat::FromEuler(rotated.ToEuler());
    EXPECT_GT(QuatDot(rotated, back), 0.99999f);

    // Negative pitch pole
    Quat negativePole = Quat::FromEuler(Vec3F(0.0f, -Math::Deg2rad(90.0f), 0.7f));
    Quat rotatedNegative = Quat::FromAxisAngle(negativePole*Vec3F(1, 0, 0), -0.4f)*negativePole;

    back = Quat::FromEuler(rotatedNegative.ToEuler());
    EXPECT_GT(QuatDot(rotatedNegative, back), 0.99999f);
}

TEST(QuatEuler, RoundTripAwayFromGimbalUnchanged)
{
    for (auto& euler : { Vec3F(0.3f, 0.2f, 0.4f), Vec3F(-1.0f, 0.8f, 2.0f), Vec3F(0.0f, 0.0f, 1.2f) })
    {
        Quat source = Quat::FromEuler(euler);
        Quat back = Quat::FromEuler(source.ToEuler());
        EXPECT_GT(QuatDot(source, back), 0.99999f);
    }
}
