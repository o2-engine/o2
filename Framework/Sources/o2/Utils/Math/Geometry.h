#pragma once

#include "o2/Utils/Math/Border.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    namespace Geometry
    {
        // Returns in-plane orthonormal basis (u, v) of the plane perpendicular to world axis,
        // right-handed: rotating by positive angle around the axis moves u towards v
        void AxisPlaneBasis(int axis, Vec3F& u, Vec3F& v);

        // Returns angle of point around the axis at center: atan2 in the axis plane basis
        float AxisPlaneAngle(const Vec3F& center, int axis, const Vec3F& point);

        // Returns minimal distance from point to the polyline
        float PointToPolylineDistance(const Vector<Vec2F>& points, const Vec2F& point);

        // Intersects ray with a finite capped cylinder from start to end with radius;
        // returns the nearest non-negative hit distance along the normalized ray direction
        bool RayIntersectsCylinder(const Vec3F& origin, const Vec3F& direction,
                                   const Vec3F& start, const Vec3F& end, float radius, float& distance);

        // Intersects ray with a two-sided parallelogram: corner and two edge vectors;
        // returns the non-negative hit distance along the normalized ray direction
        bool RayIntersectsQuad(const Vec3F& origin, const Vec3F& direction,
                               const Vec3F& corner, const Vec3F& edgeU, const Vec3F& edgeV, float& distance);

        // Function that creates mesh from points
        void CreatePolyLineMesh(const Vertex* points, int pointsCount,
                                Vertex*& verticies, UInt& vertexCount, UInt& vertexSize,
                                VertexIndex*& indexes, UInt& polyCount, UInt& polySize,
                                float width, float texBorderTop, float texBorderBottom, const Vec2F& texSize,
                                const Vec2F& invCameraScale = Vec2F(1, 1));

        // Function that clips triangle by line
        int ClipTriangleByLine(const Vertex& a, const Vertex& b, const Vertex& c,
                               const Vec2F& lineBegin, const Vec2F& lineEnd,
                               Vertex* output);

        // Function that clips triangle by rectangle
        int ClipTriangleByRectangle(const Vertex& a, const Vertex& b, const Vertex& c,
                                    Vertex* output,
                                    const RectF& clippRect);
    }
}
