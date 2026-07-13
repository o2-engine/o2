#pragma once

#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // ------------------------------------------------------------------
    // Plain 3D geometry: positions, normals, uvs and triangle indices
    // ------------------------------------------------------------------
    struct Mesh3DData
    {
        Vector<Vec3F> positions;
        Vector<Vec3F> normals;
        Vector<Vec2F> uvs;
        Vector<UInt>  indices;

        // Returns bound of positions; false when empty
        bool GetBounds(AABB& bounds) const;
    };

    namespace Mesh3DPrimitives
    {
        // Builds box centered at origin, 24 vertices with per-face normals, each face UV mapped 0..1
        Mesh3DData BuildBox(const Vec3F& size);

        // Builds UV sphere centered at origin with smooth normals; (rings + 1)*(segments + 1) vertices
        Mesh3DData BuildSphere(float radius, int segments, int rings);

        // Builds XY plane centered at origin, facing +Z (towards the default 2D camera), 4 vertices
        Mesh3DData BuildPlane(const Vec2F& size);

        // Builds cylinder centered at origin with Y axis, smooth side normals and flat caps
        Mesh3DData BuildCylinder(float radius, float height, int segments);

        // Builds cone centered at origin with Y axis, base at -height/2, apex at +height/2, flat base cap
        Mesh3DData BuildCone(float radius, float height, int segments);

        // Builds torus centered at origin around Y axis: ring of radius in the XZ plane, tube of tubeRadius
        Mesh3DData BuildTorus(float radius, float tubeRadius, int segments, int tubeSegments);

        // Builds flat ring (annulus) centered at origin in the XZ plane around Y axis:
        // outer radius = radius, inner radius = radius - width, normals +Y
        Mesh3DData BuildFlatRing(float radius, float width, int segments);

        // Builds arrow along local +Y from origin: cylinder shaft and cone or cube head
        Mesh3DData BuildArrowGeometry(float length, float shaftRadius, float headLength, float headRadius,
                                      bool cubeHead);

        // Builds flat quad in the plane perpendicular to planeNormalAxis, spanning
        // [offset, offset + size] along both in-plane axes; the normal is oriented against faceAwayDirection
        Mesh3DData BuildPlaneHandleGeometry(int planeNormalAxis, float offset, float size,
                                            const Vec3F& faceAwayDirection);

        // Builds corner bracket: two thin box arms from origin along local +X and +Y
        Mesh3DData BuildCornerHandleGeometry(float armLength, float thickness);
    }
}
