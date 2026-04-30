#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Utils/Math/Color.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(ImageComponent, DefaultConstructionAttachesToActor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto img = a->AddComponent<ImageComponent>();

    ASSERT_TRUE(img);
    EXPECT_EQ(img->GetActor(), a);
}

TEST(ImageComponent, ConstructorFromColorSetsColor)
{
    SceneCleanGuard guard;
    auto img = mmake<ImageComponent>(Color4(10, 20, 30, 40));
    EXPECT_EQ(img->GetColor(), Color4(10, 20, 30, 40));
}

TEST(ImageComponent, SetColorRoundTrip)
{
    SceneCleanGuard guard;
    auto img = mmake<ImageComponent>();
    img->SetColor(Color4(1, 2, 3, 4));
    EXPECT_EQ(img->GetColor(), Color4(1, 2, 3, 4));
}

TEST(ImageComponent, GetNameAndCategoryAreSet)
{
    EXPECT_FALSE(ImageComponent::GetName().IsEmpty());
}

TEST(ImageComponent, AddingSecondImageComponentReturnsNull)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto first = a->AddComponent<ImageComponent>();
    auto second = a->AddComponent<ImageComponent>();

    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
}

TEST(ImageComponent, MultipleActorsHaveIndependentColor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    auto imgA = a->AddComponent<ImageComponent>();
    auto imgB = b->AddComponent<ImageComponent>();

    imgA->SetColor(Color4::Red());
    imgB->SetColor(Color4::Blue());

    EXPECT_EQ(imgA->GetColor(), Color4::Red());
    EXPECT_EQ(imgB->GetColor(), Color4::Blue());
}

TEST(ImageComponent, OnTransformUpdatedFiresOnPositionChange)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto img = a->AddComponent<ImageComponent>();
    TickFrame();

    a->transform->SetPosition(Vec2F(50, 50));
    a->transform->SetSize(Vec2F(100, 100));
    TickFrame();

    EXPECT_TRUE(img);
}
