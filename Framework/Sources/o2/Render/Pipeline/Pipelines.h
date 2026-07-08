#pragma once

#include "o2/Render/Pipeline/RenderPipeline.h"

namespace o2
{
    // ----------------------------------------------------------------------------
    // Default forward pipeline: 3D content with depth test, then 2D layers content
    // ----------------------------------------------------------------------------
    class ForwardPipeline: public RenderPipeline
    {
    public:
        // Default constructor, adds Scene3DForwardPass and Scene2DPass
        ForwardPipeline();

        // Copy-constructor
        ForwardPipeline(const ForwardPipeline& other);

        SERIALIZABLE(ForwardPipeline);
        CLONEABLE_REF(ForwardPipeline);
    };

    // -------------------------------------------------------------------------------
    // Deferred lighting pipeline: G-buffer MRT pass, deferred lighting composite, then
    // 2D layers content. Falls back to the forward pipeline when MRT isn't supported
    // -------------------------------------------------------------------------------
    class DeferredPipeline: public RenderPipeline
    {
    public:
        // Default constructor, adds GBufferPass, DeferredLightingPass and Scene2DPass
        DeferredPipeline();

        // Copy-constructor
        DeferredPipeline(const DeferredPipeline& other);

        // Executes deferred passes, or the forward fallback when MRT isn't supported
        void Execute(RenderPassContext& context) override;

        // Executes with explicit MRT support flag; used by Execute and tests
        void ExecuteWithMRTSupport(RenderPassContext& context, bool mrtSupported);

        // Returns true when the forward fallback was used at least once
        bool IsFallbackUsed() const;

        SERIALIZABLE(DeferredPipeline);
        CLONEABLE_REF(DeferredPipeline);

    protected:
        bool mFallbackUsed = false;   // True when forward fallback was used
        bool mFallbackLogged = false; // One-time fallback warning flag
    };
}
// --- META ---

CLASS_BASES_META(o2::ForwardPipeline)
{
    BASE_CLASS(o2::RenderPipeline);
}
END_META;
CLASS_FIELDS_META(o2::ForwardPipeline)
{
}
END_META;
CLASS_METHODS_META(o2::ForwardPipeline)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const ForwardPipeline&);
}
END_META;

CLASS_BASES_META(o2::DeferredPipeline)
{
    BASE_CLASS(o2::RenderPipeline);
}
END_META;
CLASS_FIELDS_META(o2::DeferredPipeline)
{
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFallbackUsed);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFallbackLogged);
}
END_META;
CLASS_METHODS_META(o2::DeferredPipeline)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const DeferredPipeline&);
    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
    FUNCTION().PUBLIC().SIGNATURE(void, ExecuteWithMRTSupport, RenderPassContext&, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFallbackUsed);
}
END_META;
// --- END META ---
