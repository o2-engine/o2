#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Math.h"
#include "o2Editor/Windows/SceneWindow/SceneView3DState.h"

using namespace o2;
using namespace Editor;

namespace
{
    const Vec2F kViewport(800.0f, 600.0f);

    bool Near3(const Vec3F& a, const Vec3F& b, float eps = 0.01f)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps && Math::Abs(a.z - b.z) < eps;
    }

    bool Near2(const Vec2F& a, const Vec2F& b, float eps = 0.01f)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps;
    }
}

TEST(SceneView3DState, BuildCameraTopDownMatches2DOrientation)
{
    SceneView3DState state;
    state.target = Vec3F(10.0f, 20.0f, 0.0f);
    state.yaw = 0.0f;
    state.pitch = 0.0f;
    state.distance = 500.0f;

    Camera camera = state.BuildCamera();

    EXPECT_EQ(camera.projection, Camera::Projection::Perspective);
    EXPECT_TRUE(Near3(camera.GetPosition(), Vec3F(10.0f, 20.0f, 500.0f)));

    // Identity rotation: forward is -Z (down at the plane), up is +Y as in 2D view
    EXPECT_TRUE(Near3(camera.GetRotation()*Vec3F(0.0f, 0.0f, -1.0f), Vec3F(0.0f, 0.0f, -1.0f)));
    EXPECT_TRUE(Near3(camera.GetRotation()*Vec3F(0.0f, 1.0f, 0.0f), Vec3F(0.0f, 1.0f, 0.0f)));
}

TEST(SceneView3DState, BuildCameraPitchTiltsOnOrbitSphere)
{
    SceneView3DState state;
    state.target = Vec3F();
    state.yaw = 0.0f;
    state.pitch = Math::Deg2rad(60.0f);
    state.distance = 100.0f;

    Camera camera = state.BuildCamera();

    float s = Math::Sin(Math::Deg2rad(60.0f));
    float c = Math::Cos(Math::Deg2rad(60.0f));
    EXPECT_TRUE(Near3(camera.GetPosition(), Vec3F(0.0f, -100.0f*s, 100.0f*c)));

    // Camera up tilts from +Y towards +Z
    EXPECT_TRUE(Near3(camera.GetRotation()*Vec3F(0.0f, 1.0f, 0.0f), Vec3F(0.0f, c, s)));

    // Forward points from camera to target
    Vec3F forward = camera.GetRotation()*Vec3F(0.0f, 0.0f, -1.0f);
    EXPECT_TRUE(Near3(forward, (state.target - camera.GetPosition()).Normalized()));
}

TEST(SceneView3DState, BuildCameraYawOrbitsAroundPlaneNormal)
{
    SceneView3DState state;
    state.target = Vec3F();
    state.yaw = Math::Deg2rad(90.0f);
    state.pitch = Math::Deg2rad(60.0f);
    state.distance = 100.0f;

    Camera camera = state.BuildCamera();

    float s = Math::Sin(Math::Deg2rad(60.0f));
    float c = Math::Cos(Math::Deg2rad(60.0f));
    EXPECT_TRUE(Near3(camera.GetPosition(), Vec3F(100.0f*s, 0.0f, 100.0f*c)));

    Vec3F forward = camera.GetRotation()*Vec3F(0.0f, 0.0f, -1.0f);
    EXPECT_TRUE(Near3(forward, (state.target - camera.GetPosition()).Normalized()));
}

TEST(SceneView3DState, ScreenCenterMapsToTarget)
{
    SceneView3DState state;
    state.target = Vec3F(30.0f, -10.0f, 0.0f);
    state.yaw = Math::Deg2rad(25.0f);
    state.pitch = Math::Deg2rad(40.0f);
    state.distance = 300.0f;

    Vec2F result;
    ASSERT_TRUE(state.ScreenToPlanePoint(kViewport*0.5f, kViewport, result));
    EXPECT_TRUE(Near2(result, Vec2F(30.0f, -10.0f), 0.05f));

    EXPECT_TRUE(Near2(state.PlanePointToScreen(Vec2F(30.0f, -10.0f), kViewport), kViewport*0.5f, 0.05f));
}

