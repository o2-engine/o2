#pragma once

#ifdef PLATFORM_WINDOWS

#include "o2/Render/Windows/OpenGL.h"

namespace o2
{
    // ----------------------------------------------------------
    // Platform-specific (Windows/OpenGL) shader data and handles
    // ----------------------------------------------------------
    class ShaderBase
    {
        friend class Render;
        friend class Material;

    protected:
        GLuint mHandle = 0; // OpenGL shader object handle
    };
}

#endif // PLATFORM_WINDOWS
