#include "o2/stdafx.h"
#include "ScenePasses.h"

#include "o2/Render/Pipeline/RenderPipeline.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneDrawableCategory.h"
#include "o2/Scene/SceneLayer.h"

namespace o2
{
    void Scene3DForwardPass::Execute(RenderPassContext& context)
    {
        if (!Render::IsSingletonInitialzed())
            return;

        o2Render.SetDepthTestEnabled(true);
        DrawScene3DContent(context);
        o2Render.SetDepthTestEnabled(false);
    }

    void Scene3DForwardPass::DrawScene3DContent(const RenderPassContext& context, bool transparent /*= false*/)
    {
        if (!Scene::IsSingletonInitialzed())
            return;

        for (auto& weakComponent : o2Scene.GetDrawable3DComponents())
        {
            auto component = weakComponent.Lock();
            if (!component || !component->IsEnabledInHierarchy())
                continue;

            if (component->Is3DDrawableTransparent() != transparent)
                continue;

            auto actor = component->GetActor();
            if (!actor || !IsActorInContextLayers(context, actor))
                continue;

            component->Draw();
        }
    }

    bool Scene3DForwardPass::IsActorInContextLayers(const RenderPassContext& context, const Ref<Actor>& actor)
    {
        Ref<SceneLayer> layer = actor->GetLayer();
        if (!layer && Scene::IsSingletonInitialzed())
            layer = o2Scene.GetDefaultLayer();

        return layer && context.layers.Contains(layer);
    }

    void Scene3DTransparentPass::Execute(RenderPassContext& context)
    {
        if (!Render::IsSingletonInitialzed())
            return;

        if (useDepthTest)
            o2Render.SetDepthTestEnabled(true, false);

        Scene3DForwardPass::DrawScene3DContent(context, true);

        if (useDepthTest)
            o2Render.SetDepthTestEnabled(false);
    }

    void Scene2DPass::Execute(RenderPassContext& context)
    {
        ScenePassCategoryScope categoryScope(SceneDrawableCategory::Scene2D);

        for (auto& layer : context.layers)
        {
            for (auto& drawable : layer->GetDrawables())
                drawable->Draw();
        }
    }
}
// --- META ---

DECLARE_CLASS(o2::Scene3DForwardPass, o2__Scene3DForwardPass);

DECLARE_CLASS(o2::Scene3DTransparentPass, o2__Scene3DTransparentPass);

DECLARE_CLASS(o2::Scene2DPass, o2__Scene2DPass);
// --- END META ---
