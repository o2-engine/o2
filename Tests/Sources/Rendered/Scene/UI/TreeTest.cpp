#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/Tree.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Tree, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    ASSERT_TRUE(tree);
    EXPECT_TRUE(tree->IsFocusable());
}

// ===== Selection model =====

TEST(Tree, SetMultipleSelectionAvailableRoundTrip)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    tree->SetMultipleSelectionAvailable(false);
    EXPECT_FALSE(tree->IsMultiSelectionAvailable());
    tree->SetMultipleSelectionAvailable(true);
    EXPECT_TRUE(tree->IsMultiSelectionAvailable());
}

// ===== Rearrange type =====

TEST(Tree, SetRearrangeTypeRoundTrip)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    tree->SetRearrangeType(Tree::RearrangeType::Disabled);
    EXPECT_EQ(tree->GetRearrangeType(), Tree::RearrangeType::Disabled);
    tree->SetRearrangeType(Tree::RearrangeType::Enabled);
    EXPECT_EQ(tree->GetRearrangeType(), Tree::RearrangeType::Enabled);
    tree->SetRearrangeType(Tree::RearrangeType::OnlyReparent);
    EXPECT_EQ(tree->GetRearrangeType(), Tree::RearrangeType::OnlyReparent);
}

// ===== Selection =====

TEST(Tree, GetSelectedObjectsEmptyByDefault)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    EXPECT_EQ(tree->GetSelectedObjects().Count(), 0);
}

TEST(Tree, DeselectAllObjectsClearsSelection)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    tree->DeselectAllObjects();
    EXPECT_EQ(tree->GetSelectedObjects().Count(), 0);
}

// ===== Layout settings =====

TEST(Tree, SetNodeExpandTimerRoundTrip)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    tree->SetNodeExpandTimer(0.5f);
    EXPECT_FLOAT_EQ(tree->GetNodeExpandTimer(), 0.5f);
}

TEST(Tree, SetChildsNodesOffsetRoundTrip)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    tree->SetChildsNodesOffset(15.0f);
    EXPECT_FLOAT_EQ(tree->GetChildsNodesOffset(), 15.0f);
}

// ===== Rebuild =====

TEST(Tree, UpdateNodesViewInvokesChildrenDelegateForRoot)
{
    SceneCleanGuard guard;
    auto tree = mmake<Tree>();
    int callCount = 0;
    tree->getObjectChildrenDelegate = [&](void* parent) -> Vector<void*> {
        callCount++;
        return {};
    };
    tree->UpdateNodesView(true);
    EXPECT_GE(callCount, 1);
}
