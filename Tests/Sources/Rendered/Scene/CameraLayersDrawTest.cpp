#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace o2
{
    // 2D component drawing a filled quad, used as sprite-like 2D content in layer tests
    class LayerQuadDrawerComponent: public Component
    {
    public:
        Color4 drawColor = Color4::White();
        float  halfSize = 50.0f;

        LayerQuadDrawerComponent() {}
        LayerQuadDrawerComponent(const LayerQuadDrawerComponent& other):
            Component(other), drawColor(other.drawColor), halfSize(other.halfSize) {}

        SERIALIZABLE(LayerQuadDrawerComponent);
        CLONEABLE_REF(LayerQuadDrawerComponent);

    protected:
        void OnDraw() override
        {
            o2Render.DrawFilledPolygon({ Vec2F(-halfSize, -halfSize), Vec2F(halfSize, -halfSize),
                                         Vec2F(halfSize, halfSize), Vec2F(-halfSize, halfSize) }, drawColor);
        }
    };
}

namespace
{
    Ref<Bitmap> RenderCamerasFrame(const Vector<Ref<CameraActor>>& cameras)
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        for (auto& camera : cameras)
            camera->SetupAndDraw();
        o2Render.End();

        return captured;
    }

    const UInt8* GetPixel(const Ref<Bitmap>& bitmap, int x, int y)
    {
        Vec2I size = bitmap->GetSize();
        return bitmap->GetData() + (y*size.x + x)*4;
    }

    struct LayeredScene
    {
        Ref<CameraActor> camera3D;
        Ref<CameraActor> camera2D;
    };

    // Green 3D box at screen center in the "3D" layer; smaller red 2D quad over
    // the center in the "2D" layer; each camera renders only its own layer
    LayeredScene BuildLayeredScene()
    {
        o2Scene.AddLayer("3D");
        o2Scene.AddLayer("2D");

        LayeredScene scene;

        scene.camera3D = mmake<CameraActor>();
        scene.camera3D->SetName("camera 3d");
        scene.camera3D->drawLayers.SetLayers(Vector<String>{ "3D" });
        scene.camera3D->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
        scene.camera3D->transform->SetPosition(Vec3F(0, 0, 500));
        scene.camera3D->fillColor = Color4::Black();

        scene.camera2D = mmake<CameraActor>();
        scene.camera2D->SetName("camera 2d");
        scene.camera2D->drawLayers.SetLayers(Vector<String>{ "2D" });
        scene.camera2D->fillColor = Color4::Black();

        auto box = mmake<Actor>(ActorCreateMode::InScene);
        box->SetName("box");
        box->SetLayer("3D");
        auto boxComponent = box->AddComponent<MeshPrimitiveComponent>();
        boxComponent->SetPrimitiveType(PrimitiveType3D::Box);
        boxComponent->SetSize(Vec3F(250, 250, 250));
        boxComponent->SetColor(Color4::Green());
        boxComponent->SetShaded(false);

        auto quad = mmake<Actor>(ActorCreateMode::InScene);
        quad->SetName("quad");
        quad->SetLayer("2D");
        auto quadComponent = quad->AddComponent<LayerQuadDrawerComponent>();
        quadComponent->drawColor = Color4::Red();
        quadComponent->halfSize = 40.0f;

        return scene;
    }
}

// 3D camera with the "3D" layer filter must not draw the 2D layer content
TEST(CameraLayersDraw, Camera3DDrawsOnlyItsLayer)
{
    SceneCleanGuard guard;
    auto scene = BuildLayeredScene();
    TickFrame();

    auto captured = RenderCamerasFrame({ scene.camera3D });
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();

    // Box is visible at center; the red 2D quad from the other layer is not drawn over it
    const UInt8* center = GetPixel(captured, size.x/2, size.y/2);
    EXPECT_GT((int)center[1], 200) << "3D box must be visible at center";
    EXPECT_LT((int)center[0], 60) << "2D quad must not be drawn by the 3D camera";
}

// 2D camera with the "2D" layer filter must not draw the 3D layer content
TEST(CameraLayersDraw, Camera2DDrawsOnlyItsLayer)
{
    SceneCleanGuard guard;
    auto scene = BuildLayeredScene();
    TickFrame();

    auto captured = RenderCamerasFrame({ scene.camera2D });
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();

    // Red quad visible at center, and no green box anywhere around it
    const UInt8* center = GetPixel(captured, size.x/2, size.y/2);
    EXPECT_GT((int)center[0], 200) << "2D quad must be visible at center";
    EXPECT_LT((int)center[1], 60) << "3D box must not be drawn by the 2D camera";

    const UInt8* nearCenter = GetPixel(captured, size.x/2 + size.x/6, size.y/2);
    EXPECT_LT((int)nearCenter[1], 60) << "3D box must not be drawn next to the quad";
}

// Both cameras drawn in scene order compose the full picture: 2D content over the 3D image
TEST(CameraLayersDraw, CamerasComposeLayersInOrder)
{
    SceneCleanGuard guard;
    auto scene = BuildLayeredScene();
    scene.camera2D->fillBackground = false;
    TickFrame();

    auto captured = RenderCamerasFrame({ scene.camera3D, scene.camera2D });
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();

    // 2D quad wins at center, 3D box is visible right next to it
    const UInt8* center = GetPixel(captured, size.x/2, size.y/2);
    EXPECT_GT((int)center[0], 200);
    EXPECT_LT((int)center[1], 60);

    const UInt8* nearCenter = GetPixel(captured, size.x/2 + size.x/6, size.y/2);
    EXPECT_GT((int)nearCenter[1], 200);
}

// Regression: layered content of a LOADED scene must draw right away, without touching
// or selecting objects (the deserialized layer must be the scene layer, not a detached copy)
TEST(CameraLayersDraw, LoadedSceneDrawsLayeredContentImmediately)
{
    SceneCleanGuard guard;
    BuildLayeredScene();
    TickFrame();

    DataDocument document;
    o2Scene.Save(document);

    o2Scene.Clear(false);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(document);
    TickFrame();

    Ref<CameraActor> camera3D, camera2D;
    for (auto& weakCamera : o2Scene.GetCameras())
    {
        auto camera = weakCamera.Lock();
        if (!camera)
            continue;

        if (camera->GetName() == "camera 3d")
            camera3D = camera;
        else if (camera->GetName() == "camera 2d")
            camera2D = camera;
    }

    ASSERT_TRUE(camera3D);
    ASSERT_TRUE(camera2D);
    camera2D->fillBackground = false;

    auto captured = RenderCamerasFrame({ camera3D, camera2D });
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();

    const UInt8* center = GetPixel(captured, size.x/2, size.y/2);
    EXPECT_GT((int)center[0], 200) << "loaded 2D quad must be visible without selection";

    const UInt8* nearCenter = GetPixel(captured, size.x/2 + size.x/6, size.y/2);
    EXPECT_GT((int)nearCenter[1], 200) << "loaded 3D box must be visible without selection";
}

// --- META ---

CLASS_BASES_META(o2::LayerQuadDrawerComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::LayerQuadDrawerComponent)
{
    FIELD().PUBLIC().DEFAULT_VALUE(Color4::White()).NAME(drawColor);
    FIELD().PUBLIC().DEFAULT_VALUE(50.0f).NAME(halfSize);
}
END_META;
CLASS_METHODS_META(o2::LayerQuadDrawerComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const LayerQuadDrawerComponent&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
}
END_META;

DECLARE_CLASS(o2::LayerQuadDrawerComponent, o2__LayerQuadDrawerComponent);
// --- END META ---
