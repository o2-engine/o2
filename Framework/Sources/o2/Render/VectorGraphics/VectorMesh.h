#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Border.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // ---------------------------------------------------------------------------------------------
    // Triangles of tessellated vector image. Positions are in image space: origin is the left top
    // corner, Y axis is down, units are image units. Colors are straight alpha, packed as Color4::ABGR
    // ---------------------------------------------------------------------------------------------
    struct VectorMesh
    {
        Vector<Vec2F>       positions;                // Vertices positions
        Vector<Color32Bit>  colors;                   // Vertices colors, one per position
        Vector<VertexIndex> indexes;                  // Triangles, 3 indexes per triangle, in drawing order
        Vec2F               size;                     // Image size
        Vec2F               pixelScale = Vec2F(1, 1); // Screen pixels per image unit the fringe is built for

    public:
        // Removes all geometry
        void Clear();

        UInt GetTrianglesCount() const;

        // Returns bounds of positions, the fringe goes out of the image rectangle; false when mesh is empty
        bool GetBounds(Vec2F& min, Vec2F& max) const;

        // Splits triangles along the vertical or horizontal line, interpolating colors
        void SplitByLine(bool vertical, float coordinate);

        // Splits triangles along the slice lines, after that MapSlicedPoint can be applied per vertex
        void SplitBySlices(const BorderF& borders);

        // Fills one vertex per position: transform is the basis of the image rectangle, origin in its left bottom
        void FillVertices(Vertex* vertices, const Basis& transform, const Color4& color = Color4::White()) const;

        // Same for the image sliced by borders and stretched to targetSize; mesh must be split by slices
        void FillSlicedVertices(Vertex* vertices, const Basis& transform, const BorderF& borders,
                                const Vec2F& targetSize, const Color4& color = Color4::White()) const;

        // Returns point of the image sliced as Sprite does: corners keep size, sides and center are stretched
        static Vec2F MapSlicedPoint(const Vec2F& point, const Vec2F& imageSize, const BorderF& borders,
                                    const Vec2F& targetSize);
    };
}