TEST(SceneView3DState, PlaneScreenRoundTrip)
{
    SceneView3DState state;
    state.target = Vec3F(5.0f, 8.0f, 0.0f);
    state.distance = 400.0f;

    Vec2F points[] = { Vec2F(0.0f, 0.0f), Vec2F(100.0f, 50.0f), Vec2F(-40.0f, 80.0f), Vec2F(250.0f, -150.0f) };
    float yaws[] = { 0.0f, Math::Deg2rad(30.0f), Math::Deg2rad(-120.0f) };
    float pitches[] = { 0.0f, Math::Deg2rad(45.0f), Math::Deg2rad(-30.0f) };

    for (float yaw : yaws)
    {
        for (float pitch : pitches)
        {
            state.yaw = yaw;
            state.pitch = pitch;

            for (auto& point : points)
            {
                Vec2F screen = state.PlanePointToScreen(point, kViewport);
                Vec2F back;
                ASSERT_TRUE(state.ScreenToPlanePoint(screen, kViewport, back));
                EXPECT_TRUE(Near2(back, point, 0.5f))
                    << "yaw " << yaw << " pitch " << pitch << " point " << point.x << " " << point.y
                    << " -> " << back.x << " " << back.y;
            }
        }
    }
}

TEST(SceneView3DState, ScreenToPlaneFailsAboveHorizon)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(89.0f);
    state.distance = 100.0f;

    Vec2F result;
    EXPECT_FALSE(state.ScreenToPlanePoint(Vec2F(kViewport.x*0.5f, kViewport.y - 1.0f), kViewport, result));
}

TEST(SceneView3DState, OrbitClampsPitch)
{
    SceneView3DState state;

    state.Orbit(Vec2F(0.0f, 10.0f));
    EXPECT_FLOAT_EQ(state.pitch, SceneView3DState::maxPitch);

    state.Orbit(Vec2F(0.0f, -100.0f));
    EXPECT_FLOAT_EQ(state.pitch, -SceneView3DState::maxPitch);

    float yawBefore = state.yaw;
    state.Orbit(Vec2F(0.5f, 0.0f));
    EXPECT_FLOAT_EQ(state.yaw, yawBefore + 0.5f);
}

TEST(SceneView3DState, ZoomClampsDistance)
{
    SceneView3DState state;

    state.Zoom(1e-12f);
    EXPECT_FLOAT_EQ(state.distance, SceneView3DState::minDistance);

    state.Zoom(1e12f);
    state.Zoom(1e12f);
    EXPECT_FLOAT_EQ(state.distance, SceneView3DState::maxDistance);
}

TEST(SceneView3DState, PanKeepsGrabbedPlanePointUnderCursor)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(30.0f);
    state.yaw = Math::Deg2rad(15.0f);
    state.distance = 300.0f;

    Vec2F center = kViewport*0.5f;
    Vec2F delta(50.0f, 20.0f);

    Vec2F grabbed;
    ASSERT_TRUE(state.ScreenToPlanePoint(center, kViewport, grabbed));

    state.Pan(delta, kViewport);

    Vec2F afterPan;
    ASSERT_TRUE(state.ScreenToPlanePoint(center + delta, kViewport, afterPan));
    EXPECT_TRUE(Near2(afterPan, grabbed, 0.5f));
}

TEST(SceneView3DState, VerticalAxisZKnownCases)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(80.0f);
    state.distance = 200.0f;

    Vec2F center = kViewport*0.5f;

    // Axis through target: screen center maps to z = 0, cursor up maps to +z, cursor down to -z
    float z = 999.0f;
    ASSERT_TRUE(state.ScreenToVerticalAxisZ(center, kViewport, Vec2F(0.0f, 0.0f), z));
    EXPECT_NEAR(z, 0.0f, 0.05f);

    float zUp = 0.0f;
    ASSERT_TRUE(state.ScreenToVerticalAxisZ(center + Vec2F(0.0f, 100.0f), kViewport, Vec2F(0.0f, 0.0f), zUp));
    EXPECT_GT(zUp, 0.1f);

    float zDown = 0.0f;
    ASSERT_TRUE(state.ScreenToVerticalAxisZ(center - Vec2F(0.0f, 100.0f), kViewport, Vec2F(0.0f, 0.0f), zDown));
    EXPECT_LT(zDown, -0.1f);
}

