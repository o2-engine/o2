#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/DepthInherit.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(DepthInheritAction, RedoSetsInheritUndoRestores)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    actor->SetDrawingDepthInheritFromParent(false);
    ASSERT_FALSE(actor->IsDrawingDepthInheritedFromParent());

    auto action = mmake<DepthInheritAction>(o2::Vector<Ref<Actor>>{ actor }, true);

    action->Redo();
    EXPECT_TRUE(actor->IsDrawingDepthInheritedFromParent());

    action->Undo();
    EXPECT_FALSE(actor->IsDrawingDepthInheritedFromParent());
}

TEST(DepthInheritAction, RoundTripsAcrossUndoRedo)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    actor->SetDrawingDepthInheritFromParent(false);

    auto action = mmake<DepthInheritAction>(o2::Vector<Ref<Actor>>{ actor }, true);

    action->Redo();
    action->Undo();
    action->Redo();
    EXPECT_TRUE(actor->IsDrawingDepthInheritedFromParent());
}

TEST(DepthInheritAction, AppliesToEveryTargetActor)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    TickScene();
    a->SetDrawingDepthInheritFromParent(false);
    b->SetDrawingDepthInheritFromParent(false);

    auto action = mmake<DepthInheritAction>(o2::Vector<Ref<Actor>>{ a, b }, true);

    action->Redo();
    EXPECT_TRUE(a->IsDrawingDepthInheritedFromParent());
    EXPECT_TRUE(b->IsDrawingDepthInheritedFromParent());

    action->Undo();
    EXPECT_FALSE(a->IsDrawingDepthInheritedFromParent());
    EXPECT_FALSE(b->IsDrawingDepthInheritedFromParent());
}

TEST(DepthInheritAction, MissingActorIsTolerated)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();

    auto action = mmake<DepthInheritAction>(o2::Vector<Ref<Actor>>{ actor }, true);
    action->Redo();

    o2Scene.DestroyActor(actor);
    actor = nullptr;
    o2Scene.UpdateDestroyingEntities();

    action->Undo();
    action->Redo();
}

TEST(DepthInheritAction, GetNameIsStable)
{
    auto action = mmake<DepthInheritAction>();
    EXPECT_EQ(action->GetName(), String("Change depth inheritance"));
}
