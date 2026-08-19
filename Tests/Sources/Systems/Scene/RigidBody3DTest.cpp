#include <gtest/gtest.h>

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/Box3DConvert.h"
#include "o2/Physics/PhysicsWorld3D.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Physics3D/BoxCollider3D.h"
#include "o2/Scene/Physics3D/RigidBody3D.h"
#include "o2/Scene/Scene.h"

#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    // Cleans the scene, forces new actors into the scene so RigidBody3D creates its body, and pins a
    // deterministic 3D physics config (Y-down gravity, unit scale) so the tests don't depend on
    // whatever ProjectSettings.json ships.
    struct Physics3DGuard
    {
        SceneCleanGuard sceneGuard;
        ActorCreateMode prevMode;
        Physics3DConfig prevPhysics;

        Physics3DGuard(): prevMode(Actor::GetDefaultCreationMode()), prevPhysics(o2Config.physics3D)
        {
            Actor::SetDefaultCreationMode(ActorCreateMode::InScene);
            o2Config.physics3D.gravity = Vec3F(0, -9.81f, 0);
            o2Config.physics3D.scale = 1.0f;
            o2Config.physics3D.subStepCount = 4;
        }

        ~Physics3DGuard()
        {
            o2Config.physics3D = prevPhysics;
            Actor::SetDefaultCreationMode(prevMode);
        }
    };

    // The headless test scene tick does not run the Integration fixed loop, so drive the world by hand.
    void StepPhysics3D(int steps, float dt = 1.0f/60.0f)
    {
        for (int i = 0; i < steps; i++)
        {
            o2Physics3D.PreUpdate();
            o2Physics3D.Update(dt);
            o2Physics3D.PostUpdate();
        }
    }

    Ref<RigidBody3D> MakeBoxBody(RigidBody3D::Type type, const Vec3F& pos, const Vec3F& size)
    {
        auto body = mmake<RigidBody3D>();
        body->SetBodyType(type);
        body->transform->SetPosition(pos);

        auto collider = body->AddComponent<BoxCollider3D>();
        collider->SetSize(size);

        return body;
    }
}

TEST(Physics3D, Box3DMathRoundTrip)
{
    Vec3F v(1.0f, -2.5f, 3.25f);
    Vec3F v2 = FromBox3D(ToBox3D(v));
    EXPECT_NEAR(v2.x, v.x, 1e-6f);
    EXPECT_NEAR(v2.y, v.y, 1e-6f);
    EXPECT_NEAR(v2.z, v.z, 1e-6f);

    Quat q = Quat::FromEuler(Vec3F(0.3f, -0.7f, 1.1f)).Normalized();
    Quat q2 = FromBox3D(ToBox3D(q));
    EXPECT_NEAR(q2.x, q.x, 1e-6f);
    EXPECT_NEAR(q2.y, q.y, 1e-6f);
    EXPECT_NEAR(q2.z, q.z, 1e-6f);
    EXPECT_NEAR(q2.w, q.w, 1e-6f);
}

TEST(Physics3D, DynamicBodyFallsUnderGravity)
{
    Physics3DGuard guard;

    auto body = MakeBoxBody(RigidBody3D::Type::Dynamic, Vec3F(0, 10, 0), Vec3F(1, 1, 1));
    TickFrame(); // fires OnAddToScene, creating the Box3D body and its shape

    float startY = body->transform->GetPosition().y;

    StepPhysics3D(30); // 0.5 s

    float drop = startY - body->transform->GetPosition().y;

    // Free fall over 0.5 s is ~0.5 * 9.81 * 0.5^2 ~= 1.23; allow generous solver tolerance.
    EXPECT_GT(drop, 0.8f);
    EXPECT_LT(drop, 1.8f);
}

TEST(Physics3D, StaticBodyDoesNotMove)
{
    Physics3DGuard guard;

    auto body = MakeBoxBody(RigidBody3D::Type::Static, Vec3F(0, 0, 0), Vec3F(4, 1, 4));
    TickFrame();

    StepPhysics3D(60);

    Vec3F p = body->transform->GetPosition();
    EXPECT_NEAR(p.x, 0.0f, 1e-4f);
    EXPECT_NEAR(p.y, 0.0f, 1e-4f);
    EXPECT_NEAR(p.z, 0.0f, 1e-4f);
}

