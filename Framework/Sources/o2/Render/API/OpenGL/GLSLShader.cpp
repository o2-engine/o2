#include "GLSLShader.h"

#include "OpenGLUtils.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace
{
    constexpr const char* VS_EXT{ "vs" };
    constexpr const char* FS_EXT{ "fs" };

    o2::ShaderType GetShaderTypeByExt(const o2::String& fileName)
    {
        const auto ext = o2FileSystem.GetFileExtension(fileName);
        if (ext == VS_EXT)
        {
            return o2::ShaderType::Vertex;
        }
        if (ext == FS_EXT)
        {
            return o2::ShaderType::Fragment;
        }
        Assert(false, "Unknown extension for shader program");
        return o2::ShaderType::Vertex;
    }

    // get OpenGL shader type by o2::ShaderType
    GLenum GetGLShaderType(o2::ShaderType type)
    {
        switch (type)
        {
        case o2::ShaderType::Vertex:
            return GL_VERTEX_SHADER;
        case o2::ShaderType::Fragment:
            return GL_FRAGMENT_SHADER;
        }

        Assert(false, "Unknown shader type");
        return 0;
    }
}

namespace o2
{
    bool GLSLShader::LoadFromFile(const String& fileName)
    {
        mType = GetShaderTypeByExt(fileName);
        O2_GL_CHECK_ERROR(mId = glCreateShader(GetGLShaderType(mType)));

        mSource = o2FileSystem.ReadFile(fileName);
        if (mSource.IsEmpty())
        {
            o2Debug.LogError("Shader (%s) doesn't found!", fileName.c_str());
            return false;
        }

        const char* source = mSource.c_str();
        O2_GL_CHECK_ERROR(glShaderSource(mId, 1, &source, nullptr));
        O2_GL_CHECK_ERROR(glCompileShader(mId));

        GLint compilationStatus = 0;
        O2_GL_CHECK_ERROR(glGetShaderiv(mId, GL_COMPILE_STATUS, &compilationStatus));

        if (!compilationStatus)
        {
            GLint infoLen = 0;
            O2_GL_CHECK_ERROR(glGetShaderiv(mId, GL_INFO_LOG_LENGTH, &infoLen));
            infoLen++; // add the null terminator
            if (infoLen > 0)
            {
                char* infoLog = (char*)malloc(sizeof(char) * infoLen);
                O2_GL_CHECK_ERROR(glGetShaderInfoLog(mId, infoLen, nullptr, infoLog));
                infoLog[infoLen - 1] = '\0';
                o2Debug.LogError((String)"Error compilation shader:\n" + infoLog);
                free(infoLog);
            }

            O2_GL_CHECK_ERROR(glDeleteShader(mId));
            mId = 0;
        }

        return mId != 0;
    }
}
// --- META ---

ENUM_META(o2::ShaderType)
{
    ENUM_ENTRY(Fragment);
    ENUM_ENTRY(Vertex);
}
END_ENUM_META;
// --- END META ---
