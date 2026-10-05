#pragma once

#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Render/VectorGraphics/VectorMesh.h"
#include "o2/Utils/Bitmap/Bitmap.h"

namespace o2
{
    // Software rasterizer of vector mesh, sampling and blending as the render does; image Y is down
    namespace VectorRasterizer
    {
        // Draws mesh over the bitmap content; mesh positions are multiplied by scale and shifted by offset pixels
        void Rasterize(const VectorMesh& mesh, Bitmap& bitmap, float scale = 1.0f, const Vec2F& offset = Vec2F());

        // Returns bitmap of mesh size multiplied by scale, cleared with background, with mesh drawn over
        Ref<Bitmap> Rasterize(const VectorMesh& mesh, float scale = 1.0f,
                              const Color4& background = Color4(0, 0, 0, 0));

        // Tessellates image for the scale and returns it drawn over the background
        Ref<Bitmap> Rasterize(const VectorImage& image, float scale = 1.0f,
                              const Color4& background = Color4(0, 0, 0, 0), bool antialiasing = true);

        // Converts premultiplied RGB of bitmap drawn over transparent background into straight alpha
        void Unpremultiply(Bitmap& bitmap);

        // Returns pixel of bitmap by coordinates from the left top corner
        Color4 GetPixel(const Bitmap& bitmap, int x, int y);
    }
}
