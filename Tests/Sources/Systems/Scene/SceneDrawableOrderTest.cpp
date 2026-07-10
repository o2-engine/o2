#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// Inherited-depth draw order must follow the parent children order; sorting is
// deferred and applied on GetChildrenInheritedDepth access
namespace
{
    Vector<String> DrawOrderNames(const Ref<Actor>& parent)
    {
        return parent->GetChildrenInheritedDepth().Convert<String>(
            [](const Ref<ISceneDrawable>& x) { return DynamicCast<Actor>(x)->GetName(); });
    }

    Ref<Actor> MakeChild(const Ref<Actor>& parent, const String& name)
    {
        auto child = mmake<Actor>();
        child->SetName(name);
        parent->AddChild(child);
        return child;
    }
}

TEST(SceneDrawableOrder, AppendedChildrenKeepChildrenOrder)
{
    SceneCleanGuard guard;

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    MakeChild(parent, "a");
    MakeChild(parent, "b");
    MakeChild(parent, "c");
    TickFrame();

    EXPECT_EQ(DrawOrderNames(parent), Vector<String>({ "a", "b", "c" }));
}

TEST(SceneDrawableOrder, InsertedAtIndexChildIsSortedOnAccess)
{
    SceneCleanGuard guard;

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    MakeChild(parent, "a");
    MakeChild(parent, "b");
    MakeChild(parent, "c");
    TickFrame();

    auto inserted = mmake<Actor>();
    inserted->SetName("d");
    parent->AddChild(inserted, 1);
    TickFrame();

    EXPECT_EQ(DrawOrderNames(parent), Vector<String>({ "a", "d", "b", "c" }));
}

TEST(SceneDrawableOrder, SetIndexInSiblingsReordersDrawables)
{
    SceneCleanGuard guard;

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    MakeChild(parent, "a");
    auto b = MakeChild(parent, "b");
    MakeChild(parent, "c");
    TickFrame();

    b->SetIndexInSiblings(0);

    EXPECT_EQ(DrawOrderNames(parent), Vector<String>({ "b", "a", "c" }));
}

TEST(SceneDrawableOrder, ReenabledChildReturnsToItsPlace)
{
    SceneCleanGuard guard;

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    MakeChild(parent, "a");
    auto b = MakeChild(parent, "b");
    MakeChild(parent, "c");
    TickFrame();

    b->SetEnabled(false);
    EXPECT_EQ(DrawOrderNames(parent), Vector<String>({ "a", "c" }));

    b->SetEnabled(true);
    EXPECT_EQ(DrawOrderNames(parent), Vector<String>({ "a", "b", "c" }));
}
