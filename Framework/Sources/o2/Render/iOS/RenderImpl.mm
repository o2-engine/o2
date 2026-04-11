#include "o2/stdafx.h"

#ifdef PLATFORM_IOS

#include <simd/matrix.h>

#import <UIKit/UIKit.h>

#include "o2/Application/Application.h"
#include "o2/Application/iOS/ApplicationPlatformWrapper.h"
#include "o2/Render/Material.h"
#include "o2/Render/Render.h"
#include "o2/Render/Shader.h"
#include "o2/Render/Texture.h"
#include "o2/Render/iOS/MetalWrappers.h"
#include "o2/Render/iOS/ShaderTypes.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    MTKView*            RenderDevice::view;
    id<MTLDevice>       RenderDevice::device;
    id<MTLCommandQueue> RenderDevice::commandQueue;
    id<MTLCommandBuffer> RenderDevice::commandBuffer;

    id<MTLBuffer> RenderDevice::vertexBuffers[2];
    id<MTLBuffer> RenderDevice::indexBuffers[2];

    id<MTLBuffer> RenderDevice::vertexBuffer;
    id<MTLBuffer> RenderDevice::indexBuffer;
    int           RenderDevice::currentBufferIndex;

    namespace
    {
        bool gLoggedMissingDrawable = false;

        NSUInteger AlignBufferOffset(NSUInteger value)
        {
            static const NSUInteger alignment = 256;
            return ((value + alignment - 1) / alignment) * alignment;
        }

        void MtxConvert(const float* origin, matrix_float4x4& dst)
        {
            dst.columns[0][0] = origin[0];  dst.columns[0][1] = origin[1];  dst.columns[0][2] = origin[2];  dst.columns[0][3] = origin[3];
            dst.columns[1][0] = origin[4];  dst.columns[1][1] = origin[5];  dst.columns[1][2] = origin[6];  dst.columns[1][3] = origin[7];
            dst.columns[2][0] = origin[8];  dst.columns[2][1] = origin[9];  dst.columns[2][2] = origin[10]; dst.columns[2][3] = origin[11];
            dst.columns[3][0] = origin[12]; dst.columns[3][1] = origin[13]; dst.columns[3][2] = origin[14]; dst.columns[3][3] = origin[15];
        }

        String LoadResolvedShaderSource(const String& path)
        {
            return FileSystem::ReadFile(Shader::ResolvePlatformSourcePath(path));
        }
    }

    void RenderDevice::Initialize(UInt vertexBufferByteSize, UInt indexBufferSize)
    {
        RenderDevice::view = ApplicationPlatformWrapper::view;
        device = ApplicationPlatformWrapper::view.device;
        commandQueue = [device newCommandQueue];
        currentBufferIndex = 0;

        NSUInteger vertexBufferLength = (NSUInteger)vertexBufferByteSize;
        NSUInteger indexBufferLength = (NSUInteger)indexBufferSize * sizeof(VertexIndex);

        for (int i = 0; i < 2; i++)
        {
            vertexBuffers[i] = [device newBufferWithLength:vertexBufferLength
                                                   options:MTLResourceStorageModeShared];

            indexBuffers[i] = [device newBufferWithLength:indexBufferLength
                                                  options:MTLResourceStorageModeShared];
        }

        vertexBuffer = vertexBuffers[0];
        indexBuffer = indexBuffers[0];
    }

    void Render::InitializePlatform()
    {
        mLog->Out("Initializing Metal render..");

        mVertexBufferSize = USHRT_MAX;
        mIndexBufferSize = USHRT_MAX;
        mVertexBufferByteSize = mVertexBufferSize * sizeof(Vertex3Tex);
        mVertexData = mnew UInt8[mVertexBufferByteSize];
        mVertexIndexData = mnew VertexIndex[mIndexBufferSize];
        mCurrentBatchVertexType = Vertex3Tex::Type();

        RenderDevice::Initialize(mVertexBufferByteSize, mIndexBufferSize);
    }

    void Render::DeinitializePlatform()
    {
        delete[] mVertexData;
        delete[] mVertexIndexData;
        mVertexData = nullptr;
        mVertexIndexData = nullptr;

        RenderDevice::commandBuffer = nil;
        RenderDevice::vertexBuffer = nil;
        RenderDevice::indexBuffer = nil;
        RenderDevice::vertexBuffers[0] = nil;
        RenderDevice::vertexBuffers[1] = nil;
        RenderDevice::indexBuffers[0] = nil;
        RenderDevice::indexBuffers[1] = nil;
        RenderDevice::commandQueue = nil;
        RenderDevice::device = nil;
        RenderDevice::view = nil;
    }

    void Render::InitializeSandardShader()
    {}

    void Render::PlatformInitializeDefaultMaterial()
    {
        String basePath = GetBuiltinAssetsPath();
        String vertexPath = Shader::ResolvePlatformSourcePath(basePath + "Shaders/Default.vsh");
        String fragmentPath = Shader::ResolvePlatformSourcePath(basePath + "Shaders/Default.fsh");

        String vertexSource = LoadResolvedShaderSource(basePath + "Shaders/Default.vsh");
        String fragmentSource = LoadResolvedShaderSource(basePath + "Shaders/Default.fsh");

        if (vertexSource.IsEmpty() || fragmentSource.IsEmpty())
        {
            o2Debug.LogError("Failed to load default Metal shader files (" + vertexPath + ", " + fragmentPath + ")");
            return;
        }

        Ref<Shader> vertexShader = mmake<Shader>();
        Ref<Shader> fragmentShader = mmake<Shader>();
        vertexShader->SetFileName(vertexPath);
        fragmentShader->SetFileName(fragmentPath);
        vertexShader->Compile(vertexSource, Shader::Type::Vertex);
        fragmentShader->Compile(fragmentSource, Shader::Type::Fragment);

        if (!vertexShader->IsReady() || !fragmentShader->IsReady())
        {
            o2Debug.LogError("Failed to compile default Metal shaders");
            return;
        }

        mDefaultMaterial = mmake<Material>();
        mDefaultMaterial->SetVertexShader(vertexShader);
        mDefaultMaterial->SetFragmentShader(fragmentShader);
        mDefaultMaterial->SetBlendMode(BlendMode::Normal);
        if (!mDefaultMaterial->Build())
        {
            o2Debug.LogError("Failed to build default Metal material");
            mDefaultMaterial = nullptr;
        }
    }

    void Render::PlatformBegin()
    {
        RenderDevice::currentBufferIndex = (RenderDevice::currentBufferIndex + 1) % 2;
        RenderDevice::vertexBuffer = RenderDevice::vertexBuffers[RenderDevice::currentBufferIndex];
        RenderDevice::indexBuffer = RenderDevice::indexBuffers[RenderDevice::currentBufferIndex];

        RenderDevice::commandBuffer = [RenderDevice::commandQueue commandBuffer];
        RenderDevice::commandBuffer.label = @"Default";

        mVertexBufferOffset = 0;
        mIndexBufferOffset = 0;
        mVertexBufferIdx = 0;
        mIndexBufferIdx = 0;
    }

    void Render::PlatformDrawPrimitives()
    {
        if (!mCurrentMaterial || !mCurrentMaterial->mImpl || !mCurrentMaterial->mImpl->pipelineState)
            return;

        MTLRenderPassDescriptor* renderPassDescriptor = RenderDevice::view.currentRenderPassDescriptor;
        if (renderPassDescriptor != nil)
        {
            if (mNeedClear)
            {
                [renderPassDescriptor.colorAttachments[0] setClearColor:
                 MTLClearColorMake(mClearColor.RF(), mClearColor.GF(), mClearColor.BF(), mClearColor.AF())];
                [renderPassDescriptor.colorAttachments[0] setLoadAction:MTLLoadActionClear];

                mNeedClear = false;
            }
            else
                [renderPassDescriptor.colorAttachments[0] setLoadAction:MTLLoadActionLoad];

            [renderPassDescriptor.colorAttachments[0] setStoreAction:MTLStoreActionStore];

            if (mCurrentRenderTarget)
                renderPassDescriptor.colorAttachments[0].texture = mCurrentRenderTarget->mImpl->texture;

            NSUInteger vertexDataSize = (NSUInteger)mLastDrawVertex * sizeof(Vertex3Tex);
            NSUInteger indexDataSize = (NSUInteger)mLastDrawIdx * sizeof(VertexIndex);
            memcpy((Byte*)[RenderDevice::vertexBuffer contents] + mVertexBufferOffset, mVertexData, vertexDataSize);
            memcpy((Byte*)[RenderDevice::indexBuffer contents] + mIndexBufferOffset, mVertexIndexData, indexDataSize);

            auto renderEncoder = [RenderDevice::commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            renderEncoder.label = @"Default";

            float scale = mCurrentRenderTarget ? 1.0f : o2Application.GetGraphicsScale();
            [renderEncoder setViewport:(MTLViewport){0.0, 0.0, (double)(mCurrentResolution.x * scale), (double)(mCurrentResolution.y * scale), 0.0, 1.0 }];

            if (mScissorEnabled && mCurrentRenderTarget == nullptr)
            {
                Vec2I resolution = mCurrentResolution * scale;
                RectF scissorRectF = RectF(mScissorRect.left * scale, mScissorRect.top * scale, mScissorRect.right * scale, mScissorRect.bottom * scale)
                    .Move(resolution / 2);

                RectI scissorRect = scissorRectF;
                scissorRect.left = Math::Clamp(scissorRect.left, 0, resolution.x);
                scissorRect.right = Math::Clamp(scissorRect.right, 0, resolution.x);
                scissorRect.bottom = Math::Clamp(scissorRect.bottom, 0, resolution.y);
                scissorRect.top = Math::Clamp(scissorRect.top, 0, resolution.y);

                [renderEncoder setScissorRect:(MTLScissorRect){
                    (ULong)scissorRect.left,
                    (ULong)(resolution.y - scissorRect.bottom - scissorRect.Height()),
                    (ULong)scissorRect.Width(),
                    (ULong)scissorRect.Height()
                }];
            }

            [renderEncoder setRenderPipelineState:mCurrentMaterial->mImpl->pipelineState];
            [renderEncoder setVertexBuffer:RenderDevice::vertexBuffer offset:mVertexBufferOffset atIndex:0];

            TextureRef primaryTexture = mCurrentDrawTexture ? mCurrentDrawTexture : mWhiteTexture;
            if (primaryTexture && mCurrentMaterial->GetTextureUniform() >= 0)
                [renderEncoder setFragmentTexture:primaryTexture->mImpl->texture atIndex:(NSUInteger)mCurrentMaterial->GetTextureUniform()];

            for (int i = 0; i < mCurrentMaterial->mSamplerLocations.Count() && i < mCurrentMaterial->mSamplers.Count(); i++)
            {
                const auto& samplerLocation = mCurrentMaterial->mSamplerLocations[i];
                if (samplerLocation.textureIndex < 0)
                    continue;

                TextureRef samplerTexture = mCurrentMaterial->mSamplers[i].GetTexture();
                if (!samplerTexture)
                    continue;

                [renderEncoder setFragmentTexture:samplerTexture->mImpl->texture atIndex:(NSUInteger)samplerLocation.textureIndex];
            }

            mCurrentMaterial->ApplyParams();

            Uniforms uniforms;
            MtxConvert(mMVPMatrix, uniforms.mvpMatrix);
            [renderEncoder setVertexBytes:&uniforms length:sizeof(Uniforms) atIndex:1];

            if (mCurrentMaterial->mImpl->materialParamsIndex >= 0 && !mCurrentMaterial->mImpl->materialParamsData.empty())
            {
                const void* paramsData = mCurrentMaterial->mImpl->materialParamsData.data();
                NSUInteger paramsSize = mCurrentMaterial->mImpl->materialParamsSize;
                NSUInteger paramsIndex = (NSUInteger)mCurrentMaterial->mImpl->materialParamsIndex;

                if (mCurrentMaterial->mImpl->bindParamsToVertex)
                    [renderEncoder setVertexBytes:paramsData length:paramsSize atIndex:paramsIndex];

                if (mCurrentMaterial->mImpl->bindParamsToFragment)
                    [renderEncoder setFragmentBytes:paramsData length:paramsSize atIndex:paramsIndex];
            }

            static const MTLPrimitiveType primitiveType[3]{ MTLPrimitiveTypeTriangle, MTLPrimitiveTypeTriangle, MTLPrimitiveTypeLine };

            [renderEncoder drawIndexedPrimitives:primitiveType[(int)mCurrentPrimitiveType] indexCount:mLastDrawIdx
                indexType:MTLIndexTypeUInt32 indexBuffer:RenderDevice::indexBuffer indexBufferOffset:mIndexBufferOffset];

            [renderEncoder endEncoding];
        }

        mVertexBufferOffset = AlignBufferOffset(mVertexBufferOffset + (NSUInteger)mLastDrawVertex * sizeof(Vertex3Tex));
        mIndexBufferOffset = AlignBufferOffset(mIndexBufferOffset + (NSUInteger)mLastDrawIdx * sizeof(VertexIndex));
    }

    void Render::PlatformEnd()
    {
        if (!RenderDevice::commandBuffer)
            return;

        if (!RenderDevice::view.currentDrawable)
        {
            if (!gLoggedMissingDrawable)
            {
                o2Debug.LogError("iOS Metal backend present skipped: currentDrawable is nil");
                gLoggedMissingDrawable = true;
            }
        }
        else
            gLoggedMissingDrawable = false;

        if (!mCurrentRenderTarget && RenderDevice::view.currentDrawable)
            [RenderDevice::commandBuffer presentDrawable:RenderDevice::view.currentDrawable];

        [RenderDevice::commandBuffer commit];
    }

    void Render::PlatformResetState()
    {
        mCurrentBatchVertexType = Vertex3Tex::Type();
        mVertexBufferIdx = 0;
        mIndexBufferIdx = 0;
    }

    VertexType Render::PlatformResolveBatchVertexType(const VertexType& sourceVertexType, const Ref<Material>& material) const
    {
        return Vertex3Tex::Type();
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        mClearColor = color;
        mNeedClear = true;
    }

    void Render::PlatformFlipVerticesUV()
    {
        Vertex3Tex* dstVertexBuffer = reinterpret_cast<Vertex3Tex*>(mVertexData);
        for (UInt i = 0; i < mLastDrawVertex; i++)
            dstVertexBuffer[i].tv = 1.0f - dstVertexBuffer[i].tv;
    }

    void Render::PlatformSetupCameraTransforms(float* modelMatrix, float* viewMatrix, float* projMatrix)
    {
        if (mCurrentRenderTarget)
            modelMatrix[5] = -modelMatrix[5];

        float finalCamMtx[16];
        Math::mtxMultiply(finalCamMtx, modelMatrix, viewMatrix);
        Math::mtxMultiply(mMVPMatrix, projMatrix, finalCamMtx);

        static const float metalClipSpaceFix[16] =
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.5f, 1.0f
        };

        float metalMvp[16];
        Math::mtxMultiply(metalMvp, metalClipSpaceFix, mMVPMatrix);
        memcpy(mMVPMatrix, metalMvp, sizeof(mMVPMatrix));
    }

    void Render::PlatformEnableScissorTest()
    {
        mScissorEnabled = true;
    }

    void Render::PlatformDisableScissorTest()
    {
        mScissorEnabled = false;
    }

    void Render::PlatformSetScissorRect(const RectI& rect)
    {
        mScissorRect = rect;
    }

    void Render::PlatformBindRenderTarget(const TextureRef& renderTarget)
    {}

    Vec2I Render::GetPlatformMaxTextureSize()
    {
        return Vec2I(4096, 4096);
    }

    Vec2I Render::GetPlatformDPI()
    {
        UIScreen* screen = [UIScreen mainScreen];
        float scale = screen.scale > 0.0f ? (float)screen.scale : 1.0f;
        float baseDpi = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? 132.0f : 163.0f;
        int dpi = (int)Math::Round(baseDpi * scale);
        return Vec2I(dpi, dpi);
    }

    void Render::PlatformBindMaterial(const Ref<Material>& material)
    {}
}

#endif // PLATFORM_IOS
