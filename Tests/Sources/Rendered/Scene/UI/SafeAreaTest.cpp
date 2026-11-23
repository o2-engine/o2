#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/SafeArea.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;

namespace
{
    // Drops the emulated safe area whatever the test did
    struct SafeAreaOverrideGuard
    {
        ~SafeAreaOverrideGuard() { o2Integration.ResetSafeAreaOverride(); }
    };

    // The camera takes its frame from the resolution of the frame being drawn
    void DrawCamera(const Ref<CameraActor>& camera)
    {
        o2Render.Begin();
        camera->SetupAndDraw();
        o2Render.End();
    }

    // A portrait phone camera (the iPhone 16 screen, 1179x2556 units) with a safe area right inside it
    struct SafeAreaRig
    {
        Ref<CameraActor> camera;
        Ref<SafeArea>    area;
    };

    SafeAreaRig MakeRig()
    {
        SafeAreaRig rig;
        rig.camera = mmake<CameraActor>();
        rig.camera->SetName("ui camera");
        rig.camera->SetFittedSize(Vec2F(1179, 2556));

        rig.area = mmake<SafeArea>();
        rig.area->SetName("safe area");
        rig.camera->AddChild(rig.area);

        DrawCamera(rig.camera);
        TickAndUpdateLayout(2);
        return rig;
    }

    // Where the widget is drawn: its layout rectangle through the world transform (scaled parents included)
    RectF DrawnRect(const Ref<Widget>& widget)
    {
        Basis basis = widget->layout->GetWorldBasis();
        Vec2F a = basis.origin, b = basis.origin + basis.xv + basis.yv;
        return RectF(Math::Min(a.x, b.x), Math::Min(a.y, b.y), Math::Max(a.x, b.x), Math::Max(a.y, b.y));
    }

    // The camera's frame shrunk by insets given in points of the content
    RectF Expected(const Ref<CameraActor>& camera, const BorderF& insets)
    {
        RectF frame = camera->transform->GetWorldRect();
        Vec2F resolution = camera->GetRenderResolution();
        Vec2F unit(frame.Width()/resolution.x, frame.Height()/resolution.y);
        return RectF(frame.left + insets.left*unit.x, frame.bottom + insets.bottom*unit.y,
                     frame.right - insets.right*unit.x, frame.top - insets.top*unit.y);
    }

    void ExpectRect(const RectF& actual, const RectF& expected, const char* what)
    {
        // scene layouts are floored to whole units of the parent, which may be scaled up to 1.36
        EXPECT_NEAR(actual.left, expected.left, 1.5f) << what;
        EXPECT_NEAR(actual.bottom, expected.bottom, 1.5f) << what;
        EXPECT_NEAR(actual.right, expected.right, 1.5f) << what;
        EXPECT_NEAR(actual.top, expected.top, 1.5f) << what;
    }
}

// No insets: the area is the camera's whole frame, which covers at least the fitted size
TEST(SafeArea, WithoutInsetsTakesTheCameraFrame)
{
    SceneCleanGuard guard;
    SafeAreaOverrideGuard overrideGuard;
    o2Integration.SetSafeAreaOverride(BorderF());

    auto rig = MakeRig();
    RectF frame = rig.camera->transform->GetWorldRect();
    EXPECT_GE(frame.Width(), 1179.0f - 0.5f);
    EXPECT_GE(frame.Height(), 2556.0f - 0.5f);
    ExpectRect(DrawnRect(rig.area), frame, "area");
}

// Insets come in points of the content; the area keeps clear of them in the camera's units
TEST(SafeArea, KeepsClearOfTheInsetsInCameraUnits)
{
    SceneCleanGuard guard;
    SafeAreaOverrideGuard overrideGuard;
    BorderF insets(10, 34, 20, 59);
    o2Integration.SetSafeAreaOverride(insets);

    auto rig = MakeRig();
    ExpectRect(DrawnRect(rig.area), Expected(rig.camera, insets), "area");
    ExpectRect(rig.area->GetSafeWorldRect(), Expected(rig.camera, insets), "safe world rect");
}

