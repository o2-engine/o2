#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Button, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto b = mmake<Button>();
    ASSERT_TRUE(b);
    EXPECT_TRUE(b->IsFocusable());
}

TEST(Button, CopyClonesObject)
{
    SceneCleanGuard guard;
    auto src = mmake<Button>();
    auto copy = src->CloneAsRef<Button>();
    ASSERT_TRUE(copy);
    EXPECT_NE(src->GetID(), copy->GetID());
}

// ===== Caption =====

// Without a UI style the "caption" layer is absent, so SetCaption is a no-op
// and GetCaption returns an empty string. Lock that contract.
TEST(Button, SetCaptionWithoutStyleIsNoOp)
{
    SceneCleanGuard guard;
    auto b = mmake<Button>();
    b->SetCaption("hi");
    EXPECT_TRUE(b->GetCaption().IsEmpty());
}

// ===== Icon =====

TEST(Button, SetIconWithoutStyleIsNoOp)
{
    SceneCleanGuard guard;
    auto b = mmake<Button>();
    auto icon = mmake<Sprite>();
    b->SetIcon(icon);
    EXPECT_FALSE(b->GetIcon());
}

// ===== Focusable =====

TEST(Button, IsFocusableTrueByDefault)
{
    SceneCleanGuard guard;
    auto b = mmake<Button>();
    EXPECT_TRUE(b->IsFocusable());
}
