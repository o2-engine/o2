#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Pipeline/RenderPipeline.h"
#include "o2/Render/Render.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/GameWindow/GameWindow.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneDrawableCategory.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;
using namespace EditorVectorUITest;

namespace
{
    const SceneRenderMode allModes[] = { SceneRenderMode::Camera, SceneRenderMode::Stable, SceneRenderMode::Unlit,
                                         SceneRenderMode::Wireframe, SceneRenderMode::ShadedWireframe,
                                         SceneRenderMode::Overdraw };

    // Puts the screen back to the scene camera mode
    struct RenderModeGuard
    {
        ~RenderModeGuard()
        {
            o2EditorSceneScreen.SetRenderMode(SceneRenderMode::Camera);
            o2EditorSceneScreen.onRenderModeChanged = Function<void(SceneRenderMode)>();
            o2EditorSceneScreen.onStableCameraModeChanged = Function<void(bool)>();
        }
    };

    // Draws the scene screen in a frame, returns the triangles the frame has drawn
    int DrawScreen()
    {
        auto& screen = o2EditorSceneScreen;

        o2Render.Begin();
        screen.NeedRedraw();
        screen.Draw();
        int triangles = o2Render.GetDrawnPrimitives();
        o2Render.End();

        return triangles;
    }
}

// Every mode but the scene camera one draws through the default pipeline
TEST(SceneRenderMode, ModesResolveTheDefaultPipelineButTheCameraOne)
{
    SceneCleanGuard guard;
    RenderModeGuard modeGuard;
    auto& screen = o2EditorSceneScreen;

    auto camera = mmake<CameraActor>();
    auto customPipeline = mmake<RenderPipeline>();
    camera->SetRenderPipeline(customPipeline);
    TickScene();

    Vector<SceneRenderMode> notified;
    int stableChanges = 0;
    screen.onRenderModeChanged = [&](SceneRenderMode mode) { notified.Add(mode); };
    screen.onStableCameraModeChanged = [&](bool) { stableChanges++; };

    for (auto mode : allModes)
    {
        screen.SetRenderMode(mode);
        EXPECT_EQ(screen.GetRenderMode(), mode);
        EXPECT_EQ(screen.IsStableCameraMode(), mode != SceneRenderMode::Camera);
        EXPECT_EQ(screen.ResolveScenePipeline(), mode == SceneRenderMode::Camera ? customPipeline :
                  CameraActor::GetDefaultRenderPipeline()) << SceneEditScreen::GetRenderModeName(mode);
    }

    EXPECT_EQ(notified.Count(), 5) << "the camera mode was set already";
    EXPECT_EQ(stableChanges, 1) << "the stable camera turned on once";

    screen.SetStableCameraMode(false);
    EXPECT_EQ(screen.GetRenderMode(), SceneRenderMode::Camera);
}

// The debug modes are on while the scene is drawn only: the gizmos and the other windows are drawn as they are
TEST(SceneRenderMode, DrawingLeavesTheRenderAsItWas)
{
    SceneCleanGuard guard;
    RenderModeGuard modeGuard;
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();
    screen.SetView3DMode(false);

    auto actor = mmake<Actor>();
    actor->AddComponent<ImageComponent>();
    TickScene();

    screen.SetRenderMode(SceneRenderMode::Stable);
    int filled = DrawScreen();

    for (auto mode : allModes)
    {
        screen.SetRenderMode(mode);
        int triangles = DrawScreen();

        EXPECT_EQ(o2Render.GetDebugDrawMode(), Render::DebugDrawMode::Normal) << SceneEditScreen::GetRenderModeName(mode);
        EXPECT_FALSE(ScenePassFilters::IsRawAlbedoMode()) << SceneEditScreen::GetRenderModeName(mode);
        EXPECT_EQ(VectorSprite::GetDetailView(), VectorSprite::DetailView::Own) << SceneEditScreen::GetRenderModeName(mode);

        if (mode == SceneRenderMode::Wireframe)
            EXPECT_LT(triangles, filled) << "the triangles of the scene are drawn by their edges";

        if (mode == SceneRenderMode::ShadedWireframe)
            EXPECT_GE(triangles, filled) << "the filled triangles stay under the edges";
    }
}

