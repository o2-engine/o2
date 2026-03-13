#pragma once

#include "o2/Render/IDrawable.h"
#include "o2/Render/TextureRef.h"
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
        Vertex*      vertices; // Vertex buffer
        VertexIndex* indexes;  // Index buffer
                                          
        UInt vertexCount; // Current vertices count
        UInt polyCount;   // Current polygons in mesh

    public:
        // Constructor
        Mesh(TextureRef texture = TextureRef(), UInt vertexCount = 4, UInt polyCount = 2);

        // Copy-constructor
        Mesh(const Mesh& mesh);

        // Destructor
        ~Mesh();

        // Assign operator
        Mesh& operator=(const Mesh& other);

        // Resizing mesh buffers, looses data
        void Resize(UInt vertexCount, UInt polyCount);

        // Drawing mesh
        void Draw();

        // Sets texture
        void SetTexture(const TextureRef& texture);

        // Returns texture ptr
        const TextureRef& GetTexture() const;

        // Sets max vertex count buffer
        void SetMaxVertexCount(const UInt& count);

        // Sets max polygons count buffer
        void SetMaxPolyCount(const UInt& count);

        // Returns max vertex buffer size
        UInt GetMaxVertexCount() const;

        // Returns max polygons count
        UInt GetMaxPolyCount() const;

        CLONEABLE_REF(Mesh);

    protected:
        TextureRef mTexture; // Texture

        UInt mMaxVertexCount; // Max size of vertex buffer
        UInt mMaxPolyCount;   // Max polygons count, mMaxPolyCount*3 - is index buffer max size

        friend class Render;
        friend class Sprite;
    };
}
