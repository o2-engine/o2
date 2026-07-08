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

        static id<MTLDepthStencilState> depthStateDisabled; // Compare always, no write
        static id<MTLDepthStencilState> depthStateEnabled;  // Compare less-equal, write
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
