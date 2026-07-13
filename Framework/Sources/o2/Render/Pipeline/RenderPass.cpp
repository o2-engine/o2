#include "o2/stdafx.h"
#include "RenderPass.h"

#include "o2/Scene/SceneLayer.h"

namespace o2
{
    FORWARD_REF_IMPL(SceneLayer);

    String RenderPass::GetName() const
    {
        return GetType().GetName();
    }

    void RenderPass::SetEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    bool RenderPass::IsEnabled() const
    {
        return mEnabled;
    }
}
// --- META ---

DECLARE_CLASS(o2::RenderPass, o2__RenderPass);
// --- END META ---
