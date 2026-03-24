#pragma once

#include "o2/Render/IDrawable.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Debug/Assert.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // -----------------------------------------------------------------
    // Triangles mesh. Containing vertices, indexes of polygons, texture
    // -----------------------------------------------------------------
    class Mesh: public virtual IDrawable, public RefCounterable, public ICloneableRef
    {
    public:
        PROPERTIES(Mesh);
        PROPERTY(TextureRef, texture, SetTexture, GetTexture);                // Texture property
        PROPERTY(UInt, maxVertexCount, SetMaxVertexCount, GetMaxVertexCount); // Max vertex count property
        PROPERTY(UInt, maxPolyCount, SetMaxPolyCount, GetMaxPolyCount);       // Max polygons count property

    public:
        UInt vertexCount = 0; // Current vertices count
        UInt polyCount = 0;   // Current polygons in mesh

    public:
        // Constructor, creates mesh with Vertex type by default
        Mesh(TextureRef texture = TextureRef(), UInt vertexCount = 4, UInt polyCount = 2);

        // Copy-constructor
        Mesh(const Mesh& mesh);

        // Destructor
        ~Mesh();

        // Assign operator
        Mesh& operator=(const Mesh& other);

        // Resizing mesh buffers with current vertex type, loses data
        void Resize(UInt vertexCount, UInt polyCount);

        // Resizing mesh buffers with specified vertex type, loses data
        template<typename VertexStruct>
        void Resize(UInt vertexCount, UInt polyCount);

        // Returns typed pointer to vertex buffer with type assertion
        template<typename VertexStruct>
        VertexStruct* GetVertices();

        // Returns const typed pointer to vertex buffer with type assertion
        template<typename VertexStruct>
        const VertexStruct* GetVertices() const;

        // Returns raw vertex data pointer
        UInt8* GetVertexData();

		// Returns const raw vertex data pointer
        const UInt8* GetVertexData() const;

        // Returns vertex type descriptor
        const VertexType& GetVertexType() const;

        // Returns index buffer pointer
        VertexIndex* GetIndexes();

		// Returns const index buffer pointer
        const VertexIndex* GetIndexes() const;

        // Sets max vertex count buffer
        void SetMaxVertexCount(const UInt& count);

        // Sets max polygons count buffer
        void SetMaxPolyCount(const UInt& count);

        // Returns max vertex buffer size
        UInt GetMaxVertexCount() const;

        // Returns max polygons count
		UInt GetMaxPolyCount() const;

		// Drawing mesh
		void Draw();

		// Sets texture
		void SetTexture(const TextureRef& texture);

		// Returns texture ptr
		const TextureRef& GetTexture() const;

		// Sets texture source rect in pixel coordinates (for atlas sub-images)
		void SetTextureSrcRect(const RectI& rect);

		// Returns texture source rect
		const RectI& GetTextureSrcRect() const;

        CLONEABLE_REF(Mesh);

    protected:
        UInt8*       mVertexData = nullptr;  // Vertex data buffer (opaque bytes)
        VertexIndex* mIndexData = nullptr;   // Index buffer
        VertexType   mVertexType = Vertex::Type(); // Current vertex type descriptor

		TextureRef mTexture;        // Texture
		RectI      mTextureSrcRect; // Texture source rect in pixel coordinates

        UInt mMaxVertexCount = 0; // Max size of vertex buffer
        UInt mMaxPolyCount = 0;   // Max polygons count, mMaxPolyCount*3 - is index buffer max size

        friend class Render;
        friend class Sprite;
    };

    // --- Template implementations ---

    template<typename VertexStruct>
    void Mesh::Resize(UInt vertexCount, UInt polyCount)
    {
        if (mVertexData)
            delete[] mVertexData;

        if (mIndexData)
            delete[] mIndexData;

        mVertexType = VertexStruct::Type();

        mVertexData = mnew UInt8[vertexCount * mVertexType.GetStride()];
        mIndexData = mnew VertexIndex[polyCount * 3];

        mMaxVertexCount = vertexCount;
        mMaxPolyCount = polyCount;

        this->vertexCount = 0;
        this->polyCount = 0;
    }

    template<typename VertexStruct>
    VertexStruct* Mesh::GetVertices()
    {
        Assert(mVertexType == VertexStruct::Type(), "Mesh vertex type mismatch");
        return reinterpret_cast<VertexStruct*>(mVertexData);
    }

    template<typename VertexStruct>
    const VertexStruct* Mesh::GetVertices() const
    {
        Assert(mVertexType == VertexStruct::Type(), "Mesh vertex type mismatch");
        return reinterpret_cast<const VertexStruct*>(mVertexData);
    }
}
