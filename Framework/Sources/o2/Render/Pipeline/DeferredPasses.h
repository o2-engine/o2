#pragma once

#include "o2/Render/Material.h"
#include "o2/Utils/Math/AABB.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Pipeline/RenderPass.h"
#include "o2/Render/TextureRef.h"

namespace o2
{
    FORWARD_CLASS_REF(LightComponent);

    // ---------------------------------------------------------------------------------
    // Shadow map pass: renders 3D scene depth from the first enabled directional light
    // into a float color target (light-space depth in R channel)
    // ---------------------------------------------------------------------------------
    class ShadowMapPass: public RenderPass
    {
    public:
        // Renders scene depth from the light's point of view
        void Execute(RenderPassContext& context) override;

        // Sets shadow map resolution
        void SetShadowMapSize(int size);

        // Returns shadow map resolution
        int GetShadowMapSize() const;

        // Returns shadow map texture
        const TextureRef& GetShadowMap() const;

        // Returns light view-projection matrix used for shadow map rendering (16 floats, column major)
        const Vector<float>& GetLightViewProj() const;

        // Returns true when shadow map was rendered at current frame
        bool IsShadowAvailable() const;

        SERIALIZABLE(ShadowMapPass);
        CLONEABLE_REF(ShadowMapPass);

    protected:
        int mShadowMapSize = 2048; // Shadow map resolution @SERIALIZABLE @EDITOR_PROPERTY

        TextureRef    mShadowMap; // Light-space depth target (R16G16B16A16F)
        Ref<Material> mMaterial;  // Depth writing material, overrides scene materials

        Vector<float> mLightViewProj;          // Light view-projection matrix for depth reconstruction
        bool          mShadowAvailable = false; // True when the map was rendered this frame

    protected:
        // Creates or resizes shadow map and builds material; returns true when ready
        bool EnsureResources();

        // Returns first enabled directional light on scene
        Ref<LightComponent> FindDirectionalLight() const;

        // Collects world bounds of visible 3D components; false when there is nothing to render
        bool CollectSceneBounds(const RenderPassContext& context, AABB& bounds) const;
    };

    // -------------------------------------------------------------------------------
    // G-buffer pass: draws 3D scene content into MRT targets - raw albedo (RGBA8) and
    // raw world normals and positions (RGBA16F)
    // -------------------------------------------------------------------------------
    class GBufferPass: public RenderPass
    {
    public:
        // Renders 3D content into the G-buffer targets
        void Execute(RenderPassContext& context) override;

        // Creates a scene material from builtin shaders with the G-buffer MRT attachment formats,
        // for meshes drawn with custom materials inside this pass. Caller adds samplers and builds
        static Ref<Material> CreateSceneMaterial(const String& builtinShadersName);

        // Returns albedo target
        const TextureRef& GetAlbedoTarget() const;

        // Returns world normals target
        const TextureRef& GetNormalsTarget() const;

        // Returns world positions target
        const TextureRef& GetPositionsTarget() const;

        SERIALIZABLE(GBufferPass);
        CLONEABLE_REF(GBufferPass);

    protected:
        TextureRef mAlbedoTarget;    // Raw albedo color target
        TextureRef mNormalsTarget;   // Raw world normals target
        TextureRef mPositionsTarget; // Raw world positions target

        Ref<Material> mMaterial; // G-buffer MRT material, overrides scene materials

    protected:
        // Creates or resizes targets and builds material; returns true when ready
        bool EnsureResources(const Vec2I& size);
    };

    // ------------------------------------------------------------------------------
    // Deferred lighting pass: draws fullscreen quad sampling the G-buffer, computes
    // ambient + N*L lighting from scene lights (directional and point, up to 8) with
    // shadow mapping for the first directional light
    // ------------------------------------------------------------------------------
    class DeferredLightingPass: public RenderPass
    {
    public:
        // Composites lit 3D scene into the current camera target
        void Execute(RenderPassContext& context) override;

