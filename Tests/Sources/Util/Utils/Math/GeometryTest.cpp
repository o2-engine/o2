#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Geometry.h"
#include "o2/Utils/Math/Math.h"

using namespace o2;

TEST(Geometry, AxisPlaneBasisIsRightHandedAroundAxis)
{
    for (int axis = 0; axis < 3; axis++)
    {
        Vec3F u, v;
        Geometry::AxisPlaneBasis(axis, u, v);

        // u cross v must equal the axis for the right-handed angle convention
        Vec3F cross = u.Cross(v);
        Vec3F axisDir = Vec3F::Axis(axis);
        EXPECT_NEAR(cross.x, axisDir.x, 1e-6f);
        EXPECT_NEAR(cross.y, axisDir.y, 1e-6f);
        EXPECT_NEAR(cross.z, axisDir.z, 1e-6f);
    }
}

TEST(Geometry, AxisPlaneAngleGrowsFromUTowardsV)
{
    Vec3F center(5.0f, -3.0f, 2.0f);
    for (int axis = 0; axis < 3; axis++)
    {
        Vec3F u, v;
        Geometry::AxisPlaneBasis(axis, u, v);

        EXPECT_NEAR(Geometry::AxisPlaneAngle(center, axis, center + u*10.0f), 0.0f, 1e-5f);
        EXPECT_NEAR(Geometry::AxisPlaneAngle(center, axis, center + v*10.0f), Math::PI()*0.5f, 1e-5f);
        EXPECT_NEAR(Geometry::AxisPlaneAngle(center, axis, center + (u + v)*10.0f), Math::PI()*0.25f, 1e-5f);
    }
}

TEST(Geometry, PointToPolylineDistance)
{
    Vector<Vec2F> line = { Vec2F(0.0f, 0.0f), Vec2F(100.0f, 0.0f), Vec2F(100.0f, 100.0f) };

    EXPECT_NEAR(Geometry::PointToPolylineDistance(line, Vec2F(50.0f, 10.0f)), 10.0f, 1e-4f);
    EXPECT_NEAR(Geometry::PointToPolylineDistance(line, Vec2F(100.0f, 50.0f)), 0.0f, 1e-4f);
    EXPECT_NEAR(Geometry::PointToPolylineDistance(line, Vec2F(-30.0f, -40.0f)), 50.0f, 1e-4f);
    EXPECT_NEAR(Geometry::PointToPolylineDistance(line, Vec2F(120.0f, 120.0f)), Math::Sqrt(800.0f), 1e-3f);
}
