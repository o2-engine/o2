#pragma once

#ifdef PLATFORM_ANDROID

#include "o2/Render/Android/OpenGL.h"

namespace o2
{
    class TextureBase
    {
        friend class Render;
        friend class VectorFont;

    protected:
        GLuint mHandle = 0;      // Texture handle
        GLuint mFrameBuffer = 0; // Frame buffer for rendering into texture
    };
}

#endif // PLATFORM_ANDROID
