#include "o2/stdafx.h"

#ifdef PLATFORM_WINDOWS
#include "o2/Render/Render.h"

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/Font.h"
#include "o2/Render/Mesh.h"
#include "../API/OpenGL/OpenGLUtils.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2/Utils/Math/Interpolation.h"

namespace o2
{
    void Render::InitializePlatform()
    {
        mLog->Out("Initializing OpenGL render..");

        if (o2Application.mNeedPlatformInitialization)
        {
            GLuint pixelFormat;
            static PIXELFORMATDESCRIPTOR pfd = // pfd Tells Windows How We Want Things To Be
            {
                sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
                1,                             // Version Number
                PFD_DRAW_TO_WINDOW |           // Format Must Support Window
                PFD_SUPPORT_OPENGL |           // Format Must Support OpenGL
                PFD_DOUBLEBUFFER,              // Must Support Double Buffering
                PFD_TYPE_RGBA,                 // Request An RGBA Format
                32,                            // Select Our Color Depth
                0, 0, 0, 0, 0, 0,              // Color Bits Ignored
                0,                             // No Alpha Buffer
                0,                             // Shift Bit Ignored
                0,                             // No Accumulation Buffer
                0, 0, 0, 0,                    // Accumulation Bits Ignored
                16,                            // 16Bit Z-Buffer (Depth Buffer)  
                1,                             // No Stencil Buffer
                0,                             // No Auxiliary Buffer
                PFD_MAIN_PLANE,                // Main Drawing Layer
                0,                             // Reserved
                0, 0, 0                        // Layer Masks Ignored
            };

            mHDC = GetDC(o2Application.mHWnd);
            if (!mHDC)
            {
                mLog->Error("Can't Create A GL Device Context.\n");
                return;
            }

            pixelFormat = ChoosePixelFormat(mHDC, &pfd);
            if (!pixelFormat)
            {
                mLog->Error("Can't Find A Suitable PixelFormat.\n");
                return;
            }

            if (!SetPixelFormat(mHDC, pixelFormat, &pfd))
            {
                mLog->Error("Can't Set The PixelFormat.\n");
                return;
            }

            mGLContext = wglCreateContext(mHDC);
            if (!mGLContext)
            {
                mLog->Error("Can't Create A GL Rendering Context.\n");
                return;
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

    void Render::InitializeStandardShader()
    {
        mVertexShader = mmake<GLSLShader>();
        mVertexShader->LoadFromFile(GetAssetsPath() + String("Shaders/standard.vs"));
        mFragmentShader = mmake<GLSLShader>();
        mFragmentShader->LoadFromFile(GetAssetsPath() + String("Shaders/standard.fs"));

        mGPUProgram = mmake<GPUProgram>(mVertexShader, mFragmentShader);
        mGPUProgram->Link();

        if (const auto posAttrib = mGPUProgram->GetParameter(VS_POSITION))
        {
            O2_GL_CHECK_ERROR(glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x));
            O2_GL_CHECK_ERROR(glEnableVertexAttribArray(posAttrib));
        }

        if (const auto colorAttrib = mGPUProgram->GetParameter(VS_COLOR))
        {
            O2_GL_CHECK_ERROR(glVertexAttribPointer(colorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->color));
            O2_GL_CHECK_ERROR(glEnableVertexAttribArray(colorAttrib));
        }

        if (const auto texCoordsAttrib = mGPUProgram->GetParameter(VS_TEX_COORDS))
        {
            O2_GL_CHECK_ERROR(glVertexAttribPointer(texCoordsAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->tu));
            O2_GL_CHECK_ERROR(glEnableVertexAttribArray(texCoordsAttrib));
        }
    }

    void RenderBase::BindNextPoolBuffers()
    {
        mCurrentBufferIdx++;
        if (mCurrentBufferIdx == mBuffersPoolsSize)
            mCurrentBufferIdx = 0;

        O2_GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffersPool[mCurrentBufferIdx]));
        O2_GL_CHECK_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffersPool[mCurrentBufferIdx]));

        const auto posAttrib = mGPUProgram->GetParameter(VS_POSITION);
        O2_GL_CHECK_ERROR(glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x));
        O2_GL_CHECK_ERROR(glEnableVertexAttribArray(posAttrib));

        const auto colorAttrib = mGPUProgram->GetParameter(VS_COLOR);
        O2_GL_CHECK_ERROR(glVertexAttribPointer(colorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->color));
        O2_GL_CHECK_ERROR(glEnableVertexAttribArray(colorAttrib));

        const auto texCoordsAttrib = mGPUProgram->GetParameter(VS_TEX_COORDS);
        O2_GL_CHECK_ERROR(glVertexAttribPointer(texCoordsAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->tu));
        O2_GL_CHECK_ERROR(glEnableVertexAttribArray(texCoordsAttrib));

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
        glUniform1i(mGPUProgram->GetParameter(FS_TEX_SAMPLE), 0);
        GL_CHECK_ERROR();

        // Set blend mode
        if (mCurrentBlendMode == BlendMode::Add)
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        else
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        // Draw
        glDrawElements(primitiveType[(int)mCurrentPrimitiveType], mLastDrawIdx, GL_UNSIGNED_INT, (void*)(mIndexBufferIdx * sizeof(VertexIndex)));
        GL_CHECK_ERROR();

        mVertexBufferIdx += mLastDrawVertex;
        mIndexBufferIdx += mLastDrawIdx;
    }

    void Render::PlatformEnd()
    {
        if (o2Application.mNeedPlatformInitialization)
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

        //         glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
        //         GL_CHECK_ERROR();

        glUseProgram(mGPUProgram->GetId());
        GL_CHECK_ERROR();

        BindNextPoolBuffers();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_ERROR();

        glUniform1i(mGPUProgram->GetParameter(FS_TEX_SAMPLE), 0);
        GL_CHECK_ERROR();

        const auto posAttrib = mGPUProgram->GetParameter(VS_POSITION);
        O2_GL_CHECK_ERROR(glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x));
        O2_GL_CHECK_ERROR(glEnableVertexAttribArray(posAttrib));

        const auto colorAttrib = mGPUProgram->GetParameter(VS_COLOR);
        O2_GL_CHECK_ERROR(glVertexAttribPointer(colorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->color));
        O2_GL_CHECK_ERROR(glEnableVertexAttribArray(colorAttrib));

        const auto texCoordsAttrib = mGPUProgram->GetParameter(VS_TEX_COORDS);
        O2_GL_CHECK_ERROR(glVertexAttribPointer(texCoordsAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->tu));
        O2_GL_CHECK_ERROR(glEnableVertexAttribArray(texCoordsAttrib));
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
        float mvp[16];
        float finalCamMtx[16];
        Math::mtxMultiply(finalCamMtx, modelMatrix, viewMatrix);
        Math::mtxMultiply(mvp, projMatrix, finalCamMtx);
        
        glViewport(0, 0, mCurrentResolution.x, mCurrentResolution.y);
        glUniformMatrix4fv(mGPUProgram->GetParameter(VS_MVP), 1, GL_FALSE, mvp);

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
}

#endif // PLATFORM_WINDOWS
