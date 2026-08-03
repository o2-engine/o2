#include "o2/stdafx.h"

#ifdef PLATFORM_IOS
#include <simd/matrix.h>

#include "o2/Application/Application.h"
#include "o2/Application/iOS/ApplicationPlatformWrapper.h"
#include "o2/Render/Material.h"
#include "o2/Render/iOS/MetalWrappers.h"
#include "o2/Render/iOS/ShaderTypes.h"
#include "o2/Render/Render.h"
#include "o2/Render/Shader.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#import <UIKit/UIKit.h>

namespace o2
{
    MTKView*                    RenderDevice::view;
    id<MTLDevice>               RenderDevice::device;
    id<MTLCommandQueue>         RenderDevice::commandQueue;
    id<MTLCommandBuffer>        RenderDevice::commandBuffer;

    id<MTLBuffer> RenderDevice::vertexBuffers[2];
    id<MTLBuffer> RenderDevice::indexBuffers[2];

    id<MTLBuffer> RenderDevice::vertexBuffer;
    id<MTLBuffer> RenderDevice::indexBuffer;
    int           RenderDevice::currentBufferIndex;

    NSMutableArray* RenderDevice::retiredBuffers[2];

    id<MTLDepthStencilState> RenderDevice::depthStateDisabled;
    id<MTLDepthStencilState> RenderDevice::depthStateEnabled;
    id<MTLDepthStencilState> RenderDevice::depthStateEnabledNoWrite;

    id<MTLRenderCommandEncoder> RenderDevice::renderEncoder;
    id<MTLTexture>              RenderDevice::encoderColorTexture;
    id<MTLTexture>              RenderDevice::encoderDepthTexture;
    id<MTLTexture>              RenderDevice::encoderExtraTextures[3];

    MTLRenderPassDescriptor* RenderDevice::threadRenderPassDescriptor;
    id<CAMetalDrawable>      RenderDevice::threadDrawable;
    id<MTLTexture>           RenderDevice::threadDepthTexture;
    float                    RenderDevice::threadGraphicsScale = 1.0f;
    dispatch_semaphore_t     RenderDevice::frameSemaphore;

    id<MTLRenderCommandEncoder> RenderDevice::threadEncoder;
    id<MTLTexture>              RenderDevice::threadEncoderColorTexture;
    id<MTLTexture>              RenderDevice::threadEncoderDepthTexture;
    id<MTLTexture>              RenderDevice::threadEncoderExtraTextures[3];

    // Up to this many frames may be in flight on the GPU before the render thread waits
    static const int kMaxFramesInFlight = 2;

    namespace
    {
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

        // The macOS backend plots the real refresh period from the drawable's presented time; the iOS
        // SDK doesn't expose presentedTime/addPresentedHandler, so there is nothing to sample here
        inline void RegisterVsyncMarker(id<CAMetalDrawable>) {}
    }

    void RenderDevice::Initialize(UInt vertexBufferByteSize, UInt indexBufferSize)
    {
        RenderDevice::view = ApplicationPlatformWrapper::view;
        device = ApplicationPlatformWrapper::view.device;
        commandQueue = [device newCommandQueue];
        currentBufferIndex = 0;
        frameSemaphore = dispatch_semaphore_create(kMaxFramesInFlight);

        view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;

        MTLDepthStencilDescriptor* depthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
        depthDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
        depthDescriptor.depthWriteEnabled = NO;
        depthStateDisabled = [device newDepthStencilStateWithDescriptor:depthDescriptor];

        depthDescriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
        depthDescriptor.depthWriteEnabled = YES;
        depthStateEnabled = [device newDepthStencilStateWithDescriptor:depthDescriptor];

        depthDescriptor.depthWriteEnabled = NO;
        depthStateEnabledNoWrite = [device newDepthStencilStateWithDescriptor:depthDescriptor];

        NSUInteger vertexBufferLength = (NSUInteger)vertexBufferByteSize;
        NSUInteger indexBufferLength = (NSUInteger)indexBufferSize * sizeof(VertexIndex);

        for (int i = 0; i < 2; i++)
        {
            vertexBuffers[i] = [device newBufferWithLength:vertexBufferLength
                                                   options:MTLResourceStorageModeShared];

            indexBuffers[i] = [device newBufferWithLength:indexBufferLength
                                                  options:MTLResourceStorageModeShared];

            retiredBuffers[i] = [[NSMutableArray alloc] init];
        }

        vertexBuffer = vertexBuffers[0];
        indexBuffer = indexBuffers[0];
    }

