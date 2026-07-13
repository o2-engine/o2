#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/SceneWindow/SceneView3DState.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<Actor> MakeBoxActor(const Vec3F& position, const Vec3F& size, const Vec3F& eulerAngles = Vec3F())
    {
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->transform->SetPosition(position);
        actor->transform->SetEulerAngles(eulerAngles);

        auto component = actor->AddComponent<MeshPrimitiveComponent>();
        component->SetPrimitiveType(PrimitiveType3D::Box);
        component->SetSize(size);
        component->GetMesh();

        return actor;
    }
}

// Click ray picking must hit the box anywhere on its volume, not only at the center
TEST(SelectionPick3D, RayHitsBoxOffCenter)
{
    SceneCleanGuard guard;
    auto box = MakeBoxActor(Vec3F(0, 0, 50), Vec3F(100, 100, 100));
    TickScene();

    float distance = 0.0f;

    // Ray towards the box, offset from its center
    ASSERT_TRUE(ITransformTool::RayIntersectsObject3D(box, Vec3F(30.0f, -500.0f, 80.0f), Vec3F(0, 1, 0), distance));
    EXPECT_NEAR(distance, 450.0f, 0.5f);

    // Ray passing outside the box misses
    EXPECT_FALSE(ITransformTool::RayIntersectsObject3D(box, Vec3F(80.0f, -500.0f, 80.0f), Vec3F(0, 1, 0), distance));
    EXPECT_FALSE(ITransformTool::RayIntersectsObject3D(box, Vec3F(30.0f, -500.0f, 130.0f), Vec3F(0, 1, 0), distance));
}

// Picking respects the object rotation: the ray through the inflated axis-aligned bounds corner
// but outside the oriented box must miss
TEST(SelectionPick3D, RayRespectsBoxRotation)
{
    SceneCleanGuard guard;
    auto box = MakeBoxActor(Vec3F(0, 0, 0), Vec3F(100, 100, 100), Vec3F(0, 0, Math::Deg2rad(45.0f)));
    TickScene();

    float distance = 0.0f;

    // The rotated box reaches ~70.7 on the diagonal: the axis-aligned bound corner region is empty
    EXPECT_FALSE(ITransformTool::RayIntersectsObject3D(box, Vec3F(60.0f, 60.0f, 500.0f), Vec3F(0, 0, -1), distance));

    // The diagonal tip is inside the oriented box
    EXPECT_TRUE(ITransformTool::RayIntersectsObject3D(box, Vec3F(65.0f, 0.0f, 500.0f), Vec3F(0, 0, -1), distance));
}

// The nearest object along the ray must produce a smaller hit distance
TEST(SelectionPick3D, RayDistanceOrdersObjects)
{
    SceneCleanGuard guard;
    auto nearBox = MakeBoxActor(Vec3F(0, -100, 0), Vec3F(50, 50, 50));
    auto farBox = MakeBoxActor(Vec3F(0, 200, 0), Vec3F(50, 50, 50));
    TickScene();

    float nearDistance = 0.0f, farDistance = 0.0f;
    ASSERT_TRUE(ITransformTool::RayIntersectsObject3D(nearBox, Vec3F(0, -500, 0), Vec3F(0, 1, 0), nearDistance));
    ASSERT_TRUE(ITransformTool::RayIntersectsObject3D(farBox, Vec3F(0, -500, 0), Vec3F(0, 1, 0), farDistance));
    EXPECT_LT(nearDistance, farDistance);
}

// Frame selection: the projected screen rect of the object intersecting the frame selects it,
// a frame touching only the object edge counts, a frame beside the object doesn't
TEST(SelectionPick3D, FrameSelectionUsesProjectedRectIntersection)
{
    SceneCleanGuard guard;
    auto box = MakeBoxActor(Vec3F(0, 0, 50), Vec3F(100, 100, 100));
    TickScene();

    SceneView3DState view;
    view.pitch = Math::Deg2rad(60.0f);
    view.distance = 600.0f;

    const Vec2F viewportSize(800.0f, 600.0f);
    Function<Vec2F(const Vec3F&)> projector = [&](const Vec3F& worldPoint)
    {
        return view.WorldToScreen(worldPoint, viewportSize);
    };

    RectF objectRect;
    ASSERT_TRUE(ITransformTool::GetObjectScreenRect3D(box, projector, objectRect));
    EXPECT_GT(objectRect.Width(), 1.0f);
    EXPECT_GT(objectRect.Height(), 1.0f);

    // Frame that only grazes the left edge of the object rect still intersects
    RectF grazingFrame(Vec2F(objectRect.left - 100.0f, objectRect.bottom - 20.0f),
                       Vec2F(objectRect.left + 2.0f, objectRect.top + 20.0f));
    EXPECT_TRUE(grazingFrame.IsIntersects(objectRect));

    // Frame beside the object doesn't
    RectF missingFrame(Vec2F(objectRect.left - 100.0f, objectRect.bottom),
                       Vec2F(objectRect.left - 5.0f, objectRect.top));
    EXPECT_FALSE(missingFrame.IsIntersects(objectRect));

    // The old single-point test would miss this frame even though it covers half of the object
    Vec2F projectedCenter = projector(Vec3F(0, 0, 50));
    RectF halfCoveringFrame(Vec2F(objectRect.left - 10.0f, objectRect.bottom - 10.0f),
                            Vec2F(projectedCenter.x - 5.0f, objectRect.top + 10.0f));
    EXPECT_FALSE(halfCoveringFrame.IsInside(projectedCenter));
    EXPECT_TRUE(halfCoveringFrame.IsIntersects(objectRect));
}
