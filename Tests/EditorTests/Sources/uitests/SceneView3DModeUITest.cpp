#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

using namespace o2;
using namespace Editor;

TEST(SceneView3DModeUI, ToggleOnOffAndConversionsRoundTrip)
{
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();

    Camera cameraBefore = screen.GetCamera();

    screen.SetView3DMode(true);
    EXPECT_TRUE(screen.IsView3DMode());

    for (auto scenePoint : { Vec2F(0.0f, 0.0f), Vec2F(120.0f, -40.0f), Vec2F(-300.0f, 200.0f) })
    {
        Vec2F screenPoint = screen.SceneToScreenPoint(scenePoint);
        Vec2F back = screen.ScreenToScenePoint(screenPoint);
        EXPECT_NEAR(back.x, scenePoint.x, 0.5f);
        EXPECT_NEAR(back.y, scenePoint.y, 0.5f);

        Vec2F vectorBack = screen.SceneToScreenVector(screen.ScreenToSceneVector(Vec2F(30.0f, 15.0f)));
        EXPECT_NEAR(vectorBack.x, 30.0f, 0.5f);
        EXPECT_NEAR(vectorBack.y, 15.0f, 0.5f);
    }

    // Z axis mapping is available in 3D mode when the view is tilted
    screen.GetView3DState().pitch = Math::Deg2rad(60.0f);
    float z = 0.0f;
    EXPECT_TRUE(screen.ScreenToZAxisPoint(screen.layout->GetWorldRect().Center(), Vec2F(0.0f, 0.0f), z));

    screen.SetView3DMode(false);
    EXPECT_FALSE(screen.IsView3DMode());

    // The 2D camera returns to the same view center after the 3D round trip
    EXPECT_NEAR(screen.GetCamera().GetPosition().x, cameraBefore.GetPosition().x, 0.5f);
    EXPECT_NEAR(screen.GetCamera().GetPosition().y, cameraBefore.GetPosition().y, 0.5f);
    EXPECT_NEAR(screen.GetCamera().GetScale().y, cameraBefore.GetScale().y, 0.01f);
}

TEST(SceneView3DModeUI, RepeatedTogglingIsStable)
{
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();

    for (int i = 0; i < 5; i++)
    {
        screen.SetView3DMode(true);
        screen.SetView3DMode(false);
    }

    EXPECT_FALSE(screen.IsView3DMode());
}
