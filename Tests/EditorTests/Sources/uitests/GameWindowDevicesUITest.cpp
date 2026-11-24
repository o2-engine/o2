#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"
#include "o2/Application/Application.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2/Render/VectorGraphics/SvgParser.h"
#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2Editor/Windows/GameWindow/GameDevices.h"
#include "o2Editor/Windows/GameWindow/GameWindow.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

// The Game window emulates a device as the o2 portal does: its real pixels, its safe area, its body around the picture,
// fitted into the window; zoomed and panned like Unity's Game view, and the game still gets its clicks
namespace
{
    struct Driver
    {
        EditorShell& shell;

        void Step(int frames = 1)
        {
            for (int i = 0; i < frames; i++)
            {
                shell.Update(1.0f/60.0f);
                {
                    PushEditorScopeOnStack scope;
                    o2Render.Begin();
                    o2Render.SetCamera(Camera());
                    shell.Draw();
                    o2UI.DrawCurrentLayerTopWidgets();
                    o2Render.End();
                }
                AppTestDriver::PumpFrames(1);
            }
        }

        void Move(const Vec2F& point, int steps = 4)
        {
            Vec2F from = o2Input.GetCursorPos();
            for (int i = 1; i <= steps; i++)
            {
                o2Input.OnCursorMoved(Math::Lerp(from, point, (float)i/(float)steps), 0);
                Step(1);
            }
        }

        void LeftClick(const Vec2F& point)
        {
            Move(point);
            o2Input.OnCursorPressed(point);
            Step(2);
            o2Input.OnCursorReleased();
            Step(3);
        }

        void MiddleDrag(const Vec2F& from, const Vec2F& to)
        {
            Move(from);
            o2Input.OnAlt2CursorPressed(from);
            Step(1);
            Move(to, 8);
            o2Input.OnAlt2CursorReleased();
            Step(2);
        }

        void AltWheel(const Vec2F& point, float delta)
        {
            Move(point);
            o2Input.OnKeyPressed(VK_MENU);
            Step(1);
            o2Input.OnMouseWheel(delta);
            Step(1);
            o2Input.OnKeyReleased(VK_MENU);
            Step(2);
        }

        // O2_GAME_WINDOW_SHOTS: a folder to save the frames into
        void Shot(const String& name)
        {
            const char* folder = getenv("O2_GAME_WINDOW_SHOTS");
            if (!folder)
                return;
            Frame frame = Capture([&]() { shell.Draw(); });
            if (frame.bitmap)
                frame.bitmap->Save(String(folder) + "/" + name + ".png", Bitmap::ImageType::Png);
        }
    };

    // A camera of the game's own pixels and a button at the middle of its picture
    struct GameScene
    {
        Ref<CameraActor> camera;
        Ref<Button> button;
        int clicks = 0;

        GameScene()
        {
            ForcePopEditorScopeOnStack scope;
            camera = mmake<CameraActor>();
            camera->name = "game camera";
            camera->fillColor = Color4(40, 120, 90, 255);

            button = mmake<Button>();
            button->name = "button";
            button->AddLayer("back", mmake<Sprite>(Color4(230, 200, 60, 255)), Layout::BothStretch());
            *button->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(160, 160));
            button->onClick = [this]() { clicks++; };

            o2Scene.UpdateAddedEntities();
            o2Scene.UpdateTransforms();
        }

        ~GameScene()
        {
            ForcePopEditorScopeOnStack scope;
            o2Scene.Clear(true);
            o2Scene.UpdateDestroyingEntities();
        }
    };

    struct SafeAreaGuard
    {
        ~SafeAreaGuard() { o2Application.ResetSafeAreaOverride(); }
    };
}

class GameWindowDevicesUI: public EditorVectorUIFixture {};

