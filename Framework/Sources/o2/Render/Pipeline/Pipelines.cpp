#include "o2/stdafx.h"
#include "Pipelines.h"

#include "o2/Render/Pipeline/DeferredPasses.h"
#include "o2/Render/Pipeline/ScenePasses.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    ForwardPipeline::ForwardPipeline()
    {
        AddPass(mmake<Scene3DForwardPass>());
        AddPass(mmake<Scene3DTransparentPass>());
        AddPass(mmake<Scene2DPass>());
    }

    ForwardPipeline::ForwardPipeline(const ForwardPipeline& other):
        RenderPipeline(other)
    {}

    DeferredPipeline::DeferredPipeline()
    {
        AddPass(mmake<ShadowMapPass>());
        AddPass(mmake<GBufferPass>());
        AddPass(mmake<DeferredLightingPass>());

        // The lighting composite target has no scene depth, transparent content draws on top
        auto transparentPass = mmake<Scene3DTransparentPass>();
        transparentPass->useDepthTest = false;
        AddPass(transparentPass);

        AddPass(mmake<Scene2DPass>());
    }

    DeferredPipeline::DeferredPipeline(const DeferredPipeline& other):
        RenderPipeline(other)
    {}

    void DeferredPipeline::Execute(RenderPassContext& context)
    {
        ExecuteWithMRTSupport(context, Render::IsSingletonInitialzed() && o2Render.IsMRTSupported());
    }

    void DeferredPipeline::ExecuteWithMRTSupport(RenderPassContext& context, bool mrtSupported)
    {
        if (!mrtSupported)
        {
            if (!mFallbackLogged)
            {
                o2Debug.LogWarning("Deferred pipeline isn't supported (no MRT), falling back to forward pipeline");
                mFallbackLogged = true;
            }

            mFallbackUsed = true;

            static Ref<ForwardPipeline> fallback;
            if (!fallback)
                fallback = mmake<ForwardPipeline>();

            fallback->Execute(context);
            context.pipeline = this;
            return;
        }

        RenderPipeline::Execute(context);
    }

    bool DeferredPipeline::IsFallbackUsed() const
    {
        return mFallbackUsed;
    }
}
// --- META ---

DECLARE_CLASS(o2::ForwardPipeline, o2__ForwardPipeline);

DECLARE_CLASS(o2::DeferredPipeline, o2__DeferredPipeline);
// --- END META ---