TEST(SceneView3DState, VerticalAxisZFailsWhenLookingAlongAxis)
{
    SceneView3DState state;
    state.pitch = 0.0f;
    state.distance = 200.0f;

    // Looking straight down: the view ray through the axis anchor is parallel to the axis
    float z = 0.0f;
    EXPECT_FALSE(state.ScreenToVerticalAxisZ(kViewport*0.5f, kViewport, Vec2F(0.0f, 0.0f), z));
}

TEST(SceneView3DState, AxisParamMatchesVerticalAxisZ)
{
    SceneView3DState state;
    state.target = Vec3F(10.0f, -5.0f, 0.0f);
    state.pitch = Math::Deg2rad(50.0f);
    state.yaw = Math::Deg2rad(20.0f);
    state.distance = 300.0f;

    Vec2F points[] = { kViewport*0.5f, Vec2F(100.0f, 100.0f), Vec2F(700.0f, 500.0f) };
    for (auto& point : points)
    {
        float z = 0.0f, param = 0.0f;
        ASSERT_TRUE(state.ScreenToVerticalAxisZ(point, kViewport, Vec2F(3.0f, 4.0f), z));
        ASSERT_TRUE(state.ScreenToAxisParam(point, kViewport, Vec3F(3.0f, 4.0f, 0.0f), Vec3F(0.0f, 0.0f, 1.0f), param));
        EXPECT_NEAR(z, param, 1e-4f);
    }
}

TEST(SceneView3DState, AxisParamRecoversProjectedPointOnXAxis)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(45.0f);
    state.distance = 300.0f;

    // Project a known point on the world X axis and recover its parameter from the screen position
    Vec2F screen = state.WorldToScreen(Vec3F(50.0f, 0.0f, 0.0f), kViewport);

    float param = 0.0f;
    ASSERT_TRUE(state.ScreenToAxisParam(screen, kViewport, Vec3F(), Vec3F(1.0f, 0.0f, 0.0f), param));
    EXPECT_NEAR(param, 50.0f, 0.05f);
}

TEST(SceneView3DState, AxisParamRoundTripOnArbitraryAxis)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(35.0f);
    state.yaw = Math::Deg2rad(-70.0f);
    state.distance = 250.0f;

    Vec3F axisOrigin(20.0f, -30.0f, 10.0f);
    Vec3F axisDir = Vec3F(1.0f, 2.0f, 0.5f).Normalized();

    for (float t : { -40.0f, 0.0f, 25.0f, 80.0f })
    {
        Vec2F screen = state.WorldToScreen(axisOrigin + axisDir*t, kViewport);

        float param = 0.0f;
        ASSERT_TRUE(state.ScreenToAxisParam(screen, kViewport, axisOrigin, axisDir, param));
        EXPECT_NEAR(param, t, 0.1f) << "t " << t;
    }
}

TEST(SceneView3DState, AxisParamScalesWithDirectionLength)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(45.0f);
    state.distance = 300.0f;

    Vec2F screen = state.WorldToScreen(Vec3F(60.0f, 0.0f, 0.0f), kViewport);

    float paramUnit = 0.0f, paramDouble = 0.0f;
    ASSERT_TRUE(state.ScreenToAxisParam(screen, kViewport, Vec3F(), Vec3F(1.0f, 0.0f, 0.0f), paramUnit));
    ASSERT_TRUE(state.ScreenToAxisParam(screen, kViewport, Vec3F(), Vec3F(2.0f, 0.0f, 0.0f), paramDouble));
    EXPECT_NEAR(paramUnit, paramDouble*2.0f, 0.1f);
}

