#include "o2/stdafx.h"

#ifdef PLATFORM_WINDOWS

#include "o2/Render/Material.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
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

        return true;
    }

    void Material::PlatformDestroy()
    {
        if (mProgram)
        {
            glDeleteProgram(mProgram);
            mProgram = 0;
        }

        mReady = false;
    }

    void Material::PlatformApplyParams() const
    {
        for (const auto& param : mParams)
        {
            GLint loc = glGetUniformLocation(mProgram, param->GetName().Data());
            if (loc < 0)
                continue;

            if (auto* fp = dynamic_cast<ShaderParamFloat*>(param.Get()))
                glUniform1f(loc, fp->GetValue());
            else if (auto* v2p = dynamic_cast<ShaderParamVec2*>(param.Get()))
                glUniform2f(loc, v2p->GetValue().x, v2p->GetValue().y);
            else if (auto* cp = dynamic_cast<ShaderParamColor*>(param.Get()))
                glUniform4f(loc, cp->GetValue().RF(), cp->GetValue().GF(), cp->GetValue().BF(), cp->GetValue().AF());
            else if (auto* ip = dynamic_cast<ShaderParamInt*>(param.Get()))
                glUniform1i(loc, ip->GetValue());
        }

        GL_CHECK_ERROR();
    }
}

#endif // PLATFORM_WINDOWS
