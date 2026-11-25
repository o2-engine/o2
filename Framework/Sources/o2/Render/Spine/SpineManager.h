#pragma once

#include "o2/Utils/Singleton.h"
#include "o2/Render/Resources/TextureRef.h"

#include "spine/TextureLoader.h"
#include "spine/Atlas.h"

namespace o2
{
    // ---------------------
    // Spine textures loader
    // ---------------------
    class SpineTextureLoader : public spine::TextureLoader
    {
    public:
        // Called when the atlas loads the texture of a page.
        void load(spine::AtlasPage& page, const spine::String& path) override;

        // Called when the atlas is disposed and itself disposes its atlas pages.
        void unload(void* texture) override;

    protected:
        Map<void*, TextureRef> mUsedTextures; // List of used textures
    };

    // ------------------------------------------------------------------------------
    // Spine manager. Initializes spine subsystems and contains spine textures loader
    // ------------------------------------------------------------------------------
    class SpineManager : public Singleton<SpineManager>
    {
    public:
        SpineTextureLoader textureLoader; // Spine textures loader

    public:
        // Constructor. Initializes spine subsystems
        SpineManager(RefCounter* refCounter);
    };
}