// The emulated device draws in its real pixels, its body around the picture is fitted into the view, the game gets the
// device's safe area in those pixels; turned on its side and back to the window's own size
TEST_F(GameWindowDevicesUI, DeviceDrawsInItsPixelsWithItsBodyAndSafeArea)
{
    SafeAreaGuard guard;
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "game window"));
    Driver driver { shell };
    GameScene scene;
    driver.Step(4);

    auto game = shell.windows->GetWindow<GameWindow>();
    ASSERT_TRUE(game);
    auto view = game->GetGameViewWidget();

    game->SelectDevice("iphone-16", false);
    driver.Step(3);
    EXPECT_EQ(view->GetRenderTargetSize(), Vec2I(1179, 2556));

    RectF screen = view->GetScreenRect(), body = view->GetBodyRect(), viewRect = view->layout->GetWorldRect();
    EXPECT_NEAR(screen.Width()/screen.Height(), 393.0f/852.0f, 0.005f);
    EXPECT_TRUE(body.left < screen.left && body.right > screen.right && body.bottom < screen.bottom && body.top > screen.top)
        << "the body is around the picture";
    EXPECT_LE(body.Width(), viewRect.Width() + 0.5f);
    EXPECT_LE(body.Height(), viewRect.Height() + 0.5f);
    EXPECT_NEAR(body.Center().x, viewRect.Center().x, 0.5f);

    ASSERT_TRUE(o2Application.IsSafeAreaOverridden());
    BorderF insets = o2Application.GetSafeAreaInsets();
    EXPECT_NEAR(insets.top, 177.0f, 0.5f);
    EXPECT_NEAR(insets.bottom, 102.0f, 0.5f);
    driver.Shot("game_window_iphone16");

    game->SelectDevice("iphone-16", true);
    driver.Step(3);
    EXPECT_EQ(view->GetRenderTargetSize(), Vec2I(2556, 1179));
    insets = o2Application.GetSafeAreaInsets();
    EXPECT_NEAR(insets.left, 177.0f, 0.5f);
    EXPECT_NEAR(insets.right, 177.0f, 0.5f);
    EXPECT_NEAR(insets.bottom, 63.0f, 0.5f);
    EXPECT_NEAR(insets.top, 0.0f, 0.5f);
    driver.Shot("game_window_iphone16_landscape");

    game->SelectDevice("pixel-8", false);
    driver.Step(3);
    EXPECT_EQ(view->GetRenderTargetSize(), Vec2I(1080, 2400));
    driver.Shot("game_window_pixel8");

    game->SelectDevice("ipad", false);
    driver.Step(3);
    driver.Shot("game_window_ipad");

    game->SelectDevice("720p", false);
    driver.Step(3);
    EXPECT_EQ(view->GetRenderTargetSize(), Vec2I(1280, 720));
    driver.Shot("game_window_720p");

    game->SelectDevice("", false);
    driver.Step(3);
    EXPECT_EQ(view->GetRenderTargetSize(), (Vec2I)viewRect.Size());
    EXPECT_FALSE(o2Application.IsSafeAreaOverridden());
    EXPECT_EQ(view->GetBodyRect(), RectF());
}

// Every device of the list builds its body for both orientations
TEST_F(GameWindowDevicesUI, EveryDeviceHasABody)
{
    for (auto& device : GetGameDevices())
    {
        for (bool landscape : { false, true })
        {
            Vec2F size, offset;
            String svg = device.GetBodySvg(landscape, size, offset);
            Vec2F screen = device.GetScreenSize(landscape);
            EXPECT_TRUE(svg.StartsWith("<svg")) << device.id.Data();
            EXPECT_GE(size.x, screen.x) << device.id.Data();
            EXPECT_GE(size.y, screen.y) << device.id.Data();
            EXPECT_LE(offset.x + screen.x, size.x + 0.01f) << device.id.Data();
            EXPECT_LE(offset.y + screen.y, size.y + 0.01f) << device.id.Data();

            // the body lies over the picture's edges only: across the middle of the screen it is transparent
            VectorImage image;
            ASSERT_TRUE(SvgParser::Parse(svg, image)) << device.id.Data();
            float scale = 0.5f;
            auto bitmap = VectorRasterizer::Rasterize(image, scale);
            ASSERT_TRUE(bitmap) << device.id.Data();
            for (float u : { 0.25f, 0.5f, 0.75f })
            {
                int x = (int)((offset.x + screen.x*u)*scale), y = (int)((offset.y + screen.y*0.5f)*scale);
                EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, x, y).a, 0)
                    << device.id.Data() << (landscape ? " landscape" : " portrait") << " covers its screen at " << u;
            }
        }
    }
}

