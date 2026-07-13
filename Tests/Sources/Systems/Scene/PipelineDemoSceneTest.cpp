#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "Scene/Scene3DTestHelpers.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(PipelineDemoScene, RoundTripKeepsPipelineLightsAndLayeredCameras)
{
    SceneCleanGuard guard;
    auto demo = BuildLightingDemoScene();
    TickFrame();

    DataDocument document;
    o2Scene.Save(document);

    o2Scene.Clear(false);
    o2Scene.UpdateDestroyingEntities();
    ASSERT_EQ(o2Scene.GetLights().Count(), 0);

    o2Scene.Load(document);
    TickFrame();

    Ref<CameraActor> camera, uiCamera;
    for (auto& weakCamera : o2Scene.GetCameras())
    {
        auto sceneCamera = weakCamera.Lock();
        if (!sceneCamera)
            continue;

        if (sceneCamera->GetName() == "demo camera")
            camera = sceneCamera;
        else if (sceneCamera->GetName() == "ui camera")
            uiCamera = sceneCamera;
    }

    ASSERT_TRUE(camera);
    EXPECT_NE(DynamicCast<DeferredPipeline>(camera->GetRenderPipeline()), nullptr);

    // The layers split survives round trip: the 3D camera renders only the "3D" layer,
    // the UI camera overlays only the "2D" layer without clearing the background
    EXPECT_EQ(camera->drawLayers.GetLayersNames(), Vector<String>{ "3D" });

    ASSERT_TRUE(uiCamera);
    EXPECT_EQ(uiCamera->drawLayers.GetLayersNames(), Vector<String>{ "2D" });
    EXPECT_FALSE(uiCamera->fillBackground);

    // Cameras keep the scene order: the 3D camera draws first, the UI camera on top
    auto demoCameraIndex = o2Scene.GetCameras().IndexOf([&](auto& x) { return x.Lock() == camera; });
    auto uiCameraIndex = o2Scene.GetCameras().IndexOf([&](auto& x) { return x.Lock() == uiCamera; });
    EXPECT_LT(demoCameraIndex, uiCameraIndex);

    // Identity matters, not just the name: the passes and drawables registration compare
    // layer instances, and a detached inline copy of the layer makes objects invisible
    auto sprite = o2Scene.FindActor("sprite");
    ASSERT_TRUE(sprite);
    ASSERT_TRUE(sprite->GetLayer());
    EXPECT_EQ(sprite->GetLayer(), o2Scene.GetLayer("2D"));

    auto box = o2Scene.FindActor("box1");
    ASSERT_TRUE(box);
    ASSERT_TRUE(box->GetLayer());
    EXPECT_EQ(box->GetLayer(), o2Scene.GetLayer("3D"));

    auto& lights = o2Scene.GetLights();
    ASSERT_EQ(lights.Count(), 3);

    int directionalCount = 0, pointCount = 0;
    for (auto& weakLight : lights)
    {
        auto light = weakLight.Lock();
        ASSERT_TRUE(light);

        if (light->GetLightType() == LightComponent::Type::Directional)
            directionalCount++;
        else
            pointCount++;
    }

    EXPECT_EQ(directionalCount, 1);
    EXPECT_EQ(pointCount, 2);
}
