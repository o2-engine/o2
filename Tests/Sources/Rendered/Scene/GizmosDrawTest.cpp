#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Gizmos.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Physics/BoxCollider.h"
#include "o2/Scene/Physics/CircleCollider.h"
#include "o2/Scene/Physics/DistanceJoint.h"
#include "o2/Scene/Physics/RigidBody.h"
#include "o2/Utils/Math/Math.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    // Replaces gizmos projection with collecting one, so drawn world points can be checked
    struct GizmosCapture
    {
        Vector<Vec3F> points;

        GizmosCapture(const Vec3F& clipPlaneOrigin = Vec3F(), const Vec3F& clipPlaneNormal = Vec3F())
        {
            o2Gizmos.ResetDrawnPrimitives();
            o2Gizmos.SetProjection([this](const Vec3F& point)
                                   {
                                       points.Add(point);
                                       return Vec2F(point.x, point.y);
                                   },
                                   clipPlaneOrigin, clipPlaneNormal);
        }

        ~GizmosCapture() { o2Gizmos.ResetProjection(); }

        bool HasPoint(const Vec3F& point, float eps = 0.01f) const
        {
            return points.Contains([&](const Vec3F& x) { return (x - point).Length() < eps; });
        }
    };
}

TEST(Gizmos, BoxColliderDrawsOutlineAroundActor)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>();
    actor->transform->SetPosition(Vec2F(100, 50));
    actor->transform->SetSize2D(Vec2F(20, 10));
    auto collider = actor->AddComponent<BoxCollider>();
    TickFrame();

    GizmosCapture capture;
    collider->DrawGizmos();

    EXPECT_EQ(capture.points.Count(), 4);
    EXPECT_TRUE(capture.HasPoint(Vec3F(90, 45, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(110, 45, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(110, 55, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(90, 55, 0)));
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 1);
}

TEST(Gizmos, CircleColliderDrawsCircleWithRadius)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>();
    actor->transform->SetPosition(Vec2F(30, 40));
    actor->transform->SetSize2D(Vec2F(16, 16));
    auto collider = actor->AddComponent<CircleCollider>();
    TickFrame();

    GizmosCapture capture;
    collider->DrawGizmos();

    ASSERT_GT(capture.points.Count(), 8);
    for (auto& point : capture.points)
        EXPECT_NEAR((point - Vec3F(30, 40, 0)).Length(), collider->GetRadius(), 0.01f);
}

TEST(Gizmos, JointDrawsLinksToBothBodies)
{
    SceneCleanGuard guard;

    auto bodyA = mmake<RigidBody>();
    bodyA->transform->SetPosition(Vec2F(-100, 0));

    auto bodyB = mmake<RigidBody>();
    bodyB->transform->SetPosition(Vec2F(100, 0));

    auto jointActor = mmake<Actor>();
    jointActor->transform->SetPosition(Vec2F(0, 20));
    auto joint = jointActor->AddComponent<DistanceJoint>();
    joint->SetBodyA(bodyA);
    joint->SetBodyB(bodyB);
    TickFrame();

    GizmosCapture capture;
    joint->DrawGizmos();

    EXPECT_TRUE(capture.HasPoint(Vec3F(0, 20, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(-100, 0, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(100, 0, 0)));
}

TEST(Gizmos, PerspectiveCameraDrawsFrustum)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    camera->SetPerspective(Math::Deg2rad(60.0f), 10.0f, 100.0f);
    camera->transform->SetPosition(Vec3F(0, 0, 0));
    TickFrame();

    GizmosCapture capture;
    camera->DrawGizmos();

    ASSERT_EQ(capture.points.Count(), 16); // near and far rectangles plus four side edges

    // camera looks along -z, so near plane corners are exactly near clip away by z
    int nearPlanePoints = capture.points.Count([](const Vec3F& point) { return Math::Equals(point.z, -10.0f, 0.01f); });
    int farPlanePoints = capture.points.Count([](const Vec3F& point) { return Math::Equals(point.z, -100.0f, 0.01f); });

    EXPECT_EQ(nearPlanePoints, 8);
    EXPECT_EQ(farPlanePoints, 8);
}

TEST(Gizmos, ClipPlaneCutsLineCrossingIt)
{
    GizmosCapture capture(Vec3F(), Vec3F(0, 0, 1));

    o2Gizmos.DrawLine(Vec3F(0, 0, 10), Vec3F(20, 0, -10));

    ASSERT_EQ(capture.points.Count(), 2);
    EXPECT_TRUE(capture.HasPoint(Vec3F(0, 0, 10)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(10, 0, 0)));
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 1);
}

