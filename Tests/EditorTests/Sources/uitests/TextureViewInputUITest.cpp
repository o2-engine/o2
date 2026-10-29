#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/UI/Preview/TextureView.h"
#include "o2Editor/UIRoot.h"

using namespace o2;
using namespace Editor;

namespace
{
    // Updates and draws the editor UI root for frames, as the editor does
    void Step(int frames = 1, bool layout = false)
    {
        for (int i = 0; i < frames; i++)
        {
            {
                PushEditorScopeOnStack scope;
                o2Render.Begin();
                o2Render.SetCamera(Camera());
                o2Render.Clear(Color4(30, 31, 34, 255));
                auto root = EditorUIRoot.GetRootWidget();
                root->Update(1.0f/60.0f);
                root->UpdateChildren(1.0f/60.0f);
                if (layout)
                    root->UpdateChildrenTransforms();
                root->Draw();
                o2Render.End();
            }
            AppTestDriver::PumpFrames(1);
        }
    }

    struct TextureViewInput: public ::testing::Test
    {
        Ref<TextureView> view;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }

            PushEditorScopeOnStack scope;
            view = mmake<TextureView>();
            *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(600, 400), Vec2F(60, -40));

            auto root = EditorUIRoot.GetRootWidget();
            *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize());
            EditorUIRoot.AddWidget(view);
            view->SetEnabledForcible(false);
            view->SetEnabledForcible(true);

            Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(256, 256));
            for (int i = 0; i < 256*256; i++)
            {
                UInt8* p = bitmap.GetData() + i*4;
                p[0] = (UInt8)(i % 256); p[1] = (UInt8)(i/256); p[2] = 0; p[3] = 255;
            }

            view->SetTexture(TextureRef(bitmap));
            Step(10, true);
        }

        void TearDown() override
        {
            EditorUIRoot.RemoveWidget(view);
            view = nullptr;
        }

        Vec2F Center() const { return view->layout->GetWorldRect().Center(); }

        // Drags by the left button from the point by the offset in steps, the cursor stays at the end
        void Drag(const Vec2F& from, const Vec2F& offset, int steps = 10)
        {
            // A press soon after the previous one is a double click, not a drag
            std::this_thread::sleep_for(std::chrono::milliseconds(350));

            o2Input.OnCursorMoved(from);
            Step(2);
            o2Input.OnCursorPressed(from);
            Step();
            for (int i = 1; i <= steps; i++)
            {
                o2Input.OnCursorMoved(from + offset*((float)i/(float)steps));
                Step();
            }

            o2Input.OnCursorReleased();
            Step(30);
        }
    };
}

// The texel taken by the left button goes with the cursor at any zoom
TEST_F(TextureViewInput, LeftButtonDragKeepsTheTexelUnderTheCursor)
{
    for (float zoom : { 1.0f, 3.0f, 8.0f })
    {
        view->FitToView();
        view->SetPixelZoom(zoom);
        Step(30);

        Vec2F from = Center() + Vec2F(-50.3f, 20.4f), offset(120.0f, -70.0f);
        Vec2I taken;
        ASSERT_TRUE(view->ScreenToPixel(from, taken)) << zoom;

        Drag(from, offset);

        Vec2I under;
        ASSERT_TRUE(view->ScreenToPixel(from + offset, under)) << zoom;
        EXPECT_EQ(under, taken) << "zoom " << zoom;
    }
}

// The pixel under the cursor follows the view when the wheel zooms it, without moving the cursor
TEST_F(TextureViewInput, HoveredPixelFollowsTheWheelZoom)
{
    Vec2F at = Center() + Vec2F(90.3f, 60.4f);
    o2Input.OnCursorMoved(at);
    Step(3);

    for (int i = 0; i < 4; i++)
    {
        o2Input.OnMouseWheel(120.0f);
        Step();
    }

    Step(40);

    Vec2I hovered, real;
    Color4 color;
    ASSERT_TRUE(view->GetHoveredPixel(hovered, color));
    ASSERT_TRUE(view->ScreenToPixel(at, real));
    EXPECT_EQ(hovered, real);
}

// The pixel under the moved cursor is the one the cursor points at, at any zoom
TEST_F(TextureViewInput, HoveredPixelIsUnderTheMovedCursor)
{
    for (float zoom : { 1.0f, 3.0f, 8.0f })
    {
        view->FitToView();
        view->SetPixelZoom(zoom);
        Step(30);

        for (Vec2F offset : { Vec2F(-70.3f, 35.4f), Vec2F(45.6f, -60.3f) })
        {
            Vec2F at = Center() + offset;
            o2Input.OnCursorMoved(at);
            Step(3);

            Vec2I hovered, real;
            Color4 color;
            ASSERT_TRUE(view->GetHoveredPixel(hovered, color)) << zoom;
            ASSERT_TRUE(view->ScreenToPixel(at, real)) << zoom;
            EXPECT_EQ(hovered, real) << "zoom " << zoom;
        }
    }
}
