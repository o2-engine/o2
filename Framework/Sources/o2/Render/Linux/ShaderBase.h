#pragma once

#ifdef PLATFORM_LINUX

#include "o2/Render/Linux/OpenGL.h"

namespace o2
{
    class ShaderBase
    {
        friend class Render;
        friend class Material;

    protected:
        GLuint mHandle = 0;
    };
}

#endif // PLATFORM_LINUX
