#pragma once

#include "o2/Assets/AssetRef.h"
#include "o2/Render/Resources/TextureRef.h"
#include "o2/Utils/Math/Rect.h"

using namespace o2;

namespace o2
{
    class ActorAsset;
    class Actor;
}

namespace Editor
{
    // ------------------------------------------------------------------------------------------------------
    // Thumbnails of the prototypes: the 2D content of an actor asset, its widgets and its flat components,
    // rendered once into a texture and kept while the asset file stays the same. Rendering binds a render
    // target, so the thumbnail is made while a frame draws
    // ------------------------------------------------------------------------------------------------------
    namespace PrototypePreview
    {
        // Returns the thumbnail of the prototype if it is made for the current file, empty otherwise
        TextureRef FindThumbnail(const AssetRef<ActorAsset>& asset);

        // Returns the thumbnail of the prototype, rendering it when the cache has none for the current file;
        // empty when the prototype has no 2D content. Call it while a frame draws
        TextureRef GetThumbnail(const AssetRef<ActorAsset>& asset);

        // Returns is the prototype's thumbnail missing from the cache and not known to be empty
        bool IsThumbnailPending(const AssetRef<ActorAsset>& asset);

        // Returns the bounds of the 2D content of the actor and its children, in its world space; false without it
        bool GetContentBounds(const Ref<Actor>& actor, RectF& bounds);

        // Forgets all the thumbnails
        void ClearCache();

        // Returns how many thumbnails were rendered since the start, to check the cache
        int GetRenderedCount();

        constexpr int thumbnailSize = 128; // Longest side of a thumbnail, in pixels
    }
}
