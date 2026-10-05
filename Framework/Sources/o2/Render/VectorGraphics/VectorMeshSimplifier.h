#pragma once

#include "o2/Render/VectorGraphics/VectorMesh.h"

namespace o2
{
    // Remover of the mesh vertices that the triangles around them draw the same without
    namespace VectorMeshSimplifier
    {
        // Removes vertices whose colors the triangles around interpolate; layers start overlapping groups
        void Simplify(VectorMesh& mesh, const Vector<int>& layers, float colorTolerance);

        // Removes vertices that no triangle refers to
        void RemoveUnusedVertices(VectorMesh& mesh);
    }
}
