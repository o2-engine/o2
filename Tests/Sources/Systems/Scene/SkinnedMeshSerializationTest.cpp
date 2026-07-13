#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/SkinnedMeshComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// Skinned mesh component settings must survive the scene save/load round trip
TEST(SkinnedMesh, ComponentSettingsRoundTrip)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    actor->SetName("skinned actor");

    auto component = actor->AddComponent<SkinnedMeshComponent>();
    component->SetAnimation("Run");
    component->SetPlaying(false);
    component->SetLooped(false);
    component->SetSpeed(2.0f);
    component->SetColor(Color4(200, 100, 50, 255));
    component->SetShaded(false);

    TickFrame();

    DataDocument document;
    o2Scene.Save(document);

    o2Scene.Clear(false);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(document);
    TickFrame();

    auto loadedActor = o2Scene.FindActor("skinned actor");
    ASSERT_TRUE(loadedActor);

    auto loadedComponent = loadedActor->GetComponent<SkinnedMeshComponent>();
    ASSERT_TRUE(loadedComponent);

    EXPECT_EQ(loadedComponent->GetAnimation(), "Run");
    EXPECT_FALSE(loadedComponent->IsPlaying());
    EXPECT_FALSE(loadedComponent->IsLooped());
    EXPECT_NEAR(loadedComponent->GetSpeed(), 2.0f, 0.001f);
    EXPECT_EQ(loadedComponent->GetColor(), Color4(200, 100, 50, 255));
    EXPECT_FALSE(loadedComponent->IsShaded());

    EXPECT_EQ(loadedComponent->GetSceneDrawableCategory(), SceneDrawableCategory::Scene3D);
}
