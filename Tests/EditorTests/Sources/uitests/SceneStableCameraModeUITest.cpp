#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Pipeline/RenderPipeline.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// A custom scene camera pipeline (offscreen passes, screen shaders) distorts the edit view;
// the stable camera mode must fall back to the default forward 3D/2D pipeline
TEST(SceneStableCameraMode, TogglingSwitchesTheResolvedPipeline)
{
    SceneCleanGuard guard;
    auto& screen = o2EditorSceneScreen;

    auto camera = mmake<CameraActor>();
    camera->SetName("scene camera");
    auto customPipeline = mmake<RenderPipeline>();
    camera->SetRenderPipeline(customPipeline);
    TickScene();
    ASSERT_FALSE(o2Scene.GetCameras().IsEmpty());

    // the default mode follows the scene camera pipeline
    EXPECT_FALSE(screen.IsStableCameraMode());
    EXPECT_EQ(screen.ResolveScenePipeline(), customPipeline);

    bool notifiedStable = false;
    screen.onStableCameraModeChanged = [&](bool stable) { notifiedStable = stable; };

    screen.SetStableCameraMode(true);
    EXPECT_TRUE(screen.IsStableCameraMode());
    EXPECT_TRUE(notifiedStable);
    EXPECT_EQ(screen.ResolveScenePipeline(), CameraActor::GetDefaultRenderPipeline());

    screen.SetStableCameraMode(false);
    EXPECT_FALSE(notifiedStable);
    EXPECT_EQ(screen.ResolveScenePipeline(), customPipeline);

    screen.onStableCameraModeChanged = Function<void(bool)>();
}

// Without cameras in the scene both modes resolve to the default pipeline
TEST(SceneStableCameraMode, EmptySceneAlwaysResolvesTheDefaultPipeline)
{
    SceneCleanGuard guard;
    auto& screen = o2EditorSceneScreen;

    EXPECT_EQ(screen.ResolveScenePipeline(), CameraActor::GetDefaultRenderPipeline());

    screen.SetStableCameraMode(true);
    EXPECT_EQ(screen.ResolveScenePipeline(), CameraActor::GetDefaultRenderPipeline());
    screen.SetStableCameraMode(false);
}
