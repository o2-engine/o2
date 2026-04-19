#include "o2/stdafx.h"

#if defined(PLATFORM_LINUX) && !defined(O2_RENDER_GLES2)
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
        mLog->Out("Initializing OpenGL render..");

        if (o2Application.mNeedPlatformInitialization)
        {
            mGLContext = glXCreateContext(o2Application.mDisplay, o2Application.mVisualInfo, NULL, GL_TRUE);
            glXMakeCurrent(o2Application.mDisplay, o2Application.mWindow, mGLContext);
        }

        GetGLExtensions(mLog.Get());

        GL_CHECK_ERROR();

        mVertexBufferSize = USHRT_MAX;
        mIndexBufferSize = USHRT_MAX;
        mCurrentBatchVertexType = Vertex::Type();
        mVertexBufferByteSize = mVertexBufferSize * sizeof(Vertex);

        mVertexData = mnew UInt8[mVertexBufferByteSize];
        mVertexIndexData = mnew VertexIndex[mIndexBufferSize * sizeof(VertexIndex)];

        for (int i = 0; i < mBuffersPoolsSize; i++)
        {
            glGenBuffers(1, &mVertexBuffersPool[i]);
            glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffersPool[i]);
            glBufferData(GL_ARRAY_BUFFER, mVertexBufferByteSize, mVertexData, GL_DYNAMIC_DRAW);

            glGenBuffers(1, &mIndexBuffersPool[i]);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffersPool[i]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(mIndexBufferSize * sizeof(VertexIndex)), mVertexIndexData, GL_DYNAMIC_DRAW);
        }

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.0f);

        mLog->Out("GL_VENDOR: " + (String)(char*)glGetString(GL_VENDOR));
        mLog->Out("GL_RENDERER: " + (String)(char*)glGetString(GL_RENDERER));
        mLog->Out("GL_VERSION: " + (String)(char*)glGetString(GL_VERSION));
    }

    void Render::DeinitializePlatform()
    {
        if (mGLContext && o2Application.mDisplay)
        {
            glXMakeCurrent(o2Application.mDisplay, 0, NULL);
            glXDestroyContext(o2Application.mDisplay, mGLContext);
            mGLContext = NULL;
        }
    }

    void Render::InitializeSandardShader()
    {
    }

    void Render::PlatformInitializeDefaultMaterial()
    {
        String basePath = GetBuiltinAssetsPath();
        String vSource = FileSystem::ReadFile(basePath + "Shaders/Default.vsh");
        String fSource = FileSystem::ReadFile(basePath + "Shaders/Default.fsh");

        if (vSource.IsEmpty() || fSource.IsEmpty())
        {
            o2Debug.LogError("Failed to load default shader files (FrameworkData/Shaders/Default.vsh, FrameworkData/Shaders/Default.fsh). \n"
                             "Ensure they are in BuiltAssets.");
            return;
        }

        Ref<Shader> vShader = mmake<Shader>();
        Ref<Shader> fShader = mmake<Shader>();
        vShader->Compile(vSource, Shader::Type::Vertex);
        fShader->Compile(fSource, Shader::Type::Fragment);

        if (!vShader->IsReady() || !fShader->IsReady())
        {
            o2Debug.LogError("Failed to compile default shaders (FrameworkData/Shaders/Default.vsh, FrameworkData/Shaders/Default.fsh).");
            return;
        }

        mDefaultMaterial = mmake<Material>();
        mDefaultMaterial->SetVertexShader(vShader);
        mDefaultMaterial->SetFragmentShader(fShader);
        mDefaultMaterial->SetBlendMode(BlendMode::Normal);
        if (!mDefaultMaterial->Build())
        {
            o2Debug.LogError("Failed to build default material from Shaders/Default.");
            mDefaultMaterial = nullptr;
            return;
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

        size_t stride = mCurrentBatchVertexType.GetStride();
        if (stride == 0) stride = sizeof(Vertex);

        glVertexAttribPointer((GLuint)mActivePosAttribute, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                              (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Position));
        glEnableVertexAttribArray((GLuint)mActivePosAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveColorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, (GLsizei)stride,
                              (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Color));
        glEnableVertexAttribArray((GLuint)mActiveColorAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveUVAttribute, 2, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                              (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::TexCoord0));
        glEnableVertexAttribArray((GLuint)mActiveUVAttribute);
        GL_CHECK_ERROR();

        if (mActiveNormalAttribute >= 0 && mCurrentBatchVertexType.HasParam(VertexParam::Normal))
        {
            glVertexAttribPointer((GLuint)mActiveNormalAttribute, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Normal));
            glEnableVertexAttribArray((GLuint)mActiveNormalAttribute);
            GL_CHECK_ERROR();
        }

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

        glBufferSubData(GL_ARRAY_BUFFER, mVertexBufferIdx * stride, mLastDrawVertex * stride, mVertexData);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferIdx * sizeof(VertexIndex), mLastDrawIdx * sizeof(VertexIndex), mVertexIndexData);
        GL_CHECK_ERROR();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mCurrentDrawTexture ? mCurrentDrawTexture->mHandle : mWhiteTexture->mHandle);
        glUniform1i(mActiveTextureSample, 0);
        GL_CHECK_ERROR();

        glDrawElements(primitiveType[(int)mCurrentPrimitiveType], mLastDrawIdx, GL_UNSIGNED_INT, (void*)(mIndexBufferIdx * sizeof(VertexIndex)));
        GL_CHECK_ERROR();

        mVertexBufferIdx += mLastDrawVertex;
        mIndexBufferIdx += mLastDrawIdx;
    }

    void Render::PlatformEnd()
    {
        if (o2Application.mNeedPlatformInitialization)
            glXSwapBuffers(o2Application.mDisplay, o2Application.mWindow);

        GL_CHECK_ERROR();
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

        size_t stride = mCurrentBatchVertexType.GetStride();
        if (stride == 0) stride = sizeof(Vertex);

        glVertexAttribPointer((GLuint)mActivePosAttribute, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                              (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Position));
        glEnableVertexAttribArray((GLuint)mActivePosAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveColorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, (GLsizei)stride,
                              (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Color));
        glEnableVertexAttribArray((GLuint)mActiveColorAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveUVAttribute, 2, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                              (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::TexCoord0));
        glEnableVertexAttribArray((GLuint)mActiveUVAttribute);
        GL_CHECK_ERROR();

        if (mActiveNormalAttribute >= 0 && mCurrentBatchVertexType.HasParam(VertexParam::Normal))
        {
            glVertexAttribPointer((GLuint)mActiveNormalAttribute, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Normal));
            glEnableVertexAttribArray((GLuint)mActiveNormalAttribute);
            GL_CHECK_ERROR();
        }
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
        glScissor((int)(rect.left + mCurrentResolution.x * 0.5f), (int)(rect.bottom + mCurrentResolution.y * 0.5f),
                  (int)rect.Width(), (int)rect.Height());
    }

    void Render::PlatformBindRenderTarget(const TextureRef& renderTarget)
    {
        if (renderTarget)
            glBindFramebufferEXT(GL_FRAMEBUFFER, renderTarget->mFrameBuffer);
        else
            glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

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
        return Vec2I(90, 90);
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

            size_t stride = mCurrentBatchVertexType.GetStride();
            if (stride == 0)
                stride = sizeof(Vertex);

            glVertexAttribPointer((GLuint)mActivePosAttribute, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Position));
            glEnableVertexAttribArray((GLuint)mActivePosAttribute);

            glVertexAttribPointer((GLuint)mActiveColorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, (GLsizei)stride,
                                  (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Color));
            glEnableVertexAttribArray((GLuint)mActiveColorAttribute);

            glVertexAttribPointer((GLuint)mActiveUVAttribute, 2, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                  (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::TexCoord0));
            glEnableVertexAttribArray((GLuint)mActiveUVAttribute);

            if (mActiveNormalAttribute >= 0 && mCurrentBatchVertexType.HasParam(VertexParam::Normal))
            {
                glVertexAttribPointer((GLuint)mActiveNormalAttribute, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride,
                                      (void*)mCurrentBatchVertexType.GetParamOffset(VertexParam::Normal));
                glEnableVertexAttribArray((GLuint)mActiveNormalAttribute);
            }

            const UInt texCoordParams[] = { VertexParam::TexCoord1, VertexParam::TexCoord2 };
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

#endif // PLATFORM_LINUX
