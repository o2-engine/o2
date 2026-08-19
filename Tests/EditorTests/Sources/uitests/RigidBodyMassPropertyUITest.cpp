#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Box2D/Dynamics/b2Body.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Physics/BoxCollider.h"
#include "o2/Scene/Physics/RigidBody.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Properties.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct CreateModeGuard
    {
        ActorCreateMode prevMode;
        PhysicsConfig   prevPhysics;

        CreateModeGuard(): prevMode(Actor::GetDefaultCreationMode()), prevPhysics(o2Config.physics)
        {
            Actor::SetDefaultCreationMode(ActorCreateMode::InScene);
            o2Config.physics.scale = 10.0f; // project settings scale would shrink shapes below Box2D vertex welding
        }

        ~CreateModeGuard()
        {
            o2Config.physics = prevPhysics;
            Actor::SetDefaultCreationMode(prevMode);
        }
    };

    Ref<RigidBody> MakeBodyWithOffsetCollider()
    {
        auto body = mmake<RigidBody>();
        body->SetBodyType(RigidBody::Type::Dynamic);
        body->transform->SetPosition2D(Vec2F(0, 0));

        auto colliderActor = MakeActor();
        colliderActor->SetParent(body);
        colliderActor->transform->SetPosition2D(Vec2F(50, 0));
        colliderActor->transform->SetSize2D(Vec2F(20, 20));

        TickScene(); // the collider shape is built from the child world transform, so it must be computed first

        colliderActor->AddComponent<BoxCollider>();

        return body;
    }
}

TEST(RigidBodyMassPropertyUI, MassFieldRefreshKeepsBodyMassData)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto body = MakeBodyWithOffsetCollider();
    ASSERT_NE(body->GetBody(), nullptr);

    b2Vec2 center = body->GetBody()->GetLocalCenter();
    ASSERT_GT(center.Length(), 0.1f);

    auto field = DynamicCast<FloatProperty>(o2EditorProperties.CreateRegularField(&TypeOf(float), "mass"));
    ASSERT_NE(field, nullptr);

    field->SetValuePropertyPointers<RigidBody::mass_PROPERTY>({ &body->mass });
    field->Refresh();

    EXPECT_NEAR(field->GetCommonValue(), body->GetMass(), 0.001f);
    EXPECT_NEAR(body->GetBody()->GetMass(), body->GetMass(), 0.001f);
    EXPECT_NEAR(body->GetBody()->GetLocalCenter().x, center.x, 0.001f);
    EXPECT_NEAR(body->GetBody()->GetLocalCenter().y, center.y, 0.001f);
    EXPECT_GT(body->GetBody()->GetInertia(), 0.0f);
}
