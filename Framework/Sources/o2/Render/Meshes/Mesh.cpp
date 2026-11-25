#include "o2/stdafx.h"
#include "Mesh.h"

#include "o2/Render/Render.h"

namespace o2
{
	Mesh::Mesh(TextureRef texture /*= TextureRef()*/, UInt vertexCount /*= 4*/, UInt polyCount /*= 2*/):
		mVertexType(Vertex::Type())
	{
		SetTexture(texture);
		Resize(vertexCount, polyCount);
	}

	Mesh::Mesh(const Mesh& mesh):
		mVertexType(mesh.mVertexType)
	{
		SetTexture(mesh.mTexture);
		mTextureSrcRect = mesh.mTextureSrcRect;

		size_t stride = mVertexType.GetStride();

		mVertexData = mnew UInt8[mesh.mMaxVertexCount * stride];
		mIndexData = mnew VertexIndex[mesh.mMaxPolyCount * 3];

		mMaxVertexCount = mesh.mMaxVertexCount;
		mMaxPolyCount = mesh.mMaxPolyCount;

		vertexCount = mesh.vertexCount;
		polyCount = mesh.polyCount;

		memcpy(mVertexData, mesh.mVertexData, mesh.mMaxVertexCount * stride);
		memcpy(mIndexData, mesh.mIndexData, mesh.mMaxPolyCount * 3 * sizeof(VertexIndex));
	}

	Mesh::~Mesh()
	{
		delete[] mVertexData;
		delete[] mIndexData;
	}

	Mesh& Mesh::operator=(const Mesh& other)
	{
		SetTexture(other.mTexture);
		mTextureSrcRect = other.mTextureSrcRect;
		mVertexType = other.mVertexType;

		size_t stride = mVertexType.GetStride();

		if (mVertexData)
			delete[] mVertexData;

		if (mIndexData)
			delete[] mIndexData;

		mVertexData = mnew UInt8[other.mMaxVertexCount * stride];
		mIndexData = mnew VertexIndex[other.mMaxPolyCount * 3];

		mMaxVertexCount = other.mMaxVertexCount;
		mMaxPolyCount = other.mMaxPolyCount;

		vertexCount = other.vertexCount;
		polyCount = other.polyCount;

		memcpy(mVertexData, other.mVertexData, other.mMaxVertexCount * stride);
		memcpy(mIndexData, other.mIndexData, other.mMaxPolyCount * 3 * sizeof(VertexIndex));

		return *this;
	}

	void Mesh::Resize(UInt vertexCount, UInt polyCount)
	{
		// Grow-only: meshes that are refilled every frame with the same geometry size must not hit the
		// allocator. The buffers are capacity, the filled length is carried by vertexCount/polyCount
		if (!mVertexData || vertexCount > mMaxVertexCount)
		{
			delete[] mVertexData;
			mVertexData = mnew UInt8[vertexCount * mVertexType.GetStride()];
			mMaxVertexCount = vertexCount;
		}

		if (!mIndexData || polyCount > mMaxPolyCount)
		{
			delete[] mIndexData;
			mIndexData = mnew VertexIndex[polyCount * 3];
			mMaxPolyCount = polyCount;
		}

		this->vertexCount = 0;
		this->polyCount = 0;
	}

	UInt8* Mesh::GetVertexData()
	{ 
		return mVertexData;
	}

	const UInt8* Mesh::GetVertexData() const
	{ 
		return mVertexData;
	}

	const VertexType& Mesh::GetVertexType() const 
	{ 
		return mVertexType; 
	}

	VertexIndex* Mesh::GetIndexes()
	{ 
		return mIndexData; 
	}

	const VertexIndex* Mesh::GetIndexes() const 
	{ 
		return mIndexData;
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

	void Mesh::SetTextureSrcRect(const RectI& rect)
	{
		mTextureSrcRect = rect;
	}

	const RectI& Mesh::GetTextureSrcRect() const
	{
		return mTextureSrcRect;
	}

	void Mesh::SetMaxVertexCount(const UInt& count)
	{
		delete[] mVertexData;
		mVertexData = mnew UInt8[count * mVertexType.GetStride()];
		mMaxVertexCount = count;
		vertexCount = 0;
	}

	void Mesh::SetMaxPolyCount(const UInt& count)
	{
		delete[] mIndexData;
		mIndexData = mnew VertexIndex[count * 3];
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