TEST(SceneView3DState, AxisParamFailsWhenViewRayParallelToAxis)
{
    SceneView3DState state;
    state.pitch = 0.0f;
    state.distance = 200.0f;

    // Looking straight down: the ray through the axis anchor is parallel to the Z axis
    float param = 0.0f;
    EXPECT_FALSE(state.ScreenToAxisParam(kViewport*0.5f, kViewport, Vec3F(), Vec3F(0.0f, 0.0f, 1.0f), param));
    EXPECT_FALSE(state.ScreenToAxisParam(kViewport*0.5f, kViewport, Vec3F(), Vec3F(0.0f, 0.0f, 0.0f), param));
}

TEST(SceneView3DState, PlanePoint3DMatchesGroundPlaneVersion)
{
    SceneView3DState state;
    state.target = Vec3F(15.0f, 25.0f, 0.0f);
    state.pitch = Math::Deg2rad(40.0f);
    state.yaw = Math::Deg2rad(75.0f);
    state.distance = 350.0f;

    Vec2F points[] = { kViewport*0.5f, Vec2F(200.0f, 150.0f), Vec2F(600.0f, 400.0f) };
    for (auto& point : points)
    {
        Vec2F ground;
        Vec3F hit;
        ASSERT_TRUE(state.ScreenToPlanePoint(point, kViewport, ground));
        ASSERT_TRUE(state.ScreenToPlanePoint3D(point, kViewport, Vec3F(), Vec3F(0.0f, 0.0f, 1.0f), hit));
        EXPECT_NEAR(hit.x, ground.x, 1e-3f);
        EXPECT_NEAR(hit.y, ground.y, 1e-3f);
        EXPECT_NEAR(hit.z, 0.0f, 1e-3f);
    }
}

TEST(SceneView3DState, PlanePoint3DRecoversPointOnElevatedAndVerticalPlanes)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(55.0f);
    state.yaw = Math::Deg2rad(30.0f);
    state.distance = 300.0f;

    // Elevated horizontal plane z = 50
    Vec3F pointOnPlane(20.0f, -35.0f, 50.0f);
    Vec2F screen = state.WorldToScreen(pointOnPlane, kViewport);

    Vec3F hit;
    ASSERT_TRUE(state.ScreenToPlanePoint3D(screen, kViewport, Vec3F(0.0f, 0.0f, 50.0f), Vec3F(0.0f, 0.0f, 1.0f), hit));
    EXPECT_TRUE(Near3(hit, pointOnPlane, 0.1f));

    // Vertical plane x = 10
    Vec3F pointOnVertical(10.0f, 40.0f, 25.0f);
    screen = state.WorldToScreen(pointOnVertical, kViewport);

    ASSERT_TRUE(state.ScreenToPlanePoint3D(screen, kViewport, Vec3F(10.0f, 0.0f, 0.0f), Vec3F(1.0f, 0.0f, 0.0f), hit));
    EXPECT_TRUE(Near3(hit, pointOnVertical, 0.1f));
}

TEST(SceneView3DState, PlanePoint3DFailsWhenRayParallelToPlane)
{
    SceneView3DState state;
    state.pitch = 0.0f;
    state.distance = 200.0f;

    // Looking straight down: a vertical plane through the view ray is parallel to it
    Vec3F hit;
    EXPECT_FALSE(state.ScreenToPlanePoint3D(kViewport*0.5f, kViewport, Vec3F(), Vec3F(1.0f, 0.0f, 0.0f), hit));
}

TEST(SceneView3DState, LookKeepsCameraPositionAndChangesAngles)
{
    SceneView3DState state;
    state.target = Vec3F(10.0f, -20.0f, 0.0f);
    state.yaw = Math::Deg2rad(30.0f);
    state.pitch = Math::Deg2rad(45.0f);
    state.distance = 300.0f;

    Vec3F cameraBefore = state.GetCameraPosition();
    float yawBefore = state.yaw, pitchBefore = state.pitch;

    state.Look(Vec2F(Math::Deg2rad(15.0f), Math::Deg2rad(-10.0f)));

    EXPECT_TRUE(Near3(state.GetCameraPosition(), cameraBefore));
    EXPECT_NEAR(state.yaw, yawBefore + Math::Deg2rad(15.0f), 0.001f);
    EXPECT_NEAR(state.pitch, pitchBefore - Math::Deg2rad(10.0f), 0.001f);
}

