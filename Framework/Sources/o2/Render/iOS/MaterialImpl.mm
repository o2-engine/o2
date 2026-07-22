#include "o2/stdafx.h"

#ifdef PLATFORM_IOS

#import <Foundation/Foundation.h>

#include "o2/Render/Material.h"
#include "o2/Render/iOS/MetalWrappers.h"
#include "o2/Utils/Debug/Debug.h"

#include <algorithm>

namespace o2
{
    namespace
    {
        static UInt gNextMaterialProgramId = 1;

        std::string ToStdString(const String& value)
        {
            return std::string(value.Data(), value.Length());
        }

        String ToString(NSString* value)
        {
            return value ? String(value.UTF8String) : String();
        }

        void SetupBlendState(MTLRenderPipelineColorAttachmentDescriptor* attachment, BlendMode blendMode)
        {
            attachment.blendingEnabled = YES;
            attachment.rgbBlendOperation = MTLBlendOperationAdd;
            attachment.alphaBlendOperation = MTLBlendOperationAdd;
            attachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
            attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            attachment.destinationRGBBlendFactor = blendMode == BlendMode::Add ? MTLBlendFactorOne : MTLBlendFactorOneMinusSourceAlpha;
        }

        UInt ResolveTexCoordParam(const String& attrName)
        {
            if (attrName == "a_texCoords2")
                return VertexParam::TexCoord1;

            if (attrName == "a_texCoords3")
                return VertexParam::TexCoord2;

            return VertexParam::TexCoord0;
        }

        void CollectTextureBindings(NSArray<MTLArgument*>* arguments, std::unordered_map<std::string, int>& textureBindings)
        {
            for (MTLArgument* argument in arguments)
            {
                if (argument.type != MTLArgumentTypeTexture)
                    continue;

                textureBindings[std::string(argument.name.UTF8String)] = (int)argument.index;
            }
        }

        void CollectParamBindings(NSArray<MTLArgument*>* arguments, MTLMaterialImpl* materialImpl, bool bindToVertex)
        {
            for (MTLArgument* argument in arguments)
            {
                if (argument.type != MTLArgumentTypeBuffer || argument.index != 2 || argument.bufferDataType != MTLDataTypeStruct)
                    continue;

                materialImpl->materialParamsIndex = (int)argument.index;
                materialImpl->materialParamsSize = std::max(materialImpl->materialParamsSize, argument.bufferDataSize);
                materialImpl->bindParamsToVertex |= bindToVertex;
                materialImpl->bindParamsToFragment |= !bindToVertex;

                for (MTLStructMember* member in argument.bufferStructType.members)
                {
                    NSUInteger memberSize = 0;
                    if (member.dataType == MTLDataTypeArray && member.arrayType)
                        memberSize = member.arrayType.arrayLength*member.arrayType.stride;
                    else if (member.dataType == MTLDataTypeFloat4x4)
                        memberSize = 64;
                    else if (member.dataType == MTLDataTypeFloat4)
                        memberSize = 16;

                    materialImpl->paramBindings[std::string(member.name.UTF8String)] = { member.offset, member.dataType, memberSize };
                }
            }
        }

        void LogPipelineError(const String& message, NSError* error)
        {
            String result = message;
            if (error)
            {
                NSString* description = [error localizedDescription];
                if (description)
                    result += "\n" + ToString(description);
            }

            o2Debug.LogError(result);
        }
    }

    bool Material::PlatformBuild()
    {
        PlatformDestroy();

        if (!mVertexShader || !mVertexShader->mImpl || !mFragmentShader || !mFragmentShader->mImpl)
            return false;

        if (!RenderDevice::device || !RenderDevice::view)
        {
            o2Debug.LogError("Metal device or view is not initialized, can't build material");
            return false;
        }

        if (!mImpl)
            mImpl = mnew MTLMaterialImpl();

        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"o2Material";
        descriptor.vertexFunction = mVertexShader->mImpl->function;
        descriptor.fragmentFunction = mFragmentShader->mImpl->function;
        descriptor.depthAttachmentPixelFormat = RenderDevice::view.depthStencilPixelFormat;

        for (int i = 0; i < mColorAttachmentsCount; i++)
        {
            TextureFormat format = i < mColorAttachmentFormats.Count() ? mColorAttachmentFormats[i] : TextureFormat::R8G8B8A8;
            descriptor.colorAttachments[i].pixelFormat = format == TextureFormat::R16G16B16A16F
                ? MTLPixelFormatRGBA16Float
                : RenderDevice::view.colorPixelFormat;

            SetupBlendState(descriptor.colorAttachments[i], mBlendMode);
        }

        NSError* error = nil;
        MTLAutoreleasedRenderPipelineReflection reflection = nil;
        id<MTLRenderPipelineState> pipelineState = [RenderDevice::device newRenderPipelineStateWithDescriptor:descriptor
                                                                                                      options:MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo
                                                                                                   reflection:&reflection
                                                                                                        error:&error];
        if (!pipelineState)
        {
            LogPipelineError("Failed to build Metal pipeline for material", error);
            PlatformDestroy();
            return false;
        }

        mImpl->pipelineState = pipelineState;
        mImpl->materialParamsIndex = -1;
        mImpl->materialParamsSize = 0;
        mImpl->bindParamsToVertex = false;
        mImpl->bindParamsToFragment = false;
        mImpl->paramBindings.clear();
        mImpl->materialParamsData.clear();

        std::unordered_map<std::string, int> textureBindings;
        if (reflection)
        {
            CollectTextureBindings(reflection.fragmentArguments, textureBindings);
            CollectParamBindings(reflection.vertexArguments, mImpl, true);
            CollectParamBindings(reflection.fragmentArguments, mImpl, false);
        }

        if (mImpl->materialParamsSize > 0)
        {
            mImpl->materialParamsData.resize(mImpl->materialParamsSize);
            std::fill(mImpl->materialParamsData.begin(), mImpl->materialParamsData.end(), Byte{ 0 });
        }

        mProgram = gNextMaterialProgramId++;
        mTransformUniform = 1;
        mTextureUniform = 0;
        mPositionAttribute = 0;
        mColorAttribute = 1;
        mTexCoordsAttribute = 2;
        mNormalAttribute = 5;

        auto primaryTextureBinding = textureBindings.find("u_texture");
        if (primaryTextureBinding != textureBindings.end())
            mTextureUniform = primaryTextureBinding->second;

        mSamplerLocations.Clear();
        for (int samplerIdx = 0; samplerIdx < mSamplers.Count(); samplerIdx++)
        {
            const auto& sampler = mSamplers[samplerIdx];
            SamplerLocation location;
            location.texCoordParam = ResolveTexCoordParam(sampler.texCoordsAttrName);
            location.textureIndex = samplerIdx + 1;

            auto textureBinding = textureBindings.find(ToStdString(sampler.samplerUniformName));
            if (textureBinding != textureBindings.end())
                location.textureIndex = textureBinding->second;

            mSamplerLocations.Add(location);
        }

        mParamUniformLocations.Resize(mParams.Count());
        for (int i = 0; i < mParams.Count(); i++)
        {
            auto binding = mImpl->paramBindings.find(ToStdString(mParams[i]->GetName()));
            mParamUniformLocations[i] = binding != mImpl->paramBindings.end() ? (int)binding->second.offset : -1;
        }

        return true;
    }

