#pragma once

#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Render/VectorGraphics/VectorMesh.h"

namespace o2
{
    // ---------------------------------
    // Vector image tessellation options
    // ---------------------------------
    struct VectorTessellationParams
    {
        Vec2F pixelScale = Vec2F(1, 1); // Screen pixels per image unit by axes; the fringe is 1 screen pixel wide
        float curveTolerance = 0.05f;   // Maximum deviation of flattened curves, in screen pixels
        bool  antialiasing = true;      // Builds alpha fringe along edges; hard edges otherwise
        bool  pixelSnapped = true;      // Image is drawn from a whole screen pixel: edges on pixel bounds stay hard
        float simplification = 0.5f;    // Color levels the triangles may be off the removed vertices; 0 keeps them all
    };

    // Converter of vector image into triangles with per-vertex colors, anti-aliased by alpha fringe geometry
    namespace VectorTessellator
    {
        // Builds mesh of the whole image, shapes in drawing order
        void Tessellate(const VectorImage& image, VectorMesh& mesh,
                        const VectorTessellationParams& params = VectorTessellationParams());

        // Appends triangles of shape fill and stroke to the mesh
        void TessellateShape(const VectorShape& shape, VectorMesh& mesh,
                             const VectorTessellationParams& params = VectorTessellationParams());

        // Returns gradient stops without the ones that are linear interpolation of neighbors within tolerance
        Vector<VectorGradientStop> SimplifyStops(const Vector<VectorGradientStop>& stops, float tolerance = 1.5f);
    }
}
