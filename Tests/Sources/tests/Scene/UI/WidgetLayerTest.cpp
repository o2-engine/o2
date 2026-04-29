#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "tests/Scene/SceneTestHelpers.h"
#include "tests/Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(WidgetLayer, DefaultConstructionIsValid)
{
    auto l = mmake<WidgetLayer>();
    ASSERT_TRUE(l);
    EXPECT_TRUE(l->IsEnabled());
    EXPECT_FLOAT_EQ(l->GetTransparency(), 1.0f);
    EXPECT_FLOAT_EQ(l->GetDepth(), 0.0f);
}

TEST(WidgetLayer, CopyConstructorCopiesName)
{
    auto src = mmake<WidgetLayer>();
    src->name = "src";
    auto copy = src->CloneAsRef<WidgetLayer>();
    EXPECT_EQ(copy->name, "src");
}

// ===== Drawable =====

TEST(WidgetLayer, SetDrawableRoundTrip)
{
    auto l = mmake<WidgetLayer>();
    auto d = MakeStubRectDrawable();
    l->SetDrawable(d);
    EXPECT_EQ(l->GetDrawable(), d);
}

// ===== Hierarchy =====

TEST(WidgetLayer, AddChildSetsParent)
{
    auto parent = mmake<WidgetLayer>();
    auto child = mmake<WidgetLayer>();
    child->name = "child";
    parent->AddChild(child);
    EXPECT_EQ(child->GetParent().Lock(), parent);
    EXPECT_EQ(parent->GetChildren().Count(), 1);
}

TEST(WidgetLayer, RemoveChildClears)
{
    auto parent = mmake<WidgetLayer>();
    auto child = mmake<WidgetLayer>();
    parent->AddChild(child);
    parent->RemoveChild(child);
    EXPECT_EQ(parent->GetChildren().Count(), 0);
}

TEST(WidgetLayer, RemoveAllChildrenClears)
{
    auto parent = mmake<WidgetLayer>();
    parent->AddChild(mmake<WidgetLayer>());
    parent->AddChild(mmake<WidgetLayer>());
    parent->RemoveAllChildren();
    EXPECT_EQ(parent->GetChildren().Count(), 0);
}

// AddChildLayer(name, drawable, ...) requires owner widget setup; covered by AddChild(Ref) instead.

TEST(WidgetLayer, FindChildReturnsByName)
{
    auto parent = mmake<WidgetLayer>();
    auto child = mmake<WidgetLayer>();
    child->name = "alpha";
    parent->AddChild(child);
    EXPECT_EQ(parent->FindChild("alpha"), child);
    EXPECT_FALSE(parent->FindChild("missing"));
}

TEST(WidgetLayer, GetChildResolvesPath)
{
    auto root = mmake<WidgetLayer>();
    auto a = mmake<WidgetLayer>();
    a->name = "a";
    auto b = mmake<WidgetLayer>();
    b->name = "b";
    root->AddChild(a);
    a->AddChild(b);
    EXPECT_EQ(root->GetChild("a/b"), b);
}

// ===== Enable / Hierarchy =====

TEST(WidgetLayer, SetEnabledRoundTrip)
{
    auto l = mmake<WidgetLayer>();
    l->SetEnabled(false);
    EXPECT_FALSE(l->IsEnabled());
    l->SetEnabled(true);
    EXPECT_TRUE(l->IsEnabled());
}

TEST(WidgetLayer, SetEnabledStateOnLayerHierarchy)
{
    auto parent = mmake<WidgetLayer>();
    auto child = mmake<WidgetLayer>();
    parent->AddChild(child);
    parent->SetEnabled(false);
    EXPECT_FALSE(parent->IsEnabled());
    parent->SetEnabled(true);
    EXPECT_TRUE(parent->IsEnabled());
}

// ===== Depth =====

TEST(WidgetLayer, SetDepthRoundTrip)
{
    auto l = mmake<WidgetLayer>();
    l->SetDepth(2.5f);
    EXPECT_FLOAT_EQ(l->GetDepth(), 2.5f);
}

// ===== Transparency =====

TEST(WidgetLayer, SetTransparencyRoundTrip)
{
    auto l = mmake<WidgetLayer>();
    l->SetTransparency(0.4f);
    EXPECT_FLOAT_EQ(l->GetTransparency(), 0.4f);
}

TEST(WidgetLayer, ChildResTransparencyIsParentTimesChild)
{
    auto parent = mmake<WidgetLayer>();
    auto child = mmake<WidgetLayer>();
    parent->AddChild(child);
    parent->SetTransparency(0.5f);
    child->SetTransparency(0.5f);
    EXPECT_NEAR(child->GetResTransparency(), 0.25f, 0.001f);
}

// ===== Owner widget =====

TEST(WidgetLayer, AddingToWidgetSetsOwnerWidget)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    auto l = mmake<WidgetLayer>();
    l->name = "owned";
    w->AddLayer(l);
    EXPECT_EQ(l->GetOwnerWidget().Lock(), w);
}
