#include "o2/stdafx.h"

#ifdef PLATFORM_IOS
#include <simd/matrix.h>

#include "o2/Render/Render.h"
#include "o2/Render/iOS/MetalWrappers.h"
#include "o2/Render/iOS/ShaderTypes.h"
#include "o2/Application/Application.h"
#include "o2/Application/iOS/ApplicationPlatformWrapper.h"
#include "o2/Assets/Assets.h"
#include "o2/Render/Font.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2/Utils/Math/Interpolation.h"
#include "o2/Application/Input.h"
#include "o2/Utils/Function/Function.h"

namespace o2
{
    MTKView* RenderDevice::view;
    id<MTLDevice> RenderDevice::device;
    id<MTLCommandQueue> RenderDevice::commandQueue;
    id<MTLLibrary> RenderDevice::defaultLibrary;
    id<MTLRenderPipelineState> RenderDevice::pipelineState;
    id<MTLRenderCommandEncoder> RenderDevice::renderEncoder;
    id<MTLCommandBuffer> RenderDevice::commandBuffer;
    
    id<MTLBuffer> RenderDevice::vertexBuffers[2];
    id<MTLBuffer> RenderDevice::indexBuffers[2];
    id<MTLBuffer> RenderDevice::uniformBuffers[2];
    
    id<MTLBuffer> RenderDevice::vertexBuffer;
    id<MTLBuffer> RenderDevice::indexBuffer;
    id<MTLBuffer> RenderDevice::uniformBuffer;
    
    void RenderDevice::Initialize()
    {
        RenderDevice::view = ApplicationPlatformWrapper::view;
        device = ApplicationPlatformWrapper::view.device;
        
        for (int i = 0; i < 2; i++)
        {
            vertexBuffers[i] = [device newBufferWithLength:o2Render.mVertexBufferSize*sizeof(MetalVertex2)
                                                   options:MTLResourceStorageModeShared];
            
            indexBuffers[i] = [device newBufferWithLength:o2Render.mIndexBufferSize*sizeof(UInt)
                                                  options:MTLResourceStorageModeShared];
            
            uniformBuffers[i] = [device newBufferWithLength:o2Render.mUniformBufferSize*sizeof(Uniforms)
                                                    options:MTLResourceStorageModeShared];
        }
        
        defaultLibrary = [device newDefaultLibrary];
        
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
        
        // Set up a descriptor for creating a pipeline state object
        MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineStateDescriptor.label = @"Default";
        pipelineStateDescriptor.vertexFunction = vertexFunction;
        pipelineStateDescriptor.fragmentFunction = fragmentFunction;
        pipelineStateDescriptor.colorAttachments[0].pixelFormat                 = view.colorPixelFormat;
        pipelineStateDescriptor.colorAttachments[0].blendingEnabled             = YES;
        pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation           = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation         = MTLBlendOperationAdd;
        pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
        pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        NSError *error = NULL;
        pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor
                                                               error:&error];
        