    void Render::InitializePlatform()
    {
        mLog->Out("Initializing Metal render (iOS)..");

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
        RenderDevice::depthStateDisabled = nil;
        RenderDevice::depthStateEnabled = nil;
        RenderDevice::depthStateEnabledNoWrite = nil;
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
        [RenderDevice::retiredBuffers[RenderDevice::currentBufferIndex] removeAllObjects];

        RenderDevice::commandBuffer = [RenderDevice::commandQueue commandBuffer];
        RenderDevice::commandBuffer.label = @"Default";

        mNeedDepthClear = true;

        mVertexBufferOffset = 0;
        mIndexBufferOffset = 0;
        mVertexBufferIdx = 0;
        mIndexBufferIdx = 0;
    }

    void Render::PlatformEndPass()
    {
        if (!RenderDevice::renderEncoder)
            return;

        [RenderDevice::renderEncoder endEncoding];

        RenderDevice::renderEncoder = nil;
        RenderDevice::encoderColorTexture = nil;
        RenderDevice::encoderDepthTexture = nil;

        for (int i = 0; i < 3; i++)
            RenderDevice::encoderExtraTextures[i] = nil;
    }

    void Render::PlatformDrawPrimitives()
    {
        if (!mCurrentMaterial || !mCurrentMaterial->mImpl || !mCurrentMaterial->mImpl->pipelineState)
            return;

        MTLRenderPassDescriptor *renderPassDescriptor = RenderDevice::view.currentRenderPassDescriptor;
        if (renderPassDescriptor != nil)
        {
            MTLClearColor clearColor = MTLClearColorMake(mClearColor.RF(), mClearColor.GF(), mClearColor.BF(), mClearColor.AF());

            id<MTLTexture> colorTexture = mCurrentRenderTarget ? mCurrentRenderTarget->mImpl->texture :
                                                                 renderPassDescriptor.colorAttachments[0].texture;

            static const int maxExtraAttachments = 3;
            id<MTLTexture> extraTextures[maxExtraAttachments] = { nil, nil, nil };
            for (int i = 0; i < maxExtraAttachments; i++)
            {
                if (mCurrentRenderTarget && i < mExtraRenderTargets.Count())
                    extraTextures[i] = mExtraRenderTargets[i]->mImpl->texture;
            }

            id<MTLTexture> depthTexture = RenderDevice::view.depthStencilTexture;
            if (mCurrentRenderTarget)
            {
                auto targetImpl = mCurrentRenderTarget->mImpl;
                id<MTLTexture> targetColorTexture = targetImpl->texture;
                if (!targetImpl->depthTexture ||
                    targetImpl->depthTexture.width != targetColorTexture.width ||
                    targetImpl->depthTexture.height != targetColorTexture.height)
                {
                    MTLTextureDescriptor* depthDescriptor =
                        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                           width:targetColorTexture.width
                                                                          height:targetColorTexture.height
                                                                       mipmapped:NO];
                    depthDescriptor.usage = MTLTextureUsageRenderTarget;
                    depthDescriptor.storageMode = MTLStorageModePrivate;
                    targetImpl->depthTexture = [RenderDevice::device newTextureWithDescriptor:depthDescriptor];
                }

                depthTexture = targetImpl->depthTexture;
            }

            // A clear is a load action, so it can only happen at the start of a pass; everything else
            // this batch needs is encoder state, and can be set inside the pass already open
            bool sameAttachments = RenderDevice::renderEncoder != nil &&
                RenderDevice::encoderColorTexture == colorTexture &&
                RenderDevice::encoderDepthTexture == depthTexture;

            for (int i = 0; i < maxExtraAttachments && sameAttachments; i++)
                sameAttachments = RenderDevice::encoderExtraTextures[i] == extraTextures[i];

            if (!sameAttachments || mNeedClear || mNeedDepthClear)
            {
                PlatformEndPass();

                MTLLoadAction colorLoadAction = mNeedClear ? MTLLoadActionClear : MTLLoadActionLoad;
                if (mNeedClear)
                    [renderPassDescriptor.colorAttachments[0] setClearColor:clearColor];

                renderPassDescriptor.colorAttachments[0].texture = colorTexture;
                [renderPassDescriptor.colorAttachments[0] setLoadAction:colorLoadAction];
                [renderPassDescriptor.colorAttachments[0] setStoreAction:MTLStoreActionStore];

                // Extra MRT color attachments follow the primary attachment's clear/load behavior
                for (int i = 0; i < maxExtraAttachments; i++)
                {
                    auto attachment = renderPassDescriptor.colorAttachments[i + 1];
                    attachment.texture = extraTextures[i];

                    if (extraTextures[i])
                    {
                        attachment.clearColor = clearColor;
                        attachment.loadAction = colorLoadAction;
                        attachment.storeAction = MTLStoreActionStore;
                    }
                }

                renderPassDescriptor.depthAttachment.texture = depthTexture;
                renderPassDescriptor.depthAttachment.clearDepth = 1.0;
                renderPassDescriptor.depthAttachment.loadAction = mNeedDepthClear ? MTLLoadActionClear :
                                                                                    MTLLoadActionLoad;
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;

                RenderDevice::renderEncoder = [RenderDevice::commandBuffer
                                               renderCommandEncoderWithDescriptor:renderPassDescriptor];
                RenderDevice::renderEncoder.label = @"Default";

                RenderDevice::encoderColorTexture = colorTexture;
                RenderDevice::encoderDepthTexture = depthTexture;

                for (int i = 0; i < maxExtraAttachments; i++)
                    RenderDevice::encoderExtraTextures[i] = extraTextures[i];

                mNeedClear = false;
                mNeedDepthClear = false;
            }

            NSUInteger vertexDataSize = (NSUInteger)mLastDrawVertex * mCurrentBatchVertexType.GetStride();
            NSUInteger indexDataSize = (NSUInteger)mLastDrawIdx * sizeof(VertexIndex);

            // Frame geometry overflows the buffers: retire them until the frame ends and continue in fresh ones
            if (mVertexBufferOffset + vertexDataSize > [RenderDevice::vertexBuffer length] ||
                mIndexBufferOffset + indexDataSize > [RenderDevice::indexBuffer length])
            {
                NSMutableArray* retired = RenderDevice::retiredBuffers[RenderDevice::currentBufferIndex];
                [retired addObject:RenderDevice::vertexBuffer];
                [retired addObject:RenderDevice::indexBuffer];

                RenderDevice::vertexBuffer = [RenderDevice::device newBufferWithLength:[RenderDevice::vertexBuffers[0] length]
                                                                               options:MTLResourceStorageModeShared];
                RenderDevice::indexBuffer = [RenderDevice::device newBufferWithLength:[RenderDevice::indexBuffers[0] length]
                                                                              options:MTLResourceStorageModeShared];

                mVertexBufferOffset = 0;
                mIndexBufferOffset = 0;
            }

            memcpy((Byte*)[RenderDevice::vertexBuffer contents] + mVertexBufferOffset, mVertexData, vertexDataSize);
            memcpy((Byte*)[RenderDevice::indexBuffer contents] + mIndexBufferOffset, mVertexIndexData, indexDataSize);

            id<MTLRenderCommandEncoder> renderEncoder = RenderDevice::renderEncoder;

            float scale = mCurrentRenderTarget ? 1.0f : o2Application.GetGraphicsScale();
            [renderEncoder setViewport:(MTLViewport){0.0, 0.0, (double)(mCurrentResolution.x * scale), (double)(mCurrentResolution.y * scale), 0.0, 1.0 }];

            if (mScissorEnabled && mCurrentRenderTarget == nullptr)
            {
                Vec2I resolution = mCurrentResolution*scale;
                RectF scissorRectF = RectF(mScissorRect.left*scale, mScissorRect.top*scale, mScissorRect.right*scale, mScissorRect.bottom*scale)
                    .Move(resolution/2);

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
            else
            {
                // Scissor is encoder state: an unclipped batch sharing the pass must drop the
                // rectangle left by the clipped one before it
                [renderEncoder setScissorRect:(MTLScissorRect){
                    0, 0, (ULong)colorTexture.width, (ULong)colorTexture.height
                }];
            }

            [renderEncoder setRenderPipelineState:mCurrentMaterial->mImpl->pipelineState];
            id<MTLDepthStencilState> depthState = RenderDevice::depthStateDisabled;
            if (mDepthTestEnabled)
                depthState = mDepthWriteEnabled ? RenderDevice::depthStateEnabled : RenderDevice::depthStateEnabledNoWrite;
            [renderEncoder setDepthStencilState:depthState];

            [renderEncoder setVertexBuffer:RenderDevice::vertexBuffer offset:mVertexBufferOffset atIndex:0];

            TextureRef primaryTexture = mCurrentDrawTexture ? mCurrentDrawTexture : mWhiteTexture;
            if (primaryTexture && mCurrentMaterial->GetTextureUniform() >= 0)
            {
                NSUInteger slot = (NSUInteger)mCurrentMaterial->GetTextureUniform();
                [renderEncoder setFragmentTexture:primaryTexture->mImpl->texture atIndex:slot];
                if (primaryTexture->mImpl->samplerState)
                    [renderEncoder setFragmentSamplerState:primaryTexture->mImpl->samplerState atIndex:slot];
            }

            for (int i = 0; i < mCurrentMaterial->mSamplerLocations.Count() && i < mCurrentMaterial->mSamplers.Count(); i++)
            {
                const auto& samplerLocation = mCurrentMaterial->mSamplerLocations[i];
                if (samplerLocation.textureIndex < 0)
                    continue;

                TextureRef samplerTexture = mCurrentMaterial->mSamplers[i].GetTexture();
                if (!samplerTexture)
                    continue;

                NSUInteger slot = (NSUInteger)samplerLocation.textureIndex;
                [renderEncoder setFragmentTexture:samplerTexture->mImpl->texture atIndex:slot];
                if (samplerTexture->mImpl->samplerState)
                    [renderEncoder setFragmentSamplerState:samplerTexture->mImpl->samplerState atIndex:slot];
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
        }

        mVertexBufferOffset = AlignBufferOffset(mVertexBufferOffset + (NSUInteger)mLastDrawVertex * mCurrentBatchVertexType.GetStride());
        mIndexBufferOffset = AlignBufferOffset(mIndexBufferOffset + (NSUInteger)mLastDrawIdx * sizeof(VertexIndex));
    }

    void Render::PlatformEnd()
    {
        PlatformEndPass();

        if (!RenderDevice::commandBuffer)
            return;

        if (!mCurrentRenderTarget && RenderDevice::view.currentDrawable)
            [RenderDevice::commandBuffer presentDrawable:RenderDevice::view.currentDrawable];

        [RenderDevice::commandBuffer commit];
    }

    bool Render::PlatformSupportsMultithreadedRender()
    {
        return true;
    }

    void Render::PlatformBeginRecording()
    {
        // The depth clear flag of the frame: PlatformBeginThreaded, which would set it, runs on the
        // render thread only after the frame is recorded
        mNeedDepthClear = true;
    }

    void Render::PlatformSnapshotDrawState(RenderDrawCommand& command)
    {
        memcpy(command.mvpMatrix, mMVPMatrix, sizeof(command.mvpMatrix));
        command.scissorEnabled = mScissorEnabled;
        command.scissorRect = mScissorRect;
        command.needClear = mNeedClear;
        command.clearColor = mClearColor;
        command.needDepthClear = mNeedDepthClear;

        // The clear happens once per frame, on the first recorded batch
        mNeedClear = false;
        mNeedDepthClear = false;
    }

    void Render::PlatformAcquireFrameTarget()
    {
        // Acquired on the main thread (MTKView drawable/descriptor are main-thread affine), used by the
        // render thread to encode and present the frame
        RenderDevice::threadRenderPassDescriptor = RenderDevice::view.currentRenderPassDescriptor;
        RenderDevice::threadDrawable = RenderDevice::view.currentDrawable;
        RenderDevice::threadDepthTexture = RenderDevice::view.depthStencilTexture;
        RenderDevice::threadGraphicsScale = o2Application.GetGraphicsScale();
    }

    void Render::PlatformBeginThreaded()
    {
        // Block only if the GPU is already kMaxFramesInFlight frames behind; otherwise proceed. Paired
        // with the signal in the command buffer's completion handler in PlatformEndThreaded
        dispatch_semaphore_wait(RenderDevice::frameSemaphore, DISPATCH_TIME_FOREVER);

        RenderDevice::currentBufferIndex = (RenderDevice::currentBufferIndex + 1) % 2;
        RenderDevice::vertexBuffer = RenderDevice::vertexBuffers[RenderDevice::currentBufferIndex];
        RenderDevice::indexBuffer = RenderDevice::indexBuffers[RenderDevice::currentBufferIndex];
        [RenderDevice::retiredBuffers[RenderDevice::currentBufferIndex] removeAllObjects];

        RenderDevice::commandBuffer = [RenderDevice::commandQueue commandBuffer];
        RenderDevice::commandBuffer.label = @"Threaded";

        mVertexBufferOffset = 0;
        mIndexBufferOffset = 0;
        mVertexBufferIdx = 0;
        mIndexBufferIdx = 0;
    }

    void Render::PlatformEndThreadedPass()
    {
        if (!RenderDevice::threadEncoder)
            return;

        [RenderDevice::threadEncoder endEncoding];

        RenderDevice::threadEncoder = nil;
        RenderDevice::threadEncoderColorTexture = nil;
        RenderDevice::threadEncoderDepthTexture = nil;

        for (int i = 0; i < 3; i++)
            RenderDevice::threadEncoderExtraTextures[i] = nil;
    }

    void Render::PlatformReplayDrawCommand(const RenderDrawCommand& command)
    {
        const Ref<Material>& material = command.material;
        if (!material || !material->mImpl || !material->mImpl->pipelineState)
            return;

        MTLRenderPassDescriptor* renderPassDescriptor = RenderDevice::threadRenderPassDescriptor;
        if (renderPassDescriptor != nil)
        {
            MTLClearColor clearColor = MTLClearColorMake(command.clearColor.RF(), command.clearColor.GF(),
                                                         command.clearColor.BF(), command.clearColor.AF());

            id<MTLTexture> colorTexture = command.renderTarget ? command.renderTarget->mImpl->texture :
                                                                 RenderDevice::threadDrawable.texture;

            static const int maxExtraAttachments = 3;
            id<MTLTexture> extraTextures[maxExtraAttachments] = { nil, nil, nil };
            for (int i = 0; i < maxExtraAttachments; i++)
            {
                if (command.renderTarget && i < command.extraRenderTargets.Count())
                    extraTextures[i] = command.extraRenderTargets[i]->mImpl->texture;
            }

            id<MTLTexture> depthTexture = RenderDevice::threadDepthTexture;
            if (command.renderTarget)
            {
                auto targetImpl = command.renderTarget->mImpl;
                id<MTLTexture> targetColorTexture = targetImpl->texture;
                if (!targetImpl->depthTexture ||
                    targetImpl->depthTexture.width != targetColorTexture.width ||
                    targetImpl->depthTexture.height != targetColorTexture.height)
                {
                    MTLTextureDescriptor* depthDescriptor =
                        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                           width:targetColorTexture.width
                                                                          height:targetColorTexture.height
                                                                       mipmapped:NO];
                    depthDescriptor.usage = MTLTextureUsageRenderTarget;
                    depthDescriptor.storageMode = MTLStorageModePrivate;
                    targetImpl->depthTexture = [RenderDevice::device newTextureWithDescriptor:depthDescriptor];
                }

                depthTexture = targetImpl->depthTexture;
            }

            // A clear is a load action, so it can only happen at the start of a pass; everything else
            // this command needs is encoder state, and can be set inside the pass already open
            bool sameAttachments = RenderDevice::threadEncoder != nil &&
                RenderDevice::threadEncoderColorTexture == colorTexture &&
                RenderDevice::threadEncoderDepthTexture == depthTexture;

            for (int i = 0; i < maxExtraAttachments && sameAttachments; i++)
                sameAttachments = RenderDevice::threadEncoderExtraTextures[i] == extraTextures[i];

            if (!sameAttachments || command.needClear || command.needDepthClear)
            {
                PlatformEndThreadedPass();

                MTLLoadAction colorLoadAction = command.needClear ? MTLLoadActionClear : MTLLoadActionLoad;
                if (command.needClear)
                    [renderPassDescriptor.colorAttachments[0] setClearColor:clearColor];

                // The single reused descriptor must be re-pointed at the right color texture every pass,
                // so a render-target batch doesn't leak into the following back-buffer batch
                renderPassDescriptor.colorAttachments[0].texture = colorTexture;
                [renderPassDescriptor.colorAttachments[0] setLoadAction:colorLoadAction];
                [renderPassDescriptor.colorAttachments[0] setStoreAction:MTLStoreActionStore];

                for (int i = 0; i < maxExtraAttachments; i++)
                {
                    auto attachment = renderPassDescriptor.colorAttachments[i + 1];
                    attachment.texture = extraTextures[i];

                    if (extraTextures[i])
                    {
                        attachment.clearColor = clearColor;
                        attachment.loadAction = colorLoadAction;
                        attachment.storeAction = MTLStoreActionStore;
                    }
                }

                renderPassDescriptor.depthAttachment.texture = depthTexture;
                renderPassDescriptor.depthAttachment.clearDepth = 1.0;
                renderPassDescriptor.depthAttachment.loadAction = command.needDepthClear ? MTLLoadActionClear :
                                                                                          MTLLoadActionLoad;
                renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;

                RenderDevice::threadEncoder = [RenderDevice::commandBuffer
                                               renderCommandEncoderWithDescriptor:renderPassDescriptor];
                RenderDevice::threadEncoder.label = @"Threaded";

                RenderDevice::threadEncoderColorTexture = colorTexture;
                RenderDevice::threadEncoderDepthTexture = depthTexture;

                for (int i = 0; i < maxExtraAttachments; i++)
                    RenderDevice::threadEncoderExtraTextures[i] = extraTextures[i];
            }

            NSUInteger vertexDataSize = (NSUInteger)command.vertexCount * command.vertexStride;
            NSUInteger indexDataSize = (NSUInteger)command.indexCount * sizeof(VertexIndex);

            if (mVertexBufferOffset + vertexDataSize > [RenderDevice::vertexBuffer length] ||
                mIndexBufferOffset + indexDataSize > [RenderDevice::indexBuffer length])
            {
                NSMutableArray* retired = RenderDevice::retiredBuffers[RenderDevice::currentBufferIndex];
                [retired addObject:RenderDevice::vertexBuffer];
                [retired addObject:RenderDevice::indexBuffer];

                RenderDevice::vertexBuffer = [RenderDevice::device newBufferWithLength:[RenderDevice::vertexBuffers[0] length]
                                                                               options:MTLResourceStorageModeShared];
                RenderDevice::indexBuffer = [RenderDevice::device newBufferWithLength:[RenderDevice::indexBuffers[0] length]
                                                                              options:MTLResourceStorageModeShared];

                mVertexBufferOffset = 0;
                mIndexBufferOffset = 0;
            }

            memcpy((Byte*)[RenderDevice::vertexBuffer contents] + mVertexBufferOffset, command.vertexData.data(), vertexDataSize);
            memcpy((Byte*)[RenderDevice::indexBuffer contents] + mIndexBufferOffset, command.indexData.data(), indexDataSize);

            id<MTLRenderCommandEncoder> renderEncoder = RenderDevice::threadEncoder;

            float scale = command.renderTarget ? 1.0f : RenderDevice::threadGraphicsScale;
            [renderEncoder setViewport:(MTLViewport){0.0, 0.0, (double)(command.resolution.x * scale), (double)(command.resolution.y * scale), 0.0, 1.0 }];

            if (command.scissorEnabled && command.renderTarget == nullptr)
            {
                Vec2I resolution = command.resolution * scale;
                RectF scissorRectF = RectF(command.scissorRect.left * scale, command.scissorRect.top * scale,
                                           command.scissorRect.right * scale, command.scissorRect.bottom * scale)
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
            else
            {
                // Scissor is encoder state: an unclipped command sharing the pass must drop the
                // rectangle left by the clipped one before it
                [renderEncoder setScissorRect:(MTLScissorRect){
                    0, 0, (ULong)colorTexture.width, (ULong)colorTexture.height
                }];
            }

            [renderEncoder setRenderPipelineState:material->mImpl->pipelineState];
            id<MTLDepthStencilState> depthState = RenderDevice::depthStateDisabled;
            if (command.depthTestEnabled)
                depthState = command.depthWriteEnabled ? RenderDevice::depthStateEnabled : RenderDevice::depthStateEnabledNoWrite;
            [renderEncoder setDepthStencilState:depthState];

            [renderEncoder setVertexBuffer:RenderDevice::vertexBuffer offset:mVertexBufferOffset atIndex:0];

            TextureRef primaryTexture = command.drawTexture ? command.drawTexture : mWhiteTexture;
            if (primaryTexture && material->GetTextureUniform() >= 0)
            {
                NSUInteger slot = (NSUInteger)material->GetTextureUniform();
                [renderEncoder setFragmentTexture:primaryTexture->mImpl->texture atIndex:slot];
                if (primaryTexture->mImpl->samplerState)
                    [renderEncoder setFragmentSamplerState:primaryTexture->mImpl->samplerState atIndex:slot];
            }

            for (int i = 0; i < material->mSamplerLocations.Count() && i < material->mSamplers.Count(); i++)
            {
                const auto& samplerLocation = material->mSamplerLocations[i];
                if (samplerLocation.textureIndex < 0)
                    continue;

                TextureRef samplerTexture = material->mSamplers[i].GetTexture();
                if (!samplerTexture)
                    continue;

                NSUInteger slot = (NSUInteger)samplerLocation.textureIndex;
                [renderEncoder setFragmentTexture:samplerTexture->mImpl->texture atIndex:slot];
                if (samplerTexture->mImpl->samplerState)
                    [renderEncoder setFragmentSamplerState:samplerTexture->mImpl->samplerState atIndex:slot];
            }

            material->ApplyParams();

            Uniforms uniforms;
            MtxConvert(command.mvpMatrix, uniforms.mvpMatrix);
            [renderEncoder setVertexBytes:&uniforms length:sizeof(Uniforms) atIndex:1];

            if (material->mImpl->materialParamsIndex >= 0 && !material->mImpl->materialParamsData.empty())
            {
                const void* paramsData = material->mImpl->materialParamsData.data();
                NSUInteger paramsSize = material->mImpl->materialParamsSize;
                NSUInteger paramsIndex = (NSUInteger)material->mImpl->materialParamsIndex;

                if (material->mImpl->bindParamsToVertex)
                    [renderEncoder setVertexBytes:paramsData length:paramsSize atIndex:paramsIndex];

                if (material->mImpl->bindParamsToFragment)
                    [renderEncoder setFragmentBytes:paramsData length:paramsSize atIndex:paramsIndex];
            }

            static const MTLPrimitiveType primitiveType[3]{ MTLPrimitiveTypeTriangle, MTLPrimitiveTypeTriangle, MTLPrimitiveTypeLine };

            [renderEncoder drawIndexedPrimitives:primitiveType[command.primitiveType] indexCount:command.indexCount
                indexType:MTLIndexTypeUInt32 indexBuffer:RenderDevice::indexBuffer indexBufferOffset:mIndexBufferOffset];
        }

        mVertexBufferOffset = AlignBufferOffset(mVertexBufferOffset + (NSUInteger)command.vertexCount * command.vertexStride);
        mIndexBufferOffset = AlignBufferOffset(mIndexBufferOffset + (NSUInteger)command.indexCount * sizeof(VertexIndex));
    }

    void Render::PlatformEndThreaded()
    {
        PlatformEndThreadedPass();

        if (!RenderDevice::commandBuffer)
        {
            // Nothing was submitted this frame, keep the semaphore balanced with PlatformBeginThreaded
            dispatch_semaphore_signal(RenderDevice::frameSemaphore);
            return;
        }

        // Release one frame slot once the GPU finishes this frame. This lets the render thread commit
        // without blocking on GPU/vsync, so the main thread (which rendezvouses with it) isn't stalled
        // until the display refresh and the display link keeps running at full rate
        __block dispatch_semaphore_t semaphore = RenderDevice::frameSemaphore;
        [RenderDevice::commandBuffer addCompletedHandler:^(id<MTLCommandBuffer>) {
            dispatch_semaphore_signal(semaphore);
        }];

        if (RenderDevice::threadDrawable)
        {
            [RenderDevice::commandBuffer presentDrawable:RenderDevice::threadDrawable];
            RegisterVsyncMarker(RenderDevice::threadDrawable);
        }

        [RenderDevice::commandBuffer commit];

        // Only wait for GPU completion when capturing a frame (screenshot read-back needs the rendered
        // target). Normal frames stay non-blocking, bounded instead by the frames-in-flight semaphore
        if (mCaptureTarget)
            [RenderDevice::commandBuffer waitUntilCompleted];

        RenderDevice::threadRenderPassDescriptor = nil;
        RenderDevice::threadDrawable = nil;
        RenderDevice::threadDepthTexture = nil;
    }

    void Render::PlatformResetState()
    {
        mCurrentBatchVertexType = Vertex3Tex::Type();
        mVertexBufferIdx = 0;
        mIndexBufferIdx = 0;
    }

    VertexType Render::PlatformResolveBatchVertexType(const VertexType& sourceVertexType, const Ref<Material>& material) const
    {
        // Skinned vertices are drawn as-is by the skinned shaders, everything else batches as Vertex3Tex
        if (sourceVertexType.HasParam(VertexParam::BoneIndices))
            return sourceVertexType;

        return Vertex3Tex::Type();
    }

    void Render::Clear(const Color4& color /*= Color4::Blur()*/)
    {
        mClearColor = color;
        mNeedClear = true;
        mNeedDepthClear = true;
    }

    void Render::PlatformFlipVerticesUV()
    {
        if (!mCurrentBatchVertexType.HasParam(VertexParam::TexCoord0))
            return;

        size_t stride = mCurrentBatchVertexType.GetStride();
        size_t uvOffset = mCurrentBatchVertexType.GetParamOffset(VertexParam::TexCoord0);
        for (UInt i = 0; i < mLastDrawVertex; i++)
        {
            float* tv = reinterpret_cast<float*>(mVertexData + i*stride + uvOffset) + 1;
            *tv = 1.0f - *tv;
        }
    }

    void Render::PlatformSetupCameraTransforms(float* modelMatrix, float* viewMatrix, float* projMatrix)
    {
        if (mCurrentRenderTarget)
            modelMatrix[5] = -modelMatrix[5]; // Flip by Y for render targets

        float finalCamMtx[16];
        Math::mtxMultiply(finalCamMtx, modelMatrix, viewMatrix);
        Math::mtxMultiply(mMVPMatrix, projMatrix, finalCamMtx);

        // Shared projection matrices are OpenGL-style and produce clip-space z in [-w, w].
        // Metal expects clip-space z in [0, w], so convert it once at the platform level.
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
    {
        if (renderTarget)
            mNeedDepthClear = true;
    }

    void Render::PlatformSetDepthTest(bool enabled, bool writeEnabled)
    {}

    Vec2I Render::GetPlatformMaxTextureSize()
    {
        return Vec2I(4096, 4096);
    }

    bool Render::PlatformSupportsMRT() const
    {
        return true;
    }

    void Render::PlatformSyncRenderTargetAttachments()
    {} // MRT attachments are set on the render pass descriptor at draw time

    Vec2I Render::GetPlatformDPI()
    {
        float scale = [[UIScreen mainScreen] scale];
        // iOS doesn't expose physical screen size easily; return a reasonable default
        // 163 PPI is the standard for non-retina iOS, multiplied by scale gives effective PPI
        int dpi = (int)(163.0f * scale);
        return Vec2I(dpi, dpi);
    }

    void Render::PlatformBindMaterial(const Ref<Material>& material)
    {}
}

#endif // PLATFORM_IOS
