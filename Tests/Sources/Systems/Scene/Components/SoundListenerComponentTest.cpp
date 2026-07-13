#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/SoundListenerComponent.h"
#include "o2/Sound/SoundListener.h"
#include "o2/Sound/SoundSystem.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(SoundListenerComponent, StandaloneListenerObjectDrivesSystem)
{
    {
        auto listener = mmake<SoundListener>();
        listener->SetPosition(Vec3F(5, 6, 7));

        o2Sounds.Update(0.016f);

        EXPECT_TRUE(listener->IsActiveListener());

        auto position = o2Sounds.GetListenerPosition();
        EXPECT_NEAR(position.x, 5.0f, 0.001f);
        EXPECT_NEAR(position.y, 6.0f, 0.001f);
        EXPECT_NEAR(position.z, 7.0f, 0.001f);
    }

    // Destroyed listener unregisters itself
    EXPECT_FALSE(o2Sounds.GetActiveListener());
    o2Sounds.SetListenerPosition(Vec3F());
}

TEST(SoundListenerComponent, ListenerFollowsComponentTransform)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto listener = actor->AddComponent<SoundListenerComponent>();

    actor->transform->SetPosition(Vec3F(10, 20, 30));
    TickFrame();

    o2Sounds.Update(0.016f);

    EXPECT_TRUE(listener->IsActiveListener());

    auto position = o2Sounds.GetListenerPosition();
    EXPECT_NEAR(position.x, 10.0f, 0.01f);
    EXPECT_NEAR(position.y, 20.0f, 0.01f);
    EXPECT_NEAR(position.z, 30.0f, 0.01f);

    o2Sounds.SetListenerPosition(Vec3F());
}

TEST(SoundListenerComponent, FirstEnabledListenerWins)
{
    SceneCleanGuard guard;
    auto firstActor = mmake<Actor>(ActorCreateMode::InScene);
    auto first = firstActor->AddComponent<SoundListenerComponent>();
    firstActor->transform->SetPosition(Vec3F(100, 0, 0));

    auto secondActor = mmake<Actor>(ActorCreateMode::InScene);
    auto second = secondActor->AddComponent<SoundListenerComponent>();
    secondActor->transform->SetPosition(Vec3F(0, 200, 0));

    TickFrame();
    o2Sounds.Update(0.016f);

    EXPECT_TRUE(first->IsActiveListener());
    EXPECT_FALSE(second->IsActiveListener());
    EXPECT_NEAR(o2Sounds.GetListenerPosition().x, 100.0f, 0.01f);

    first->SetEnabled(false);
    o2Sounds.Update(0.016f);

    EXPECT_FALSE(first->IsActiveListener());
    EXPECT_TRUE(second->IsActiveListener());
    EXPECT_NEAR(o2Sounds.GetListenerPosition().y, 200.0f, 0.01f);

    o2Sounds.SetListenerPosition(Vec3F());
}

TEST(SoundListenerComponent, NoListenerKeepsManualPosition)
{
    SceneCleanGuard guard;

    EXPECT_FALSE(o2Sounds.GetActiveListener());

    // Headless has no render camera, so without listener components position stays as set
    o2Sounds.SetListenerPosition(Vec3F(1, 2, 3));
    o2Sounds.Update(0.016f);

    auto position = o2Sounds.GetListenerPosition();
    EXPECT_NEAR(position.x, 1.0f, 0.001f);
    EXPECT_NEAR(position.y, 2.0f, 0.001f);
    EXPECT_NEAR(position.z, 3.0f, 0.001f);

    o2Sounds.SetListenerPosition(Vec3F());
}

TEST(SoundListenerComponent, RemovedFromSceneStopsDriving)
{
    SceneCleanGuard guard;
    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto listener = actor->AddComponent<SoundListenerComponent>();
    actor->transform->SetPosition(Vec3F(50, 0, 0));

    TickFrame();
    o2Sounds.Update(0.016f);
    ASSERT_TRUE(listener->IsActiveListener());

    Ref<SoundListenerComponent> zombie = listener;
    o2Scene.DestroyActor(actor);
    o2Scene.UpdateDestroyingEntities();

    EXPECT_FALSE(zombie->IsActiveListener());
    EXPECT_FALSE(o2Sounds.GetActiveListener());

    o2Sounds.SetListenerPosition(Vec3F());
}
