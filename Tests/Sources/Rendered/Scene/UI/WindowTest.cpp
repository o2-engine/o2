#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Window, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto w = mmake<Window>();
    ASSERT_TRUE(w);
    EXPECT_TRUE(w->IsFocusable());
}

TEST(Window, CopyClonesObject)
{
    SceneCleanGuard guard;
    auto src = mmake<Window>();
    auto copy = src->CloneAsRef<Window>();
    ASSERT_TRUE(copy);
    EXPECT_NE(src->GetID(), copy->GetID());
}

// ===== Caption / Icon =====
//
// Without a UI style the caption/icon layers don't exist, so SetCaption/SetIcon
// are no-ops. Lock that contract explicitly.

TEST(Window, SetCaptionWithoutStyleIsNoOp)
{
    SceneCleanGuard guard;
    auto w = mmake<Window>();
    w->SetCaption("title");
    EXPECT_TRUE(w->GetCaption().IsEmpty());
}

TEST(Window, SetIconWithoutStyleIsNoOp)
{
    SceneCleanGuard guard;
    auto w = mmake<Window>();
    w->SetIcon(mmake<Sprite>());
    EXPECT_FALSE(w->GetIcon());
}

TEST(Window, SetIconLayoutWithoutStyleIsNoOp)
{
    SceneCleanGuard guard;
    auto w = mmake<Window>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    w->SetIconLayout(layout);
    EXPECT_EQ(w->GetIconLayout(), Layout());
}

// ===== Modal =====

TEST(Window, SetModalRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Window>();
    w->SetModal(true);
    EXPECT_TRUE(w->IsModal());
    w->SetModal(false);
    EXPECT_FALSE(w->IsModal());
}

// ===== Options menu =====

TEST(Window, GetOptionsMenuNotNull)
{
    SceneCleanGuard guard;
    auto w = mmake<Window>();
    EXPECT_TRUE(w->GetOptionsMenu());
}
