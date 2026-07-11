#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include "o2/Render/Material.h"
#include "o2/Utils/Debug/Debug.h"

#include <cstring>

namespace o2
{
    namespace
    {
        // Finds the declared type and array size of an active uniform by name ("name" or "name[0]")
        void QueryUniformInfo(GLuint program, const char* name, GLenum& outType, GLint& outSize)
        {
            outType = 0;
            outSize = 0;

            GLint count = 0;
            glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
            for (GLint i = 0; i < count; i++)
            {
                char uniformName[256];
                GLsizei nameLength = 0;
                GLint uniformSize = 0;
                GLenum uniformType = 0;
                glGetActiveUniform(program, (GLuint)i, sizeof(uniformName), &nameLength, &uniformSize, &uniformType, uniformName);

                if (nameLength > 3 && strcmp(uniformName + nameLength - 3, "[0]") == 0)
                    uniformName[nameLength - 3] = '\0';

                if (strcmp(uniformName, name) == 0)
                {
                    outType = uniformType;
                    outSize = uniformSize;
                    return;
                }
            }
        }

        void ResolveParamUniforms(GLuint program, const Vector<Ref<IShaderParam>>& params,
                                  Vector<GLint>& locations, Vector<GLenum>& types, Vector<GLint>& sizes)
        {
            locations.Resize(params.Count());
            types.Resize(params.Count());
            sizes.Resize(params.Count());
            for (int i = 0; i < params.Count(); i++)
            {
                locations[i] = -1;
                types[i] = 0;
                sizes[i] = 0;

                if (!params[i])
                    continue;

                locations[i] = glGetUniformLocation(program, params[i]->GetName().Data());
                if (locations[i] >= 0)
                    QueryUniformInfo(program, params[i]->GetName().Data(), types[i], sizes[i]);
            }
        }
    }

    bool Material::PlatformBuild()
    {
        GLuint program = glCreateProgram();
        if (program)
        {
            glAttachShader(program, mVertexShader->mHandle);
            glAttachShader(program, mFragmentShader->mHandle);

            GLint linkStatus;
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

            if (!linkStatus)
            {
                GLint infoLen = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);

                if (infoLen > 0)
                {
                    char* infoLog = (char*)malloc(sizeof(char) * infoLen);
                    glGetProgramInfoLog(program, infoLen, NULL, infoLog);
                    o2Debug.LogError((String)"Error linking material shader program:\n" + infoLog);
                    free(infoLog);
                }

                glDeleteProgram(program);
                return false;
            }
        }
        else
        {
            return false;
        }

        mProgram = program;
        mTransformUniform = glGetUniformLocation(program, "u_transformMatrix");
        mTextureUniform = glGetUniformLocation(program, "u_texture");
        mPositionAttribute = glGetAttribLocation(program, "a_position");
        mColorAttribute = glGetAttribLocation(program, "a_color");
        mTexCoordsAttribute = glGetAttribLocation(program, "a_texCoords");
        mNormalAttribute = glGetAttribLocation(program, "a_normal");
        mBoneIndicesAttribute = glGetAttribLocation(program, "a_boneIndices");
        mBoneWeightsAttribute = glGetAttribLocation(program, "a_boneWeights");

        ResolveParamUniforms(program, mParams, mParamUniformLocations, mParamUniformTypes, mParamUniformSizes);

        mSamplerLocations.Resize(mSamplers.Count());
        for (int i = 0; i < mSamplers.Count(); i++)
        {
            mSamplerLocations[i].samplerUniform = glGetUniformLocation(program, mSamplers[i].samplerUniformName.Data());
            mSamplerLocations[i].texCoordsAttribute = glGetAttribLocation(program, mSamplers[i].texCoordsAttrName.Data());
        }

        return true;
    }

    void Material::PlatformDestroy()
    {
        if (mProgram)
        {
            glDeleteProgram(mProgram);
            mProgram = 0;
        }

        mParamUniformLocations.Clear();
        mParamUniformTypes.Clear();
        mParamUniformSizes.Clear();
        mSamplerLocations.Clear();
        mReady = false;
    }

    void Material::PlatformApplyParams() const
    {
        if (mParamUniformLocations.Count() != mParams.Count())
            ResolveParamUniforms(mProgram, mParams, mParamUniformLocations, mParamUniformTypes, mParamUniformSizes);

        for (int i = 0; i < mParams.Count(); i++)
        {
            GLint loc = mParamUniformLocations[i];
            if (loc < 0)
                continue;

            const auto& param = mParams[i];
            if (auto* fp = dynamic_cast<ShaderParamFloat*>(param.Get()))
                glUniform1f(loc, fp->GetValue());
            else if (auto* v2p = dynamic_cast<ShaderParamVec2*>(param.Get()))
                glUniform2f(loc, v2p->GetValue().x, v2p->GetValue().y);
            else if (auto* cp = dynamic_cast<ShaderParamColor*>(param.Get()))
                glUniform4f(loc, cp->GetValue().RF(), cp->GetValue().GF(), cp->GetValue().BF(), cp->GetValue().AF());
            else if (auto* ip = dynamic_cast<ShaderParamInt*>(param.Get()))
                glUniform1i(loc, ip->GetValue());
            else if (auto* fvp = dynamic_cast<ShaderParamFloatVector*>(param.Get()))
            {
                const auto& values = fvp->GetValue();
                if (values.IsEmpty())
                    continue;

                GLenum type = i < mParamUniformTypes.Count() ? mParamUniformTypes[i] : GL_FLOAT_VEC4;
                GLint declaredCount = Math::Max(i < mParamUniformSizes.Count() ? mParamUniformSizes[i] : 1, 1);

                if (type == GL_FLOAT_MAT4)
                    glUniformMatrix4fv(loc, Math::Min(values.Count()/16, declaredCount), GL_FALSE, values.data());
                else if (type == GL_FLOAT)
                    glUniform1fv(loc, Math::Min(values.Count(), declaredCount), values.data());
                else
                    glUniform4fv(loc, Math::Min(values.Count()/4, declaredCount), values.data());
            }
        }

        GL_CHECK_ERROR();
    }
}

#endif // PLATFORM_ANDROID