// A new safe area (a turned device, an emulated one) moves the area on the next frame; sides can be let through
TEST(SafeArea, FollowsTheSafeAreaAndItsSides)
{
    SceneCleanGuard guard;
    SafeAreaOverrideGuard overrideGuard;
    o2Integration.SetSafeAreaOverride(BorderF(0, 34, 0, 59));
    auto rig = MakeRig();

    BorderF turned(59, 21, 59, 0);
    o2Integration.SetSafeAreaOverride(turned);
    TickAndUpdateLayout(1);
    ExpectRect(DrawnRect(rig.area), Expected(rig.camera, turned), "turned");

    rig.area->left = false;
    TickAndUpdateLayout(1);
    ExpectRect(DrawnRect(rig.area), Expected(rig.camera, BorderF(0, 21, 59, 0)), "left side let through");
}

// A screen designed in its own units is scaled into the camera: the area inside it is still drawn over the safe part
TEST(SafeArea, InsideAScaledScreen)
{
    SceneCleanGuard guard;
    SafeAreaOverrideGuard overrideGuard;
    BorderF insets(0, 34, 0, 59);
    o2Integration.SetSafeAreaOverride(insets);

    auto camera = mmake<CameraActor>();
    camera->SetFittedSize(Vec2F(1179, 2556));

    auto screen = MakeWidget("screen");
    camera->AddChild(screen);
    *screen->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(868, 1813));
    screen->layout->SetPivot(Vec2F(0.5f, 0.5f));
    screen->layout->SetScale(Vec3F(1179.0f/868.0f, 1179.0f/868.0f, 1.0f));

    auto area = mmake<SafeArea>();
    screen->AddChild(area);
    auto button = MakeChildWidget(area, "button");
    *button->layout = WidgetLayout::BothStretch();

    DrawCamera(camera);
    TickAndUpdateLayout(3);

    ExpectRect(DrawnRect(area), Expected(camera, insets), "area");
    ExpectRect(DrawnRect(button), Expected(camera, insets), "child stretched over the area");
}

// The root of a screen may be a plain scaled actor (graphics, effects and the interface under it)
TEST(SafeArea, InsideAScaledActor)
{
    SceneCleanGuard guard;
    SafeAreaOverrideGuard overrideGuard;
    BorderF insets(0, 34, 0, 59);
    o2Integration.SetSafeAreaOverride(insets);

    auto camera = mmake<CameraActor>();
    camera->SetFittedSize(Vec2F(1179, 2556));

    auto root = mmake<Actor>();
    root->SetName("screen root");
    camera->AddChild(root);
    root->transform->SetScale(Vec3F(1179.0f/868.0f, 1179.0f/868.0f, 1.0f));

    auto area = mmake<SafeArea>();
    root->AddChild(area);
    auto panel = MakeChildWidget(area, "panel");
    *panel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 100);

    DrawCamera(camera);
    TickAndUpdateLayout(3);

    RectF expected = Expected(camera, insets);
    ExpectRect(DrawnRect(area), expected, "area");
    RectF drawnPanel = DrawnRect(panel);
    EXPECT_NEAR(drawnPanel.top, expected.top, 1.5f);
    EXPECT_NEAR(drawnPanel.Height(), 100.0f*1179.0f/868.0f, 1.5f);
}

// Outside a camera actor (a prototype opened alone) there is no frame to keep clear in: the area keeps its layout
TEST(SafeArea, WithoutACameraKeepsItsLayout)
{
    SceneCleanGuard guard;
    SafeAreaOverrideGuard overrideGuard;
    o2Integration.SetSafeAreaOverride(BorderF(0, 34, 0, 59));

    auto panel = MakeWidget("panel");
    *panel->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(300, 200));
    auto area = mmake<SafeArea>();
    panel->AddChild(area);
    *area->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(40, 30));
    TickAndUpdateLayout(2);

    RectF drawn = DrawnRect(area);
    EXPECT_NEAR(drawn.Width(), 40.0f, 0.01f);
    EXPECT_NEAR(drawn.Height(), 30.0f, 0.01f);
    EXPECT_NEAR(drawn.left, DrawnRect(panel).left, 0.01f);
}

// A copy keeps the sides
TEST(SafeArea, CopyKeepsTheSides)
{
    SceneCleanGuard guard;
    auto area = mmake<SafeArea>();
    area->top = false;
    area->right = false;

    auto copy = area->CloneAsRef<SafeArea>();
    EXPECT_FALSE(copy->top);
    EXPECT_FALSE(copy->right);
    EXPECT_TRUE(copy->left);
    EXPECT_TRUE(copy->bottom);
}
