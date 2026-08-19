#include <gtest/gtest.h>

#include "Box2D/Dynamics/b2Body.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Physics/BoxCollider.h"
#include "o2/Scene/Physics/RigidBody.h"

#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    struct MassGuard
    {
        SceneCleanGuard sceneGuard;
        ActorCreateMode prevMode;
        PhysicsConfig   prevPhysics;

        MassGuard(): prevMode(Actor::GetDefaultCreationMode()), prevPhysics(o2Config.physics)
        {
            Actor::SetDefaultCreationMode(ActorCreateMode::InScene);
            o2Config.physics.scale = 10.0f; // project settings scale would shrink shapes below Box2D vertex welding
        }

        ~MassGuard()
        {
            o2Config.physics = prevPhysics;
            Actor::SetDefaultCreationMode(prevMode);
        }
    };

    Ref<RigidBody> MakeBodyWithOffsetCollider()
    {
        auto body = mmake<RigidBody>();
        body->SetBodyType(RigidBody::Type::Dynamic);
        body->transform->SetPosition(Vec2F(0, 0));

        auto colliderActor = mmake<Actor>();
        colliderActor->SetParent(body);
        colliderActor->transform->SetPosition(Vec2F(50, 0));

        TickFrame(); // the collider shape is built from the child world transform, so it must be computed first

        colliderActor->AddComponent<BoxCollider>()->SetSize(Vec2F(10, 10));

        return body;
    }
}

TEST(PhysicsRigidBodyMass, SetMassKeepsCenterOfMass)
{
    MassGuard guard;

    auto body = MakeBodyWithOffsetCollider();
    ASSERT_NE(body->GetBody(), nullptr);

    b2Vec2 center = body->GetBody()->GetLocalCenter();
    ASSERT_GT(center.Length(), 0.1f);

    body->SetMass(5.0f);

    EXPECT_NEAR(body->GetBody()->GetMass(), 5.0f, 0.001f);
    EXPECT_NEAR(body->GetBody()->GetLocalCenter().x, center.x, 0.001f);
    EXPECT_NEAR(body->GetBody()->GetLocalCenter().y, center.y, 0.001f);
    EXPECT_GT(body->GetBody()->GetInertia(), 0.0f);
}

TEST(PhysicsRigidBodyMass, SetInertiaWithOffsetCenterStaysPositive)
{
    MassGuard guard;

    auto body = MakeBodyWithOffsetCollider();
    ASSERT_NE(body->GetBody(), nullptr);

    body->SetMass(5.0f);
    body->SetInertia(2.0f);

    b2Vec2 center = body->GetBody()->GetLocalCenter();
    float mass = body->GetBody()->GetMass();
    float inertiaAboutCenter = body->GetBody()->GetInertia() - mass*b2Dot(center, center);

    EXPECT_NEAR(inertiaAboutCenter, 2.0f, 0.001f);
}

TEST(PhysicsRigidBodyMass, MassAppliedOnBodyCreation)
{
    MassGuard guard;

    auto body = mmake<RigidBody>();
    body->SetBodyType(RigidBody::Type::Dynamic);
    body->SetMass(7.0f);

    TickFrame();

    ASSERT_NE(body->GetBody(), nullptr);
    EXPECT_NEAR(body->GetBody()->GetMass(), 7.0f, 0.001f);
}
