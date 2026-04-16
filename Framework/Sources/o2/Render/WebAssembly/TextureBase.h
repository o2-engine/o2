#pragma once

#ifdef PLATFORM_WASM

#include "o2/Render/WebAssembly/OpenGL.h"

namespace o2
{
    class TextureBase
    {
        friend class Render;
        friend class VectorFont;

    protected:
        GLuint mHandle = 0;      // WebGL texture handle
        GLuint mFrameBuffer = 0; // Frame buffer for rendering into texture
    };
}

#endif // PLATFORM_WASM
