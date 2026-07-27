#pragma once

#ifdef PLATFORM_ANDROID

#include "o2/Render/Android/OpenGL.h"

namespace o2
{
    class TextureBase
    {
        friend class AndroidVideoDecoder;
        friend class Render;
        friend class VectorFont;

    protected:
        GLuint mHandle = 0;            // Texture handle
        GLuint mFrameBuffer = 0;       // Frame buffer for rendering into texture
        GLuint mDepthRenderBuffer = 0; // Depth attachment for render targets
    };
}

#endif // PLATFORM_ANDROID
