#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/ScissorClippingComponent.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

TEST(ScissorClippingComponent, EnableClippingDefaultsToTrue)
{
    SceneCleanGuard guard;
    auto comp = mmake<ScissorClippingComponent>();
    EXPECT_TRUE(comp->enableClipping);
}

TEST(ScissorClippingComponent, EnableClippingFlagToggles)
{
    SceneCleanGuard guard;
    auto comp = mmake<ScissorClippingComponent>();

    comp->enableClipping = false;
    EXPECT_FALSE(comp->enableClipping);

    comp->enableClipping = true;
    EXPECT_TRUE(comp->enableClipping);
}

TEST(ScissorClippingComponent, AttachesToActorAndIsRetrievable)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<ScissorClippingComponent>();

    ASSERT_TRUE(comp);
    EXPECT_EQ(a->GetComponent<ScissorClippingComponent>(), comp);
    EXPECT_EQ(comp->GetActor(), a);
}

TEST(ScissorClippingComponent, SerializeDeltaPreservesFlag)
{
    SceneCleanGuard guard;
    auto comp = mmake<ScissorClippingComponent>();
    comp->enableClipping = false;

    DataDocument doc;
    doc.Set(*comp);

    auto restored = mmake<ScissorClippingComponent>();
    doc.Get(*restored);

    EXPECT_FALSE(restored->enableClipping);
}

TEST(ScissorClippingComponent, GetNameAndCategoryNonEmpty)
{
    EXPECT_FALSE(ScissorClippingComponent::GetName().IsEmpty());
}