        // Sets ambient intensity
        void SetAmbient(float ambient);

        // Returns ambient intensity
        float GetAmbient() const;

        SERIALIZABLE(DeferredLightingPass);
        CLONEABLE_REF(DeferredLightingPass);

    protected:
        static constexpr int maxLights = 8;

        float mAmbient = 0.35f; // Ambient light intensity @SERIALIZABLE @EDITOR_PROPERTY

        Ref<Material> mMaterial;          // Deferred lighting material
        Ref<Mesh>     mQuad;              // Fullscreen quad mesh
        TextureRef    mFallbackShadowMap; // White 1x1 texture bound when shadows are disabled

    protected:
        // Builds material with G-buffer samplers and light params; returns true when ready
        bool EnsureResources();

        // Collects enabled scene lights into material params; returns packed index of the first directional light or -1
        int UpdateLightsParams();

        // Applies shadow map, light matrix and shadow params from the shadow pass
        void UpdateShadowParams(const RenderPassContext& context, int shadowLightIndex);
    };
}
// --- META ---

CLASS_BASES_META(o2::ShadowMapPass)
{
    BASE_CLASS(o2::RenderPass);
}
END_META;
CLASS_FIELDS_META(o2::ShadowMapPass)
{
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(2048).NAME(mShadowMapSize);
    FIELD().PROTECTED().NAME(mShadowMap);
    FIELD().PROTECTED().NAME(mMaterial);
    FIELD().PROTECTED().NAME(mLightViewProj);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mShadowAvailable);
}
END_META;
CLASS_METHODS_META(o2::ShadowMapPass)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetShadowMapSize, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetShadowMapSize);
    FUNCTION().PUBLIC().SIGNATURE(const TextureRef&, GetShadowMap);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<float>&, GetLightViewProj);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsShadowAvailable);
    FUNCTION().PROTECTED().SIGNATURE(bool, EnsureResources);
    FUNCTION().PROTECTED().SIGNATURE(Ref<LightComponent>, FindDirectionalLight);
    FUNCTION().PROTECTED().SIGNATURE(bool, CollectSceneBounds, const RenderPassContext&, AABB&);
}
END_META;

CLASS_BASES_META(o2::GBufferPass)
{
    BASE_CLASS(o2::RenderPass);
}
END_META;
CLASS_FIELDS_META(o2::GBufferPass)
{
    FIELD().PROTECTED().NAME(mAlbedoTarget);
    FIELD().PROTECTED().NAME(mNormalsTarget);
    FIELD().PROTECTED().NAME(mPositionsTarget);
    FIELD().PROTECTED().NAME(mMaterial);
}
END_META;
CLASS_METHODS_META(o2::GBufferPass)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<Material>, CreateSceneMaterial, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const TextureRef&, GetAlbedoTarget);
    FUNCTION().PUBLIC().SIGNATURE(const TextureRef&, GetNormalsTarget);
    FUNCTION().PUBLIC().SIGNATURE(const TextureRef&, GetPositionsTarget);
    FUNCTION().PROTECTED().SIGNATURE(bool, EnsureResources, const Vec2I&);
}
END_META;

CLASS_BASES_META(o2::DeferredLightingPass)
{
    BASE_CLASS(o2::RenderPass);
}
END_META;
CLASS_FIELDS_META(o2::DeferredLightingPass)
{
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.35f).NAME(mAmbient);
    FIELD().PROTECTED().NAME(mMaterial);
    FIELD().PROTECTED().NAME(mQuad);
    FIELD().PROTECTED().NAME(mFallbackShadowMap);
}
END_META;
CLASS_METHODS_META(o2::DeferredLightingPass)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAmbient, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAmbient);
    FUNCTION().PROTECTED().SIGNATURE(bool, EnsureResources);
    FUNCTION().PROTECTED().SIGNATURE(int, UpdateLightsParams);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateShadowParams, const RenderPassContext&, int);
}
END_META;
// --- END META ---