    void Material::PlatformDestroy()
    {
        if (mImpl)
        {
            mImpl->pipelineState = nil;
            delete mImpl;
            mImpl = nullptr;
        }

        mProgram = 0;
        mTransformUniform = -1;
        mTextureUniform = -1;
        mPositionAttribute = -1;
        mColorAttribute = -1;
        mTexCoordsAttribute = -1;
        mNormalAttribute = -1;
        mParamUniformLocations.Clear();
        mSamplerLocations.Clear();
        mReady = false;
    }

    void Material::PlatformApplyParams() const
    {
        if (!mImpl || mImpl->materialParamsData.empty())
            return;

        if (mParamUniformLocations.Count() != mParams.Count())
        {
            mParamUniformLocations.Resize(mParams.Count());
            for (int i = 0; i < mParams.Count(); i++)
            {
                auto binding = mImpl->paramBindings.find(ToStdString(mParams[i]->GetName()));
                mParamUniformLocations[i] = binding != mImpl->paramBindings.end() ? (int)binding->second.offset : -1;
            }
        }

        std::fill(mImpl->materialParamsData.begin(), mImpl->materialParamsData.end(), Byte{ 0 });

        for (int i = 0; i < mParams.Count(); i++)
        {
            const auto& param = mParams[i];
            auto bindingIt = mImpl->paramBindings.find(ToStdString(param->GetName()));
            if (bindingIt == mImpl->paramBindings.end())
                continue;

            const auto& binding = bindingIt->second;
            Byte* dst = mImpl->materialParamsData.data() + binding.offset;

            if (auto* floatParam = dynamic_cast<ShaderParamFloat*>(param.Get()))
            {
                if (binding.dataType == MTLDataTypeFloat)
                {
                    float value = floatParam->GetValue();
                    memcpy(dst, &value, sizeof(value));
                }
            }
            else if (auto* vec2Param = dynamic_cast<ShaderParamVec2*>(param.Get()))
            {
                if (binding.dataType == MTLDataTypeFloat2)
                {
                    float value[2] = { vec2Param->GetValue().x, vec2Param->GetValue().y };
                    memcpy(dst, value, sizeof(value));
                }
            }
            else if (auto* colorParam = dynamic_cast<ShaderParamColor*>(param.Get()))
            {
                if (binding.dataType == MTLDataTypeFloat4)
                {
                    float value[4] = { colorParam->GetValue().RF(), colorParam->GetValue().GF(),
                                       colorParam->GetValue().BF(), colorParam->GetValue().AF() };
                    memcpy(dst, value, sizeof(value));
                }
            }
            else if (auto* intParam = dynamic_cast<ShaderParamInt*>(param.Get()))
            {
                if (binding.dataType == MTLDataTypeInt)
                {
                    int value = intParam->GetValue();
                    memcpy(dst, &value, sizeof(value));
                }
            }
            else if (auto* floatVectorParam = dynamic_cast<ShaderParamFloatVector*>(param.Get()))
            {
                if ((binding.dataType == MTLDataTypeArray || binding.dataType == MTLDataTypeFloat4x4 ||
                     binding.dataType == MTLDataTypeFloat4) && binding.size > 0)
                {
                    const auto& values = floatVectorParam->GetValue();
                    size_t bytes = std::min((size_t)binding.size, values.Count()*sizeof(float));
                    if (bytes > 0)
                        memcpy(dst, values.data(), bytes);
                }
            }
        }
    }
}

#endif // PLATFORM_IOS
