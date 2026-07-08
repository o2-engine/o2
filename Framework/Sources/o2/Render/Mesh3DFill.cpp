#include "o2/stdafx.h"
#include "Mesh3DFill.h"

#include "o2/Render/Mesh.h"
#include "o2/Utils/Math/Vertex.h"

namespace o2::Mesh3DPrimitives
{
    bool GetMeshBounds(const Mesh& mesh, AABB& bounds)
    {
        if (mesh.vertexCount == 0)
            return false;

        const UInt8* data = mesh.GetVertexData();
        const VertexType& type = mesh.GetVertexType();
        size_t stride = type.GetStride();
        size_t positionOffset = type.GetParamOffset(VertexParam::Position);

        Vec3F boundsMin(FLT_MAX, FLT_MAX, FLT_MAX);
        Vec3F boundsMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (UInt i = 0; i < mesh.vertexCount; i++)
        {
            const float* p = reinterpret_cast<const float*>(data + i*stride + positionOffset);
            boundsMin.x = Math::Min(boundsMin.x, p[0]); boundsMax.x = Math::Max(boundsMax.x, p[0]);
            boundsMin.y = Math::Min(boundsMin.y, p[1]); boundsMax.y = Math::Max(boundsMax.y, p[1]);
            boundsMin.z = Math::Min(boundsMin.z, p[2]); boundsMax.z = Math::Max(boundsMax.z, p[2]);
        }

        bounds = AABB(boundsMin, boundsMax);
        return true;
    }

    Vec3F BakedLightDirection()
    {
        return Vec3F(0.3f, -0.5f, -0.8f).Normalized();
    }

    void FillMesh(Mesh& mesh, const Mesh3DData& data, const Mat4& worldTransform, const Color4& color,
                  const TextureSource& textureSource, bool shaded, float ambient /*= 0.35f*/)
    {
        UInt vertexCount = data.positions.Count();
        UInt polyCount = data.indices.Count()/3;

        if (vertexCount == 0 || polyCount == 0)
        {
            mesh.vertexCount = 0;
            mesh.polyCount = 0;
            return;
        }

        mesh.Resize(vertexCount, polyCount);

        RectF uvRect(0.0f, 1.0f, 1.0f, 0.0f);
        auto texture = textureSource.texture;
        if (texture)
        {
            Vec2F invTexSize(1.0f/texture->GetSize().x, 1.0f/texture->GetSize().y);
            RectF rect = textureSource.sourceRect;
            uvRect = RectF(rect.left*invTexSize.x, 1.0f - rect.top*invTexSize.y,
                           rect.right*invTexSize.x, 1.0f - rect.bottom*invTexSize.y);
        }

        static const Vec3F lightDir = BakedLightDirection();

        Vertex* verts = mesh.GetVertices<Vertex>();
        for (UInt i = 0; i < vertexCount; i++)
        {
            Vec3F pos = worldTransform.TransformPoint(data.positions[i]);
            Vec3F normal = worldTransform.TransformDirection(data.normals[i]).Normalized();

            Color4 vertexColor = color;
            if (shaded)
            {
                float intensity = ambient + (1.0f - ambient)*Math::Max(normal.Dot(lightDir*-1.0f), 0.0f);
                vertexColor = Color4((int)(color.r*intensity), (int)(color.g*intensity), (int)(color.b*intensity),
                                     color.a);
            }

            Vec2F uv = data.uvs[i];
            verts[i] = Vertex(pos.x, pos.y, pos.z, vertexColor.ABGR(),
                              uvRect.left + uv.x*uvRect.Width(), uvRect.bottom + uv.y*uvRect.Height());
            verts[i].SetNormal(normal.x, normal.y, normal.z);
        }

        VertexIndex* indexes = mesh.GetIndexes();
        for (UInt i = 0; i < polyCount*3; i++)
            indexes[i] = data.indices[i];

        mesh.SetTexture(texture);
        mesh.vertexCount = vertexCount;
        mesh.polyCount = polyCount;
    }
}
