#include "o2/stdafx.h"

#ifdef PLATFORM_WASM
#include "o2/Render/Render.h"

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/Font.h"
#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Shader.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2/Utils/Math/Interpolation.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace o2
{
    namespace
    {
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gWebGLContext = 0;

        // WebGL2 requires EXT_color_buffer_float to render into RGBA16F targets (G-buffer, shadow map)
        bool gFloatTargetsSupported = false;
    }

    void Render::InitializePlatform()
    {
        mLog->Out("Initializing WebGL 2 render..");

        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        attrs.alpha = EM_FALSE;
        attrs.depth = EM_TRUE;
        attrs.stencil = EM_FALSE;
        attrs.antialias = EM_FALSE;
        attrs.preserveDrawingBuffer = EM_FALSE;
        attrs.premultipliedAlpha = EM_FALSE;
        attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;

        gWebGLContext = emscripten_webgl_create_context("#canvas", &attrs);
        if (gWebGLContext <= 0)
        {
            mLog->Error("Failed to create WebGL 2 context.");
            return;
        }

        if (emscripten_webgl_make_context_current(gWebGLContext) != EMSCRIPTEN_RESULT_SUCCESS)
        {
            mLog->Error("Failed to activate WebGL 2 context.");
            return;
        }

        GL_CHECK_ERROR();

        gFloatTargetsSupported = emscripten_webgl_enable_extension(gWebGLContext, "EXT_color_buffer_float") == EM_TRUE;
        if (!gFloatTargetsSupported)
            mLog->WarningStr("EXT_color_buffer_float is not supported, deferred render pipeline will fall back to forward");

        mVertexBufferSize = USHRT_MAX;
        mIndexBufferSize = USHRT_MAX;
        mCurrentBatchVertexType = Vertex::Type();
        mVertexBufferByteSize = mVertexBufferSize * sizeof(Vertex);

        mVertexData = mnew UInt8[mVertexBufferByteSize];
        mVertexIndexData = mnew VertexIndex[mIndexBufferSize];

        for (int i = 0; i < mBuffersPoolsSize; i++)
        {
            glGenBuffers(1, &mVertexBuffersPool[i]);
            glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffersPool[i]);
            glBufferData(GL_ARRAY_BUFFER, mVertexBufferByteSize, mVertexData, GL_DYNAMIC_DRAW);

            glGenBuffers(1, &mIndexBuffersPool[i]);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffersPool[i]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(mIndexBufferSize * sizeof(VertexIndex)),
                         mVertexIndexData, GL_DYNAMIC_DRAW);
        }

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        const GLubyte* vendor   = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version  = glGetString(GL_VERSION);
        if (vendor)   mLog->Out((String)"GL_VENDOR: "   + (const char*)vendor);
        if (renderer) mLog->Out((String)"GL_RENDERER: " + (const char*)renderer);
        if (version)  mLog->Out((String)"GL_VERSION: "  + (const char*)version);
    }

    void Render::DeinitializePlatform()
    {
        if (gWebGLContext > 0)
        {
            emscripten_webgl_destroy_context(gWebGLContext);
            gWebGLContext = 0;
        }
    }

    void Render::InitializeSandardShader() {}

    void Render::PlatformInitializeDefaultMaterial()
    {
        String basePath = GetBuiltinAssetsPath();
        String vSource = FileSystem::ReadFile(basePath + "Shaders/Default.vsh");
        String fSource = FileSystem::ReadFile(basePath + "Shaders/Default.fsh");

        if (vSource.IsEmpty() || fSource.IsEmpty())
        {
            o2Debug.LogError("Failed to load default shader files. Ensure BuiltAssets contains Shaders/Default.{vsh,fsh}.");
            return;
        }

        Ref<Shader> vShader = mmake<Shader>();
        Ref<Shader> fShader = mmake<Shader>();
        vShader->Compile(vSource, Shader::Type::Vertex);
        fShader->Compile(fSource, Shader::Type::Fragment);

        if (!vShader->IsReady() || !fShader->IsReady())
        {
            o2Debug.LogError("Failed to compile default shaders.");
            return;
        }

        mDefaultMaterial = mmake<Material>();
        mDefaultMaterial->SetVertexShader(vShader);
        mDefaultMaterial->SetFragmentShader(fShader);
        mDefaultMaterial->SetBlendMode(BlendMode::Normal);
        if (!mDefaultMaterial->Build())
        {
            o2Debug.LogError("Failed to build default material.");
            mDefaultMaterial = nullptr;
        }
    }

    static void BindBatchAttributes(const VertexType& vtype, GLint pos, GLint color, GLint uv, GLint normal,
                                    GLint boneIndices, GLint boneWeights)
    {
        // Disable everything first: stale arrays from a previous vertex layout may point past the buffer
        static GLint maxAttributes = 0;
        if (maxAttributes == 0)
            glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttributes);

        for (GLint i = 0; i < maxAttributes; i++)
            glDisableVertexAttribArray((GLuint)i);

        size_t stride = vtype.GetStride();
        if (stride == 0) stride = sizeof(Vertex);

        if (pos >= 0)
        {
            glVertexAttribPointer((GLuint)pos, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)vtype.GetParamOffset(VertexParam::Position));
            glEnableVertexAttribArray((GLuint)pos);
        }
        if (color >= 0)
        {
            glVertexAttribPointer((GLuint)color, 4, GL_UNSIGNED_BYTE, GL_TRUE, (GLsizei)stride,
                                  (void*)vtype.GetParamOffset(VertexParam::Color));
            glEnableVertexAttribArray((GLuint)color);
        }
        if (uv >= 0)
        {
            glVertexAttribPointer((GLuint)uv, 2, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)vtype.GetParamOffset(VertexParam::TexCoord0));
            glEnableVertexAttribArray((GLuint)uv);
        }
        if (normal >= 0 && vtype.HasParam(VertexParam::Normal))
        {
            glVertexAttribPointer((GLuint)normal, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)vtype.GetParamOffset(VertexParam::Normal));
            glEnableVertexAttribArray((GLuint)normal);
        }
        if (boneIndices >= 0 && vtype.HasParam(VertexParam::BoneIndices))
        {
            glVertexAttribPointer((GLuint)boneIndices, 4, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)vtype.GetParamOffset(VertexParam::BoneIndices));
            glEnableVertexAttribArray((GLuint)boneIndices);
        }
        if (boneWeights >= 0 && vtype.HasParam(VertexParam::BoneWeights))
        {
            glVertexAttribPointer((GLuint)boneWeights, 4, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)vtype.GetParamOffset(VertexParam::BoneWeights));
            glEnableVertexAttribArray((GLuint)boneWeights);
        }
    }

    void Render::PlatformBindNextPoolBuffers()
    {
        mCurrentBufferIdx++;
        if (mCurrentBufferIdx == mBuffersPoolsSize)
            mCurrentBufferIdx = 0;

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffersPool[mCurrentBufferIdx]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffersPool[mCurrentBufferIdx]);
        GL_CHECK_ERROR();

        // Attribute pointers refer to the previous pool buffer, rebind lazily at the next draw
        mBoundAttributesVertexType = VertexType();

        mVertexBufferIdx = 0;
        mIndexBufferIdx = 0;
    }

    void Render::PlatformBegin()
    {
        PlatformBindNextPoolBuffers();
    }

    void Render::PlatformDrawPrimitives()
    {
        static const GLenum primitiveType[3]{ GL_TRIANGLES, GL_TRIANGLES, GL_LINES };

        if (mRenderTargetAttachmentsDirty)
            PlatformSyncRenderTargetAttachments();

        if (mCurrentBatchVertexType != mBoundAttributesVertexType)
        {
            BindBatchAttributes(mCurrentBatchVertexType,
                                mActivePosAttribute, mActiveColorAttribute,
                                mActiveUVAttribute, mActiveNormalAttribute,
                                mActiveBoneIndicesAttribute, mActiveBoneWeightsAttribute);

            if (mCurrentMaterial)
            {
                const UInt texCoordParams[] = { VertexParam::TexCoord1, VertexParam::TexCoord2 };
                size_t attrStride = mCurrentBatchVertexType.GetStride();
                if (attrStride == 0) attrStride = sizeof(Vertex);
                for (int i = 0; i < mCurrentMaterial->mSamplerLocations.Count(); i++)
                {
                    GLint attrLoc = mCurrentMaterial->mSamplerLocations[i].texCoordsAttribute;
                    if (attrLoc >= 0 && i < 2 && mCurrentBatchVertexType.HasParam(texCoordParams[i]))
                    {
                        glVertexAttribPointer((GLuint)attrLoc, 2, GL_FLOAT, GL_FALSE, (GLsizei)attrStride,
                                              (void*)mCurrentBatchVertexType.GetParamOffset(texCoordParams[i]));
                        glEnableVertexAttribArray((GLuint)attrLoc);
                    }
                }
            }

            mBoundAttributesVertexType = mCurrentBatchVertexType;
            GL_CHECK_ERROR();
        }

        size_t stride = mCurrentBatchVertexType.GetStride();

        glBufferSubData(GL_ARRAY_BUFFER, mVertexBufferIdx * stride,
                        mLastDrawVertex * stride, mVertexData);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferIdx * sizeof(VertexIndex),
                        mLastDrawIdx * sizeof(VertexIndex), mVertexIndexData);
        GL_CHECK_ERROR();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mCurrentDrawTexture ? mCurrentDrawTexture->mHandle : mWhiteTexture->mHandle);
        glUniform1i(mActiveTextureSample, 0);
        GL_CHECK_ERROR();

        glDrawElements(primitiveType[(int)mCurrentPrimitiveType], mLastDrawIdx,
                       GL_UNSIGNED_INT, (void*)(mIndexBufferIdx * sizeof(VertexIndex)));
        GL_CHECK_ERROR();

        mVertexBufferIdx += mLastDrawVertex;
        mIndexBufferIdx += mLastDrawIdx;
    }

    void Render::PlatformEnd()
    {
        GL_CHECK_ERROR();
        // Emscripten automatically presents on main loop return.
    }

    void Render::PlatformResetState()
    {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_SCISSOR_TEST);
        GL_CHECK_ERROR();

        BindMaterial(mDefaultMaterial);
        PlatformBindNextPoolBuffers();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_ERROR();

        glUniform1i(mActiveTextureSample, 0);
        GL_CHECK_ERROR();
    }

    VertexType Render::PlatformResolveBatchVertexType(const VertexType& sourceVertexType, const Ref<Material>& material) const
    {
        return ResolveBatchVertexTypeByMaterial(sourceVertexType, material);
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        PROFILE_SAMPLE_FUNC();

        // Extra MRT targets must be attached before the clear covers them
        if (mRenderTargetAttachmentsDirty)
            PlatformSyncRenderTargetAttachments();

        glClearColor(color.RF(), color.GF(), color.BF(), color.AF());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GL_CHECK_ERROR();
    }

    void Render::PlatformSetupCameraTransforms(float* modelMatrix, float* viewMatrix, float* projMatrix)
    {
        float finalCamMtx[16];
        Math::mtxMultiply(finalCamMtx, modelMatrix, viewMatrix);
        Math::mtxMultiply(mCurrentMvp, projMatrix, finalCamMtx);

        glViewport(0, 0, mCurrentResolution.x, mCurrentResolution.y);
        glUniformMatrix4fv(mActiveMvpUniform, 1, GL_FALSE, mCurrentMvp);

        GL_CHECK_ERROR();
    }

    void Render::PlatformFlipVerticesUV()
    {
        size_t stride = mCurrentBatchVertexType.GetStride();
        size_t tvOffset = mCurrentBatchVertexType.GetParamOffset(VertexParam::TexCoord0) + sizeof(float);

        for (UInt i = 0; i < mLastDrawVertex; i++)
        {
            float& tv = *reinterpret_cast<float*>(&mVertexData[i * stride + tvOffset]);
            tv = 1.0f - tv;
        }
    }

    void Render::PlatformSetDepthTest(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
        }
        else
            glDisable(GL_DEPTH_TEST);

        GL_CHECK_ERROR();
    }

    bool Render::PlatformSupportsMRT() const
    {
        // WebGL2 always provides draw buffers, but the deferred pipeline also needs float color targets
        return gFloatTargetsSupported;
    }

    void Render::PlatformSyncRenderTargetAttachments()
    {
        mRenderTargetAttachmentsDirty = false;

        if (!mCurrentRenderTarget)
            return;

        static const int maxExtraAttachments = 3;
        int extraCount = Math::Min(mExtraRenderTargets.Count(), maxExtraAttachments);
        for (int i = 0; i < maxExtraAttachments; i++)
        {
            GLuint handle = i < extraCount ? mExtraRenderTargets[i]->mHandle : 0;
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 + i, GL_TEXTURE_2D, handle, 0);
        }

        static const GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                               GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(1 + extraCount, drawBuffers);
        GL_CHECK_ERROR();
    }

    void Render::PlatformEnableScissorTest()
    {
        glEnable(GL_SCISSOR_TEST);
        GL_CHECK_ERROR();
    }

    void Render::PlatformDisableScissorTest()
    {
        glDisable(GL_SCISSOR_TEST);
        GL_CHECK_ERROR();
    }

    void Render::PlatformSetScissorRect(const RectI& rect)
    {
        glScissor((int)(rect.left + mCurrentResolution.x * 0.5f),
                  (int)(rect.bottom + mCurrentResolution.y * 0.5f),
                  (int)rect.Width(), (int)rect.Height());
    }

    void Render::PlatformBindRenderTarget(const TextureRef& renderTarget)
    {
        if (renderTarget)
            glBindFramebuffer(GL_FRAMEBUFFER, renderTarget->mFrameBuffer);
        else
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Extra targets are assigned after this call, sync attachments lazily at the next clear or draw
        mRenderTargetAttachmentsDirty = renderTarget != nullptr;

        GL_CHECK_ERROR();
    }

    Vec2I Render::GetPlatformMaxTextureSize()
    {
        int size = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
        return Vec2I(size, size);
    }

    Vec2I Render::GetPlatformDPI()
    {
        int dpi = (int)(96 * emscripten_get_device_pixel_ratio());
        return Vec2I(dpi, dpi);
    }

    void Render::PlatformBindMaterial(const Ref<Material>& material)
    {
        if (material->mProgram != mActiveProgram)
        {
            mActiveProgram = material->mProgram;
            mActiveMvpUniform = material->mTransformUniform;
            mActiveTextureSample = material->mTextureUniform;
            mActivePosAttribute = material->mPositionAttribute;
            mActiveColorAttribute = material->mColorAttribute;
            mActiveUVAttribute = material->mTexCoordsAttribute;
            mActiveNormalAttribute = material->mNormalAttribute;
            mActiveBoneIndicesAttribute = material->mBoneIndicesAttribute;
            mActiveBoneWeightsAttribute = material->mBoneWeightsAttribute;

            glUseProgram(mActiveProgram);
            GL_CHECK_ERROR();

            // Attribute locations belong to the new program, rebind pointers at the next draw
            mBoundAttributesVertexType = VertexType();

            glUniformMatrix4fv(mActiveMvpUniform, 1, GL_FALSE, mCurrentMvp);
            GL_CHECK_ERROR();
        }

        material->ApplyParams();

        for (int i = 0; i < material->mSamplerLocations.Count() && i < material->mSamplers.Count(); i++)
        {
            const auto& loc = material->mSamplerLocations[i];
            TextureRef tex = material->mSamplers[i].GetTexture();
            if (!tex)
                continue;

            GLint texUnit = i + 1;
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, tex->mHandle);

            if (loc.samplerUniform >= 0)
                glUniform1i(loc.samplerUniform, texUnit);
        }
        glActiveTexture(GL_TEXTURE0);

        if (material->GetBlendMode() == BlendMode::Add)
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        else
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        GL_CHECK_ERROR();
    }
}

#endif // PLATFORM_WASM