TEST(Physics3D, StaticFloorStopsDynamicBox)
{
    Physics3DGuard guard;

    MakeBoxBody(RigidBody3D::Type::Static, Vec3F(0, 0, 0), Vec3F(20, 1, 20));
    auto box = MakeBoxBody(RigidBody3D::Type::Dynamic, Vec3F(0, 4, 0), Vec3F(1, 1, 1));
    TickFrame();

    StepPhysics3D(240); // 4 s to settle

    float y = box->transform->GetPosition().y;

    // Floor top at y=0.5, box half-height 0.5 -> resting center near y=1.0.
    EXPECT_NEAR(y, 1.0f, 0.2f);
    EXPECT_LT(box->GetLinearVelocity().Length(), 0.25f);
}

TEST(Physics3D, BoxesStackWithoutInterpenetration)
{
    Physics3DGuard guard;

    MakeBoxBody(RigidBody3D::Type::Static, Vec3F(0, 0, 0), Vec3F(20, 1, 20));
    auto lower = MakeBoxBody(RigidBody3D::Type::Dynamic, Vec3F(0, 1.05f, 0), Vec3F(1, 1, 1));
    auto upper = MakeBoxBody(RigidBody3D::Type::Dynamic, Vec3F(0, 2.15f, 0), Vec3F(1, 1, 1));
    TickFrame();

    StepPhysics3D(360); // 6 s

    float ly = lower->transform->GetPosition().y;
    float uy = upper->transform->GetPosition().y;

    EXPECT_NEAR(ly, 1.0f, 0.25f);
    EXPECT_NEAR(uy, 2.0f, 0.3f);
    EXPECT_GT(uy - ly, 0.85f); // boxes are 1 unit tall, so centers stay ~1 apart
}

TEST(Physics3D, RestingBodyGoesToSleep)
{
    Physics3DGuard guard;

    MakeBoxBody(RigidBody3D::Type::Static, Vec3F(0, 0, 0), Vec3F(20, 1, 20));
    auto box = MakeBoxBody(RigidBody3D::Type::Dynamic, Vec3F(0, 4, 0), Vec3F(1, 1, 1));
    TickFrame();

    StepPhysics3D(300); // 5 s: land and settle

    // The per-frame actor->body sync must recognize its own writeback as "not moved externally",
    // otherwise it teleports and wakes every body every step, and the world never stops solving
    EXPECT_TRUE(box->IsSleeping());
}

TEST(Physics3D, SleepingBodyWakesOnExternalMove)
{
    Physics3DGuard guard;

    MakeBoxBody(RigidBody3D::Type::Static, Vec3F(0, 0, 0), Vec3F(20, 1, 20));
    auto box = MakeBoxBody(RigidBody3D::Type::Dynamic, Vec3F(0, 4, 0), Vec3F(1, 1, 1));
    TickFrame();

    StepPhysics3D(300);
    ASSERT_TRUE(box->IsSleeping());

    box->transform->SetPosition(Vec3F(0, 6, 0));
    StepPhysics3D(1);

    EXPECT_FALSE(box->IsSleeping());

    StepPhysics3D(60);
    EXPECT_LT(box->transform->GetPosition().y, 6.0f); // followed the teleport, then fell again
}

TEST(Physics3D, SleepingBodyWakesOnSmallExternalRotation)
{
    Physics3DGuard guard;

    MakeBoxBody(RigidBody3D::Type::Static, Vec3F(0, 0, 0), Vec3F(20, 1, 20));
    auto box = MakeBoxBody(RigidBody3D::Type::Dynamic, Vec3F(0, 4, 0), Vec3F(1, 1, 1));
    TickFrame();

    StepPhysics3D(300);
    ASSERT_TRUE(box->IsSleeping());

    box->transform->SetEulerAngles(box->transform->GetEulerAngles() + Vec3F(0.05f, 0, 0));
    StepPhysics3D(1);

    EXPECT_FALSE(box->IsSleeping());
}
