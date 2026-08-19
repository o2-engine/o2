#include <gtest/gtest.h>

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/PhysicsWorld.h"
#include "o2/Physics/PhysicsWorld3D.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Physics/BoxCollider.h"
#include "o2/Scene/Physics/DistanceJoint.h"
#include "o2/Scene/Physics/RigidBody.h"
#include "o2/Scene/Physics3D/BoxCollider3D.h"
#include "o2/Scene/Physics3D/DistanceJoint3D.h"
#include "o2/Scene/Physics3D/RigidBody3D.h"
#include "o2/Scene/Scene.h"

#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    struct JointsGuard
    {
        SceneCleanGuard sceneGuard;
        ActorCreateMode prevMode;
        PhysicsConfig   prevPhysics;
        Physics3DConfig prevPhysics3D;

        JointsGuard(): prevMode(Actor::GetDefaultCreationMode()), prevPhysics(o2Config.physics),
            prevPhysics3D(o2Config.physics3D)
        {
            Actor::SetDefaultCreationMode(ActorCreateMode::InScene);
            o2Config.physics.scale = 10.0f; // project settings scale would shrink shapes below Box2D vertex welding
            o2Config.physics3D.gravity = Vec3F(0, -9.81f, 0);
            o2Config.physics3D.scale = 1.0f;
        }

        ~JointsGuard()
        {
            o2Config.physics = prevPhysics;
            o2Config.physics3D = prevPhysics3D;
            Actor::SetDefaultCreationMode(prevMode);
        }
    };

    void Step2D(float dt = 1.0f/60.0f) { o2Physics.PreUpdate(); o2Physics.Update(dt); o2Physics.PostUpdate(); }
    void Step3D(float dt = 1.0f/60.0f) { o2Physics3D.PreUpdate(); o2Physics3D.Update(dt); o2Physics3D.PostUpdate(); }
}

TEST(PhysicsJoints, Distance2DHoldsHangingBody)
{
    JointsGuard guard;

    auto anchor = mmake<RigidBody>();
    anchor->SetBodyType(RigidBody::Type::Static);
    anchor->transform->SetPosition(Vec2F(0, 0));
    anchor->AddComponent<BoxCollider>()->SetSize(Vec2F(10, 10));

    auto body = mmake<RigidBody>();
    body->SetBodyType(RigidBody::Type::Dynamic);
    body->transform->SetPosition(Vec2F(0, -50));
    body->AddComponent<BoxCollider>()->SetSize(Vec2F(10, 10));

    auto jointActor = mmake<Actor>();
    auto joint = jointActor->AddComponent<DistanceJoint>();
    joint->SetBodyA(anchor);
    joint->SetBodyB(body);

    TickFrame(); // create bodies + joint

    for (int i = 0; i < 120; i++) { Step2D(); TickFrame(); }

    float dist = (body->transform->GetWorldPosition2D() - anchor->transform->GetWorldPosition2D()).Length();
    EXPECT_NEAR(dist, 50.0f, 8.0f);                            // distance joint keeps the length
    EXPECT_LT(body->transform->GetWorldPosition2D().y, -30.0f); // hangs below the static anchor
}

TEST(PhysicsJoints, Distance3DHoldsHangingBody)
{
    JointsGuard guard;

    auto anchor = mmake<RigidBody3D>();
    anchor->SetBodyType(RigidBody3D::Type::Static);
    anchor->transform->SetPosition(Vec3F(0, 0, 0));
    anchor->AddComponent<BoxCollider3D>()->SetSize(Vec3F(1, 1, 1));

    auto body = mmake<RigidBody3D>();
    body->SetBodyType(RigidBody3D::Type::Dynamic);
    body->transform->SetPosition(Vec3F(0, -5, 0));
    body->AddComponent<BoxCollider3D>()->SetSize(Vec3F(1, 1, 1));

    auto jointActor = mmake<Actor>();
    auto joint = jointActor->AddComponent<DistanceJoint3D>();
    joint->SetLength(5.0f);
    joint->SetBodyA(anchor);
    joint->SetBodyB(body);

    TickFrame();

    for (int i = 0; i < 180; i++) { Step3D(); TickFrame(); }

    float dist = (body->transform->GetWorldPosition() - anchor->transform->GetWorldPosition()).Length();
    EXPECT_NEAR(dist, 5.0f, 1.0f);
    EXPECT_LT(body->transform->GetWorldPosition().y, -3.0f);
}
