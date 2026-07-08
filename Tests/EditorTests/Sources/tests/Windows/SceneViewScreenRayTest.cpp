#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Windows/SceneWindow/SceneView3DState.h"

using namespace o2;
using namespace Editor;

TEST(SceneViewScreenRay, CenterRayPointsFromCameraTowardsTarget)
{
    SceneView3DState view;
    view.target = Vec3F(50.0f, -20.0f, 10.0f);
    view.yaw = 0.7f;
    view.pitch = Math::Deg2rad(35.0f);
    view.distance = 300.0f;

    Vec2F viewportSize(800.0f, 600.0f);

    Vec3F origin, direction;
    ASSERT_TRUE(view.GetScreenRay(viewportSize*0.5f, viewportSize, origin, direction));

    Vec3F camera = view.GetCameraPosition();
    Vec3F expected = (view.target - camera).Normalized();

    EXPECT_NEAR(direction.x, expected.x, 1e-3f);
    EXPECT_NEAR(direction.y, expected.y, 1e-3f);
    EXPECT_NEAR(direction.z, expected.z, 1e-3f);
    EXPECT_NEAR(direction.Length(), 1.0f, 1e-4f);

    // Origin is on the near plane, close to the camera
    EXPECT_LT((origin - camera).Length(), 1.0f);
}

TEST(SceneViewScreenRay, RayMatchesPlaneUnprojection)
{
    SceneView3DState view;
    view.pitch = Math::Deg2rad(45.0f);
    view.yaw = 0.3f;
    view.distance = 400.0f;

    Vec2F viewportSize(800.0f, 600.0f);
    Vec2F viewportPoint(600.0f, 200.0f);

    Vec3F origin, direction;
    ASSERT_TRUE(view.GetScreenRay(viewportPoint, viewportSize, origin, direction));

    Vec2F planePoint;
    ASSERT_TRUE(view.ScreenToPlanePoint(viewportPoint, viewportSize, planePoint));

    // Manual z=0 plane intersection of the ray reproduces ScreenToPlanePoint
    ASSERT_GT(Math::Abs(direction.z), 1e-6f);
    float t = -origin.z/direction.z;
    Vec3F hit = origin + direction*t;

    EXPECT_NEAR(hit.x, planePoint.x, 0.01f);
    EXPECT_NEAR(hit.y, planePoint.y, 0.01f);
}

TEST(SceneViewScreenRay, DegenerateViewportFails)
{
    SceneView3DState view;

    Vec3F origin, direction;
    EXPECT_FALSE(view.GetScreenRay(Vec2F(), Vec2F(0.0f, 0.0f), origin, direction));
}