// The scene window is a look at the game: its zoom magnifies vector sprites as the Game window details them
TEST(SceneRenderMode, VectorSpritesKeepTheDetailOfTheGameWindow)
{
    SceneCleanGuard guard;
    RenderModeGuard modeGuard;
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();
    screen.SetView3DMode(false);

    // The game camera shows 25x18.75 units in 400x300 pixels: 16 pixels a unit, unlike the scene window at any density
    auto camera = mmake<CameraActor>();
    camera->SetFixedSize(Vec2F(25.0f, 18.75f));

    auto widget = mmake<Widget>();
    auto sprite = mmake<VectorSprite>(AssetRef<VectorImageAsset>("ui/UI4_Background.svg"));
    widget->AddLayer("image", sprite);
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(8.0f, 5.0f));
    TickScene();

    Ref<GameWindow::GameView> gameView;
    {
        PushEditorScopeOnStack scope;
        gameView = mmake<GameWindow::GameView>();
    }

    gameView->resolution = Vec2I(400, 300);
    *gameView->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(400.0f, 300.0f));
    gameView->UpdateSelfTransform();
    gameView->UpdateRenderTargetSize();

    o2Render.Begin();
    gameView->Draw();
    o2Render.End();

    Vec2F gameMeshScale = sprite->GetMeshPixelScale();
    ASSERT_NE(gameMeshScale, Vec2F()) << "the Game window draws the sprite";

    Camera zoomed;
    zoomed.SetScale(Vec2F(0.5f, 0.5f));
    screen.SetCamera(zoomed);
    DrawScreen();

    EXPECT_EQ(sprite->GetMeshPixelScale(), gameMeshScale) << "the zoom of the scene window does not detail the sprite";
    EXPECT_EQ(VectorSprite::GetDetailView(), VectorSprite::DetailView::Own);

    screen.SetCamera(Camera());
}

class SceneRenderModeToolbar: public EditorVectorUIFixture
{};

// The toolbar button shows the mode and opens the menu of all the modes, the chosen one checked
TEST_F(SceneRenderModeToolbar, ButtonShowsTheMode)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    RenderModeGuard modeGuard;

    auto window = o2EditorSceneWindow.GetWindow();
    ASSERT_TRUE(window);

    auto button = window->FindChildByTypeAndName<Button>("render mode");
    ASSERT_TRUE(button);
    EXPECT_EQ(button->GetCaption(), WString("Scene camera"));

    auto menu = window->FindChildByTypeAndName<ContextMenu>("render modes");
    ASSERT_TRUE(menu);

    auto checkedItems = [&]()
    {
        Vector<WString> names;
        for (auto& item : menu->GetItems())
        {
            if (item->checkable && item->checked)
                names.Add(item->text);
        }

        return names;
    };

    EXPECT_EQ(menu->GetItems().Count(), 6);
    EXPECT_EQ(checkedItems(), Vector<WString>({ "Scene camera" }));

    o2EditorSceneScreen.SetRenderMode(SceneRenderMode::Overdraw);
    EXPECT_EQ(button->GetCaption(), WString("Overdraw"));
    EXPECT_EQ(checkedItems(), Vector<WString>({ "Overdraw" }));

    // Choosing an item of the menu sets its mode
    for (auto& item : menu->GetItems())
    {
        if (item->text == WString("Wireframe"))
            item->onChecked(true);
    }

    EXPECT_EQ(o2EditorSceneScreen.GetRenderMode(), SceneRenderMode::Wireframe);
    EXPECT_EQ(button->GetCaption(), WString("Wireframe"));
}
