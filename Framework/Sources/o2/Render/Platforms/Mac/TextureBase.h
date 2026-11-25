#pragma once

#ifdef PLATFORM_MAC

namespace o2
{
    struct MTLTextureImpl;
    
    class TextureBase
    {
        friend class Render;
        friend class VectorFont;

    protected:
        TextureBase();
        
    protected:
        MTLTextureImpl* mImpl;
    };
}

#endif // PLATFORM_MAC
