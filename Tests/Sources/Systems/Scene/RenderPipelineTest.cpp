#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Pipeline/DeferredPasses.h"
#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Render/Pipeline/ScenePasses.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Components/Mesh3DComponent.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(RenderPipeline, ForwardPipelineDefaultComposition)
{
    auto pipeline = mmake<ForwardPipeline>();
    auto& passes = pipeline->GetPasses();

    ASSERT_EQ(passes.Count(), 3);
    EXPECT_NE(DynamicCast<Scene3DForwardPass>(passes[0]), nullptr);
    EXPECT_NE(DynamicCast<Scene3DTransparentPass>(passes[1]), nullptr);
    EXPECT_NE(DynamicCast<Scene2DPass>(passes[2]), nullptr);
}

TEST(RenderPipeline, DeferredPipelineComposition)
{
    auto pipeline = mmake<DeferredPipeline>();
    auto& passes = pipeline->GetPasses();

    ASSERT_EQ(passes.Count(), 5);
    EXPECT_NE(DynamicCast<ShadowMapPass>(passes[0]), nullptr);
    EXPECT_NE(DynamicCast<GBufferPass>(passes[1]), nullptr);
    EXPECT_NE(DynamicCast<DeferredLightingPass>(passes[2]), nullptr);
    EXPECT_NE(DynamicCast<Scene3DTransparentPass>(passes[3]), nullptr);
    EXPECT_NE(DynamicCast<Scene2DPass>(passes[4]), nullptr);
}

TEST(RenderPipeline, CameraUsesDefaultForwardPipelineWhenNotSet)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    EXPECT_EQ(camera->GetRenderPipeline(), nullptr);

    auto& defaultPipeline = CameraActor::GetDefaultRenderPipeline();
    ASSERT_NE(defaultPipeline, nullptr);
    EXPECT_NE(DynamicCast<ForwardPipeline>(defaultPipeline), nullptr);
}

TEST(RenderPipeline, AddRemoveInsertAndGetPass)
{
    auto pipeline = mmake<RenderPipeline>();
    EXPECT_TRUE(pipeline->GetPasses().IsEmpty());

    auto scene2D = mmake<Scene2DPass>();
    auto scene3D = mmake<Scene3DForwardPass>();

    pipeline->AddPass(scene2D);
    pipeline->InsertPass(scene3D, 0);

    ASSERT_EQ(pipeline->GetPasses().Count(), 2);
    EXPECT_EQ(pipeline->GetPasses()[0], scene3D);
    EXPECT_EQ(pipeline->GetPasses()[1], scene2D);

    EXPECT_EQ(pipeline->GetPass<Scene2DPass>(), scene2D);
    EXPECT_EQ(pipeline->GetPass<Scene3DForwardPass>(), scene3D);
    EXPECT_EQ(pipeline->GetPass<GBufferPass>(), nullptr);

    pipeline->RemovePass(scene3D);
    ASSERT_EQ(pipeline->GetPasses().Count(), 1);
    EXPECT_EQ(pipeline->GetPasses()[0], scene2D);
}

TEST(RenderPipeline, ExecuteRunsOnlyEnabledPasses)
{
    class CountingPass: public RenderPass
    {
    public:
        int executions = 0;

        void Execute(RenderPassContext& context) override { executions++; }
    };

    auto pipeline = mmake<RenderPipeline>();
    auto first = mmake<CountingPass>();
    auto second = mmake<CountingPass>();
    pipeline->AddPass(first);
    pipeline->AddPass(second);

    second->SetEnabled(false);
    EXPECT_TRUE(first->IsEnabled());
    EXPECT_FALSE(second->IsEnabled());

    RenderPassContext context;
    pipeline->Execute(context);

    EXPECT_EQ(first->executions, 1);
    EXPECT_EQ(second->executions, 0);
    EXPECT_EQ(context.pipeline, pipeline.Get());
}

