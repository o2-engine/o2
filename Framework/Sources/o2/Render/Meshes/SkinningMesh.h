#pragma once

#include "o2/Render/IDrawable.h"
#include "o2/Render/Resources/TextureRef.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Math/Vertex.h"

#include "o2/Utils/Property.h"

namespace o2
{

    // ------------------------------------------------------------------------------------------
    // Triangles SkinnableMesh. Containing bones, skinning vertices, indexes of polygons, texture
    // ------------------------------------------------------------------------------------------
    class SkinningMesh : public virtual IDrawable, public virtual RefCounterable, public ICloneableRef
    {
    public:
        struct Bone
        {
            Basis baseTransform;
            Basis releaseTransform;

            Bone* parentBone = nullptr;
            Vector<Bone*> childrenBones;
        };

        struct SkinningVertex : public Vertex
        {
            UInt8 bones[4] = { 0, 0, 0, 0 };
            float boneWeights[4] = { 1, 0, 0, 0 };
        };

    public:
        PROPERTIES(SkinningMesh);
        PROPERTY(TextureRef, texture, SetTexture, GetTexture);                // Texture property
        PROPERTY(UInt, maxVertexCount, SetMaxVertexCount, GetMaxVertexCount); // Max vertex count property
        PROPERTY(UInt, maxPolyCount, SetMaxPolyCount, GetMaxPolyCount);       // Max polygons count property
        PROPERTY(UInt, maxBonesCount, SetMaxBonesCount, GetMaxBonesCount);    // Max bones count property

    public:
        Basis transform; // Root transform

        Bone* bones = nullptr; // Bones buffer
        UInt  bonesCount = 0;  // Current bones count

        SkinningVertex* vertices = nullptr; // Vertex buffer
        UInt            vertexCount = 0;    // Current vertices count

        VertexIndex* indexes = nullptr;  // Index buffer
        UInt         polyCount = 0;      // Current polygons in SkinnableMesh                                          

    public:
        // Constructor
        SkinningMesh(TextureRef texture = TextureRef(), UInt vertexCount = 4, UInt polyCount = 2, UInt bonesCount = 16);

        // Copy-constructor
        SkinningMesh(const SkinningMesh& other);

        // Destructor
        ~SkinningMesh();

        // Assign operator
        SkinningMesh& operator=(const SkinningMesh& other);

        // Resizing SkinnableMesh buffers, looses data
        void Resize(UInt vertexCount, UInt polyCount, UInt bonesCount);

        // Updates vertices by bones transformations
        void Reskin();

        // Drawing SkinnableMesh
        void Draw() override;

        // Sets texture
        void SetTexture(TextureRef texture);

        // Returns texture ptr
        TextureRef GetTexture() const;

        // Sets max vertex count buffer
        void SetMaxVertexCount(const UInt& count);

        // Sets max polygons count buffer
        void SetMaxPolyCount(const UInt& count);

        // Sets max bones count buffer
        void SetMaxBonesCount(const UInt& count);

        // Returns max vertex buffer size
        UInt GetMaxVertexCount() const;

        // Returns max polygons count
        UInt GetMaxPolyCount() const;

        // Returns max bones count
        UInt GetMaxBonesCount() const;

        CLONEABLE_REF(SkinningMesh);

    protected:
        TextureRef mTexture; // Texture

        UInt mMaxBonesCount = 0;  // Max size of bones buffer
        UInt mMaxVertexCount = 0; // Max size of vertex buffer
        UInt mMaxPolyCount = 0;   // Max polygons count, mMaxPolyCount*3 - is index buffer max size

        Vertex* mRenderVertexBuffer = nullptr; // Vertex list, used for rendering. Obtained from origin vertex skinning

        friend class Render;
    };
}
