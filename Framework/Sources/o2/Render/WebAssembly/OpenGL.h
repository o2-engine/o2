#pragma once

#ifdef PLATFORM_WASM

#include <GLES3/gl3.h>

namespace o2
{
    class LogStream;
}

const char* GetGLErrorDesc(GLenum errorId);

void glCheckError(const char* filename = nullptr, unsigned int line = 0);

#if RENDER_DEBUG
#    define GL_CHECK_ERROR() glCheckError(__FILE__, __LINE__);
#else
#    define GL_CHECK_ERROR()
#endif

#endif // PLATFORM_WASM
