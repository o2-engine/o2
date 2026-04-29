#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(PopupWidget, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto pw = mmake<PopupWidget>();
    ASSERT_TRUE(pw);
    EXPECT_TRUE(pw->fitByChildren);
}

TEST(PopupWidget, CopyClonesObject)
{
    SceneCleanGuard guard;
    auto src = mmake<PopupWidget>();
    auto copy = src->CloneAsRef<PopupWidget>();
    ASSERT_TRUE(copy);
}

// ===== Public methods =====

TEST(PopupWidget, IsScrollableTrueByDefault)
{
    SceneCleanGuard guard;
    auto pw = mmake<PopupWidget>();
    EXPECT_TRUE(pw->IsScrollable());
}

TEST(PopupWidget, IsInputTransparentFalseByDefault)
{
    SceneCleanGuard guard;
    auto pw = mmake<PopupWidget>();
    EXPECT_FALSE(pw->IsInputTransparent());
}

// ===== Show =====

TEST(PopupWidget, ShowEnablesWidget)
{
    SceneCleanGuard guard;
    auto pw = mmake<PopupWidget>();
    pw->Hide(true);
    EXPECT_FALSE(pw->IsEnabled());
    pw->Show(Vec2F(100, 100));
    EXPECT_TRUE(pw->IsEnabled());
}