TEST(RenderPipeline, ComponentsCategoryClassification)
{
    SceneCleanGuard guard;

    auto sprite2D = mmake<TestComponent>();
    auto meshPrimitive = mmake<MeshPrimitiveComponent>();
    auto mesh3D = mmake<Mesh3DComponent>();
    auto light = mmake<LightComponent>();

    EXPECT_EQ(sprite2D->GetSceneDrawableCategory(), SceneDrawableCategory::Scene2D);
    EXPECT_EQ(meshPrimitive->GetSceneDrawableCategory(), SceneDrawableCategory::Scene3D);
    EXPECT_EQ(mesh3D->GetSceneDrawableCategory(), SceneDrawableCategory::Scene3D);
    EXPECT_EQ(light->GetSceneDrawableCategory(), SceneDrawableCategory::Scene2D);
}

TEST(RenderPipeline, Scene3DComponentsRegistry)
{
    SceneCleanGuard guard;

    auto meshActor = mmake<Actor>(ActorCreateMode::InScene);
    auto meshComponent = meshActor->AddComponent<MeshPrimitiveComponent>();

    auto spriteActor = mmake<Actor>(ActorCreateMode::InScene);
    auto spriteComponent = spriteActor->AddComponent<TestComponent>();

    TickFrame();

    auto& drawables3D = o2Scene.GetDrawable3DComponents();
    EXPECT_GE(drawables3D.IndexOf([&](auto& x) { return x.Lock().Get() == meshComponent.Get(); }), 0);
    EXPECT_LT(drawables3D.IndexOf([&](auto& x) { return x.Lock().Get() == spriteComponent.Get(); }), 0);

    meshActor->Destroy();
    TickFrame();

    EXPECT_TRUE(o2Scene.GetDrawable3DComponents().IsEmpty());
}

TEST(RenderPipeline, LightsRegistry)
{
    SceneCleanGuard guard;

    auto lightActor = mmake<Actor>(ActorCreateMode::InScene);
    auto light = lightActor->AddComponent<LightComponent>();
    light->SetIntensity(2.0f);

    TickFrame();

    ASSERT_EQ(o2Scene.GetLights().Count(), 1);
    EXPECT_EQ(o2Scene.GetLights()[0].Lock(), light);

    lightActor->Destroy();
    TickFrame();

    EXPECT_TRUE(o2Scene.GetLights().IsEmpty());
}

TEST(RenderPipeline, CameraPipelineSerializationRoundTrip)
{
    SceneCleanGuard guard;

    auto source = mmake<CameraActor>();
    auto pipeline = mmake<DeferredPipeline>();
    pipeline->GetPasses()[3]->SetEnabled(false);
    source->SetRenderPipeline(pipeline);

    DataDocument data;
    source->Serialize(data);

    auto restored = mmake<CameraActor>();
    restored->Deserialize(data);

    auto restoredPipeline = DynamicCast<DeferredPipeline>(restored->GetRenderPipeline());
    ASSERT_NE(restoredPipeline, nullptr);

    auto& passes = restoredPipeline->GetPasses();
    ASSERT_EQ(passes.Count(), 5);
    EXPECT_NE(DynamicCast<ShadowMapPass>(passes[0]), nullptr);
    EXPECT_NE(DynamicCast<GBufferPass>(passes[1]), nullptr);
    EXPECT_NE(DynamicCast<DeferredLightingPass>(passes[2]), nullptr);
    EXPECT_NE(DynamicCast<Scene3DTransparentPass>(passes[3]), nullptr);
    EXPECT_NE(DynamicCast<Scene2DPass>(passes[4]), nullptr);

    EXPECT_TRUE(passes[0]->IsEnabled());
    EXPECT_FALSE(passes[3]->IsEnabled());
}

TEST(RenderPipeline, DeferredFallsBackToForwardWithoutMRT)
{
    auto pipeline = mmake<DeferredPipeline>();
    EXPECT_FALSE(pipeline->IsFallbackUsed());

    RenderPassContext context;
    pipeline->ExecuteWithMRTSupport(context, true);
    EXPECT_FALSE(pipeline->IsFallbackUsed());

    pipeline->ExecuteWithMRTSupport(context, false);
    EXPECT_TRUE(pipeline->IsFallbackUsed());
    EXPECT_EQ(context.pipeline, pipeline.Get());
}
