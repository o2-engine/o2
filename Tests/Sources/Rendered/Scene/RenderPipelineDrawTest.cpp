#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Pipeline/DeferredPasses.h"
#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "Scene/Scene3DTestHelpers.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace o2
{
    // 2D component drawing a filled polygon, used as sprite-like 2D content in pipeline tests
    class PolygonDrawerComponent: public Component
    {
    public:
        Color4 drawColor = Color4::White();
        float  halfSize = 50.0f;

        PolygonDrawerComponent() {}
        PolygonDrawerComponent(const PolygonDrawerComponent& other):
            Component(other), drawColor(other.drawColor), halfSize(other.halfSize) {}

        SERIALIZABLE(PolygonDrawerComponent);
        CLONEABLE_REF(PolygonDrawerComponent);

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
    Ref<Bitmap> RenderSceneFrame(const Ref<CameraActor>& camera)
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        camera->SetupAndDraw();
        o2Render.End();

        return captured;
    }

    const UInt8* GetPixel(const Ref<Bitmap>& bitmap, int x, int y)
    {
        Vec2I size = bitmap->GetSize();
        return bitmap->GetData() + (y*size.x + x)*4;
    }

    // Mean green channel over the top and bottom halves of the bitmap
    void GetHalvesBrightness(const Ref<Bitmap>& bitmap, float& topMean, float& bottomMean)
    {
        Vec2I size = bitmap->GetSize();
        const UInt8* data = bitmap->GetData();

        double topSum = 0.0, bottomSum = 0.0;
        int halfPixels = (size.y/2)*size.x;

        for (int y = 0; y < size.y/2; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                topSum += data[(y*size.x + x)*4 + 1];
                bottomSum += data[((y + size.y/2)*size.x + x)*4 + 1];
            }
        }

        topMean = (float)(topSum/halfPixels);
        bottomMean = (float)(bottomSum/halfPixels);
    }

    // Builds a scene with a white box in the UPPER half of the view and a directional light
    Ref<CameraActor> BuildUpperHalfBoxScene()
    {
        auto camera = mmake<CameraActor>();
        camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
        camera->transform->SetPosition(Vec3F(0, 0, 500));
        camera->fillColor = Color4::Black();

        auto box = mmake<Actor>(ActorCreateMode::InScene);
        auto boxComponent = box->AddComponent<MeshPrimitiveComponent>();
        boxComponent->SetPrimitiveType(PrimitiveType3D::Box);
        boxComponent->SetSize(Vec3F(180, 180, 180));
        boxComponent->SetColor(Color4::White());
        box->transform->SetPosition(Vec3F(0, 160, 0));

        auto lightActor = mmake<Actor>(ActorCreateMode::InScene);
        auto light = lightActor->AddComponent<LightComponent>();
        light->SetLightType(LightComponent::Type::Directional);
        light->SetColor(Color4::White());
        light->SetIntensity(1.0f);

        return camera;
    }
}

TEST(RenderPipelineDraw, Sprite2DDrawsOver3DMeshAtCenter)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
    camera->transform->SetPosition(Vec3F(0, 0, 500));
    camera->fillColor = Color4::Black();

    // Green box occupies screen center; red 2D polygon is smaller and drawn by the 2D pass over it
    auto box = mmake<Actor>(ActorCreateMode::InScene);
    auto boxComponent = box->AddComponent<MeshPrimitiveComponent>();
    boxComponent->SetPrimitiveType(PrimitiveType3D::Box);
    boxComponent->SetSize(Vec3F(250, 250, 250));
    boxComponent->SetColor(Color4::Green());
    boxComponent->SetShaded(false);

    auto polygon = mmake<Actor>(ActorCreateMode::InScene);
    auto polygonComponent = polygon->AddComponent<PolygonDrawerComponent>();
    polygonComponent->drawColor = Color4::Red();
    polygonComponent->halfSize = 40.0f;

    TickFrame();

    auto captured = RenderSceneFrame(camera);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);

    // 2D polygon wins at center: red content, so green channel is low
    const UInt8* center = GetPixel(captured, size.x/2, size.y/2);
    EXPECT_LT((int)center[1], 60);

    // Box is visible next to the polygon: green channel is high
    const UInt8* nearCenter = GetPixel(captured, size.x/2 + size.x/6, size.y/2);
    EXPECT_GT((int)nearCenter[1], 200);
}

