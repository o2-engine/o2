#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include "o2/Render/Android/OpenGL.h"
#include "o2/Utils/Debug/Log/LogStream.h"

const char* GetGLErrorDesc(GLenum errorId)
{
    switch (errorId)
    {
        case GL_NO_ERROR:                      return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
        default:                               return "Unknown GL error";
    }
}

void glCheckError(const char* filename /*= nullptr*/, unsigned int line /*= 0*/)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        o2::String msg = "GL error: ";
        msg += GetGLErrorDesc(err);
        if (filename)
        {
            msg += " at ";
            msg += filename;
            msg += ":";
            msg += o2::String((int)line);
        }
        o2Debug.LogError(msg);
    }
}

#endif // PLATFORM_ANDROID
