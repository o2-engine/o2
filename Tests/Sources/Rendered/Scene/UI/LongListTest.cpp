#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(LongList, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    ASSERT_TRUE(list);
}

// ===== Item sample =====

TEST(LongList, SetItemSampleStores)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    auto sample = mmake<Widget>();
    list->SetItemSample(sample);
    EXPECT_TRUE(list->GetItemSample());
}

// ===== Selection =====

TEST(LongList, SelectItemAtStoresPositionWhenInRange)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    list->getItemsCountFunc = []() { return 10; };
    list->SelectItemAt(3);
    EXPECT_EQ(list->GetSelectedItemPosition(), 3);
}

TEST(LongList, SelectItemAtOutOfRangeStoresMinusOne)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    list->getItemsCountFunc = []() { return 5; };
    list->SelectItemAt(99);
    EXPECT_EQ(list->GetSelectedItemPosition(), -1);
}

// ===== Drawables =====

TEST(LongList, GetSelectionDrawableExistsByDefault)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    EXPECT_TRUE(list->GetSelectionDrawable());
}

TEST(LongList, SetSelectionDrawableLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    list->SetSelectionDrawableLayout(layout);
    auto retrieved = list->GetSelectionDrawableLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}

TEST(LongList, SetHoverDrawableLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    list->SetHoverDrawableLayout(layout);
    auto retrieved = list->GetHoverDrawableLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}

// ===== Callbacks =====


// ===== Scrolling =====

namespace
{
    struct TenItems
    {
        Ref<LongList> list = mmake<LongList>();
        Vector<int>   items;
        Vector<int>   shown;

        TenItems()
        {
            for (int i = 0; i < 10; i++)
                items.Add(i);

            auto sample = mmake<Widget>();
            sample->layout->minHeight = 25;
            list->SetItemSample(sample);
            list->getItemsCountFunc = [this]() { return items.Count(); };
            list->getItemsRangeFunc = [this](int min, int max)
            {
                Vector<void*> res;
                for (int i = Math::Max(0, min); i < max && i < items.Count(); i++)
                    res.Add(&items[i]);
                return res;
            };
            list->setupItemFunc = [this](const Ref<Widget>&, void* object) { shown.Add(*(int*)object); };
            *list->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(200, 100));
            Update();
        }

        void Update()
        {
            list->UpdateSelfTransform();
            list->UpdateChildrenTransforms();
        }
    };
}

TEST(LongList, ScrollRangeCoversExactlyTheItems)
{
    SceneCleanGuard guard;
    TenItems t;
    auto range = t.list->GetScrollRange();
    EXPECT_FLOAT_EQ(range.bottom, 0.0f);
    EXPECT_FLOAT_EQ(range.top, 10 * 25.0f - 100.0f);
}

TEST(LongList, ScrolledToTheEndShowsTheLastItem)
{
    SceneCleanGuard guard;
    TenItems t;
    t.shown.Clear();
    t.list->SetScrollForcible(Vec2F(0, t.list->GetScrollRange().top));
    t.Update();
    EXPECT_FALSE(t.list->GetChildWidgets().IsEmpty());
    EXPECT_TRUE(t.shown.Contains(9));
}
