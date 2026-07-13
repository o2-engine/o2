#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "Scene/Scene3DTestHelpers.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    bool Vec3Near(const Vec3F& a, const Vec3F& b, float eps = kEps)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps && Math::Abs(a.z - b.z) < eps;
    }
}

TEST(Scene3D, BuildTest3DSceneCreatesExpectedActors)
{
    SceneCleanGuard guard;
    auto scene = BuildTest3DScene();
    TickFrame();

    EXPECT_EQ(scene.camera->GetCameraType(), CameraActor::Type::Perspective);
    EXPECT_TRUE(Vec3Near(scene.camera->transform->GetPosition(), Vec3F(0, -500, 250)));

    for (auto& actor : { scene.ground, scene.box1, scene.box2, scene.sphere, scene.cylinder })
    {
        ASSERT_TRUE(actor);
        auto component = actor->GetComponent<MeshPrimitiveComponent>();
        ASSERT_TRUE(component);
        EXPECT_GT(component->GetMesh().vertexCount, 0u);
    }
}

TEST(Scene3D, SceneSaveLoadRoundTripPreserves3DScene)
{
    SceneCleanGuard guard;
    auto scene = BuildTest3DScene();
    TickFrame();

    Vec3F savedBox2Position = scene.box2->transform->GetPosition();
    Vec3F savedBox2Euler = scene.box2->transform->GetEulerAngles();
    int savedRootCount = o2Scene.GetRootActors().Count();

    DataDocument savedDoc;
    o2Scene.Save(savedDoc);

    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
    EXPECT_EQ(o2Scene.GetRootActors().Count(), 0);

    o2Scene.Load(savedDoc);
    TickFrame();

    EXPECT_EQ(o2Scene.GetRootActors().Count(), savedRootCount);

    auto camera = DynamicCast<CameraActor>(o2Scene.FindActor("camera3d"));
    ASSERT_TRUE(camera);
    EXPECT_EQ(camera->GetCameraType(), CameraActor::Type::Perspective);

    auto box2 = o2Scene.FindActor("box2");
    ASSERT_TRUE(box2);
    EXPECT_TRUE(Vec3Near(box2->transform->GetPosition(), savedBox2Position));
    EXPECT_TRUE(Vec3Near(box2->transform->GetEulerAngles(), savedBox2Euler));

    auto box2Component = box2->GetComponent<MeshPrimitiveComponent>();
    ASSERT_TRUE(box2Component);
    EXPECT_EQ(box2Component->GetPrimitiveType(), PrimitiveType3D::Box);
    EXPECT_EQ(box2Component->GetSize(), Vec3F(80, 120, 60));

    auto sphere = o2Scene.FindActor("sphere");
    ASSERT_TRUE(sphere);
    auto sphereComponent = sphere->GetComponent<MeshPrimitiveComponent>();
    ASSERT_TRUE(sphereComponent);
    EXPECT_EQ(sphereComponent->GetPrimitiveType(), PrimitiveType3D::Sphere);
}
