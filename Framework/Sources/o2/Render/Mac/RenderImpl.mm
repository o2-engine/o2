#include "o2/stdafx.h"

#ifdef PLATFORM_MAC
#include <simd/matrix.h>

#include "o2/Render/Render.h"
#include "o2/Render/Mac/MetalWrappers.h"
#include "o2/Render/Mac/ShaderTypes.h"
#include "o2/Application/Application.h"
#include "o2/Application/Mac/ApplicationPlatformWrapper.h"
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
    MTKView*                    RenderDevice::view;
    id<MTLDevice>               RenderDevice::device;
    id<MTLCommandQueue>         RenderDevice::commandQueue;
    id<MTLLibrary>              RenderDevice::defaultLibrary;
    id<MTLRenderPipelineState>  RenderDevice::pipelineState;
    id<MTLRenderCommandEncoder> RenderDevice::renderEncoder;
    id<MTLCommandBuffer>        RenderDevice::commandBuffer;

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

    void Render::InitializePlatform()
    {
        mLog->Out("Initializing Metal render..");

        RenderDevice::Initialize();
    }

    void Render::DeinitializePlatform()
    {}

    void Render::InitializeSandardShader()
    {}

    void Render::PlatformBegin()
    {
        int currentBuffers = RenderDevice::vertexBuffer == RenderDevice::vertexBuffers[0] ? 1 : 0;
        RenderDevice::vertexBuffer = RenderDevice::vertexBuffers[currentBuffers];
        RenderDevice::indexBuffer = RenderDevice::indexBuffers[currentBuffers];
        RenderDevice::uniformBuffer = RenderDevice::uniformBuffers[currentBuffers];
        
        RenderDevice::commandBuffer = [RenderDevice::commandQueue commandBuffer];
        RenderDevice::commandBuffer.label = @"Default";
    }

    void Render::PlatformUploadBuffers(Vertex* vertices, UInt verticesCount, VertexIndex* indexes, UInt indexesCount)
    {
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
    }
    
    void MtxConvert(float* origin, matrix_float4x4& dst)
    {
        dst.columns[0][0] = origin[0];  dst.columns[0][1] = origin[1];  dst.columns[0][2] = origin[2];  dst.columns[0][3] = origin[3];
        dst.columns[1][0] = origin[4];  dst.columns[1][1] = origin[5];  dst.columns[1][2] = origin[6];  dst.columns[1][3] = origin[7];
        dst.columns[2][0] = origin[8];  dst.columns[2][1] = origin[9];  dst.columns[2][2] = origin[10]; dst.columns[2][3] = origin[11];
        dst.columns[3][0] = origin[12]; dst.columns[3][1] = origin[13]; dst.columns[3][2] = origin[14]; dst.columns[3][3] = origin[15];
    }
    
    void Render::PlatformDrawPrimitives()
    {
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
                [renderPassDescriptor.colorAttachments[0] setLoadAction:MTLLoadActionDontCare];
            
            if (mCurrentRenderTarget)
                renderPassDescriptor.colorAttachments[0].texture = mCurrentRenderTarget->mImpl->texture;

            auto renderEncoder = [RenderDevice::commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            renderEncoder.label = @"Default";
            
            [renderEncoder setViewport:(MTLViewport){0.0, 0.0, (double)mCurrentResolution.x, (double)mCurrentResolution.y, -1.0, 1.0 }];
            
            if (mScissorEnabled)
            {
                Basis scissorRect(mScissorRect);
                
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
            
            if (mCurrentDrawTexture) {
                [renderEncoder setFragmentTexture:mCurrentDrawTexture->mImpl->texture atIndex:0];
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
    }

    void Render::PlatformEnd()
    {
        [RenderDevice::commandBuffer presentDrawable:RenderDevice::view.currentDrawable];
        [RenderDevice::commandBuffer commit];
    }

    void Render::PlatformResetState()
    {
        
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        mClearColor = color;
        mNeedClear = true;
    }

    void Render::PlatformFlipVerticesUV()
    {
        MetalVertex2* dstVertexBuffer = (MetalVertex2*)((Byte*)[RenderDevice::vertexBuffer contents] + mVertexBufferOffset);
        for (UInt i = 0; i < mLastDrawVertex; i++)
            dstVertexBuffer[i].tv = 1.0f - dstVertexBuffer[i].tv;
    }

    void Render::PlatformSetupCameraTransforms(float* matrix)
    {}

    void Render::PlatformBeginStencilDrawing()
    {}

    void Render::PlatformEndStencilDrawing()
    {}

    void Render::PlatformEnableStencilTest()
    {}

    void Render::PlatformDisableStencilTest()
    {}

    void Render::ClearStencil()
    {}

    void Render::PlatformEnableScissorTest()
    {}

    void Render::PlatformDisableScissorTest()
    {}

    void Render::PlatformSetScissorRect(const RectI& rect)
    {}

    void Render::PlatformBindRenderTarget(const TextureRef& renderTarget)
    {}

    Vec2I Render::GetPlatformMaxTextureSize()
    {
        return Vec2I(4096, 4096);
    }

    Vec2I Render::GetPlatformDPI()
    {
        NSScreen *screen = [NSScreen mainScreen];
        NSDictionary *description = [screen deviceDescription];
        NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
        CGSize displayPhysicalSize = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);
        
        const float mmPerInch = 25.4f;
        return Vec2I((displayPixelSize.width / displayPhysicalSize.width) * mmPerInch,
                     (displayPixelSize.height / displayPhysicalSize.height) * mmPerInch);
    }
}

#endif // PLATFORM_MAC
