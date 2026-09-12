#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/WidgetLayout.h"
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

// ===== Clicks =====

namespace
{
    // Открывает обработчик двойного клика: слой событий шлёт его вместо нажатия при быстром повторе
    class ClickableButton: public Button
    {
    public:
        ClickableButton(RefCounter* refCounter): Button(refCounter) {}

        using Button::OnCursorDblClicked;

        void CoverScreen() { mDrawingScissorRect = RectF(-10000, -10000, 10000, 10000); }
    };
}

TEST(Button, QuickSecondPressReportedAsDoubleClickCountsAsClick)
{
    SceneCleanGuard guard;
    auto button = mmake<ClickableButton>();
    button->layout->anchorMin = Vec2F(0, 0);
    button->layout->anchorMax = Vec2F(0, 0);
    button->layout->offsetMin = Vec2F(0, 0);
    button->layout->offsetMax = Vec2F(100, 40);
    button->CoverScreen();
    TickFrames(2);

    int clicks = 0;
    button->onClick = [&]() { clicks++; };

    button->OnCursorDblClicked(Input::Cursor(Vec2F(50, 20)));
    EXPECT_EQ(clicks, 1);

    button->OnCursorDblClicked(Input::Cursor(Vec2F(500, 20)));
    EXPECT_EQ(clicks, 1) << "double click outside the button is not a click";
}
