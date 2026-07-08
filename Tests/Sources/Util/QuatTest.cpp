#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Quaternion.h"

using namespace o2;

static void ExpectVecNear(const Vec3F& a, const Vec3F& b, float tolerance = 1e-5f)
{
    EXPECT_NEAR(a.x, b.x, tolerance);
    EXPECT_NEAR(a.y, b.y, tolerance);
    EXPECT_NEAR(a.z, b.z, tolerance);
}

TEST(Quat, Identity) {
    Quat q;
    EXPECT_TRUE(q == Quat::Identity());

    Vec3F v(1.0f, 2.0f, 3.0f);
    ExpectVecNear(q*v, v);
    EXPECT_NEAR(q.Length(), 1.0f, 1e-6f);
}

TEST(Quat, AxisAngleRotation) {
    float halfPi = Math::PI()*0.5f;

    Quat aboutZ = Quat::FromAxisAngle(Vec3F::ZAxis(), halfPi);
    ExpectVecNear(aboutZ*Vec3F::XAxis(), Vec3F::YAxis());

    Quat aboutX = Quat::FromAxisAngle(Vec3F::XAxis(), halfPi);
    ExpectVecNear(aboutX*Vec3F::YAxis(), Vec3F::ZAxis());

    Quat aboutY = Quat::FromAxisAngle(Vec3F::YAxis(), halfPi);
    ExpectVecNear(aboutY*Vec3F::ZAxis(), Vec3F::XAxis());
}

TEST(Quat, EulerRoundTrip) {
    Vec3F angles(0.3f, -0.5f, 1.1f);
    Vec3F back = Quat::FromEuler(angles).ToEuler();
    ExpectVecNear(back, angles, 1e-4f);

    Vec3F zAngles(0.0f, 0.0f, 0.7f);
    Quat qz = Quat::FromEuler(zAngles);
    Quat axisZ = Quat::FromAxisAngle(Vec3F::ZAxis(), 0.7f);
    EXPECT_TRUE(qz == axisZ);
}

TEST(Quat, Composition) {
    Quat a = Quat::FromAxisAngle(Vec3F::XAxis(), 0.6f);
    Quat b = Quat::FromAxisAngle(Vec3F::ZAxis(), -1.2f);
    Vec3F v(1.0f, 2.0f, 3.0f);

    ExpectVecNear((b*a)*v, b*(a*v));

    Quat halfTwice = Quat::FromAxisAngle(Vec3F::YAxis(), 0.4f)*Quat::FromAxisAngle(Vec3F::YAxis(), 0.4f);
    Quat full = Quat::FromAxisAngle(Vec3F::YAxis(), 0.8f);
    EXPECT_TRUE(halfTwice == full);
}

TEST(Quat, SlerpEndpointsAndMidpoint) {
    Quat a = Quat::Identity();
    Quat b = Quat::FromAxisAngle(Vec3F::ZAxis(), Math::PI()*0.5f);

    EXPECT_TRUE(Quat::Slerp(a, b, 0.0f) == a);
    EXPECT_TRUE(Quat::Slerp(a, b, 1.0f) == b);

    Quat mid = Quat::Slerp(a, b, 0.5f);
    Quat expected = Quat::FromAxisAngle(Vec3F::ZAxis(), Math::PI()*0.25f);
    EXPECT_TRUE(mid == expected);
}

TEST(Quat, Inverse) {
    Quat q = Quat::FromEuler(Vec3F(0.3f, 0.8f, -0.4f));
    Vec3F v(1.0f, -2.0f, 3.0f);

    EXPECT_TRUE(q*q.Inverted() == Quat::Identity());
    ExpectVecNear(q.Inverted()*(q*v), v);
}

TEST(Quat, NormalizeAndDot) {
    Quat q(2.0f, 0.0f, 0.0f, 0.0f);
    Quat n = q.Normalized();
    EXPECT_NEAR(n.Length(), 1.0f, 1e-6f);
    EXPECT_NEAR(n.x, 1.0f, 1e-6f);

    EXPECT_NEAR(Quat::Identity().Dot(Quat::Identity()), 1.0f, 1e-6f);
}

TEST(Quat, FromToRotationMapsFromToTo) {
    struct Case { Vec3F from, to; };
    Case cases[] =
    {
        { Vec3F::YAxis(), Vec3F::XAxis() },
        { Vec3F::YAxis(), Vec3F::ZAxis() },
        { Vec3F(1.0f, 2.0f, -0.5f), Vec3F(-3.0f, 0.2f, 1.0f) },
        { Vec3F::XAxis(), Vec3F::XAxis() },
    };

    for (auto& c : cases)
    {
        Quat rotation = Quat::FromToRotation(c.from, c.to);
        EXPECT_NEAR(rotation.Length(), 1.0f, 1e-5f);
        ExpectVecNear(rotation*c.from.Normalized(), c.to.Normalized(), 1e-5f);
    }
}

TEST(Quat, FromToRotationOppositeVectorsGiveHalfTurn) {
    struct Case { Vec3F from; };
    Case cases[] = { { Vec3F::XAxis() }, { Vec3F::YAxis() }, { Vec3F::ZAxis() }, { Vec3F(1.0f, 1.0f, 1.0f) } };

    for (auto& c : cases)
    {
        Vec3F from = c.from.Normalized();
        Quat rotation = Quat::FromToRotation(from, from*-1.0f);
        EXPECT_NEAR(rotation.Length(), 1.0f, 1e-5f);
        ExpectVecNear(rotation*from, from*-1.0f, 1e-5f);
    }
}

TEST(Quat, FromToRotationFromYMatchesAxisRotations) {
    // The gizmos orient +Y geometry along axes through this rotation
    ExpectVecNear(Quat::FromToRotation(Vec3F::YAxis(), Vec3F::Axis(0))*Vec3F::YAxis(), Vec3F::XAxis(), 1e-5f);
    ExpectVecNear(Quat::FromToRotation(Vec3F::YAxis(), Vec3F::Axis(1))*Vec3F::YAxis(), Vec3F::YAxis(), 1e-5f);
    ExpectVecNear(Quat::FromToRotation(Vec3F::YAxis(), Vec3F::Axis(2))*Vec3F::YAxis(), Vec3F::ZAxis(), 1e-5f);
}
