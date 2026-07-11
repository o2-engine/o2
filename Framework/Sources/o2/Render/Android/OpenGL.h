#pragma once

#ifdef PLATFORM_ANDROID

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// ES 3.0 enums missing from the ES 2 headers
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 0x8CE1
#endif
#ifndef GL_COLOR_ATTACHMENT2
#define GL_COLOR_ATTACHMENT2 0x8CE2
#endif
#ifndef GL_COLOR_ATTACHMENT3
#define GL_COLOR_ATTACHMENT3 0x8CE3
#endif

namespace o2
{
    class LogStream;

    // ------------------------------------------------------------------------------
    // OpenGL ES 3 functions and capabilities, resolved at runtime: the app links to
    // GLESv2 and stays loadable on ES 2 only devices, ES 3 features light up when
    // the Java-side surface provides an ES 3 context
    // ------------------------------------------------------------------------------
    namespace GLES3
    {
        typedef void (GL_APIENTRY* PFNGLDRAWBUFFERS)(GLsizei n, const GLenum* bufs);

        extern bool available;                // True when the current context is OpenGL ES 3.0+
        extern PFNGLDRAWBUFFERS glDrawBuffers; // Draw buffers entry point, null until initialized

        // Detects context version and loads ES 3 entry points via eglGetProcAddress
        void Initialize();
    }
}

const char* GetGLErrorDesc(GLenum errorId);

void glCheckError(const char* filename = nullptr, unsigned int line = 0);

#if RENDER_DEBUG
#    define GL_CHECK_ERROR() glCheckError(__FILE__, __LINE__);
#else
#    define GL_CHECK_ERROR()
#endif

#endif // PLATFORM_ANDROID
