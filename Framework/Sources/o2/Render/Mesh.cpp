#include "o2/stdafx.h"
#include "Mesh.h"

#include "o2/Render/Render.h"

namespace o2
{
    Mesh::Mesh(TextureRef texture /*= TextureRef()*/, UInt vertexCount /*= 4*/, UInt polyCount /*= 2*/):
        vertices(nullptr), indexes(nullptr), mMaxPolyCount(0), mMaxVertexCount(0), vertexCount(0), polyCount(0)
    {
        SetTexture(texture);
        Resize(vertexCount, polyCount);
    }

    Mesh::Mesh(const Mesh& mesh):
        vertices(nullptr), indexes(nullptr), mMaxPolyCount(0), mMaxVertexCount(0), vertexCount(0), polyCount(0)
    {
        SetTexture(mesh.mTexture);
        Resize(mesh.mMaxVertexCount, mesh.mMaxPolyCount);

        vertexCount = mesh.vertexCount;
        polyCount = mesh.polyCount;

        memcpy(vertices, mesh.vertices, mesh.mMaxVertexCount*sizeof(Vertex));
        memcpy(indexes, mesh.indexes, mesh.mMaxPolyCount*3*sizeof(VertexIndex));
    }

    Mesh::~Mesh()
    {
        delete[] vertices;
        delete[] indexes;
    }

    Mesh& Mesh::operator=(const Mesh& other)
    {
        SetTexture(other.mTexture);
        Resize(other.mMaxVertexCount, other.mMaxPolyCount);

        vertexCount = other.vertexCount;
        polyCount = other.polyCount;

        memcpy(vertices, other.vertices, other.mMaxVertexCount*sizeof(Vertex));
        memcpy(indexes, other.indexes, other.mMaxPolyCount*3*sizeof(VertexIndex));

        return *this;
    }

    void Mesh::Resize(UInt vertexCount, UInt polyCount)
    {
        if (vertices) delete[] vertices;
        if (indexes) delete[] indexes;

        vertices = new Vertex[vertexCount];
        indexes = new VertexIndex[polyCount*3];

        mMaxVertexCount = vertexCount;
        mMaxPolyCount = polyCount;

        vertexCount = 0;
        polyCount = 0;
    }

    void Mesh::Draw()
    {
        o2Render.DrawMesh(this);
        OnDrawn();
    }

    void Mesh::SetTexture(const TextureRef& texture)
    {
        mTexture = texture;
    }

    const TextureRef& Mesh::GetTexture() const
    {
        return mTexture;
    }

    void Mesh::SetMaxVertexCount(const UInt& count)
    {
        delete[] vertices;
        vertices = new Vertex[count];
        mMaxVertexCount = count;
        vertexCount = 0;
    }

    void Mesh::SetMaxPolyCount(const UInt& count)
    {
        delete[] indexes;
        indexes = new VertexIndex[count*3];
        mMaxPolyCount = count;
        polyCount = 0;
    }

    UInt Mesh::GetMaxVertexCount() const
    {
        return mMaxVertexCount;
    }

    UInt Mesh::GetMaxPolyCount() const
    {
        return mMaxPolyCount;
    }
}
