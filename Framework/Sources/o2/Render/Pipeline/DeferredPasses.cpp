#include "o2/stdafx.h"
#include "DeferredPasses.h"

#include "o2/EngineSettings.h"
#include "o2/Render/Pipeline/RenderPipeline.h"
#include "o2/Render/Pipeline/ScenePasses.h"
#include "o2/Render/Render.h"
#include "o2/Render/Shader.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneDrawableCategory.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    FORWARD_REF_IMPL(LightComponent);

    namespace
    {
        // Replicates the platform camera transforms chain (see Render::UpdateCameraTransforms and
        // Metal PlatformSetupCameraTransforms) to reconstruct clip coordinates of a render-target pass
        Vector<float> BuildRenderTargetViewProj(const Mat4& proj, const Mat4& view)
        {
            float model[16] =
            {
                1, 0, 0, 0,
                0, -1, 0, 0, // Render targets are drawn y-flipped
                0, 0, 1, 0,
                0, 0, 0, 1
            };

            static const float metalClipSpaceFix[16] =
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.5f, 0.0f,
                0.0f, 0.0f, 0.5f, 1.0f
            };

            float modelView[16], viewProj[16], fixedViewProj[16];
            Math::mtxMultiply(modelView, model, view.m);
            Math::mtxMultiply(viewProj, proj.m, modelView);
            Math::mtxMultiply(fixedViewProj, metalClipSpaceFix, viewProj);

            Vector<float> result;
            for (int i = 0; i < 16; i++)
                result.Add(fixedViewProj[i]);

            return result;
        }
    }

    void ShadowMapPass::Execute(RenderPassContext& context)
    {
        mShadowAvailable = false;

        if (!Render::IsSingletonInitialzed() || !o2Render.IsMRTSupported())
            return;

        auto light = FindDirectionalLight();
        if (!light)
            return;

        AABB sceneBounds;
        if (!CollectSceneBounds(context, sceneBounds))
            return;

        if (!EnsureResources())
            return;

        Vec3F center = sceneBounds.GetCenter();
        float radius = Math::Max(sceneBounds.GetSize().Length()*0.5f, 1.0f)*1.05f + 1.0f;
        Vec3F lightDirection = light->GetWorldDirection();

        Camera lightCamera = Camera::Orthographic3D(radius*2.0f, radius*2.0f, 1.0f, radius*2.0f + 2.0f);
        lightCamera.position = center - lightDirection*(radius + 1.0f);
        lightCamera.rotation = light->GetWorldRotation();

        o2Render.PushRenderTargets({ mShadowMap });
        o2Render.Clear(Color4::White());
        o2Render.SetCamera(lightCamera);
        o2Render.SetDepthTestEnabled(true);
        o2Render.SetOverrideMaterial(mMaterial);
        ScenePassFilters::SetRawAlbedoMode(true);

        Scene3DForwardPass::DrawScene3DContent(context);

        ScenePassFilters::SetRawAlbedoMode(false);
        o2Render.SetOverrideMaterial(nullptr);
        o2Render.SetDepthTestEnabled(false);

        Vec2F mapSize((float)mShadowMapSize, (float)mShadowMapSize);
        mLightViewProj = BuildRenderTargetViewProj(lightCamera.GetProjectionMatrix(mapSize),
                                                   lightCamera.GetViewMatrix3D());

        o2Render.PopRenderTargets();
        o2Render.SetCamera(context.camera);

        mShadowAvailable = true;
    }

    void ShadowMapPass::SetShadowMapSize(int size)
    {
        if (mShadowMapSize == size)
            return;

        mShadowMapSize = size;
        mShadowMap = TextureRef();
    }

    int ShadowMapPass::GetShadowMapSize() const
    {
        return mShadowMapSize;
    }

    const TextureRef& ShadowMapPass::GetShadowMap() const
    {
        return mShadowMap;
    }

    const Vector<float>& ShadowMapPass::GetLightViewProj() const
    {
        return mLightViewProj;
    }

    bool ShadowMapPass::IsShadowAvailable() const
    {
        return mShadowAvailable;
    }

    bool ShadowMapPass::EnsureResources()
    {
        // Size can change through the field directly (inspector, deserialization), recreate on mismatch
        if (!mShadowMap || ((Vec2I)mShadowMap->GetSize()).x != mShadowMapSize)
            mShadowMap = TextureRef(Vec2I(mShadowMapSize, mShadowMapSize), TextureFormat::R16G16B16A16F,
                                    Texture::Usage::RenderTarget);

        if (!mMaterial)
        {
            mMaterial = Material::CreateFromBuiltinShaders("ShadowDepth");
            if (mMaterial)
            {
                mMaterial->SetColorAttachmentFormats({ TextureFormat::R16G16B16A16F });
                mMaterial->Build();
            }
        }

        return mShadowMap && mMaterial && mMaterial->IsReady();
    }

    Ref<LightComponent> ShadowMapPass::FindDirectionalLight() const
    {
        if (!Scene::IsSingletonInitialzed())
            return nullptr;

        for (auto& weakLight : o2Scene.GetLights())
        {
            auto light = weakLight.Lock();
            if (light && light->IsEnabledInHierarchy() && light->GetLightType() == LightComponent::Type::Directional)
                return light;
        }

        return nullptr;
    }

    bool ShadowMapPass::CollectSceneBounds(const RenderPassContext& context, AABB& bounds) const
    {
        if (!Scene::IsSingletonInitialzed())
            return false;

        bool hasBounds = false;
        for (auto& weakComponent : o2Scene.GetDrawable3DComponents())
        {
            auto component = weakComponent.Lock();
            if (!component || !component->IsEnabledInHierarchy())
                continue;

            auto actor = component->GetActor();
            if (!actor || !Scene3DForwardPass::IsActorInContextLayers(context, actor))
                continue;

            AABB componentBounds;
            if (!component->Get3DDrawableBounds(componentBounds))
                continue;

            bounds = hasBounds ? bounds.Expand(componentBounds) : componentBounds;
            hasBounds = true;
        }

        return hasBounds;
    }

    void GBufferPass::Execute(RenderPassContext& context)
    {
        if (!Render::IsSingletonInitialzed() || !o2Render.IsMRTSupported())
            return;

        TextureRef previousTarget = o2Render.GetRenderTexture();
        Vec2I size = previousTarget ? (Vec2I)previousTarget->GetSize() : o2Render.GetResolution();
        if (size.x < 1 || size.y < 1 || !EnsureResources(size))
            return;

        o2Render.PushRenderTargets({ mAlbedoTarget, mNormalsTarget, mPositionsTarget });

        Color4 clearColor = context.fillColor;
        clearColor.a = 0;
        o2Render.Clear(clearColor);

        o2Render.SetCamera(context.camera);
        o2Render.SetDepthTestEnabled(true);
        o2Render.SetOverrideMaterial(mMaterial);
        ScenePassFilters::SetRawAlbedoMode(true);

        Scene3DForwardPass::DrawScene3DContent(context);

        ScenePassFilters::SetRawAlbedoMode(false);
        o2Render.SetOverrideMaterial(nullptr);
        o2Render.SetDepthTestEnabled(false);

        o2Render.PopRenderTargets();
        o2Render.SetCamera(context.camera);

        // The camera's background clear was consumed by the passes targets: re-arm it
        // for the main target, the lighting composite keeps background pixels transparent
        if (context.fillBackground)
            o2Render.Clear(context.fillColor);
    }

    Ref<Material> GBufferPass::CreateSceneMaterial(const String& builtinShadersName)
    {
        auto material = Material::CreateFromBuiltinShaders(builtinShadersName);
        if (material)
        {
            material->SetColorAttachmentFormats({ TextureFormat::R8G8B8A8, TextureFormat::R16G16B16A16F,
                                                  TextureFormat::R16G16B16A16F });
        }

        return material;
    }

    const TextureRef& GBufferPass::GetAlbedoTarget() const
    {
        return mAlbedoTarget;
    }

    const TextureRef& GBufferPass::GetNormalsTarget() const
    {
        return mNormalsTarget;
    }

    const TextureRef& GBufferPass::GetPositionsTarget() const
    {
        return mPositionsTarget;
    }

    bool GBufferPass::EnsureResources(const Vec2I& size)
    {
        if (!mAlbedoTarget || (Vec2I)mAlbedoTarget->GetSize() != size)
        {
            mAlbedoTarget = TextureRef(size, TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
            mNormalsTarget = TextureRef(size, TextureFormat::R16G16B16A16F, Texture::Usage::RenderTarget);
            mPositionsTarget = TextureRef(size, TextureFormat::R16G16B16A16F, Texture::Usage::RenderTarget);
        }

        if (!mMaterial)
        {
            mMaterial = Material::CreateFromBuiltinShaders("GBuffer");
            if (mMaterial)
            {
                mMaterial->SetColorAttachmentFormats({ TextureFormat::R8G8B8A8, TextureFormat::R16G16B16A16F,
                                                       TextureFormat::R16G16B16A16F });
                mMaterial->Build();
            }
        }

        return mAlbedoTarget && mNormalsTarget && mPositionsTarget && mMaterial && mMaterial->IsReady();
    }

    void DeferredLightingPass::Execute(RenderPassContext& context)
    {
        if (!Render::IsSingletonInitialzed() || !o2Render.IsMRTSupported() || !context.pipeline)
            return;

        auto gBufferPass = context.pipeline->GetPass<GBufferPass>();
        if (!gBufferPass || !gBufferPass->GetAlbedoTarget())
            return;

        if (!EnsureResources())
            return;

        mMaterial->SetSamplerTextureOverride("u_normalsTex", gBufferPass->GetNormalsTarget());
        mMaterial->SetSamplerTextureOverride("u_positionsTex", gBufferPass->GetPositionsTarget());

        int shadowLightIndex = UpdateLightsParams();
        UpdateShadowParams(context, shadowLightIndex);
        mMaterial->InvalidateHash();

        Camera sceneCamera = o2Render.GetCamera();
        o2Render.SetCamera(Camera());

        Vec2F halfSize = (Vec2F)o2Render.GetCurrentResolution()*0.5f;
        ULong white = Color4::White().ABGR();

        mQuad->SetTexture(gBufferPass->GetAlbedoTarget());

        // G-buffer targets are rendered y-flipped (world-up at v = 1), the quad samples them back upright
        Vertex* vertices = mQuad->GetVertices<Vertex>();
        vertices[0] = Vertex(-halfSize.x, halfSize.y, 0.0f, white, 0.0f, 1.0f);
        vertices[1] = Vertex(halfSize.x, halfSize.y, 0.0f, white, 1.0f, 1.0f);
        vertices[2] = Vertex(halfSize.x, -halfSize.y, 0.0f, white, 1.0f, 0.0f);
        vertices[3] = Vertex(-halfSize.x, -halfSize.y, 0.0f, white, 0.0f, 0.0f);

        VertexIndex* indexes = mQuad->GetIndexes();
        indexes[0] = 0; indexes[1] = 1; indexes[2] = 2;
        indexes[3] = 0; indexes[4] = 2; indexes[5] = 3;

        mQuad->vertexCount = 4;
        mQuad->polyCount = 2;

        mQuad->Draw();

        o2Render.SetCamera(sceneCamera);
    }

    void DeferredLightingPass::SetAmbient(float ambient)
    {
        mAmbient = ambient;
    }

    float DeferredLightingPass::GetAmbient() const
    {
        return mAmbient;
    }

    bool DeferredLightingPass::EnsureResources()
    {
        if (!mMaterial)
        {
            mMaterial = Material::CreateFromBuiltinShaders("DeferredLighting");
            if (mMaterial)
            {
                TextureSampler normalsSampler;
                normalsSampler.samplerUniformName = "u_normalsTex";
                normalsSampler.texCoordsAttrName = "a_texCoords2";
                mMaterial->AddTextureSampler(normalsSampler);

                TextureSampler positionsSampler;
                positionsSampler.samplerUniformName = "u_positionsTex";
                positionsSampler.texCoordsAttrName = "a_texCoords3";
                mMaterial->AddTextureSampler(positionsSampler);

                TextureSampler shadowSampler;
                shadowSampler.samplerUniformName = "u_shadowMap";
                shadowSampler.texCoordsAttrName = "a_texCoords2";
                mMaterial->AddTextureSampler(shadowSampler);

                mMaterial->AddParam(mmake<ShaderParamFloat>("u_lightsCount", 0.0f));
                mMaterial->AddParam(mmake<ShaderParamFloat>("u_ambient", mAmbient));
                mMaterial->AddParam(mmake<ShaderParamFloat>("u_shadowsEnabled", 0.0f));
                mMaterial->AddParam(mmake<ShaderParamFloat>("u_shadowLightIndex", -1.0f));
                mMaterial->AddParam(mmake<ShaderParamFloat>("u_shadowMapSize", 1.0f));
                mMaterial->AddParam(mmake<ShaderParamFloatVector>("u_lightVP", Vector<float>()));
                mMaterial->AddParam(mmake<ShaderParamFloatVector>("u_lightColors", Vector<float>()));
                mMaterial->AddParam(mmake<ShaderParamFloatVector>("u_lightVectors", Vector<float>()));

                mMaterial->Build();
            }
        }

        if (!mFallbackShadowMap)
        {
            Bitmap whiteBitmap(PixelFormat::R8G8B8A8, Vec2I(1, 1));
            whiteBitmap.Fill(Color4::White());
            mFallbackShadowMap = TextureRef(whiteBitmap);
        }

        if (!mQuad)
            mQuad = mmake<Mesh>(TextureRef(), 4, 2);

        if (mQuad && mMaterial)
            mQuad->SetMaterial(mMaterial);

        return mMaterial && mMaterial->IsReady() && mQuad;
    }

    int DeferredLightingPass::UpdateLightsParams()
    {
        Vector<float> colors;
        Vector<float> vectors;
        int lightsCount = 0;
        int firstDirectionalIndex = -1;

        if (Scene::IsSingletonInitialzed())
        {
            for (auto& weakLight : o2Scene.GetLights())
            {
                if (lightsCount >= maxLights)
                    break;

                auto light = weakLight.Lock();
                if (!light || !light->IsEnabledInHierarchy())
                    continue;

                const Color4& color = light->GetColor();
                float intensity = light->GetIntensity();
                bool isPoint = light->GetLightType() == LightComponent::Type::Point;

                colors.Add(color.RF()*intensity);
                colors.Add(color.GF()*intensity);
                colors.Add(color.BF()*intensity);
                colors.Add(isPoint ? 1.0f : 0.0f);

                if (isPoint)
                {
                    Vec3F position = light->GetWorldPosition();
                    vectors.Add(position.x);
                    vectors.Add(position.y);
                    vectors.Add(position.z);
                    vectors.Add(light->GetRange());
                }
                else
                {
                    if (firstDirectionalIndex < 0)
                        firstDirectionalIndex = lightsCount;

                    Vec3F directionToLight = light->GetWorldDirection()*-1.0f;
                    vectors.Add(directionToLight.x);
                    vectors.Add(directionToLight.y);
                    vectors.Add(directionToLight.z);
                    vectors.Add(0.0f);
                }

                lightsCount++;
            }
        }

        DynamicCast<ShaderParamFloat>(mMaterial->GetShaderParam("u_lightsCount"))->SetValue((float)lightsCount);
        DynamicCast<ShaderParamFloat>(mMaterial->GetShaderParam("u_ambient"))->SetValue(mAmbient);
        DynamicCast<ShaderParamFloatVector>(mMaterial->GetShaderParam("u_lightColors"))->SetValue(colors);
        DynamicCast<ShaderParamFloatVector>(mMaterial->GetShaderParam("u_lightVectors"))->SetValue(vectors);

        return firstDirectionalIndex;
    }

    void DeferredLightingPass::UpdateShadowParams(const RenderPassContext& context, int shadowLightIndex)
    {
        Ref<ShadowMapPass> shadowPass = context.pipeline ? context.pipeline->GetPass<ShadowMapPass>() : nullptr;
        bool shadowsEnabled = shadowPass && shadowPass->IsEnabled() && shadowPass->IsShadowAvailable() &&
            shadowPass->GetShadowMap() && shadowLightIndex >= 0;

        TextureRef shadowMap = shadowsEnabled ? shadowPass->GetShadowMap() : mFallbackShadowMap;
        mMaterial->SetSamplerTextureOverride("u_shadowMap", shadowMap);

        DynamicCast<ShaderParamFloat>(mMaterial->GetShaderParam("u_shadowsEnabled"))->SetValue(shadowsEnabled ? 1.0f : 0.0f);
        DynamicCast<ShaderParamFloat>(mMaterial->GetShaderParam("u_shadowLightIndex"))->SetValue((float)shadowLightIndex);
        DynamicCast<ShaderParamFloat>(mMaterial->GetShaderParam("u_shadowMapSize"))
            ->SetValue(shadowsEnabled ? (float)shadowPass->GetShadowMapSize() : 1.0f);
        DynamicCast<ShaderParamFloatVector>(mMaterial->GetShaderParam("u_lightVP"))
            ->SetValue(shadowsEnabled ? shadowPass->GetLightViewProj() : Vector<float>());
    }
}
// --- META ---

DECLARE_CLASS(o2::ShadowMapPass, o2__ShadowMapPass);

DECLARE_CLASS(o2::GBufferPass, o2__GBufferPass);

DECLARE_CLASS(o2::DeferredLightingPass, o2__DeferredLightingPass);
// --- END META ---
