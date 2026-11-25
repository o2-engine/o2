#pragma once

#if defined(PLATFORM_LINUX) && !defined(O2_RENDER_GLES2)

#include "o2/Render/Platforms/Linux/OpenGL.h"

namespace o2
{
    class TextureBase
    {
        friend class Render;
        friend class VectorFont;

    protected:
        GLuint mHandle;      // Texture handle
        GLuint mFrameBuffer; // Frame buffer for rendering into texture
    };
}

#endif // PLATFORM_LINUX