TEST(RenderPipelineDraw, DeferredDirectionalLightLitAndShadedSides)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
    camera->transform->SetPosition(Vec3F(0, 0, 500));
    camera->fillColor = Color4::Black();
    camera->SetRenderPipeline(mmake<DeferredPipeline>());

    // White box rotated 45 degrees around Y: camera sees two faces with normals (+-0.707, 0, 0.707)
    auto box = mmake<Actor>(ActorCreateMode::InScene);
    auto boxComponent = box->AddComponent<MeshPrimitiveComponent>();
    boxComponent->SetPrimitiveType(PrimitiveType3D::Box);
    boxComponent->SetSize(Vec3F(300, 300, 300));
    boxComponent->SetColor(Color4::White());
    box->transform->eulerAngles = Vec3F(0.0f, Math::Deg2rad(45.0f), 0.0f);

    // Greenish directional light shining from (+x, +z) direction: to-light vector (0.707, 0, 0.707)
    auto lightActor = mmake<Actor>(ActorCreateMode::InScene);
    auto light = lightActor->AddComponent<LightComponent>();
    light->SetLightType(LightComponent::Type::Directional);
    light->SetColor(Color4(0.2f, 1.0f, 0.2f, 1.0f));
    light->SetIntensity(1.0f);
    lightActor->transform->eulerAngles = Vec3F(0.0f, Math::Deg2rad(45.0f), 0.0f);

    TickFrame();

    // Light direction must be (-0.707, 0, -0.707), so direction to light is (0.707, 0, 0.707)
    Vec3F lightDirection = light->GetWorldDirection();
    EXPECT_NEAR(lightDirection.x, -0.7071f, 0.01f);
    EXPECT_NEAR(lightDirection.y, 0.0f, 0.01f);
    EXPECT_NEAR(lightDirection.z, -0.7071f, 0.01f);

    auto captured = RenderSceneFrame(camera);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);

    // Right face points to the light: bright and tinted green (green channel much higher than red)
    const UInt8* litPixel = GetPixel(captured, size.x/2 + size.y/8, size.y/2);
    EXPECT_GT((int)litPixel[1], 200);
    EXPECT_GT((int)litPixel[1] - (int)litPixel[0], 60);

    // Left face points away: only ambient, noticeably darker but not black
    const UInt8* shadedPixel = GetPixel(captured, size.x/2 - size.y/8, size.y/2);
    EXPECT_LT((int)shadedPixel[1], 150);
    EXPECT_GT((int)shadedPixel[1], 40);

    // Background stays with camera fill color
    const UInt8* backgroundPixel = GetPixel(captured, size.x/16, size.y/16);
    EXPECT_LT((int)backgroundPixel[1], 30);
}

