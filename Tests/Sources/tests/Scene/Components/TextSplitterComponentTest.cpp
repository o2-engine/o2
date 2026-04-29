#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Text.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/TextSplitterComponent.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

TEST(TextSplitterComponent, AttachesToActorAndIsRetrievable)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto t = a->AddComponent<TextSplitterComponent>();
    ASSERT_TRUE(t);
    EXPECT_EQ(a->GetComponent<TextSplitterComponent>(), t);
    EXPECT_EQ(t->GetActor(), a);
}

TEST(TextSplitterComponent, AttachedHasReasonableDefaults)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto t = a->AddComponent<TextSplitterComponent>();

    EXPECT_EQ(t->GetHeight(), 11);
    EXPECT_FLOAT_EQ(t->GetSymbolsDistanceCoef(), 1.0f);
    EXPECT_FLOAT_EQ(t->GetLinesDistanceCoef(), 1.0f);
    EXPECT_EQ(t->GetHorAlign(), HorAlign::Left);
    EXPECT_EQ(t->GetVerAlign(), VerAlign::Top);
    EXPECT_FALSE(t->GetWordWrap());
    EXPECT_FALSE(t->IsDotsEndings());
}
