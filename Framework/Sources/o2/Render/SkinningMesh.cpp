#include "o2/stdafx.h"
#include "SkinningMesh.h"

#include "o2/Render/Render.h"

namespace o2
{
    SkinningMesh::SkinningMesh(TextureRef texture /*= TextureRef()*/, UInt vertexCount /*= 4*/, UInt polyCount /*= 2*/,
                                 UInt bonesCount /*= 16*/)
    {
        SetTexture(texture);
        Resize(vertexCount, polyCount, bonesCount);
    }

    SkinningMesh::SkinningMesh(const SkinningMesh& other)
    {
        SetTexture(other.mTexture);
        Resize(other.mMaxVertexCount, other.mMaxPolyCount, other.mMaxBonesCount);

        bonesCount = other.bonesCount;
        vertexCount = other.vertexCount;
        polyCount = other.polyCount;

        memcpy(bones, other.bones, other.mMaxBonesCount*sizeof(Bone));
        memcpy(vertices, other.vertices, other.mMaxVertexCount*sizeof(SkinningVertex));
        memcpy(indexes, other.indexes, other.mMaxPolyCount*3*sizeof(VertexIndex));
    }

    SkinningMesh::~SkinningMesh()
    {
        delete[] bones;
        delete[] vertices;
        delete[] indexes;
        delete[] mRenderVertexBuffer;
    }

    SkinningMesh& SkinningMesh::operator=(const SkinningMesh& other)
    {
        SetTexture(other.mTexture);
        Resize(other.mMaxVertexCount, other.mMaxPolyCount, other.mMaxBonesCount);

        bonesCount = other.bonesCount;
        vertexCount = other.vertexCount;
        polyCount = other.polyCount;

        memcpy(bones, other.bones, other.mMaxBonesCount*sizeof(Bone));
        memcpy(vertices, other.vertices, other.mMaxVertexCount*sizeof(SkinningVertex));
        memcpy(indexes, other.indexes, other.mMaxPolyCount*3*sizeof(VertexIndex));

        return *this;
    }

    void SkinningMesh::Resize(UInt vertexCount, UInt polyCount, UInt bonesCount)
    {
        if (bones) delete[] bones;
        if (vertices) delete[] vertices;
        if (indexes) delete[] indexes;
        if (mRenderVertexBuffer) delete[] mRenderVertexBuffer;

        bones = mnew Bone[bonesCount];
        vertices = mnew SkinningVertex[vertexCount];
        mRenderVertexBuffer = mnew Vertex[vertexCount];
        indexes = mnew VertexIndex[polyCount*3];

        mMaxBonesCount = bonesCount;
        mMaxVertexCount = vertexCount;
        mMaxPolyCount = polyCount;

        bonesCount = 0;
        vertexCount = 0;
        polyCount = 0;
    }

    void SkinningMesh::Reskin()
    {
        for (UInt i = 0; i < vertexCount; i++)
        {
            auto& v = vertices[i];
            Vec2F p = Vec2F(v.x, v.y);
            Vec2F res = 
                 p*bones[v.bones[0]].releaseTransform*v.boneWeights[0] +
                 p*bones[v.bones[1]].releaseTransform*v.boneWeights[1] + 
                 p*bones[v.bones[2]].releaseTransform*v.boneWeights[2] + 
                 p*bones[v.bones[3]].releaseTransform*v.boneWeights[3];

            mRenderVertexBuffer[i].Set(res, v.color, v.tu, v.tv);
        }
    }

    void SkinningMesh::Draw()
    {
        o2Render.DrawBuffer(PrimitiveType::Polygon, mRenderVertexBuffer, vertexCount, indexes, polyCount, mTexture, 
                            BlendMode::Normal);
        OnDrawn();
    }

    void SkinningMesh::SetTexture(TextureRef texture)
    {
        mTexture = texture;
    }

    TextureRef SkinningMesh::GetTexture() const
    {
        return mTexture;
    }

    void SkinningMesh::SetMaxVertexCount(const UInt& count)
    {
        delete[] vertices;
        delete[] mRenderVertexBuffer;

        vertices = mnew SkinningVertex[count];
        mRenderVertexBuffer = mnew Vertex[count];
        mMaxVertexCount = count;
        vertexCount = 0;
    }

    void SkinningMesh::SetMaxPolyCount(const UInt& count)
    {
        delete[] indexes;

        indexes = new VertexIndex[count*3];
        mMaxPolyCount = count;
        polyCount = 0;
    }

    void SkinningMesh::SetMaxBonesCount(const UInt& count)
    {
        delete[] bones;

        bones = new Bone[count];
        mMaxBonesCount = count;
        bonesCount = 0;
    }

    UInt SkinningMesh::GetMaxVertexCount() const
    {
        return mMaxVertexCount;
    }

    UInt SkinningMesh::GetMaxPolyCount() const
    {
        return mMaxPolyCount;
    }

    UInt SkinningMesh::GetMaxBonesCount() const
    {
        return mMaxBonesCount;
    }

}
