#pragma once

#import <MetalKit/MetalKit.h>

#include "o2/Utils/Types/CommonTypes.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace o2
{
    struct RenderDevice
    {
    public:
        // Initializes renderer
        static void Initialize(UInt vertexBufferByteSize, UInt indexBufferSize);

    public:
        static MTKView*                    view;
        static id<MTLDevice>               device;
        static id<MTLCommandQueue>         commandQueue;
        static id<MTLCommandBuffer>        commandBuffer;

        static id<MTLBuffer> vertexBuffers[2];
        static id<MTLBuffer> indexBuffers[2];

        static id<MTLBuffer> vertexBuffer;
        static id<MTLBuffer> indexBuffer;
        static int           currentBufferIndex;

        static NSMutableArray* retiredBuffers[2]; // Overflowed frame buffers, kept alive until their slot is reused

        static id<MTLDepthStencilState> depthStateDisabled;       // Compare always, no write
        static id<MTLDepthStencilState> depthStateEnabled;        // Compare less-equal, write
        static id<MTLDepthStencilState> depthStateEnabledNoWrite; // Compare less-equal, no write

        // Open render pass of the frame on the single-threaded path. Consecutive batches that draw into
        // the same attachments share it: a pass per batch makes the tiled GPU reload and store the
        // whole render target every time
        static id<MTLRenderCommandEncoder> renderEncoder;
        static id<MTLTexture>              encoderColorTexture; // Attachments the open pass was created for
        static id<MTLTexture>              encoderDepthTexture;
        static id<MTLTexture>              encoderExtraTextures[3];

        // Frame target acquired on the main thread and consumed by the render thread (multithreaded render)
        static MTLRenderPassDescriptor* threadRenderPassDescriptor; // Back-buffer render pass descriptor
        static id<CAMetalDrawable>      threadDrawable;             // Back-buffer drawable to present
        static id<MTLTexture>           threadDepthTexture;         // Back-buffer depth texture
        static float                    threadGraphicsScale;        // Graphics scale captured on the main thread

        // Open render pass of the replayed frame, the render thread's counterpart of renderEncoder
        static id<MTLRenderCommandEncoder> threadEncoder;
        static id<MTLTexture>              threadEncoderColorTexture;
        static id<MTLTexture>              threadEncoderDepthTexture;
        static id<MTLTexture>              threadEncoderExtraTextures[3];

        // Limits how many frames the CPU may queue ahead of the GPU, so the render thread never blocks
        // on GPU completion (and thus on vsync) each frame — signaled from the command buffer's handler
        static dispatch_semaphore_t     frameSemaphore;
    };

    struct MTLTextureImpl
    {
        id<MTLTexture>      texture;
        id<MTLSamplerState> samplerState; // Wrap+filter state, rebuilt on PlatformSetWrap/Filter
        id<MTLTexture>      depthTexture; // Lazily created depth attachment for render targets
    };

    struct MTLShaderImpl
    {
        id<MTLLibrary> library = nil;
        id<MTLFunction> function = nil;
    };

    struct MetalParamBinding
    {
        NSUInteger offset = 0;
        MTLDataType dataType = MTLDataTypeNone;
        NSUInteger size = 0; // Total member size in bytes; for arrays: length*stride
    };

    struct MTLMaterialImpl
    {
        id<MTLRenderPipelineState> pipelineState = nil;

        int materialParamsIndex = -1;
        bool bindParamsToVertex = false;
        bool bindParamsToFragment = false;

        NSUInteger materialParamsSize = 0;
        std::unordered_map<std::string, MetalParamBinding> paramBindings;
        std::vector<Byte> materialParamsData;
    };
}