TEST(SceneView3DState, LookCanRiseAboveHorizon)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(85.0f);
    state.distance = 100.0f;

    Vec3F cameraBefore = state.GetCameraPosition();
    state.Look(Vec2F(0.0f, Math::Deg2rad(30.0f)));

    // Pitch is free to pass the horizon (90 degrees): the camera looks up into the sky
    EXPECT_NEAR(state.pitch, Math::Deg2rad(115.0f), 0.001f);
    EXPECT_TRUE(Near3(state.GetCameraPosition(), cameraBefore));

    Vec3F forward = state.GetRotation()*Vec3F(0.0f, 0.0f, -1.0f);
    EXPECT_GT(forward.z, 0.1f);
}

TEST(SceneView3DState, LookClampsPitchBeforePoleFlip)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(170.0f);
    state.distance = 100.0f;

    Vec3F cameraBefore = state.GetCameraPosition();
    state.Look(Vec2F(0.0f, Math::Deg2rad(30.0f)));

    EXPECT_NEAR(state.pitch, SceneView3DState::maxPitch, 0.001f);
    EXPECT_TRUE(Near3(state.GetCameraPosition(), cameraBefore));
}

TEST(SceneView3DState, ViewMathStaysValidAboveHorizon)
{
    SceneView3DState state;
    state.pitch = Math::Deg2rad(120.0f);
    state.distance = 200.0f;

    // The view center ray still passes through the orbit target on the plane
    Vec2F planePoint;
    ASSERT_TRUE(state.ScreenToPlanePoint(kViewport*0.5f, kViewport, planePoint));
    EXPECT_TRUE(Near2(planePoint, Vec2F(), 0.1f));

    // Pan must not blow up on sky-facing rays (camera-plane fallback covers misses)
    Vec3F targetBefore = state.target;
    state.Pan(Vec2F(30.0f, 10.0f), kViewport);
    EXPECT_TRUE((state.target - targetBefore).Length() < 1000.0f);

    // Axis projection math keeps working with the sky-facing view
    float param = 0.0f;
    EXPECT_TRUE(state.ScreenToAxisParam(kViewport*0.5f, kViewport, Vec3F(), Vec3F(1.0f, 0.0f, 0.0f), param));
}

TEST(SceneView3DState, FlyForwardMovesCameraTowardLookDirection)
{
    SceneView3DState state;
    state.target = Vec3F();
    state.yaw = 0.0f;
    state.pitch = Math::Deg2rad(90.0f - 0.5f);
    state.distance = 100.0f;

    Vec3F cameraBefore = state.GetCameraPosition();
    Quat rotation = state.GetRotation();
    Vec3F forward = rotation*Vec3F(0.0f, 0.0f, -1.0f);

    state.Fly(Vec3F(0.0f, 0.0f, 50.0f));

    EXPECT_TRUE(Near3(state.GetCameraPosition(), cameraBefore + forward*50.0f));
    EXPECT_NEAR(state.distance, 100.0f, 0.001f);
}

TEST(SceneView3DState, FlyRightAndUpMoveAlongCameraAxes)
{
    SceneView3DState state;
    state.yaw = Math::Deg2rad(40.0f);
    state.pitch = Math::Deg2rad(30.0f);
    state.distance = 200.0f;

    Vec3F cameraBefore = state.GetCameraPosition();
    Quat rotation = state.GetRotation();
    Vec3F right = rotation*Vec3F(1.0f, 0.0f, 0.0f);
    Vec3F up = rotation*Vec3F(0.0f, 1.0f, 0.0f);

    state.Fly(Vec3F(10.0f, 20.0f, 0.0f));

    EXPECT_TRUE(Near3(state.GetCameraPosition(), cameraBefore + right*10.0f + up*20.0f));
}
