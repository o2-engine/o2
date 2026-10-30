#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/GridLayoutScrollArea.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(GridLayoutScrollArea, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayoutScrollArea>();
    ASSERT_TRUE(g);
}

// ===== Item sample =====

TEST(GridLayoutScrollArea, SetItemSampleStores)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayoutScrollArea>();
    auto sample = mmake<Widget>();
    g->SetItemSample(sample);
    EXPECT_TRUE(g->GetItemSample());
}

// ===== Spacing =====

TEST(GridLayoutScrollArea, SetItemsSpacingRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayoutScrollArea>();
    g->SetItemsSpacing(Vec2F(5, 7));
    EXPECT_EQ(g->GetItemsSpacing(), Vec2F(5, 7));
}

// ===== Callbacks =====


// ===== Pool =====

// An item put into the pool leaves the area: shown again there, it doesn't register in the drawables of the area, where
// it would stay after the pool lets it go
TEST(GridLayoutScrollArea, PooledItemDoesNotRegisterInTheDrawablesOfTheArea)
{
    SceneCleanGuard guard;

    static int values[100];
    auto grid = mmake<GridLayoutScrollArea>();
    auto sample = mmake<Widget>();
    sample->layout->minSize = Vec2F(100, 20);
    grid->SetItemSample(sample);
    grid->getItemsCountFunc = []() { return 100; };
    grid->getItemsRangeFunc = [](int start, int end) {
        Vector<void*> res;
        for (int i = start; i < end; i++)
            res.Add(&values[i]);

        return res;
    };
    grid->setupItemFunc = [](const Ref<Widget>&, void*) {};

    *grid->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(100, 100));
    grid->SetEnabledForcible(true);
    grid->UpdateSelfTransform();
    grid->UpdateSelfTransform();
    ASSERT_FALSE(grid->GetChildWidgets().IsEmpty());

    Ref<Widget> pooled = grid->GetChildWidgets().Last();
    *grid->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(100, 30));
    grid->UpdateSelfTransform();
    grid->UpdateSelfTransform();
    ASSERT_FALSE(grid->GetChildWidgets().Contains(pooled));

    pooled->SetEnabledForcible(false);
    pooled->SetEnabledForcible(true);

    EXPECT_FALSE(grid->GetChildrenInheritedDepth().Contains(Ref<ISceneDrawable>(pooled)));
    EXPECT_EQ(grid->GetChildrenInheritedDepth().Count(), grid->GetChildWidgets().Count());
}
