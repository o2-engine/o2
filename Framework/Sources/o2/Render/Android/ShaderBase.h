#pragma once

#ifdef PLATFORM_ANDROID

#include "o2/Render/Android/OpenGL.h"

namespace o2
{
    // --------------------------------------------------------------
    // Platform-specific (Android/GLES2) shader data and handles
    // --------------------------------------------------------------
    class ShaderBase
    {
        friend class Render;
        friend class Material;

    protected:
        GLuint mHandle = 0; // OpenGL shader object handle
    };
}

#endif // PLATFORM_ANDROID