TEST(RenderPipelineDraw, DeferredGBufferMRTContainsPlausibleData)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
    camera->transform->SetPosition(Vec3F(0, 0, 500));
    camera->fillColor = Color4::Black();

    auto pipeline = mmake<DeferredPipeline>();
    camera->SetRenderPipeline(pipeline);

    // Red plane facing the camera (+Z normal) covers the view center; z = 30 to check the positions target
    auto plane = mmake<Actor>(ActorCreateMode::InScene);
    auto planeComponent = plane->AddComponent<MeshPrimitiveComponent>();
    planeComponent->SetPrimitiveType(PrimitiveType3D::Plane);
    planeComponent->SetSize(Vec3F(300, 300, 0));
    planeComponent->SetColor(Color4::Red());
    plane->transform->SetPosition(Vec3F(0, 0, 30));

    TickFrame();

    auto captured = RenderSceneFrame(camera);
    ASSERT_TRUE(captured);

    auto gBufferPass = pipeline->GetPass<GBufferPass>();
    ASSERT_NE(gBufferPass, nullptr);
    ASSERT_TRUE(gBufferPass->GetAlbedoTarget());
    ASSERT_TRUE(gBufferPass->GetNormalsTarget());
    ASSERT_TRUE(gBufferPass->GetPositionsTarget());

    // Float targets read back as 8-bit with clamping to [0, 1]
    auto normals = gBufferPass->GetNormalsTarget()->GetData();
    ASSERT_TRUE(normals);

    Vec2I size = normals->GetSize();
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);

    // Raw plane normal +Z: z channel (blue, or red in BGRA) saturated, others zero
    const UInt8* normalPixel = GetPixel(normals, size.x/2, size.y/2);
    EXPECT_LT((int)normalPixel[1], 16);
    EXPECT_GT(Math::Max((int)normalPixel[0], (int)normalPixel[2]), 240);
    EXPECT_LT(Math::Min((int)normalPixel[0], (int)normalPixel[2]), 16);
    EXPECT_GT((int)normalPixel[3], 240);

    // Background pixels are cleared with zero alpha
    const UInt8* normalBackground = GetPixel(normals, size.x/16, size.y/16);
    EXPECT_LT((int)normalBackground[3], 16);

    // Raw world position (0, 0, 30) at center: z channel clamps to saturation in the 8-bit readback
    auto positions = gBufferPass->GetPositionsTarget()->GetData();
    ASSERT_TRUE(positions);
    const UInt8* positionPixel = GetPixel(positions, size.x/2, size.y/2);
    EXPECT_GT(Math::Max((int)positionPixel[0], (int)positionPixel[2]), 240);
    EXPECT_GT((int)positionPixel[3], 240);

    const UInt8* positionBackground = GetPixel(positions, size.x/16, size.y/16);
    EXPECT_LT((int)positionBackground[3], 16);
}

TEST(RenderPipelineDraw, DeferredPointLightGradientIsSmooth)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
    camera->transform->SetPosition(Vec3F(0, 0, 500));
    camera->fillColor = Color4::Black();
    camera->SetRenderPipeline(mmake<DeferredPipeline>());

    // White plane facing the camera, lit by a point light in front of its center
    auto plane = mmake<Actor>(ActorCreateMode::InScene);
    auto planeComponent = plane->AddComponent<MeshPrimitiveComponent>();
    planeComponent->SetPrimitiveType(PrimitiveType3D::Plane);
    planeComponent->SetSize(Vec3F(3000, 3000, 0));
    planeComponent->SetColor(Color4::White());

    auto lightActor = mmake<Actor>(ActorCreateMode::InScene);
    auto light = lightActor->AddComponent<LightComponent>();
    light->SetLightType(LightComponent::Type::Point);
    light->SetColor(Color4::White());
    light->SetIntensity(1.0f);
    light->SetRange(700.0f);
    lightActor->transform->SetPosition(Vec3F(0, 0, 150));

    TickFrame();

    auto captured = RenderSceneFrame(camera);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);

    // Brightness must fade smoothly from the center: no visible quantization steps
    int y = size.y/2;
    int from = size.x/2;
    int to = size.x/2 + size.x/3;

    int maxAdjacentDelta = 0;
    int previous = GetPixel(captured, from, y)[1];
    for (int x = from + 1; x <= to; x++)
    {
        int value = GetPixel(captured, x, y)[1];
        maxAdjacentDelta = Math::Max(maxAdjacentDelta, Math::Abs(value - previous));
        previous = value;
    }

    int totalDelta = (int)GetPixel(captured, from, y)[1] - (int)GetPixel(captured, to, y)[1];

    EXPECT_GE(totalDelta, 30);
    EXPECT_LE(maxAdjacentDelta, 6);
}

