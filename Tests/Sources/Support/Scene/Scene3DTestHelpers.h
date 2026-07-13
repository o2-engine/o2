#pragma once

#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"

namespace o2
{
    struct Test3DScene
    {
        Ref<CameraActor> camera;
        Ref<Actor> ground;
        Ref<Actor> box1;
        Ref<Actor> box2;
        Ref<Actor> sphere;
        Ref<Actor> cylinder;
    };

    // Builds test 3D scene: perspective camera at (0, 0, 500) looking down -Z at origin,
    // ground plane and a few primitives with 3D positions and rotations
    Test3DScene BuildTest3DScene();

    struct LightingDemoScene
    {
        Test3DScene base;

        Ref<Actor> sun;
        Ref<Actor> warmLight;
        Ref<Actor> coldLight;

        Ref<CameraActor> uiCamera;
        Ref<Actor>       sprite2D;
    };

    // Builds the deferred pipeline demo, mirroring the game demo scene structure: the test
    // 3D scene in the "3D" layer with DeferredPipeline on the camera, a directional sun and
    // two colored point lights, plus a "2D" layer with a sprite actor rendered by a second
    // orthographic camera over the 3D image
    LightingDemoScene BuildLightingDemoScene();
}
