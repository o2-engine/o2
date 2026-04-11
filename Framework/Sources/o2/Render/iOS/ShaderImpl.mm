#include "o2/stdafx.h"

#ifdef PLATFORM_IOS

#import <Foundation/Foundation.h>

#include "o2/Render/Shader.h"
#include "o2/Render/iOS/MetalWrappers.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    namespace
    {
        String BuildMetalShaderSource(const String& source)
        {
            static const char* preamble = R"metal(#include <metal_stdlib>
using namespace metal;

struct O2VertexIn
{
    float x;
    float y;
    float z;
    uint color;
    packed_float2 texCoord0;
    packed_float2 texCoord1;
    packed_float2 texCoord2;
    packed_float3 normal;
};

struct O2Uniforms
{
    float4x4 mvpMatrix;
};

struct O2RasterizerData
{
    float4 position [[position]];
    float4 color;
    float2 texCoords;
    float2 texCoords2;
    float2 texCoords3;
    float3 normal;
};

inline float4 o2_unpackColor(uint color)
{
    return float4(float(color & 0xFFu) / 255.0,
                  float((color >> 8) & 0xFFu) / 255.0,
                  float((color >> 16) & 0xFFu) / 255.0,
                  float((color >> 24) & 0xFFu) / 255.0);
}

#line 1
)metal";

            return String(preamble) + source;
        }

        NSString* ToNSString(const String& value)
        {
            const char* data = value.Data();
            return [NSString stringWithUTF8String:data ? data : ""];
        }

        NSString* GetEntryPointName(Shader::Type type)
        {
            return type == Shader::Type::Vertex ? @"vertexShader" : @"fragmentShader";
        }

        String GetCompileErrorText(NSError* error)
        {
            if (!error)
                return "Unknown Metal compiler error";

            NSString* description = [error localizedDescription];
            NSString* failureReason = [error localizedFailureReason];
            NSString* recovery = [error localizedRecoverySuggestion];

            String result = description ? description.UTF8String : "Unknown Metal compiler error";
            if (failureReason && failureReason.length > 0)
                result += "\n" + String(failureReason.UTF8String);

            if (recovery && recovery.length > 0)
                result += "\n" + String(recovery.UTF8String);

            return result;
        }
    }

    bool Shader::PlatformCompile(const String& source, Type type)
    {
        PlatformDestroy();

        if (!RenderDevice::device)
        {
            o2Debug.LogError("Metal device is not initialized, can't compile shader " + mFileName);
            return false;
        }

        if (!mImpl)
            mImpl = mnew MTLShaderImpl();

        NSError* error = nil;
        NSString* sourceString = ToNSString(BuildMetalShaderSource(source));
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        options.fastMathEnabled = YES;

        id<MTLLibrary> library = [RenderDevice::device newLibraryWithSource:sourceString options:options error:&error];
        if (!library)
        {
            String shaderName = mFileName.IsEmpty() ? String("<runtime>") : mFileName;
            o2Debug.LogError("Error compiling Metal shader " + shaderName + ":\n" + GetCompileErrorText(error));
            PlatformDestroy();
            return false;
        }

        id<MTLFunction> function = [library newFunctionWithName:GetEntryPointName(type)];
        if (!function)
        {
            String shaderName = mFileName.IsEmpty() ? String("<runtime>") : mFileName;
            o2Debug.LogError("Metal shader entry point wasn't found in " + shaderName + ". Expected function name: " +
                             String(type == Type::Vertex ? "vertexShader" : "fragmentShader"));
            PlatformDestroy();
            return false;
        }

        mImpl->library = library;
        mImpl->function = function;
        return true;
    }

    void Shader::PlatformDestroy()
    {
        if (mImpl)
        {
            mImpl->function = nil;
            mImpl->library = nil;
            delete mImpl;
            mImpl = nullptr;
        }

        mReady = false;
    }
}

#endif // PLATFORM_IOS