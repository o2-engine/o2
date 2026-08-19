#include "o2/stdafx.h"

#ifdef PLATFORM_WINDOWS
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
    namespace
    {
        typedef HGLRC(WINAPI* PFN_wglCreateContextAttribsARB)(HDC hDC, HGLRC hShareContext, const int* piAttribList);

        bool CreateGLContextWithAttribs(HDC hdc, HGLRC& outContext, o2::LogStream* log)
        {
            static PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR), 1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
            };

            int pixelFormat = ChoosePixelFormat(hdc, &pfd);
            if (!pixelFormat || !SetPixelFormat(hdc, pixelFormat, &pfd))
            {
                if (log) log->Error("Can't Set The PixelFormat.");
                return false;
            }

            HGLRC tempCtx = wglCreateContext(hdc);
            if (!tempCtx || !wglMakeCurrent(hdc, tempCtx))
            {
                if (tempCtx) wglDeleteContext(tempCtx);
                if (log) log->Error("Can't create temporary GL context.");
                return false;
            }

            auto wglCreateContextAttribsARB = (PFN_wglCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB");
            if (!wglCreateContextAttribsARB)
            {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(tempCtx);
                if (log) log->Out("WGL_ARB_create_context not available.");
                return false;
            }

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(tempCtx);

            struct VersionRequest { int major; int minor; int profileMask; };
            VersionRequest versions[] = {
                { 3, 3, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB },
                { 3, 2, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB },
                { 3, 0, 0 },
                { 2, 1, 0 },
                { 0, 0, 0 }
            };

            for (int i = 0; versions[i].major != 0; i++)
            {
                int attribs[16];
                int n = 0;
                if (versions[i].major > 0)
                {
                    attribs[n++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
                    attribs[n++] = versions[i].major;
                    attribs[n++] = WGL_CONTEXT_MINOR_VERSION_ARB;
                    attribs[n++] = versions[i].minor;
                }
                if (versions[i].profileMask != 0)
                {
                    attribs[n++] = WGL_CONTEXT_PROFILE_MASK_ARB;
                    attribs[n++] = versions[i].profileMask;
                }
                attribs[n] = 0;

                outContext = wglCreateContextAttribsARB(hdc, NULL, attribs);
                if (outContext)
                {
                    if (log) log->Out("Created OpenGL " + o2::String(versions[i].major) + "." + o2::String(versions[i].minor) + " via wglCreateContextAttribsARB");
                    return true;
                }
                SetLastError(0);
            }

            if (log) log->Error("wglCreateContextAttribsARB failed for all versions.");
            return false;
        }
    }

    void Render::InitializePlatform()
    {
        mLog->Out("Initializing OpenGL render..");

        if constexpr (IS_PLATFORM_INITIALIZATION_ENABLED)
        {
            mHDC = GetDC(o2Application.mHWnd);
            if (!mHDC)
            {
                mLog->Error("Can't Create A GL Device Context.\n");
                return;
            }

            if (!CreateGLContextWithAttribs(mHDC, mGLContext, mLog.Get()))
            {
                int pf = GetPixelFormat(mHDC);
                if (!pf)
                {
                    static PIXELFORMATDESCRIPTOR pfd = {
                        sizeof(PIXELFORMATDESCRIPTOR), 1,
                        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
                    };
                    pf = ChoosePixelFormat(mHDC, &pfd);
                    if (!pf || !SetPixelFormat(mHDC, pf, &pfd))
                    {
                        mLog->Error("Can't Set The PixelFormat.\n");
                        return;
                    }
                }
                mGLContext = wglCreateContext(mHDC);
                if (!mGLContext)
                {
                    mLog->Error("Can't Create A GL Rendering Context.\n");
                    return;
                }
                mLog->Out("Using legacy wglCreateContext.");
            }

            if (!wglMakeCurrent(mHDC, mGLContext))
            {
                mLog->Error("Can't Activate The GL Rendering Context.\n");
                return;
            }
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
        
        wglSwapIntervalEXT(0);

        mLog->Out("GL_VENDOR: " + (String)(char*)glGetString(GL_VENDOR));
        mLog->Out("GL_RENDERER: " + (String)(char*)glGetString(GL_RENDERER));
        mLog->Out("GL_VERSION: " + (String)(char*)glGetString(GL_VERSION));
    }

    void Render::DeinitializePlatform()
    {
        if (mGLContext)
        {
            if (!wglMakeCurrent(NULL, NULL))
                mLog->Error("Release of DC And RC Failed.\n");

            if (!wglDeleteContext(mGLContext))
                mLog->Error("Release Rendering Context Failed.\n");

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
        if constexpr (IS_PLATFORM_INITIALIZATION_ENABLED)
            SwapBuffers(mHDC);

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

    void Render::PlatformSetDepthTest(bool enabled, bool writeEnabled)
    {
        if (enabled)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
        }
        else
            glDisable(GL_DEPTH_TEST);

        // Depth mask also gates depth clears, keep it enabled while the test is off
        glDepthMask(enabled && !writeEnabled ? GL_FALSE : GL_TRUE);

        GL_CHECK_ERROR();
    }

    bool Render::PlatformSupportsMRT() const
    {
        return false;
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

    void Render::PlatformFlushPendingClear()
    {} // Clears are immediate on GL, nothing is pending

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
        Vec2I dpi;
        HDC dc = GetDC(0);
        dpi.x = GetDeviceCaps(dc, LOGPIXELSX);
        dpi.y = GetDeviceCaps(dc, LOGPIXELSY);
        ReleaseDC(0, dc);

        return dpi;
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

            // Bind additional texcoord attributes for extra samplers
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

        // Bind additional texture samplers
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

    // This backend draws while the frame is recorded: state changes, clears and material binds go
    // straight to the GPU API on the main thread, so a frame can't be replayed elsewhere yet. Moving
    // submission to the render thread needs the whole frame recorded first (and the context handed
    // over to that thread), after which these hooks replace the direct calls
    bool Render::PlatformSupportsMultithreadedRender()
    {
        return false;
    }

    void Render::PlatformBeginRecording()
    {}

    void Render::PlatformSnapshotDrawState(RenderDrawCommand& command)
    {}

    void Render::PlatformAcquireFrameTarget()
    {}

    void Render::PlatformBeginThreaded()
    {}

    void Render::PlatformReplayDrawCommand(const RenderDrawCommand& command)
    {}

    void Render::PlatformEndThreadedPass()
    {}

    void Render::PlatformEndThreaded()
    {}

    void Render::PlatformEndPass()
    {}
}

#endif // PLATFORM_WINDOWS
