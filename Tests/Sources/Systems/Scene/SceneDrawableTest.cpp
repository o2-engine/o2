#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ISceneDrawable.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== drawDepth round-trip =====

TEST(SceneDrawable, SetDrawingDepthRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetDrawingDepth(7.5f);
    EXPECT_FLOAT_EQ(a->GetDrawingDepth(), 7.5f);
}

TEST(SceneDrawable, DefaultDrawingDepthIsZero)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    EXPECT_FLOAT_EQ(a->GetDrawingDepth(), 0.0f);
}

// ===== Inherit-from-parent flag =====

TEST(SceneDrawable, DefaultInheritsDepthFromParent)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    EXPECT_TRUE(a->IsDrawingDepthInheritedFromParent());
}

TEST(SceneDrawable, SetInheritFromParentTogglesFlag)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->SetDrawingDepthInheritFromParent(false);
    EXPECT_FALSE(a->IsDrawingDepthInheritedFromParent());

    a->SetDrawingDepthInheritFromParent(true);
    EXPECT_TRUE(a->IsDrawingDepthInheritedFromParent());
}

// ===== Children-inherited-depth registry =====

TEST(SceneDrawable, ChildWithInheritGoesToParentChildrenInheritedDepth)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);
    TickFrame();

    child->SetDrawingDepthInheritFromParent(true);
    TickFrame();

    bool found = false;
    for (auto& c : parent->GetChildrenInheritedDepth())
    {
        if (c.Get() == child.Get()) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(SceneDrawable, ChildWithoutInheritIsNotInParentChildrenInheritedDepth)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);
    TickFrame();

    child->SetDrawingDepthInheritFromParent(false);
    TickFrame();

    bool found = false;
    for (auto& c : parent->GetChildrenInheritedDepth())
    {
        if (c.Get() == child.Get()) { found = true; break; }
    }
    EXPECT_FALSE(found);
}

TEST(SceneDrawable, TogglingInheritReregisters)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);
    TickFrame();

    child->SetDrawingDepthInheritFromParent(false);
    TickFrame();

    int countWhenNotInheriting = 0;
    for (auto& c : parent->GetChildrenInheritedDepth())
        if (c.Get() == child.Get()) ++countWhenNotInheriting;
    EXPECT_EQ(countWhenNotInheriting, 0);

    child->SetDrawingDepthInheritFromParent(true);
    TickFrame();

    int countWhenInheriting = 0;
    for (auto& c : parent->GetChildrenInheritedDepth())
        if (c.Get() == child.Get()) ++countWhenInheriting;
    EXPECT_EQ(countWhenInheriting, 1);
}

// ===== SceneLayer::GetDrawables =====

TEST(SceneDrawable, NonInheritActorAppearsInLayerDrawables)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("drawable_layer_42");
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetLayer(layer);
    a->SetDrawingDepthInheritFromParent(false);
    TickFrame();

    bool found = false;
    for (auto& d : layer->GetDrawables())
        if (d.Get() == static_cast<ISceneDrawable*>(a.Get())) { found = true; break; }

    EXPECT_TRUE(found);
}

// ===== Multiple drawables ordering by depth =====

TEST(SceneDrawable, DrawablesAreSortedInLayerByDepth)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("ordering_layer_42");

    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    auto c = mmake<Actor>(ActorCreateMode::InScene);

    a->SetLayer(layer);
    b->SetLayer(layer);
    c->SetLayer(layer);

    a->SetDrawingDepthInheritFromParent(false);
    b->SetDrawingDepthInheritFromParent(false);
    c->SetDrawingDepthInheritFromParent(false);

    a->SetDrawingDepth(0.0f);
    b->SetDrawingDepth(5.0f);
    c->SetDrawingDepth(-3.0f);

    TickFrame();

    int idxA = -1, idxB = -1, idxC = -1;
    int i = 0;
    for (auto& d : layer->GetDrawables())
    {
        auto raw = d.Get();
        if (raw == static_cast<ISceneDrawable*>(a.Get())) idxA = i;
        if (raw == static_cast<ISceneDrawable*>(b.Get())) idxB = i;
        if (raw == static_cast<ISceneDrawable*>(c.Get())) idxC = i;
        ++i;
    }

    ASSERT_GE(idxA, 0);
    ASSERT_GE(idxB, 0);
    ASSERT_GE(idxC, 0);

    // Lower drawDepth is drawn first
    EXPECT_LT(idxC, idxA);
    EXPECT_LT(idxA, idxB);
}

// ===== ChangeDepth dynamic =====

TEST(SceneDrawable, ChangingDepthReorders)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("dyn_layer_42");

    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    a->SetLayer(layer);
    b->SetLayer(layer);
    a->SetDrawingDepthInheritFromParent(false);
    b->SetDrawingDepthInheritFromParent(false);

    a->SetDrawingDepth(1.0f);
    b->SetDrawingDepth(2.0f);
    TickFrame();

    int idxAFirst = -1, idxBFirst = -1;
    int i = 0;
    for (auto& d : layer->GetDrawables())
    {
        if (d.Get() == static_cast<ISceneDrawable*>(a.Get())) idxAFirst = i;
        if (d.Get() == static_cast<ISceneDrawable*>(b.Get())) idxBFirst = i;
        ++i;
    }
    EXPECT_LT(idxAFirst, idxBFirst);

    a->SetDrawingDepth(10.0f);
    TickFrame();

    int idxASecond = -1, idxBSecond = -1;
    i = 0;
    for (auto& d : layer->GetDrawables())
    {
        if (d.Get() == static_cast<ISceneDrawable*>(a.Get())) idxASecond = i;
        if (d.Get() == static_cast<ISceneDrawable*>(b.Get())) idxBSecond = i;
        ++i;
    }
    EXPECT_GT(idxASecond, idxBSecond);
}
