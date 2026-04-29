#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Spoiler, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto s = mmake<Spoiler>();
    ASSERT_TRUE(s);
}

TEST(Spoiler, CopyPreservesHeadHeight)
{
    SceneCleanGuard guard;
    auto src = mmake<Spoiler>();
    src->SetHeadHeight(20.0f);
    auto copy = src->CloneAsRef<Spoiler>();
    EXPECT_FLOAT_EQ(copy->GetHeadHeight(), 20.0f);
}

// ===== Caption =====

TEST(Spoiler, SetCaptionWithoutStyleIsNoOp)
{
    SceneCleanGuard guard;
    auto s = mmake<Spoiler>();
    s->SetCaption("title");
    EXPECT_TRUE(s->GetCaption().IsEmpty());
}

// ===== Head height =====

TEST(Spoiler, SetHeadHeightRoundTrip)
{
    SceneCleanGuard guard;
    auto s = mmake<Spoiler>();
    s->SetHeadHeight(33.0f);
    EXPECT_FLOAT_EQ(s->GetHeadHeight(), 33.0f);
}

// ===== Expanded state =====

TEST(Spoiler, ExpandSetsIsExpandedTrue)
{
    SceneCleanGuard guard;
    auto s = mmake<Spoiler>();
    s->Collapse(true);
    EXPECT_FALSE(s->IsExpanded());
    s->Expand(true);
    EXPECT_TRUE(s->IsExpanded());
}

TEST(Spoiler, CollapseSetsIsExpandedFalse)
{
    SceneCleanGuard guard;
    auto s = mmake<Spoiler>();
    s->Expand(true);
    s->Collapse(true);
    EXPECT_FALSE(s->IsExpanded());
}

TEST(Spoiler, SetExpandedRoundTrip)
{
    SceneCleanGuard guard;
    auto s = mmake<Spoiler>();
    s->SetExpanded(true, true);
    EXPECT_TRUE(s->IsExpanded());
    s->SetExpanded(false, true);
    EXPECT_FALSE(s->IsExpanded());
}

// ===== Events =====

TEST(Spoiler, OnExpandFiresOnExpandNotOnCollapse)
{
    SceneCleanGuard guard;
    auto s = mmake<Spoiler>();
    int count = 0;
    s->onExpand = [&]() { count++; };
    s->Collapse(true);
    EXPECT_EQ(count, 0);
    s->Expand(true);
    EXPECT_EQ(count, 1);
    s->Collapse(true);
    EXPECT_EQ(count, 1);
}
