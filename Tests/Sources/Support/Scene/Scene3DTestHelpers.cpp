#include "o2/stdafx.h"

#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Math/Math.h"
#include "Scene/Scene3DTestHelpers.h"

namespace o2
{
    static Ref<Actor> MakePrimitiveActor(const String& name, PrimitiveType3D type, const Vec3F& size,
                                         const Vec3F& position, const Vec3F& eulerAngles, const Color4& color)
    {
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->SetName(name);
        actor->transform->SetPosition(position);
        actor->transform->SetEulerAngles(eulerAngles);

        auto component = actor->AddComponent<MeshPrimitiveComponent>();
        component->SetPrimitiveType(type);
        component->SetSize(size);
        component->SetColor(color);

        return actor;
    }

    Test3DScene BuildTest3DScene()
    {
        Test3DScene scene;

        // Z-up scene: ground is the XY plane at z = 0, camera looks along +Y slightly down at the origin
        scene.camera = mmake<CameraActor>();
        scene.camera->SetName("camera3d");
        scene.camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
        scene.camera->transform->SetPosition(Vec3F(0, -500, 250));
        scene.camera->transform->SetEulerAngles(Vec3F(Math::Deg2rad(63.0f), 0, 0));
        scene.camera->fillColor = Color4::Black();

        scene.ground = MakePrimitiveActor("ground", PrimitiveType3D::Plane, Vec3F(1000, 1000, 0),
                                          Vec3F(0, 0, 0), Vec3F(),
                                          Color4(80, 120, 80));

        scene.box1 = MakePrimitiveActor("box1", PrimitiveType3D::Box, Vec3F(100, 100, 100),
                                        Vec3F(0, 0, 50), Vec3F(0, 0, 0), Color4(200, 60, 60));

        scene.box2 = MakePrimitiveActor("box2", PrimitiveType3D::Box, Vec3F(80, 120, 60),
                                        Vec3F(150, 100, 150), Vec3F(0.4f, 0.2f, 0.6f), Color4(60, 60, 200));

        scene.sphere = MakePrimitiveActor("sphere", PrimitiveType3D::Sphere, Vec3F(90, 90, 90),
                                          Vec3F(-150, -50, 45), Vec3F(0, 0, 0), Color4(220, 200, 60));

        scene.cylinder = MakePrimitiveActor("cylinder", PrimitiveType3D::Cylinder, Vec3F(60, 140, 60),
                                            Vec3F(60, -100, 70), Vec3F(Math::Deg2rad(90.0f), 0, 0.5f),
                                            Color4(60, 200, 200));

        return scene;
    }

    static Ref<Actor> MakeLightActor(const String& name, LightComponent::Type type, const Color4& color,
                                     float intensity, float range, const Vec3F& position, const Vec3F& eulerAngles)
    {
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->SetName(name);
        actor->transform->SetPosition(position);
        actor->transform->SetEulerAngles(eulerAngles);

        auto light = actor->AddComponent<LightComponent>();
        light->SetLightType(type);
        light->SetColor(color);
        light->SetIntensity(intensity);
        light->SetRange(range);

        return actor;
    }

    LightingDemoScene BuildLightingDemoScene()
    {
        LightingDemoScene demo;
        demo.base = BuildTest3DScene();

        o2Scene.AddLayer("3D");
        o2Scene.AddLayer("2D");

        demo.base.camera->SetName("demo camera");
        demo.base.camera->SetLayer("3D");
        demo.base.camera->drawLayers.SetLayers(Vector<String>{ "3D" });
        demo.base.camera->SetRenderPipeline(mmake<DeferredPipeline>());
        demo.base.camera->fillColor = Color4(12, 12, 18);

        demo.base.ground->SetLayer("3D");
        demo.base.box1->SetLayer("3D");
        demo.base.box2->SetLayer("3D");
        demo.base.sphere->SetLayer("3D");
        demo.base.cylinder->SetLayer("3D");

        demo.sun = MakeLightActor("sun", LightComponent::Type::Directional, Color4(255, 250, 235), 0.9f, 0.0f,
                                  Vec3F(0, -300, 400), Vec3F(Math::Deg2rad(35.0f), 0, Math::Deg2rad(25.0f)));

        demo.warmLight = MakeLightActor("warm point", LightComponent::Type::Point, Color4(255, 160, 60), 1.6f, 450.0f,
                                        Vec3F(180, -80, 220), Vec3F());

        demo.coldLight = MakeLightActor("cold point", LightComponent::Type::Point, Color4(80, 140, 255), 1.4f, 400.0f,
                                        Vec3F(-220, 40, 160), Vec3F());

        demo.sun->SetLayer("3D");
        demo.warmLight->SetLayer("3D");
        demo.coldLight->SetLayer("3D");

        demo.uiCamera = mmake<CameraActor>();
        demo.uiCamera->SetName("ui camera");
        demo.uiCamera->SetLayer("2D");
        demo.uiCamera->drawLayers.SetLayers(Vector<String>{ "2D" });
        demo.uiCamera->SetFittedSize(Vec2F(1280, 1024));
        demo.uiCamera->fillBackground = false;

        demo.sprite2D = mmake<Actor>(ActorCreateMode::InScene);
        demo.sprite2D->SetName("sprite");
        demo.sprite2D->SetLayer("2D");
        demo.sprite2D->transform->SetSize2D(Vec2F(100, 100));
        demo.sprite2D->transform->SetPosition2D(Vec2F(-500, 400));

        return demo;
    }
}