// Alt + wheel zooms around the cursor (the point of the picture stays under it), the middle button pans, the zoom
// button fits it again; the game's button takes clicks wherever its picture is drawn, and none from outside the view
TEST_F(GameWindowDevicesUI, ZoomAndPanKeepTheGameClickable)
{
    SafeAreaGuard guard;
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "game window"));
    Driver driver { shell };
    GameScene scene;
    driver.Step(4);

    auto game = shell.windows->GetWindow<GameWindow>();
    ASSERT_TRUE(game);
    auto view = game->GetGameViewWidget();
    game->SelectDevice("iphone-16", false);
    driver.Step(3);

    // the button is at the middle of the picture (the test runner's own frame also draws the scene as a game does, so
    // a click may count twice: what matters is whether it reaches the button)
    driver.LeftClick(view->GetScreenRect().Center());
    EXPECT_GT(scene.clicks, 0);
    int clicks = scene.clicks;

    // Alt + wheel over a point of the picture: zoomed in, the same point of the picture under the cursor
    RectF before = view->GetScreenRect();
    Vec2F pivot = before.Center() + Vec2F(before.Width()*0.2f, before.Height()*0.15f);
    Vec2F uv((pivot.x - before.left)/before.Width(), (pivot.y - before.bottom)/before.Height());
    driver.AltWheel(pivot, 240.0f);
    RectF zoomed = view->GetScreenRect();
    EXPECT_GT(view->GetZoom(), 1.5f);
    EXPECT_NEAR(zoomed.Width()/before.Width(), view->GetZoom(), 0.01f);
    EXPECT_NEAR(zoomed.left + uv.x*zoomed.Width(), pivot.x, 1.0f);
    EXPECT_NEAR(zoomed.bottom + uv.y*zoomed.Height(), pivot.y, 1.0f);
    EXPECT_EQ(scene.clicks, clicks) << "Alt + wheel is the view's, not the game's";
    driver.Shot("game_window_zoomed");

    // the middle button drags the picture
    Vec2F panFrom = view->layout->GetWorldRect().Center();
    driver.MiddleDrag(panFrom, panFrom + Vec2F(-40.0f, 30.0f));
    RectF panned = view->GetScreenRect();
    EXPECT_NEAR(panned.left - zoomed.left, -40.0f, 1.5f);
    EXPECT_NEAR(panned.bottom - zoomed.bottom, 30.0f, 1.5f);
    EXPECT_EQ(scene.clicks, clicks) << "panning does not click the game";

    // the button, wherever it is drawn now
    Vec2F buttonCenter = panned.Center();
    RectF viewRect = view->layout->GetWorldRect();
    if (viewRect.IsInside(buttonCenter))
    {
        driver.LeftClick(buttonCenter);
        EXPECT_GT(scene.clicks, clicks) << "the zoomed and panned picture takes clicks where it is drawn";
    }

    // the picture reaches out of the view: a click there is not the game's
    view->SetZoom(10.0f, viewRect.Center());
    driver.Step(2);
    RectF huge = view->GetScreenRect();
    ASSERT_LT(huge.left, viewRect.left - 20.0f);
    clicks = scene.clicks;
    driver.LeftClick(Vec2F(viewRect.left - 10.0f, viewRect.Center().y));
    EXPECT_EQ(scene.clicks, clicks) << "outside the view the click is not the game's";

    // the zoom button fits the picture again
    view->ResetZoom();
    driver.Step(2);
    EXPECT_NEAR(view->GetZoom(), 1.0f, 0.001f);
    EXPECT_EQ(view->GetScreenRect(), before);
}

