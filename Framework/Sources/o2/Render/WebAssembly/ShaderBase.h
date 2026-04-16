#pragma once

#ifdef PLATFORM_WASM

#include "o2/Render/WebAssembly/OpenGL.h"

namespace o2
{
    // --------------------------------------------------------------
    // Platform-specific (WebAssembly/WebGL) shader data and handles
    // --------------------------------------------------------------
    class ShaderBase
    {
        friend class Render;
        friend class Material;

    protected:
        GLuint mHandle = 0; // OpenGL shader object handle
    };
}

#endif // PLATFORM_WASM