        commandQueue = [device newCommandQueue];
    }
    
    Render::Render() :
        mReady(false), mClippingEverything(false)
    {
        RenderDevice::Initialize();
        
        mLog = mnew LogStream("Render");
        o2Debug.GetLog()->BindStream(mLog);
        
        mResolution = o2Application.GetContentSize();
        
        Bitmap b(PixelFormat::R8G8B8A8, Vec2I(16, 16));
        b.Fill(Color4::White());
        mWhiteTexture = TextureRef(&b);
        
        InitializeFreeType();
        InitializeLinesIndexBuffer();
        InitializeLinesTextures();

        mCurrentRenderTarget = TextureRef();

//        if (IsDevMode())
//            o2Assets.onAssetsRebuilt += THIS_FUNC(&Render::OnAssetsRebuilded);

        mReady = true;
    }

    Render::~Render()
    {
        if (!mReady)
            return;

//        if (IsDevMode())
//            o2Assets.onAssetsRebuilded -= Func(this, &Render::OnAssetsRebuilded);

        mSolidLineTexture = TextureRef::Null();
        mDashLineTexture = TextureRef::Null();

        DeinitializeFreeType();

        mReady = false;
    }

    void Render::CheckCompatibles()
    {
        //get max texture size
//        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize.x);
//        mMaxTextureSize.y = mMaxTextureSize.x;
    }

    void Render::Begin()
    {
        if (!mReady)
            return;
        
        int currentBuffers = RenderDevice::vertexBuffer == RenderDevice::vertexBuffers[0] ? 1 : 0;
        RenderDevice::vertexBuffer = RenderDevice::vertexBuffers[currentBuffers];
        RenderDevice::indexBuffer = RenderDevice::indexBuffers[currentBuffers];
        RenderDevice::uniformBuffer = RenderDevice::uniformBuffers[currentBuffers];
        
        mLastDrawTexture = NULL;
        mLastDrawVertex = 0;
        mLastDrawIdx = 0;
        mVertexBufferOffset = 0;
        mIndexBufferOffset = 0;
        mUniformBufferOffset = 0;
        mTrianglesCount = 0;
        mFrameTrianglesCount = 0;
        mDIPCount = 0;
        mCurrentPrimitiveType = PrimitiveType::Polygon;

        mDrawingDepth = 0.0f;

        mScissorInfos.Clear();
        mStackScissors.Clear();

        mClippingEverything = false;
        
        RenderDevice::commandBuffer = [RenderDevice::commandQueue commandBuffer];
        RenderDevice::commandBuffer.label = @"Default";

        SetupViewMatrix(mResolution);
        UpdateCameraTransforms();

        preRender();
        preRender.Clear();
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
        
        if (mLastDrawTexture != texture.mTexture ||
            mLastDrawVertex + verticesCount >= mVertexBufferSize ||
            mLastDrawIdx + indexesCount >= mIndexBufferSize ||
            mCurrentPrimitiveType != primitiveType)
        {
            DrawPrimitives();
            
            mLastDrawTexture = texture.mTexture;
            mCurrentPrimitiveType = primitiveType;
        }
        
        MetalVertex2* dstVertexBuffer = (MetalVertex2*)((Byte*)[RenderDevice::vertexBuffer contents] + mVertexBufferOffset);
        for (UInt i = 0; i < verticesCount; i++)
        {
            UInt vi = mLastDrawVertex + i;
            dstVertexBuffer[vi].x = vertices[i].x;
            dstVertexBuffer[vi].y = vertices[i].y;
            dstVertexBuffer[vi].z = vertices[i].z;
            dstVertexBuffer[vi].tu = vertices[i].tu;
            dstVertexBuffer[vi].tv = vertices[i].tv;
            
            Color4 c; c.SetABGR(vertices[i].color);
            dstVertexBuffer[vi].color = { c.RF(), c.GF(), c.BF(), c.AF() };
        }            
        
        UInt* dstIndexBuffer =(UInt*)((Byte*)[RenderDevice::indexBuffer contents] + mIndexBufferOffset);
        for (UInt i = mLastDrawIdx, j = 0; j < indexesCount; i++, j++)
            dstIndexBuffer[i] = mLastDrawVertex + indexes[j];
        
        if (primitiveType != PrimitiveType::Line)
            mTrianglesCount += elementsCount;
        
        mLastDrawVertex += verticesCount;
        mLastDrawIdx += indexesCount;
    }

    
    void MtxMultiply(float* ret, const float* lhs, const float* rhs)
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
    
    void MtxConvert(float* origin, matrix_float4x4& dst)
    {
        dst.columns[0][0] = origin[0];  dst.columns[0][1] = origin[1];  dst.columns[0][2] = origin[2];  dst.columns[0][3] = origin[3];
        dst.columns[1][0] = origin[4];  dst.columns[1][1] = origin[5];  dst.columns[1][2] = origin[6];  dst.columns[1][3] = origin[7];
        dst.columns[2][0] = origin[8];  dst.columns[2][1] = origin[9];  dst.columns[2][2] = origin[10]; dst.columns[2][3] = origin[11];
        dst.columns[3][0] = origin[12]; dst.columns[3][1] = origin[13]; dst.columns[3][2] = origin[14]; dst.columns[3][3] = origin[15];
    }
    
    void Render::DrawPrimitives()
    {
        if (mLastDrawVertex < 1)
            return;
        
        MTLRenderPassDescriptor *renderPassDescriptor = RenderDevice::view.currentRenderPassDescriptor;
        if(renderPassDescriptor != nil)
        {
            if (mNeedClear)
            {
                [renderPassDescriptor.colorAttachments[0] setClearColor:
                 MTLClearColorMake(mClearColor.RF(), mClearColor.GF(), mClearColor.BF(), mClearColor.AF())];
                
                mNeedClear = false;
            }
            else
                [renderPassDescriptor.colorAttachments[0] setLoadAction:MTLLoadActionLoad];
            
            if (mCurrentRenderTarget)
                renderPassDescriptor.colorAttachments[0].texture = mCurrentRenderTarget->mImpl->texture;

            auto renderEncoder = [RenderDevice::commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            renderEncoder.label = @"Default";
            
            [renderEncoder setViewport:(MTLViewport){0.0, 0.0, (double)mCurrentResolution.x, (double)mCurrentResolution.y, -1.0, 1.0 }];
            
            if (mScissorEnabled)
            {
                Basis scissorRect(mScissorRect);
                Basis screenScissorRect = scissorRect*mCamera.GetBasis().Inverted()*Camera().GetBasis();
                
                RectI clipRect = scissorRect.AABB().Move(Vec2I(mCurrentResolution.x/2, mCurrentResolution.y/2));
                clipRect.left = Math::Clamp(clipRect.left, 0, mCurrentResolution.x);
                clipRect.right = Math::Clamp(clipRect.right, 0, mCurrentResolution.x);
                clipRect.bottom = Math::Clamp(clipRect.bottom, 0, mCurrentResolution.y);
                clipRect.top = Math::Clamp(clipRect.top, 0, mCurrentResolution.y);
                
                [renderEncoder setScissorRect:(MTLScissorRect){
                    (ULong)clipRect.left, (ULong)mCurrentResolution.y - clipRect.bottom - clipRect.Height(),
                    (ULong)clipRect.Width(), (ULong)clipRect.Height()
                }];
            }
            
            [renderEncoder setRenderPipelineState:RenderDevice::pipelineState];
            
            [renderEncoder setVertexBuffer:RenderDevice::vertexBuffer offset:mVertexBufferOffset atIndex:0];
            
            if (mLastDrawTexture) {
                [renderEncoder setFragmentTexture:mLastDrawTexture->mImpl->texture atIndex:0];
            }
            else {
                [renderEncoder setFragmentTexture:mWhiteTexture->mImpl->texture atIndex:0];
            }
            
            Uniforms uniforms;
            MtxConvert(mProjMatrix, uniforms.projectionMatrix);
            MtxConvert(mViewModelMatrix, uniforms.modelViewMatrix);
            
            Uniforms* dstUniformsBuffer = (Uniforms*)((Byte*)[RenderDevice::uniformBuffer contents] + mUniformBufferOffset);
            memcpy(dstUniformsBuffer, &uniforms, sizeof(Uniforms));
            [renderEncoder setVertexBuffer:RenderDevice::uniformBuffer offset:mUniformBufferOffset atIndex:1];
            mUniformBufferOffset += (sizeof(Uniforms)/256 + 1)*256;
            
            static const MTLPrimitiveType primitiveType[3]{ MTLPrimitiveTypeTriangle, MTLPrimitiveTypeTriangle, MTLPrimitiveTypeLine };
            
            [renderEncoder drawIndexedPrimitives:primitiveType[(int)mCurrentPrimitiveType] indexCount:mLastDrawIdx
                indexType:MTLIndexTypeUInt32 indexBuffer:RenderDevice::indexBuffer indexBufferOffset:mIndexBufferOffset];
            
            [renderEncoder endEncoding];
        }

        mVertexBufferOffset += (sizeof(MetalVertex2)*mLastDrawVertex/256 + 1)*256;
        mIndexBufferOffset += (sizeof(UInt)*mLastDrawIdx/256 + 1)*256;
        
        mFrameTrianglesCount += mTrianglesCount;
        mLastDrawVertex = mTrianglesCount = mLastDrawIdx = 0;

        mDIPCount++;
    }

    void Render::SetupViewMatrix(const Vec2I& viewSize)
    {
        mCurrentResolution = viewSize;
        
        Math::OrthoProjMatrix(mProjMatrix, 0.0f, (float)viewSize.x, (float)viewSize.y, 0.0f, -2.0f, 2.0f);
        UpdateCameraTransforms();
    }

    void Render::End()
    {
        if (!mReady)
            return;

        postRender();
        postRender.Clear();

        DrawPrimitives();

        [RenderDevice::commandBuffer presentDrawable:RenderDevice::view.currentDrawable];
        [RenderDevice::commandBuffer commit];
        
        //CheckTexturesUnloading();
        CheckFontsUnloading();
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        DrawPrimitives();
        mClearColor = color;
        mNeedClear = true;
    }
    
    void Render::UpdateCameraTransforms()
    {
        Vec2F resf = (Vec2F)mCurrentResolution;
        
        float sign = mCurrentRenderTarget ? 1 : -1;
        
        float modelMatrix[16] =
        {
            1,           0,            0, 0,
            0,        sign,            0, 0,
            0,           0,            1, 0,
            Math::Round(resf.x*0.5f), Math::Round(resf.y*0.5f), -1, 1
        };
        
        Basis defaultCameraBasis((Vec2F)mCurrentResolution*-0.5f, Vec2F::Right()*resf.x, Vec2F().Up()*resf.y);
        Basis camTransf = mCamera.GetBasis().Inverted()*defaultCameraBasis;
        mViewScale = Vec2F(camTransf.xv.Length(), camTransf.yv.Length());
        mInvViewScale = Vec2F(1.0f / mViewScale.x, 1.0f / mViewScale.y);
        
        float camTransfMatr[16] =
        {
            camTransf.xv.x,   camTransf.xv.y,   0, 0,
            camTransf.yv.x,   camTransf.yv.y,   0, 0,
            0,                0,                0, 0,
            camTransf.origin.x, camTransf.origin.y, 0, 1
        };
        
        MtxMultiply(mViewModelMatrix, modelMatrix, camTransfMatr);
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
            mScissorInfos.Last().mEndDepth = mDrawingDepth;
            
            if (!mStackScissors.Last().mRenderTarget)
            {
                RectI lastSummaryClipRect = mStackScissors.Last().mSummaryScissorRect;
                mClippingEverything = !summaryScissorRect.IsIntersects(lastSummaryClipRect);
                summaryScissorRect = summaryScissorRect.GetIntersection(lastSummaryClipRect);
            }
            else
            {
                mScissorEnabled = true;
                mClippingEverything = false;
            }
        }
        else
        {
            mScissorEnabled = true;
            mClippingEverything = false;
        }
        
        mScissorInfos.Add(ScissorInfo(summaryScissorRect, mDrawingDepth));
        mStackScissors.Add(ScissorStackEntry(rect, summaryScissorRect));
        
        mScissorRect = CalculateScreenSpaceScissorRect(summaryScissorRect);
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
            mScissorEnabled = false;
            
            while (!mStackScissors.IsEmpty() && !mStackScissors.Last().mRenderTarget)
                mStackScissors.PopBack();
            
            mScissorInfos.Last().mEndDepth = mDrawingDepth;
        }
        else
        {
            if (mStackScissors.Count() == 1)
            {
                mScissorEnabled = false;
                mStackScissors.PopBack();
                
                mScissorInfos.Last().mEndDepth = mDrawingDepth;
                mClippingEverything = false;
            }
            else
            {
                mStackScissors.PopBack();
                RectI lastClipRect = mStackScissors.Last().mSummaryScissorRect;
                
                mScissorInfos.Last().mEndDepth = mDrawingDepth;
                mScissorInfos.Add(ScissorInfo(lastClipRect, mDrawingDepth));
                
                if (mStackScissors.Last().mRenderTarget)
                {
                    mScissorEnabled = false;
                    mClippingEverything = false;
                }
                else
                {
                    mScissorRect = CalculateScreenSpaceScissorRect(lastClipRect);
                    mClippingEverything = lastClipRect == RectI();
                }
            }
        }
    }

    void Render::BindRenderTexture(TextureRef renderTarget)
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
            mScissorInfos.Last().mEndDepth = mDrawingDepth;
            mScissorEnabled = false;
        }

        mStackScissors.Add(ScissorStackEntry(RectI(), RectI(), true));

        mCurrentRenderTarget = renderTarget;
        
        SetupViewMatrix(renderTarget->GetSize());
    }

    void Render::UnbindRenderTexture()
    {
        if (!mCurrentRenderTarget)
            return;

        DrawPrimitives();

        mCurrentRenderTarget = TextureRef();
        
        SetupViewMatrix(mResolution);

        DisableScissorTest(true);
        mStackScissors.PopBack();
        if (!mStackScissors.IsEmpty())
        {
            mScissorEnabled = true;
            mScissorRect = CalculateScreenSpaceScissorRect(mStackScissors.Last().mSummaryScissorRect);
            mClippingEverything = mScissorRect == RectI();
        }
    }
}

#endif // PLATFORM_IOS
