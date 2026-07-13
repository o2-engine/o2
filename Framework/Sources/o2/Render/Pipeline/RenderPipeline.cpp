#include "o2/stdafx.h"
#include "RenderPipeline.h"

namespace o2
{
    RenderPipeline::RenderPipeline()
    {}

    RenderPipeline::RenderPipeline(const RenderPipeline& other)
    {
        for (auto& pass : other.mPasses)
            mPasses.Add(pass->CloneAsRef<RenderPass>());
    }

    void RenderPipeline::AddPass(const Ref<RenderPass>& pass)
    {
        mPasses.Add(pass);
    }

    void RenderPipeline::InsertPass(const Ref<RenderPass>& pass, int position)
    {
        mPasses.Insert(pass, position);
    }

    void RenderPipeline::RemovePass(const Ref<RenderPass>& pass)
    {
        mPasses.Remove(pass);
    }

    const Vector<Ref<RenderPass>>& RenderPipeline::GetPasses() const
    {
        return mPasses;
    }

    void RenderPipeline::Execute(RenderPassContext& context)
    {
        context.pipeline = this;

        for (auto& pass : mPasses)
        {
            if (pass && pass->IsEnabled())
                pass->Execute(context);
        }
    }
}
// --- META ---

DECLARE_CLASS(o2::RenderPipeline, o2__RenderPipeline);
// --- END META ---
