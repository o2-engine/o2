#pragma once

#ifdef PLATFORM_WASM

#include "o2/Render/WebAssembly/OpenGL.h"

namespace o2
{
    class TextureBase
    {
        friend class Render;
        friend class VectorFont;
        friend class WasmVideoDecoder;

    protected:
        GLuint mHandle = 0;            // WebGL texture handle
        GLuint mFrameBuffer = 0;       // Frame buffer for rendering into texture
        GLuint mDepthRenderBuffer = 0; // Depth attachment for render targets
    };
}

#endif // PLATFORM_WASM
