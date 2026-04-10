#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include "Render/Render.h"

#include "Application/Application.h"
#include "Assets/Assets.h"
#include "Render/Font.h"
#include "Render/Mesh.h"
#include "Render/Sprite.h"
#include "Render/Texture.h"
#include "Utils/Debug/Debug.h"
#include "Utils/Debug/Log/LogStream.h"
#include "Utils/Math/Geometry.h"
#include "Utils/Math/Interpolation.h"
#include "Application/Input.h"

namespace o2
{
    Render::Render() :
        mReady(false), mClippingEverything(false)
    {
        mVertexBufferSize = USHRT_MAX;
        mIndexBufferSize = USHRT_MAX;

        mLog = mmake<LogStream>("Render");
        o2Debug.GetLog()->BindStream(mLog);

        mResolution = o2Application.GetContentSize();

        CheckCompatibles();

        mVertexData = mnew UInt8[mVertexBufferSize * sizeof(Vertex2)];
        mVertexIndexData = mnew UInt16[mIndexBufferSize * sizeof(UInt16)];
        
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glLineWidth(1.0f);

        glGenBuffers(1, &mVertexBufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferObject);
        glBufferData(GL_ARRAY_BUFFER, mVertexBufferSize * sizeof(Vertex2), mVertexData, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &mIndexBufferObject);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferObject);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(mIndexBufferSize * sizeof(UInt16)), mVertexIndexData, GL_DYNAMIC_DRAW);

        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

        InitializeSandardShader();

        GL_CHECK_ERROR();

        mLog->Out("GL_VENDOR: " + (String)(char*)glGetString(GL_VENDOR));
        mLog->Out("GL_RENDERER: " + (String)(char*)glGetString(GL_RENDERER));
        mLog->Out("GL_VERSION: " + (String)(char*)glGetString(GL_VERSION));

        mDPI = Vec2I(100, 100);

        InitializeFreeType();
        InitializeLinesIndexBuffer();
        InitializeLinesTextures();

        mCurrentRenderTarget = TextureRef();

        if (IsDevMode())
            o2Assets.onAssetsRebuilded += Func(this, &Render::OnAssetsRebuilt);

        mReady = true;
    }

    Render::~Render()
    {
        if (!mReady)
            return;

        if (IsDevMode())
            o2Assets.onAssetsRebuilded -= Func(this, &Render::OnAssetsRebuilt);

        mSolidLineTexture = TextureRef::Null();
        mDashLineTexture = TextureRef::Null();

        DeinitializeFreeType();

        mReady = false;
    }

    GLuint RenderBase::LoadShader(GLenum shaderType, const char *source)
    {
        GLuint shader = glCreateShader(shaderType);

        if (shader)
        {
            glShaderSource(shader, 1, &source, NULL);
            glCompileShader(shader);

            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

            if (!compiled)
            {
                GLint infoLen = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

                if (infoLen > 0)
                {
                    char *infoLog = (char *) malloc(sizeof(char) * infoLen);
                    glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
                    o2Debug.LogError((String)"Error compiling shader:\n" + infoLog);
                    free(infoLog);
                }

                glDeleteShader(shader);
                shader = 0;
            }
        }

        return shader;
    }

    GLuint RenderBase::BuildShaderProgram(const char *vertexSource, const char *fragmentSource)
    {
        GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vertexSource);
        if (!vertexShader)
            return 0;

        GLuint fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!fragmentShader)
            return 0;

        GLuint program = glCreateProgram();
        if (program)
        {
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);

            GLint linkStatus;
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

            if (!linkStatus)
            {
                GLint infoLen = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);

                if (infoLen > 0)
                {
                    char *infoLog = (char *) malloc(sizeof(char) * infoLen);
                    glGetProgramInfoLog(program, infoLen, NULL, infoLog);
                    o2Debug.LogError((String)"Error linking shader:\n" + infoLog);
                    free(infoLog);
                }

                glDeleteProgram(program);
                program = 0;
            }
        }

        return program;
    }

    void RenderBase::InitializeSandardShader()
    {
        const char* fragShader = " precision mediump float;             \n \
                                                                        \n \
        varying lowp vec4 v_color;                                      \n \
        varying vec2 v_texCoords;                                       \n \
                                                                        \n \
        uniform sampler2D u_texture;                                    \n \
                                                                        \n \
        void main()                                                     \n \
        {                                                               \n \
            gl_FragColor = v_color * texture2D(u_texture, v_texCoords); \n \
        }";

        const char* vtxShader = " uniform mat4 u_transformMatrix; \n \
                                                                  \n \
        attribute vec4 a_position;                                \n \
        attribute vec4 a_color;                                   \n \
        attribute vec2 a_texCoords;                               \n \
                                                                  \n \
        varying vec4 v_color;                                     \n \
        varying vec2 v_texCoords;                                 \n \
                                                                  \n \
        void main()                                               \n \
        {                                                         \n \
            v_color = a_color;                                    \n \
            v_texCoords = a_texCoords;                            \n \
            gl_Position = u_transformMatrix * a_position;         \n \
        }";

        mStdShader = BuildShaderProgram(vtxShader, fragShader);
        GL_CHECK_ERROR();

        mStdShaderMvpUniform = glGetUniformLocation(mStdShader, "u_transformMatrix");
        GL_CHECK_ERROR();

        mStdShaderTextureSample = glGetUniformLocation(mStdShader, "u_texture");
        GL_CHECK_ERROR();

        mStdShaderPosAttribute = glGetAttribLocation(mStdShader, "a_position");
        GL_CHECK_ERROR();

        mStdShaderColorAttribute = glGetAttribLocation(mStdShader, "a_color");
        GL_CHECK_ERROR();

        mStdShaderUVAttribute = glGetAttribLocation(mStdShader, "a_texCoords");
        GL_CHECK_ERROR();

        glUseProgram(mStdShader);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mStdShaderPosAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2), &((Vertex2*)0)->x);
        glEnableVertexAttribArray((GLuint)mStdShaderPosAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mStdShaderColorAttribute, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex2), &((Vertex2*)0)->color);
        glEnableVertexAttribArray((GLuint)mStdShaderColorAttribute);
        GL_CHECK_ERROR();

        glVertexAttribPointer((GLuint)mStdShaderUVAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2), &((Vertex2*)0)->tu);
        glEnableVertexAttribArray((GLuint)mStdShaderUVAttribute);
        GL_CHECK_ERROR();
    }

    void Render::CheckCompatibles()
    {
        //get max texture size
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize.x);
        mMaxTextureSize.y = mMaxTextureSize.x;
    }

    void Render::Begin()
    {
        if (!mReady)
            return;

        mCurrentDrawTexture = NULL;
        mLastDrawVertex = 0;
        mLastDrawIdx = 0;
        mTrianglesCount = 0;
        mFrameTrianglesCount = 0;
        mDrawCallsCount = 0;
        mCurrentPrimitiveType = PrimitiveType::Polygon;

        mDrawingDepth = 0.0f;

        mScissorInfos.Clear();
        mStackScissors.Clear();

        mClippingEverything = false;

        SetupViewMatrix(mResolution);
        UpdateCameraTransforms();

        glClearColor(1, 0, 0, 1);

        preRender();
        preRender.Clear();
    }

    void Render::DrawPrimitives()
    {
        if (mLastDrawVertex < 1)
            return;

        static const GLenum primitiveType[3]{ GL_TRIANGLES, GL_TRIANGLES, GL_LINES };

        glBufferData(GL_ARRAY_BUFFER, mLastDrawVertex * sizeof(Vertex2), mVertexData, GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(mLastDrawIdx * sizeof(UInt16)), mVertexIndexData, GL_DYNAMIC_DRAW);

        glDrawElements(primitiveType[(int)mCurrentPrimitiveType], mLastDrawIdx, GL_UNSIGNED_SHORT, (void*)0);

        GL_CHECK_ERROR();

        mFrameTrianglesCount += mTrianglesCount;
        mLastDrawVertex = mTrianglesCount = mLastDrawIdx = 0;

        mDrawCallsCount++;
    }

    void Render::SetupViewMatrix(const Vec2I& viewSize)
    {
        mCurrentResolution = viewSize;
        UpdateCameraTransforms();
    }

    void Render::End()
    {
        if (!mReady)
            return;

        postRender();
        postRender.Clear();

        DrawPrimitives();

        GL_CHECK_ERROR();

        CheckTexturesUnloading();
        CheckFontsUnloading();
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        glClearColor(color.RF(), color.GF(), color.BF(), color.AF());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GL_CHECK_ERROR();
    }

    void mtxMultiply(float* ret, const float* lhs, const float* rhs)
    {
        // [ 0 4  8 12 ]   [ 0 4  8 12 ]
        // [ 1 5  9 13 ] x [ 1 5  9 13 ]
        // [ 2 6 10 14 ]   [ 2 6 10 14 ]
        // [ 3 7 11 15 ]   [ 3 7 11 15 ]
        ret[0] = lhs[0]*rhs[0] + lhs[4]*rhs[1] + lhs[8]*rhs[2] + lhs[12]*rhs[3];
        ret[1] = lhs[1]*rhs[0] + lhs[5]*rhs[1] + lhs[9]*rhs[2] + lhs[13]*rhs[3];
        ret[2] = lhs[2]*rhs[0] + lhs[6]*rhs[1] + lhs[10]*rhs[2] + lhs[14]*rhs[3];
        ret[3] = lhs[3]*rhs[0] + lhs[7]*rhs[1] + lhs[11]*rhs[2] + lhs[15]*rhs[3];

        ret[4] = lhs[0]*rhs[4] + lhs[4]*rhs[5] + lhs[8]*rhs[6] + lhs[12]*rhs[7];
        ret[5] = lhs[1]*rhs[4] + lhs[5]*rhs[5] + lhs[9]*rhs[6] + lhs[13]*rhs[7];
        ret[6] = lhs[2]*rhs[4] + lhs[6]*rhs[5] + lhs[10]*rhs[6] + lhs[14]*rhs[7];
        ret[7] = lhs[3]*rhs[4] + lhs[7]*rhs[5] + lhs[11]*rhs[6] + lhs[15]*rhs[7];

        ret[8] = lhs[0]*rhs[8] + lhs[4]*rhs[9] + lhs[8]*rhs[10] + lhs[12]*rhs[11];
        ret[9] = lhs[1]*rhs[8] + lhs[5]*rhs[9] + lhs[9]*rhs[10] + lhs[13]*rhs[11];
        ret[10] = lhs[2]*rhs[8] + lhs[6]*rhs[9] + lhs[10]*rhs[10] + lhs[14]*rhs[11];
        ret[11] = lhs[3]*rhs[8] + lhs[7]*rhs[9] + lhs[11]*rhs[10] + lhs[15]*rhs[11];

        ret[12] = lhs[0]*rhs[12] + lhs[4]*rhs[13] + lhs[8]*rhs[14] + lhs[12]*rhs[15];
        ret[13] = lhs[1]*rhs[12] + lhs[5]*rhs[13] + lhs[9]*rhs[14] + lhs[13]*rhs[15];
        ret[14] = lhs[2]*rhs[12] + lhs[6]*rhs[13] + lhs[10]*rhs[14] + lhs[14]*rhs[15];
        ret[15] = lhs[3]*rhs[12] + lhs[7]*rhs[13] + lhs[11]*rhs[14] + lhs[15]*rhs[15];
    }

    void Render::UpdateCameraTransforms()
    {
        if (mCurrentResolution == mPrevResolution && mCamera == mPrevCamera)
            return;

        DrawPrimitives();

        float projMat[16];
        Math::OrthoProjMatrix(projMat, 0.0f, (float)mCurrentResolution.x, (float)mCurrentResolution.y, 0.0f, 0.0f, 10.0f);
        glViewport(0, 0, mCurrentResolution.x, mCurrentResolution.y);

        float modelMatrix[16] =
        {
            1,                                      0,                                       0, 0,
            0,                                     -1,                                       0, 0,
            0,                                      0,                                       1, 0,
            Math::Round(mCurrentResolution.x*0.5f), Math::Round(mCurrentResolution.y*0.5f), -1, 1
        };

        Basis defaultCameraBasis((Vec2F)mCurrentResolution*-0.5f, Vec2F::Right()*mCurrentResolution.x, Vec2F().Up()*mCurrentResolution.y);
        Basis camTransf = mCamera.GetBasis().Inverted()*defaultCameraBasis;

        float camTransfMatr[16] =
        {
            camTransf.xv.x,     camTransf.xv.y,     0, 0,
            camTransf.yv.x,     camTransf.yv.y,     0, 0,
            0,                  0,                  0, 0,
            camTransf.origin.x, camTransf.origin.y, 0, 1
        };

        float mvp[16];
        float finalCamMtx[16];
        mtxMultiply(finalCamMtx, modelMatrix, camTransfMatr);
        mtxMultiply(mvp, projMat, finalCamMtx);

        glUniformMatrix4fv(mStdShaderMvpUniform, 1, GL_FALSE, mvp);

        mPrevCamera = mCamera;
        mPrevResolution = mCurrentResolution;

        GL_CHECK_ERROR();

    }

    VertexType Render::PlatformResolveBatchVertexType(const VertexType& sourceVertexType, const Ref<Material>& material) const
    {
        return ResolveBatchVertexTypeByMaterial(sourceVertexType, material);
    }

    void Render::EnableScissorTest(const RectI& rect)
    {
        DrawPrimitives();

        RectI summaryScissorRect = rect;
        if (!mStackScissors.IsEmpty())
        {
            RectI lastSummaryClipRect = mStackScissors.Last().summaryScissorRect;
            mClippingEverything = !summaryScissorRect.IsIntersects(lastSummaryClipRect);
            summaryScissorRect = summaryScissorRect.GetIntersection(lastSummaryClipRect);
            mScissorInfos.Last().endDepth = mDrawingDepth;
        }
        else
        {
            glEnable(GL_SCISSOR_TEST);
            GL_CHECK_ERROR();
        }

        mScissorInfos.Add(ScissorInfo(summaryScissorRect, mDrawingDepth));
        mStackScissors.Add(ScissorStackItem(rect, summaryScissorRect));

        glScissor((int)(summaryScissorRect.left + mCurrentResolution.x*0.5f),
            (int)(summaryScissorRect.bottom + mCurrentResolution.y*0.5f),
                  (int)summaryScissorRect.Width(),
                  (int)summaryScissorRect.Height());
    }

    void Render::DisableScissorTest(bool forcible /*= false*/)
    {
        if (mStackScissors.IsEmpty())
        {
            mLog->WarningStr("Can't disable scissor test - no scissor were enabled!");
            return;
        }

        DrawPrimitives();

        if (forcible)
        {
            glDisable(GL_SCISSOR_TEST);
            GL_CHECK_ERROR();

            while (!mStackScissors.IsEmpty() && !mStackScissors.Last().renderTarget)
                mStackScissors.PopBack();

            mScissorInfos.Last().endDepth = mDrawingDepth;
        }
        else
        {
            if (mStackScissors.Count() == 1)
            {
                glDisable(GL_SCISSOR_TEST);
                GL_CHECK_ERROR();
                mStackScissors.PopBack();

                mScissorInfos.Last().endDepth = mDrawingDepth;
                mClippingEverything = false;
            }
            else
            {
                mStackScissors.PopBack();
                RectI lastClipRect = mStackScissors.Last().summaryScissorRect;
                glScissor((int)(lastClipRect.left + mCurrentResolution.x*0.5f),
                    (int)(lastClipRect.bottom + mCurrentResolution.y*0.5f),
                          (int)lastClipRect.Width(),
                          (int)lastClipRect.Height());

                mScissorInfos.Last().endDepth = mDrawingDepth;
                mScissorInfos.Add(ScissorInfo(lastClipRect, mDrawingDepth));

                mClippingEverything = lastClipRect == RectI();
            }
        }
    }

    void Render::DrawBuffer(PrimitiveType primitiveType, Vertex2* vertices, UInt verticesCount,
                            UInt16* indexes, UInt elementsCount, const TextureRef& texture)
    {
        if (!mReady)
            return;

        mDrawingDepth += 1.0f;

        if (mClippingEverything)
            return;

        UInt indexesCount;
        if (primitiveType == PrimitiveType::Line)
            indexesCount = elementsCount*2;
        else
            indexesCount = elementsCount*3;

        if (mCurrentDrawTexture != texture.mTexture ||
            mLastDrawVertex + verticesCount >= mVertexBufferSize ||
            mLastDrawIdx + indexesCount >= mIndexBufferSize ||
            mCurrentPrimitiveType != primitiveType)
        {
            DrawPrimitives();

            mCurrentDrawTexture = texture.mTexture;
            mCurrentPrimitiveType = primitiveType;

            if (mCurrentDrawTexture)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mCurrentDrawTexture->mHandle);
                glUniform1i(mStdShaderTextureSample, 0);

                GL_CHECK_ERROR();
            }
            else
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1i(mStdShaderTextureSample, 0);

                GL_CHECK_ERROR();
            }
        }

        memcpy(&mVertexData[sizeof(Vertex2)*mLastDrawVertex], vertices, sizeof(Vertex2)*verticesCount);

        for (UInt i = mLastDrawIdx, j = 0; j < indexesCount * 3; i++, j++)
            mVertexIndexData[i] = mLastDrawVertex + indexes[j];

        if (primitiveType != PrimitiveType::Line)
            mTrianglesCount += elementsCount;

        mLastDrawVertex += verticesCount;
        mLastDrawIdx += indexesCount;
    }

    void Render::SetRenderTexture(TextureRef renderTarget)
    {
        if (!renderTarget)
        {
            UnbindRenderTexture();
            return;
        }

        if (renderTarget->mUsage != Texture::Usage::RenderTarget)
        {
            mLog->Error("Can't set texture as render target: not render target texture");
            UnbindRenderTexture();
            return;
        }

        if (!renderTarget->IsReady())
        {
            mLog->Error("Can't set texture as render target: texture isn't ready");
            UnbindRenderTexture();
            return;
        }

        DrawPrimitives();

        if (!mStackScissors.IsEmpty())
        {
            mScissorInfos.Last().endDepth = mDrawingDepth;
            glDisable(GL_SCISSOR_TEST);
            GL_CHECK_ERROR();
        }

        mStackScissors.Add(ScissorStackItem(RectI(), RectI(), true));

        glBindFramebuffer(GL_FRAMEBUFFER, renderTarget->mFrameBuffer);
        GL_CHECK_ERROR();

        SetupViewMatrix(renderTarget->GetSize());

        mCurrentRenderTarget = renderTarget;
    }

    void Render::UnbindRenderTexture()
    {
        if (!mCurrentRenderTarget)
            return;

        DrawPrimitives();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_CHECK_ERROR();

        SetupViewMatrix(mResolution);

        mCurrentRenderTarget = TextureRef();

        DisableScissorTest(true);
        mStackScissors.PopBack();
        if (!mStackScissors.IsEmpty())
        {
            glEnable(GL_SCISSOR_TEST);
            GL_CHECK_ERROR();

            auto clipRect = mStackScissors.Last().summaryScissorRect;

            glScissor((int)(clipRect.left + mCurrentResolution.x*0.5f),
                (int)(clipRect.bottom + mCurrentResolution.y*0.5f),
                      (int)clipRect.Width(),
                      (int)clipRect.Height());

            mClippingEverything = clipRect == RectI();
        }
    }
}

#endif // PLATFORM_ANDROID