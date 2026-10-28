#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Input.h"
#include "o2/Render/Sprite.h"
#include "o2/Utils/Editor/DragHandle.h"

using namespace o2;

namespace
{
    struct HandleProbe: DragHandle
    {
        HandleProbe(RefCounter* refCounter): DragHandle(refCounter) {}

        void Press(const Vec2F& point)
        {
            mIsPressed = true;
            OnCursorPressed(Input::Cursor(point, 0));
        }

        void StillDown(const Vec2F& point, const Vec2F& delta)
        {
            Input::Cursor cursor(point, 0);
            cursor.delta = delta;
            OnCursorStillDown(cursor);
        }
    };
}

// A slow drag moves the cursor by a fraction of a pixel per frame: the handle must still follow
TEST(DragHandleSlowDragUI, SubPixelFrameDeltasAccumulate)
{
    auto handle = mmake<HandleProbe>();
    handle->SetPosition(Vec2F(100, 100));

    int changes = 0;
    handle->onChangedPos = [&](const Vec2F&) { changes++; };

    handle->Press(Vec2F(100, 100));

    Vec2F cursor(100, 100);
    for (int i = 0; i < 40; i++)
    {
        cursor.x += 0.3f;
        handle->StillDown(cursor, Vec2F(0.3f, 0.0f));
    }

    EXPECT_NEAR(handle->GetPosition().x, 112.0f, 0.5f);
    EXPECT_GT(changes, 0);

    // a still cursor must not keep reporting moves
    int changesBefore = changes;
    for (int i = 0; i < 5; i++)
        handle->StillDown(cursor, Vec2F());

    EXPECT_EQ(changes, changesBefore);
}

// A new handle shows its regular look: the hover, pressed and selected looks fade in only when their state comes
TEST(DragHandleLooksUI, StateLooksStartHidden)
{
    auto make = []() { return mmake<Sprite>(); };
    auto handle = mmake<DragHandle>(make(), make(), make(), make(), make(), make());
    EXPECT_FLOAT_EQ(handle->GetRegularDrawable()->GetTransparency(), 1.0f);
    EXPECT_FLOAT_EQ(handle->GetHoverDrawable()->GetTransparency(), 0.0f);
    EXPECT_FLOAT_EQ(handle->GetPressedDrawable()->GetTransparency(), 0.0f);
    EXPECT_FLOAT_EQ(handle->GetSelectedDrawable()->GetTransparency(), 0.0f);
    EXPECT_FLOAT_EQ(handle->GetSelectedHoveredDrawable()->GetTransparency(), 0.0f);
    EXPECT_FLOAT_EQ(handle->GetSelectedPressedDrawable()->GetTransparency(), 0.0f);

    auto other = mmake<DragHandle>();
    other->SetRegularDrawable(make());
    other->SetHoverDrawable(make());
    other->SetPressedDrawable(make());
    other->SetSelectedDrawable(make());
    EXPECT_FLOAT_EQ(other->GetRegularDrawable()->GetTransparency(), 1.0f);
    EXPECT_FLOAT_EQ(other->GetHoverDrawable()->GetTransparency(), 0.0f);
    EXPECT_FLOAT_EQ(other->GetPressedDrawable()->GetTransparency(), 0.0f);
    EXPECT_FLOAT_EQ(other->GetSelectedDrawable()->GetTransparency(), 0.0f);
}
