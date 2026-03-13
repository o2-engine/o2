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

        // Get OpenGL extensions
        GetGLExtensions(mLog.Get());

        GL_CHECK_ERROR();

        // Initialize buffers
        mVertexBufferSize = USHRT_MAX;
        mIndexBufferSize = USHRT_MAX;

        mVertexData = mnew UInt8[mVertexBufferSize * sizeof(Vertex)];
        mVertexIndexData = mnew VertexIndex[mIndexBufferSize * sizeof(VertexIndex)];

        for (int i = 0; i < mBuffersPoolsSize; i++)
        {
            glGenBuffers(1, &mVertexBuffersPool[i]);
            glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffersPool[i]);
            glBufferData(GL_ARRAY_BUFFER, mVertexBufferSize * sizeof(Vertex), mVertexData, GL_DYNAMIC_DRAW);

            glGenBuffers(1, &mIndexBuffersPool[i]);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffersPool[i]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(mIndexBufferSize * sizeof(VertexIndex)), mVertexIndexData, GL_DYNAMIC_DRAW);
        }

        // Configure OpenGL
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.0f);
        
        // Disable VSync
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
        // Not used on Windows: default material is built in PlatformInitializeDefaultMaterial().
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

    void RenderBase::BindNextPoolBuffers()
    {
        mCurrentBufferIdx++;
        if (mCurrentBufferIdx == mBuffersPoolsSize)
            mCurrentBufferIdx = 0;

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffersPool[mCurrentBufferIdx]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffersPool[mCurrentBufferIdx]);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActivePosAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x);
        glEnableVertexAttribArray((GLuint)mActivePosAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveColorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->color);
        glEnableVertexAttribArray((GLuint)mActiveColorAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveUVAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->tu);
        glEnableVertexAttribArray((GLuint)mActiveUVAttribute);
        GL_CHECK_ERROR();

        mVertexBufferIdx = 0;
        mIndexBufferIdx = 0;
    }

    void Render::PlatformBegin()
    {
        BindNextPoolBuffers();
    }

    void Render::PlatformUploadBuffers(Vertex* vertices, UInt verticesCount, VertexIndex* indexes, UInt indexesCount)
    {
        memcpy(&mVertexData[mLastDrawVertex * sizeof(Vertex)], vertices, sizeof(Vertex) * verticesCount);

        for (UInt i = mLastDrawIdx, j = 0; j < indexesCount; i++, j++)
            mVertexIndexData[i] = mVertexBufferIdx + mLastDrawVertex + indexes[j];
    }

    void Render::PlatformDrawPrimitives()
    {
        static const GLenum primitiveType[3]{ GL_TRIANGLES, GL_TRIANGLES, GL_LINES };

        // Upload data to GPU
        glBufferSubData(GL_ARRAY_BUFFER, mVertexBufferIdx * sizeof(Vertex), mLastDrawVertex * sizeof(Vertex), mVertexData);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferIdx * sizeof(VertexIndex), mLastDrawIdx * sizeof(VertexIndex), mVertexIndexData);
        GL_CHECK_ERROR();

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mCurrentDrawTexture ? mCurrentDrawTexture->mHandle : mWhiteTexture->mHandle);
        glUniform1i(mActiveTextureSample, 0);
        GL_CHECK_ERROR();

        // Draw
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

        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        GL_CHECK_ERROR();

		BindMaterial(mDefaultMaterial);
        BindNextPoolBuffers();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_ERROR();

        glUniform1i(mActiveTextureSample, 0);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActivePosAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x);
        glEnableVertexAttribArray((GLuint)mActivePosAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveColorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->color);
        glEnableVertexAttribArray((GLuint)mActiveColorAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mActiveUVAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->tu);
        glEnableVertexAttribArray((GLuint)mActiveUVAttribute);
        GL_CHECK_ERROR();
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        PROFILE_SAMPLE_FUNC();

        glClearColor(color.RF(), color.GF(), color.BF(), color.AF());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
        for (UInt i = 0; i < mLastDrawVertex; i++)
        {
            Vertex& v = ((Vertex*)mVertexData)[i];
            v.tv = 1.0f - v.tv;
        }
    }

    void Render::PlatformBeginStencilDrawing()
    {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 0x1, 0xffffffff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        GL_CHECK_ERROR();
    }

    void Render::PlatformEndStencilDrawing()
    {
        glDisable(GL_STENCIL_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        GL_CHECK_ERROR();
    }

    void Render::PlatformEnableStencilTest()
    {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 0x1, 0xffffffff);

        GL_CHECK_ERROR();
    }

    void Render::PlatformDisableStencilTest()
    {
        glDisable(GL_STENCIL_TEST);
    }

    void Render::ClearStencil()
    {
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);

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

            glUseProgram(mActiveProgram);
            GL_CHECK_ERROR();

            glVertexAttribPointer((GLuint)mActivePosAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x);
            glEnableVertexAttribArray((GLuint)mActivePosAttribute);

            glVertexAttribPointer((GLuint)mActiveColorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->color);
            glEnableVertexAttribArray((GLuint)mActiveColorAttribute);

            glVertexAttribPointer((GLuint)mActiveUVAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->tu);
            glEnableVertexAttribArray((GLuint)mActiveUVAttribute);
            GL_CHECK_ERROR();

            glUniformMatrix4fv(mActiveMvpUniform, 1, GL_FALSE, mCurrentMvp);
            GL_CHECK_ERROR();
        }

        material->ApplyParams();

        if (material->GetBlendMode() == BlendMode::Add)
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        else
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        GL_CHECK_ERROR();
    }
}

#endif // PLATFORM_WINDOWS
