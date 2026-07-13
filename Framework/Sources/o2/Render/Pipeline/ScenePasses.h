#pragma once

#include "o2/Render/Pipeline/RenderPass.h"

namespace o2
{
    class Actor;

    // ---------------------------------------------------------------------------
    // Forward 3D scene pass: draws 3D category components with depth test + write
    // ---------------------------------------------------------------------------
    class Scene3DForwardPass: public RenderPass
    {
    public:
        // Draws scene 3D components with depth test enabled
        void Execute(RenderPassContext& context) override;

        // Draws registered 3D category components visible for the context camera,
        // opaque by default; transparent selects transparent components instead
        static void DrawScene3DContent(const RenderPassContext& context, bool transparent = false);

        // Returns true when actor's layer (default layer when not set) is in the context layers list
        static bool IsActorInContextLayers(const RenderPassContext& context, const Ref<Actor>& actor);

        SERIALIZABLE(Scene3DForwardPass);
        CLONEABLE_REF(Scene3DForwardPass);
    };

    // -------------------------------------------------------------------------------
    // Transparent 3D scene pass: draws transparent 3D components (particles) after
    // opaque content. Depth is read-only in forward; deferred runs it without depth
    // because the composited target has no depth buffer content
    // -------------------------------------------------------------------------------
    class Scene3DTransparentPass: public RenderPass
    {
    public:
        // When false, depth test is not used (deferred pipeline after lighting composite) @SERIALIZABLE
        bool useDepthTest = true;

        // Draws transparent scene 3D components
        void Execute(RenderPassContext& context) override;

        SERIALIZABLE(Scene3DTransparentPass);
        CLONEABLE_REF(Scene3DTransparentPass);
    };

    // ------------------------------------------------------------------------------
    // 2D scene pass: draws layers content in painter's order, skips 3D components
    // ------------------------------------------------------------------------------
    class Scene2DPass: public RenderPass
    {
    public:
        // Draws camera layers drawables with 2D category filter
        void Execute(RenderPassContext& context) override;

        SERIALIZABLE(Scene2DPass);
        CLONEABLE_REF(Scene2DPass);
    };
}
// --- META ---

CLASS_BASES_META(o2::Scene3DForwardPass)
{
    BASE_CLASS(o2::RenderPass);
}
END_META;
CLASS_FIELDS_META(o2::Scene3DForwardPass)
{
}
END_META;
CLASS_METHODS_META(o2::Scene3DForwardPass)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, DrawScene3DContent, const RenderPassContext&, bool);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsActorInContextLayers, const RenderPassContext&, const Ref<Actor>&);
}
END_META;

CLASS_BASES_META(o2::Scene3DTransparentPass)
{
    BASE_CLASS(o2::RenderPass);
}
END_META;
CLASS_FIELDS_META(o2::Scene3DTransparentPass)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(useDepthTest);
}
END_META;
CLASS_METHODS_META(o2::Scene3DTransparentPass)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
}
END_META;

CLASS_BASES_META(o2::Scene2DPass)
{
    BASE_CLASS(o2::RenderPass);
}
END_META;
CLASS_FIELDS_META(o2::Scene2DPass)
{
}
END_META;
CLASS_METHODS_META(o2::Scene2DPass)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
}
END_META;
// --- END META ---
