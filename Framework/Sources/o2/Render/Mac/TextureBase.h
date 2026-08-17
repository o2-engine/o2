#pragma once

#ifdef PLATFORM_MAC

namespace o2
{
    struct MTLTextureImpl;
    
    class TextureBase
    {
        friend class Render;
        friend class VectorFont;

    public:
        // Platform texture payload accessor for external renderer integrations
        // (cocos2d draws straight into o2 render targets in the editor)
        MTLTextureImpl* GetPlatformTextureImpl() const { return mImpl; }

    protected:
        TextureBase();

    protected:
        MTLTextureImpl* mImpl;
    };
}

#endif // PLATFORM_MAC