TEST(RenderPipelineDraw, DeferredDirectionalLightCastsShadow)
{
    SceneCleanGuard guard;

    // Top-down camera looking at the ground plane
    auto camera = mmake<CameraActor>();
    camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
    camera->transform->SetPosition(Vec3F(0, 400, 0));
    camera->transform->SetEulerAngles(Vec3F(-Math::Deg2rad(90.0f), 0, 0));
    camera->fillColor = Color4::Black();
    camera->SetRenderPipeline(mmake<DeferredPipeline>());

    auto ground = mmake<Actor>(ActorCreateMode::InScene);
    auto groundComponent = ground->AddComponent<MeshPrimitiveComponent>();
    groundComponent->SetPrimitiveType(PrimitiveType3D::Plane);
    groundComponent->SetSize(Vec3F(2000, 2000, 0));
    groundComponent->SetColor(Color4::White());
    ground->transform->SetEulerAngles(Vec3F(-Math::Deg2rad(90.0f), 0, 0));

    // Box floats above the ground, aside from the view center
    auto box = mmake<Actor>(ActorCreateMode::InScene);
    auto boxComponent = box->AddComponent<MeshPrimitiveComponent>();
    boxComponent->SetPrimitiveType(PrimitiveType3D::Box);
    boxComponent->SetSize(Vec3F(100, 100, 100));
    boxComponent->SetColor(Color4::White());
    box->transform->SetPosition(Vec3F(150, 150, 0));

    // Directional light tilted 45 degrees: box shadow is offset by 150 units along Z on the ground
    auto lightActor = mmake<Actor>(ActorCreateMode::InScene);
    auto light = lightActor->AddComponent<LightComponent>();
    light->SetLightType(LightComponent::Type::Directional);
    light->SetColor(Color4::White());
    light->SetIntensity(1.0f);
    lightActor->transform->SetEulerAngles(Vec3F(-Math::Deg2rad(45.0f), 0, 0));

    TickFrame();

    Vec3F lightDirection = light->GetWorldDirection();
    EXPECT_NEAR(lightDirection.y, -0.7071f, 0.01f);
    EXPECT_NEAR(Math::Abs(lightDirection.z), 0.7071f, 0.01f);

    auto captured = RenderSceneFrame(camera);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);

    // Visible ground height at the plane: 2*400*tan(30) = 462 world units
    float pixelsPerUnit = (float)size.y/462.0f;
    int boxScreenX = size.x/2 + (int)(150.0f*pixelsPerUnit);
    int shadowOffset = (int)(150.0f*pixelsPerUnit);

    // The shadow is displaced along world Z; screen orientation of Z is covered by checking both sides
    int candidateUp = GetPixel(captured, boxScreenX, size.y/2 - shadowOffset)[1];
    int candidateDown = GetPixel(captured, boxScreenX, size.y/2 + shadowOffset)[1];

    // Lit ground reference away from the box and its shadow
    int reference = GetPixel(captured, size.x/2 - (int)(150.0f*pixelsPerUnit), size.y/2)[1];

    EXPECT_GT(reference, 180);

    int shadowValue = Math::Min(candidateUp, candidateDown);
    int litValue = Math::Max(candidateUp, candidateDown);

    // Shadowed ground is noticeably darker, but not black (ambient); the opposite side stays lit
    EXPECT_LT((float)shadowValue, (float)reference*0.6f);
    EXPECT_GT(shadowValue, 30);
    EXPECT_GT((float)litValue, (float)reference*0.8f);
}

// Orientation-strict: the same upper-half-box scene must land in the TOP half of the captured
// bitmap for both forward and deferred pipelines
TEST(RenderPipelineDraw, ForwardAndDeferredOrientationMatchInCapture)
{
    SceneCleanGuard guard;
    auto camera = BuildUpperHalfBoxScene();
    TickFrame();

    auto forwardFrame = RenderSceneFrame(camera);
    ASSERT_TRUE(forwardFrame);

    camera->SetRenderPipeline(mmake<DeferredPipeline>());
    auto deferredFrame = RenderSceneFrame(camera);
    ASSERT_TRUE(deferredFrame);

    float forwardTop, forwardBottom, deferredTop, deferredBottom;
    GetHalvesBrightness(forwardFrame, forwardTop, forwardBottom);
    GetHalvesBrightness(deferredFrame, deferredTop, deferredBottom);

    EXPECT_GT(forwardTop, forwardBottom + 10.0f) << "forward: box must be in the top half of the capture"
        << " ft=" << forwardTop << " fb=" << forwardBottom << " dt=" << deferredTop << " db=" << deferredBottom;
    EXPECT_GT(deferredTop, deferredBottom + 10.0f) << "deferred: box must be in the top half of the capture";
}

