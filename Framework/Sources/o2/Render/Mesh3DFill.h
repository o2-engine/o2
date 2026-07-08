#pragma once

#include "o2/Render/TextureSource.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"

namespace o2
{
    class Mesh;

    // Render side of the 3D primitives toolkit: filling drawable meshes from geometry data
    namespace Mesh3DPrimitives
    {
        // Returns bound of drawable mesh vertices; false when the mesh is empty
        bool GetMeshBounds(const Mesh& mesh, AABB& bounds);

        // Returns the fixed light direction baked by FillMesh when shaded
        Vec3F BakedLightDirection();

        // Fills drawable mesh from data: transforms positions and normals by world matrix,
        // maps UVs into texture source rect and optionally bakes lambert lighting into vertex colors;
        // ambient sets the minimum lighting intensity of shaded faces
        void FillMesh(Mesh& mesh, const Mesh3DData& data, const Mat4& worldTransform, const Color4& color,
                      const TextureSource& textureSource, bool shaded, float ambient = 0.35f);
    }
}
