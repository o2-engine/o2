#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Material.h"
#include "o2/Render/Pipeline/DeferredPasses.h"
#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Render/Render.h"
#include "o2/Render/Shader.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

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

    // Standard deviation of the green channel over a horizontal strip at screen center
    float CenterStripDeviation(const Ref<Bitmap>& bitmap)
    {
        Vec2I size = bitmap->GetSize();
        int y = size.y/2;
        int fromX = size.x/2 - size.x/8;
        int toX = size.x/2 + size.x/8;

        double sum = 0.0, squaresSum = 0.0;
        int count = 0;
        for (int x = fromX; x < toX; x++)
        {
            float value = (float)GetPixel(bitmap, x, y)[1];
            sum += value;
            squaresSum += value*value;
            count++;
        }

        double mean = sum/count;
        return (float)Math::Sqrt(Math::Max(0.0, squaresSum/count - mean*mean));
    }

    // Camera looking down -Z at a white box filling the screen center
    struct BoxScene
    {
        Ref<CameraActor> camera;
        Ref<MeshPrimitiveComponent> boxComponent;
    };

    BoxScene BuildBoxScene(bool withLight)
    {
        BoxScene scene;

        scene.camera = mmake<CameraActor>();
        scene.camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
        scene.camera->transform->SetPosition(Vec3F(0, 0, 500));
        scene.camera->fillColor = Color4::Black();

        auto box = mmake<Actor>(ActorCreateMode::InScene);
        scene.boxComponent = box->AddComponent<MeshPrimitiveComponent>();
        scene.boxComponent->SetPrimitiveType(PrimitiveType3D::Box);
        scene.boxComponent->SetSize(Vec3F(300, 300, 300));
        scene.boxComponent->SetColor(Color4::White());
        scene.boxComponent->SetShaded(false);

        if (withLight)
        {
            auto lightActor = mmake<Actor>(ActorCreateMode::InScene);
            auto light = lightActor->AddComponent<LightComponent>();
            light->SetLightType(LightComponent::Type::Directional);
            light->SetColor(Color4::White());
            light->SetIntensity(1.0f);
            lightActor->transform->SetEulerAngles(Vec3F(0.0f, Math::Deg2rad(35.0f), 0.0f));
        }

        return scene;
    }

    // Solid magenta forward material compiled from inline Metal sources
    Ref<Material> CreateMagentaMaterial()
    {
        const char* vertexSource = R"(
vertex O2RasterizerData vertexShader(uint vertexID [[vertex_id]],
                                     constant O2VertexIn* vertices [[buffer(0)]],
                                     constant O2Uniforms& uniforms [[buffer(1)]])
{
    O2VertexIn inputVertex = vertices[vertexID];

    O2RasterizerData output;
    output.position = uniforms.mvpMatrix * float4(inputVertex.x, inputVertex.y, inputVertex.z, 1.0);
    output.color = o2_unpackColor(inputVertex.color);
    output.texCoords = inputVertex.texCoord0;
    output.normal = inputVertex.normal;
    return output;
}
)";

        const char* fragmentSource = R"(
fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]],
                               sampler textureSampler [[sampler(0)]])
{
    return float4(1.0, 0.0, 1.0, 1.0);
}
)";

        auto vertexShader = mmake<Shader>();
        auto fragmentShader = mmake<Shader>();
        if (!vertexShader->Compile(vertexSource, Shader::Type::Vertex) ||
            !fragmentShader->Compile(fragmentSource, Shader::Type::Fragment))
        {
            return nullptr;
        }

        auto material = mmake<Material>();
        material->SetVertexShader(vertexShader);
        material->SetFragmentShader(fragmentShader);
        material->SetBlendMode(BlendMode::Normal);
        material->Build();

        return material->IsReady() ? material : nullptr;
    }

    // Vertical stripes of left/right tilted normals: strong repeating bump pattern
    TextureRef GenerateStripesNormalMap(int size, int stripeWidth)
    {
        Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(size, size));
        UInt8* data = bitmap.GetData();

        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float tilt = (x/stripeWidth) % 2 == 0 ? 0.7f : -0.7f;
                Vec3F normal = Vec3F(tilt, 0.0f, 0.7f).Normalized();

                UInt8* pixel = data + (y*size + x)*4;
                pixel[0] = (UInt8)((normal.x*0.5f + 0.5f)*255.0f);
                pixel[1] = (UInt8)((normal.y*0.5f + 0.5f)*255.0f);
                pixel[2] = (UInt8)((normal.z*0.5f + 0.5f)*255.0f);
                pixel[3] = 255;
            }
        }

        return TextureRef(bitmap);
    }

    Ref<Material> CreateBumpTestMaterial()
    {
        auto material = GBufferPass::CreateSceneMaterial("GBufferBump");
        if (!material)
            return nullptr;

        TextureSampler normalMapSampler;
        normalMapSampler.samplerUniformName = "u_normalMap";
        normalMapSampler.texCoordsAttrName = "a_texCoords";
        material->AddTextureSampler(normalMapSampler);

        material->Build();
        if (!material->IsReady())
            return nullptr;

        material->SetSamplerTextureOverride("u_normalMap", GenerateStripesNormalMap(256, 8));
        return material;
    }
}

// Custom component material must be applied by the forward pass instead of the default one
TEST(MeshMaterialDraw, MaterialAppliesInForwardPass)
{
    SceneCleanGuard guard;
    auto scene = BuildBoxScene(false);
    TickFrame();

    auto material = CreateMagentaMaterial();
    if (!material)
        GTEST_SKIP() << "inline Metal test material is unavailable on this platform";

    auto baseline = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(baseline);

    Vec2I size = baseline->GetSize();
    const UInt8* baselineCenter = GetPixel(baseline, size.x/2, size.y/2);
    EXPECT_GT((int)baselineCenter[1], 200) << "default material draws the white box";

    scene.boxComponent->SetMaterial(material);

    auto withMaterial = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(withMaterial);

    const UInt8* center = GetPixel(withMaterial, size.x/2, size.y/2);
    EXPECT_GT((int)center[0], 200) << "custom material must draw magenta";
    EXPECT_LT((int)center[1], 60) << "custom material must replace the default one";
    EXPECT_GT((int)center[2], 200);
}

// Normal mapped G-buffer material must be applied inside the deferred G-buffer pass
// instead of the pass override material: lighting shows the bump pattern
TEST(MeshMaterialDraw, BumpMaterialAppliesInDeferredGBufferPass)
{
    if (!o2Render.IsMRTSupported())
        GTEST_SKIP() << "deferred pipeline requires MRT support";

    SceneCleanGuard guard;
    auto scene = BuildBoxScene(true);
    scene.camera->SetRenderPipeline(mmake<DeferredPipeline>());
    TickFrame();

    auto baseline = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(baseline);

    float baselineDeviation = CenterStripDeviation(baseline);

    auto material = CreateBumpTestMaterial();
    ASSERT_TRUE(material) << "bump material must build from builtin shaders";

    scene.boxComponent->SetMaterial(material);

    auto withBump = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(withBump);

    float bumpDeviation = CenterStripDeviation(withBump);

    // Flat face is lit uniformly; bump normals stripe the lighting across the face
    EXPECT_GT(bumpDeviation, baselineDeviation*3.0f + 5.0f)
        << "bump lighting variation expected, baseline dev " << baselineDeviation
        << ", bump dev " << bumpDeviation;

    // The face is still lit (the material replaced only the G-buffer shading, not visibility)
    const UInt8* center = GetPixel(withBump, baseline->GetSize().x/2, baseline->GetSize().y/2);
    EXPECT_GT((int)center[1], 30);
}
