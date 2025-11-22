#include "GPUProgram.h"

#include "OpenGLUtils.h"

namespace o2
{
    GPUProgram::GPUProgram(Ref<GLSLShader> vertexShader, Ref<GLSLShader> fragmentShader)
        : mId(0), mVertexShader(vertexShader), mFragmentShader(fragmentShader)
    {
    }

    bool GPUProgram::Link()
    {
        Release();

        if (!mVertexShader || !mFragmentShader)
        {
            return false;
        }

        O2_GL_CHECK_ERROR(mId = glCreateProgram());
        O2_GL_CHECK_ERROR(glAttachShader(mId, mVertexShader.Lock()->GetId()))
        O2_GL_CHECK_ERROR(glAttachShader(mId, mFragmentShader.Lock()->GetId()))
        O2_GL_CHECK_ERROR(glLinkProgram(mId));

        GLint linkStatus;
        O2_GL_CHECK_ERROR(glGetProgramiv(mId, GL_LINK_STATUS, &linkStatus));

        if (!linkStatus)
        {
            GLint infoLen = 0;
            O2_GL_CHECK_ERROR(glGetProgramiv(mId, GL_INFO_LOG_LENGTH, &infoLen));

            if (infoLen > 0)
            {
                char* infoLog = (char*)malloc(sizeof(char) * infoLen);
                O2_GL_CHECK_ERROR(glGetProgramInfoLog(mId, infoLen, nullptr, infoLog));
                o2Debug.LogError((String)"Error linking shader:\n" + infoLog);
                free(infoLog);
            }

            O2_GL_CHECK_ERROR(glDeleteProgram(mId));
            mId = 0;
        }

        if (!mId)
        {
            return false;
        }

        O2_GL_CHECK_ERROR(glUseProgram(mId));

        UpdateParameters();
        return true;
    }

    void GPUProgram::Release()
    {
        // TODO: check using
        glDeleteProgram(mId);

        mId = 0;
        // mVertexShader = nullptr;
        // mFragmentShader = nullptr;
        mParameters.clear();
    }

    bool GPUProgram::HasParameter(const String& name) const
    {
        return mParameters.contains(name);
    }

    GLuint GPUProgram::GetParameter(const String& name) const
    {
        Assert(HasParameter(name), "Parameter '" + name + "' not found");
        return mParameters.at(name);
    }

    void GPUProgram::UpdateParameters()
    {
        const int MAX_NAME_LENGTH = 256;
        char nameBuffer[MAX_NAME_LENGTH];
        int attribsCount = 0, uniformLength, elementCount, nameLength;
        GLenum type;

        glGetProgramiv(mId, GL_ACTIVE_ATTRIBUTES, &attribsCount);
        for (int i = 0; i < attribsCount; ++i)
        {
            glGetActiveAttrib(mId, i, MAX_NAME_LENGTH, &nameLength, &elementCount, &type, nameBuffer);
            const auto name = String(nameBuffer);

            int location = glGetAttribLocation(mId, name.c_str());
            o2Debug.Log("Found vertex attribute %s location %d", name.c_str(), location);
            mParameters[name] = location;
        }

        glGetProgramiv(mId, GL_ACTIVE_UNIFORMS, &uniformLength);
        for (int i = 0; i < uniformLength; ++i)
        {
            glGetActiveUniform(mId, i, MAX_NAME_LENGTH, &nameLength, &elementCount, &type, nameBuffer);
            const auto name = String(nameBuffer);
            int location = glGetUniformLocation(mId, nameBuffer);
            o2Debug.Log("Found uniform attribute %s location %d", nameBuffer, location);
            mParameters[name] = location;
        }
    }
}