// Orientation-strict: rendering through a nested render target (like the editor Game window does)
// must produce the same orientation for forward and deferred pipelines
TEST(RenderPipelineDraw, ForwardAndDeferredOrientationMatchInNestedRT)
{
    SceneCleanGuard guard;
    auto camera = BuildUpperHalfBoxScene();
    TickFrame();

    auto renderToTarget = [&]() -> Ref<Bitmap>
    {
        TextureRef target(Vec2I(256, 256), TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);

        o2Render.Begin();
        o2Render.BindRenderTexture(target);
        camera->SetupAndDraw();
        o2Render.UnbindRenderTexture();
        o2Render.End();

        return target->GetData();
    };

    auto forwardFrame = renderToTarget();
    ASSERT_TRUE(forwardFrame);

    camera->SetRenderPipeline(mmake<DeferredPipeline>());
    auto deferredFrame = renderToTarget();
    ASSERT_TRUE(deferredFrame);

    float forwardTop, forwardBottom, deferredTop, deferredBottom;
    GetHalvesBrightness(forwardFrame, forwardTop, forwardBottom);
    GetHalvesBrightness(deferredFrame, deferredTop, deferredBottom);

    // Both pipelines must agree with the forward render target orientation convention
    bool forwardBoxOnTop = forwardTop > forwardBottom + 10.0f;
    bool deferredBoxOnTop = deferredTop > deferredBottom + 10.0f;

    EXPECT_TRUE(forwardBoxOnTop || forwardBottom > forwardTop + 10.0f) << "forward: box must be visible";
    EXPECT_TRUE(deferredBoxOnTop || deferredBottom > deferredTop + 10.0f) << "deferred: box must be visible";
    EXPECT_EQ(forwardBoxOnTop, deferredBoxOnTop) << "deferred orientation must match forward in a nested RT"
        << " ft=" << forwardTop << " fb=" << forwardBottom << " dt=" << deferredTop << " db=" << deferredBottom;
}
// --- META ---

CLASS_BASES_META(o2::PolygonDrawerComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::PolygonDrawerComponent)
{
    FIELD().PUBLIC().DEFAULT_VALUE(Color4::White()).NAME(drawColor);
    FIELD().PUBLIC().DEFAULT_VALUE(50.0f).NAME(halfSize);
}
END_META;
CLASS_METHODS_META(o2::PolygonDrawerComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PolygonDrawerComponent&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
}
END_META;

DECLARE_CLASS(o2::PolygonDrawerComponent, o2__PolygonDrawerComponent);
// --- END META ---

// The code-built lighting demo renders through the deferred pipeline: lit content at center,
// clear color only in the corner
TEST(RenderPipelineDraw, LightingDemoSceneDrawsLit)
{
    SceneCleanGuard guard;
    auto demo = BuildLightingDemoScene();
    TickFrame();

    Ref<Bitmap> captured;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

    o2Render.Begin();
    demo.base.camera->SetupAndDraw();
    o2Render.End();

    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    const UInt8* data = captured->GetData();

    int centerOffset = ((size.y/2)*size.x + size.x/2)*4;
    int centerBrightness = (int)data[centerOffset] + data[centerOffset + 1] + data[centerOffset + 2];
    EXPECT_GT(centerBrightness, 60) << "center must show the lit red box";

    int cornerOffset = (5*size.x + 5)*4;
    int cornerBrightness = (int)data[cornerOffset] + data[cornerOffset + 1] + data[cornerOffset + 2];
    EXPECT_LT(cornerBrightness, 60) << "corner must keep the dark clear color";
}
