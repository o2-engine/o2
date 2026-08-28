#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// RemoveChild without events (pooled editor widgets use it) must still detach the child from the
// parent's drawing registry and parent-widget link, otherwise the old parent keeps drawing it
TEST(WidgetRemoveChild, WithoutEventsDetachesDrawingAndParentWidget)
{
    SceneCleanGuard guard;

    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto child = mmake<Widget>(ActorCreateMode::InScene);

    parent->AddChild(child);
    o2Scene.UpdateAddedEntities();
    ASSERT_TRUE(parent->GetChildrenInheritedDepth().Contains(child));
    ASSERT_EQ(child->GetParentWidget().Lock(), parent);

    parent->RemoveChild(child, false);

    EXPECT_FALSE(parent->GetChildrenInheritedDepth().Contains(child));
    EXPECT_EQ(child->GetParentWidget().Lock(), nullptr);
    EXPECT_EQ(child->GetParent().Lock(), nullptr);

    // re-adding registers it again exactly once
    parent->AddChild(child);
    EXPECT_EQ(parent->GetChildrenInheritedDepth().Count([&](auto& x) { return x == child; }), 1);
}

TEST(WidgetRemoveChild, ActorWithoutEventsLeavesDrawingRegistry)
{
    SceneCleanGuard guard;

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);

    parent->AddChild(child);
    o2Scene.UpdateAddedEntities();
    ASSERT_TRUE(parent->GetChildrenInheritedDepth().Contains(child));

    parent->RemoveChild(child, false);
    EXPECT_FALSE(parent->GetChildrenInheritedDepth().Contains(child));
}
