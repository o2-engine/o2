#pragma once

#if RENDER_DEBUG
#include "o2/Utils/Debug/Debug.h"
#    define O2_GL_CHECK_ERROR(glFunc) \
{ \
    glFunc; \
    GLenum errorCode = glGetError(); \
    if (errorCode != GL_NO_ERROR) { \
        o2::String errorMsg; \
        switch (errorCode) { \
            case GL_INVALID_ENUM:                    errorMsg = "GL_INVALID_ENUM";                   break; \
            case GL_INVALID_VALUE:                   errorMsg = "GL_INVALID_VALUE";                  break; \
            case GL_INVALID_OPERATION:               errorMsg = "GL_INVALID_OPERATION";              break; \
            case GL_STACK_OVERFLOW:                  errorMsg = "GL_STACK_OVERFLOW";                 break; \
            case GL_STACK_UNDERFLOW:                 errorMsg = "GL_STACK_UNDERFLOW";                break; \
            case GL_OUT_OF_MEMORY:                   errorMsg = "GL_OUT_OF_MEMORY";                  break; \
            case GL_INVALID_FRAMEBUFFER_OPERATION:   errorMsg = "GL_INVALID_FRAMEBUFFER_OPERATION";  break; \
            default:                                                                                 break; \
        } \
        o2::String funcName = #glFunc; \
        funcName = funcName.substr(0, funcName.find('(')); \
        o2Debug.LogError("OpenGL: %s failed with '%s' in %s at %s (%d)", \
            funcName.c_str(), errorMsg, __FUNCTION__, __FILE__, __LINE__); \
    } \
}
#else
#    define O2_GL_CHECK_ERROR(glFunc) { glFunc; }
#endif
