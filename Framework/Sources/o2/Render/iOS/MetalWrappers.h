#pragma once

#import <MetalKit/MetalKit.h>

#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace o2
{
    class IShaderParam;
    class Type;

    struct RenderDevice
    {
    public:
        // Initializes renderer
        static void Initialize(UInt vertexBufferByteSize, UInt indexBufferSize);

    public:
        static MTKView*            view;
        static id<MTLDevice>       device;
        static id<MTLCommandQueue> commandQueue;
        static id<MTLCommandBuffer> commandBuffer;

        static id<MTLBuffer> vertexBuffers[2];
        static id<MTLBuffer> indexBuffers[2];

        static id<MTLBuffer> vertexBuffer;
        static id<MTLBuffer> indexBuffer;
        static int           currentBufferIndex;
    };

    struct MTLTextureImpl
    {
        id<MTLTexture> texture;
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
    };

    struct MetalParamWriter
    {
        using WriteFunction = void (*)(const IShaderParam&, Byte*);

        int paramIndex = -1;
        NSUInteger offset = 0;
        WriteFunction write = nullptr;
    };

    struct MTLMaterialImpl
    {
        id<MTLRenderPipelineState> pipelineState = nil;

        int materialParamsIndex = -1;
        bool bindParamsToVertex = false;
        bool bindParamsToFragment = false;

        NSUInteger materialParamsSize = 0;
        std::unordered_map<std::string, MetalParamBinding> paramBindings;
        Vector<String> cachedParamNames;
        Vector<const Type*> cachedParamTypes;
        std::vector<MetalParamWriter> paramWriters;
        std::vector<Byte> materialParamsData;
    };
}
