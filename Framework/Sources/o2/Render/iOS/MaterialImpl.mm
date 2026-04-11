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
                    materialImpl->paramBindings[std::string(member.name.UTF8String)] = { member.offset, member.dataType };
            }
        }

        void WriteFloatParam(const IShaderParam& param, Byte* dst)
        {
            float value = static_cast<const ShaderParamFloat&>(param).GetValue();
            memcpy(dst, &value, sizeof(value));
        }

        void WriteVec2Param(const IShaderParam& param, Byte* dst)
        {
            const auto& value = static_cast<const ShaderParamVec2&>(param).GetValue();
            float vec[2] = { value.x, value.y };
            memcpy(dst, vec, sizeof(vec));
        }

        void WriteColorParam(const IShaderParam& param, Byte* dst)
        {
            const auto& value = static_cast<const ShaderParamColor&>(param).GetValue();
            float color[4] = { value.RF(), value.GF(), value.BF(), value.AF() };
            memcpy(dst, color, sizeof(color));
        }

        void WriteIntParam(const IShaderParam& param, Byte* dst)
        {
            int value = static_cast<const ShaderParamInt&>(param).GetValue();
            memcpy(dst, &value, sizeof(value));
        }

        MetalParamWriter::WriteFunction ResolveParamWriteFunction(const Ref<IShaderParam>& param, MTLDataType dataType)
        {
            if (!param)
                return nullptr;

            if (dataType == MTLDataTypeFloat && dynamic_cast<ShaderParamFloat*>(param.Get()))
                return &WriteFloatParam;

            if (dataType == MTLDataTypeFloat2 && dynamic_cast<ShaderParamVec2*>(param.Get()))
                return &WriteVec2Param;

            if (dataType == MTLDataTypeFloat4 && dynamic_cast<ShaderParamColor*>(param.Get()))
                return &WriteColorParam;

            if (dataType == MTLDataTypeInt && dynamic_cast<ShaderParamInt*>(param.Get()))
                return &WriteIntParam;

            return nullptr;
        }

        bool IsParamWriterCacheValid(const Vector<Ref<IShaderParam>>& params, const MTLMaterialImpl* materialImpl)
        {
            if (!materialImpl || materialImpl->cachedParamNames.Count() != params.Count() || materialImpl->cachedParamTypes.Count() != params.Count())
                return false;

            for (int i = 0; i < params.Count(); i++)
            {
                const auto& param = params[i];
                const Type* paramType = param ? &param->GetType() : nullptr;
                const String& paramName = param ? param->GetName() : String::empty;
                if (materialImpl->cachedParamTypes[i] != paramType || materialImpl->cachedParamNames[i] != paramName)
                    return false;
            }

            return true;
        }

        void RefreshParamWriterCache(const Vector<Ref<IShaderParam>>& params, MTLMaterialImpl* materialImpl, Vector<int>& paramUniformLocations)
        {
            materialImpl->cachedParamNames.Resize(params.Count());
            materialImpl->cachedParamTypes.Resize(params.Count());
            materialImpl->paramWriters.clear();

            paramUniformLocations.Resize(params.Count());
            for (int i = 0; i < params.Count(); i++)
            {
                const auto& param = params[i];
                materialImpl->cachedParamNames[i] = param ? param->GetName() : String();
                materialImpl->cachedParamTypes[i] = param ? &param->GetType() : nullptr;

                if (!param)
                {
                    paramUniformLocations[i] = -1;
                    continue;
                }

                auto binding = materialImpl->paramBindings.find(ToStdString(param->GetName()));
                if (binding == materialImpl->paramBindings.end())
                {
                    paramUniformLocations[i] = -1;
                    continue;
                }

                paramUniformLocations[i] = (int)binding->second.offset;

                auto writeFunction = ResolveParamWriteFunction(param, binding->second.dataType);
                if (!writeFunction)
                    continue;

                materialImpl->paramWriters.push_back({ i, binding->second.offset, writeFunction });
            }
        }

        void LogPipelineError(const String& message, NSError* error)
        {
            String result = message;
            if (error)
            {
                NSString* description = [error localizedDescription];
                if (description)
                    result += "\n" + String(description.UTF8String);
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
        descriptor.colorAttachments[0].pixelFormat = RenderDevice::view.colorPixelFormat;
        SetupBlendState(descriptor.colorAttachments[0], mBlendMode);

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

        RefreshParamWriterCache(mParams, mImpl, mParamUniformLocations);

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

        if (!IsParamWriterCacheValid(mParams, mImpl))
            RefreshParamWriterCache(mParams, mImpl, mParamUniformLocations);

        std::fill(mImpl->materialParamsData.begin(), mImpl->materialParamsData.end(), Byte{ 0 });

        for (const auto& paramWriter : mImpl->paramWriters)
        {
            if (paramWriter.paramIndex < 0 || paramWriter.paramIndex >= mParams.Count())
                continue;

            const auto& param = mParams[paramWriter.paramIndex];
            if (!param || !paramWriter.write)
                continue;

            paramWriter.write(*param, mImpl->materialParamsData.data() + paramWriter.offset);
        }
    }
}

#endif // PLATFORM_IOS