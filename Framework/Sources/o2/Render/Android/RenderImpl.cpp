#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID
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

namespace o2
{
    void Render::InitializePlatform()
    {
        mLog->Out("Initializing OpenGL ES 2 render (Android)..");

        // GL context is created by the Java-side GLSurfaceView; here we only
        // configure state and allocate GPU resources.
        GL_CHECK_ERROR();

        mResolution = o2Application.GetContentSize();

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
        for (int i = 0; i < mBuffersPoolsSize; i++)
        {
            if (mVertexBuffersPool[i]) glDeleteBuffers(1, &mVertexBuffersPool[i]);
            if (mIndexBuffersPool[i])  glDeleteBuffers(1, &mIndexBuffersPool[i]);
            mVertexBuffersPool[i] = 0;
            mIndexBuffersPool[i] = 0;
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

    static void BindBatchAttributes(const VertexType& vtype, GLint pos, GLint color, GLint uv, GLint normal)
    {
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
    }

    void Render::PlatformBindNextPoolBuffers()
    {
        mCurrentBufferIdx++;
        if (mCurrentBufferIdx == mBuffersPoolsSize)
            mCurrentBufferIdx = 0;

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffersPool[mCurrentBufferIdx]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffersPool[mCurrentBufferIdx]);
        GL_CHECK_ERROR();

        BindBatchAttributes(mCurrentBatchVertexType,
                            mActivePosAttribute, mActiveColorAttribute,
                            mActiveUVAttribute, mActiveNormalAttribute);
        GL_CHECK_ERROR();

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
        // Presentation is handled by GLSurfaceView (eglSwapBuffers).
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

        BindBatchAttributes(mCurrentBatchVertexType,
                            mActivePosAttribute, mActiveColorAttribute,
                            mActiveUVAttribute, mActiveNormalAttribute);
        GL_CHECK_ERROR();
    }

    VertexType Render::PlatformResolveBatchVertexType(const VertexType& sourceVertexType, const Ref<Material>& material) const
    {
        return ResolveBatchVertexTypeByMaterial(sourceVertexType, material);
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        PROFILE_SAMPLE_FUNC();

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
        // Android DisplayMetrics DPI is available via JNI; use a reasonable
        // default here until we thread it through the Java bridge.
        return Vec2I(160, 160);
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

            glUseProgram(mActiveProgram);
            GL_CHECK_ERROR();

            BindBatchAttributes(mCurrentBatchVertexType,
                                mActivePosAttribute, mActiveColorAttribute,
                                mActiveUVAttribute, mActiveNormalAttribute);

            const UInt texCoordParams[] = { VertexParam::TexCoord1, VertexParam::TexCoord2 };
            size_t stride = mCurrentBatchVertexType.GetStride();
            if (stride == 0) stride = sizeof(Vertex);
            for (int i = 0; i < material->mSamplerLocations.Count(); i++)
            {
                GLint attrLoc = material->mSamplerLocations[i].texCoordsAttribute;
                if (attrLoc >= 0 && i < 2 && mCurrentBatchVertexType.HasParam(texCoordParams[i]))
                {
                    glVertexAttribPointer((GLuint)attrLoc, 2, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                          (void*)mCurrentBatchVertexType.GetParamOffset(texCoordParams[i]));
                    glEnableVertexAttribArray((GLuint)attrLoc);
                }
            }

            GL_CHECK_ERROR();

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

#endif // PLATFORM_ANDROID