TEST(Gizmos, ClipPlaneSkipsGeometryFullyBehindIt)
{
    GizmosCapture capture(Vec3F(), Vec3F(0, 0, 1));

    o2Gizmos.DrawLine(Vec3F(0, 0, -10), Vec3F(20, 0, -5));
    o2Gizmos.DrawPoint(Vec3F(5, 5, -1));

    EXPECT_TRUE(capture.points.IsEmpty());
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 0);
}

TEST(Gizmos, ClipPlaneSplitsPolyLineIntoVisibleRuns)
{
    GizmosCapture capture(Vec3F(), Vec3F(0, 0, 1));

    o2Gizmos.DrawPolyLine({ Vec3F(0, 0, 10), Vec3F(0, 0, -10), Vec3F(5, 0, -10), Vec3F(5, 0, 10) });

    ASSERT_EQ(capture.points.Count(), 4);
    EXPECT_TRUE(capture.HasPoint(Vec3F(0, 0, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(5, 0, 0)));
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 2);
}

TEST(Gizmos, ClipPlaneCutsClosedShape)
{
    GizmosCapture capture(Vec3F(), Vec3F(0, 0, 1));

    o2Gizmos.DrawRect(Vec3F(), Vec3F(10, 0, 0), Vec3F(0, 0, 10));

    ASSERT_EQ(capture.points.Count(), 4);
    EXPECT_TRUE(capture.HasPoint(Vec3F(10, 0, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(-10, 0, 0)));

    for (auto& point : capture.points)
        EXPECT_GE(point.z, -0.01f);

    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 1);
}

TEST(Gizmos, WithoutClipPlaneAllPointsAreProjected)
{
    GizmosCapture capture;

    o2Gizmos.DrawLine(Vec3F(0, 0, 10), Vec3F(20, 0, -10));
    o2Gizmos.DrawPoint(Vec3F(5, 5, -1));

    EXPECT_EQ(capture.points.Count(), 3);
    EXPECT_TRUE(capture.HasPoint(Vec3F(20, 0, -10)));
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 3); // one line and two point strokes
}

TEST(Gizmos, ComponentWithoutGizmosDrawsNothing)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>();
    auto image = actor->AddComponent<ImageComponent>();
    TickFrame();

    GizmosCapture capture;
    image->DrawGizmos();
    actor->DrawGizmos();

    EXPECT_EQ(capture.points.Count(), 0);
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 0);
}

TEST(Gizmos, BoxDrawsAllTwelveEdgesAsSixPolyLines)
{
    GizmosCapture capture;

    Vec3F center(10, 20, 30);
    Vec3F halfX(5, 0, 0), halfY(0, 6, 0), halfZ(0, 0, 7);
    o2Gizmos.DrawBox(center, halfX, halfY, halfZ);

    // Two face rings and the four edges between them cover the same 8 corners as 12 separate segments
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 6);

    for (int x = -1; x <= 1; x += 2)
    {
        for (int y = -1; y <= 1; y += 2)
        {
            for (int z = -1; z <= 1; z += 2)
                EXPECT_TRUE(capture.HasPoint(center + halfX*(float)x + halfY*(float)y + halfZ*(float)z));
        }
    }
}

TEST(Gizmos, RepeatedDrawingKeepsProjectedPointsOfEachPrimitive)
{
    GizmosCapture capture;

    // The projected points buffer is reused between primitives: it must be refilled, not appended to
    o2Gizmos.DrawLine(Vec3F(0, 0, 0), Vec3F(1, 0, 0));
    o2Gizmos.DrawLine(Vec3F(2, 0, 0), Vec3F(3, 0, 0));
    o2Gizmos.DrawRect(Vec3F(10, 10, 0), Vec3F(1, 0, 0), Vec3F(0, 1, 0));

    EXPECT_EQ(capture.points.Count(), 8);
    EXPECT_EQ(o2Gizmos.GetDrawnPrimitives(), 3);
    EXPECT_TRUE(capture.HasPoint(Vec3F(3, 0, 0)));
    EXPECT_TRUE(capture.HasPoint(Vec3F(11, 11, 0)));
}
