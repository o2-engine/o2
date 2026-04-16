#include "o2/stdafx.h"

#ifdef PLATFORM_WASM

#include "o2/Render/Shader.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    bool Shader::PlatformCompile(const String& source, Type type)
    {
        GLenum glType = (type == Type::Vertex) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
        GLuint shader = glCreateShader(glType);

        if (shader)
        {
            // GLSL ES requires fragment shaders to declare default float precision.
            String patched = (type == Type::Fragment && !source.Contains("precision"))
                ? (String)"precision mediump float;\n" + source
                : source;
            const char* src = patched.Data();
            glShaderSource(shader, 1, &src, NULL);
            glCompileShader(shader);

            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

            if (!compiled)
            {
                GLint infoLen = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

                if (infoLen > 0)
                {
                    char* infoLog = (char*)malloc(sizeof(char) * infoLen);
                    glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
                    o2Debug.LogError((String)"Error compiling shader " + mFileName + ":\n" + infoLog);
                    free(infoLog);
                }

                glDeleteShader(shader);
                shader = 0;
            }
        }

        mHandle = shader;
        return shader != 0;
    }

    void Shader::PlatformDestroy()
    {
        if (mHandle)
        {
            glDeleteShader(mHandle);
            mHandle = 0;
        }

        mReady = false;
    }
}

#endif // PLATFORM_WASM
