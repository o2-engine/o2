#pragma once

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/IRectDrawable.h"
#include "o2/Render/RenderGeometry.h"
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Math/Border.h"
#include "o2/Utils/Math/Vertex.h"

namespace o2
{
    // ---------------------------------------------------------------------------------
    // Vector image sprite: draws triangles of a vector image asset with per-vertex colors
    // ---------------------------------------------------------------------------------
    class VectorSprite: public IRectDrawable
    {
    public:
        // ----------------------------------------------
        // Counters of all vector sprites since the start
        // ----------------------------------------------
        struct Statistics
        {
            UInt64 draws = 0;             // Draw calls that have sent triangles to render
            UInt64 culledDraws = 0;       // Draw calls skipped: the sprite was transparent or out of the clipping
            UInt64 drawnTriangles = 0;    // Triangles sent to render
            UInt64 drawnVertices = 0;     // Vertices sent to render
            UInt64 rebuiltVertices = 0;   // Vertices whose positions were rebuilt from the image
            UInt64 shiftedVertices = 0;   // Vertices whose positions were only moved with the sprite
            UInt64 recoloredVertices = 0; // Vertices whose colors were written
        };

    public:
        PROPERTIES(VectorSprite);
        PROPERTY(AssetRef<VectorImageAsset>, image, SetImageAsset, GetImageAsset); // Sets image asset @SCRIPTABLE
        PROPERTY(String, imageName, LoadFromImage, GetImageName);                  // Sets image asset path @SCRIPTABLE
        PROPERTY(SpriteMode, mode, SetMode, GetMode);                              // Sprite drawing mode property @SCRIPTABLE
        PROPERTY(BorderI, sliceBorder, SetSliceBorder, GetSliceBorder);            // Slice border property @SCRIPTABLE

    public:
        // Default constructor
        VectorSprite();

        // Constructor from image asset
        explicit VectorSprite(const AssetRef<VectorImageAsset>& image);

        // Constructor from image asset by path @SCRIPTABLE
        explicit VectorSprite(const String& imagePath);

        // Copy-constructor
        VectorSprite(const VectorSprite& other);

        // Assign operator
        VectorSprite& operator=(const VectorSprite& other);

        // Equals operator
        bool operator==(const VectorSprite& other) const;

        // Not equals operator
        bool operator!=(const VectorSprite& other) const;

        // Loads sprite from image asset @SCRIPTABLE_NAME(LoadFromImageRef)
        void LoadFromImage(const AssetRef<VectorImageAsset>& image, bool setSizeByImage = true);

        // Loads sprite from image asset by path @SCRIPTABLE
        void LoadFromImage(const String& imagePath, bool setSizeByImage = true);

        // Loads sprite from image asset by id @SCRIPTABLE_NAME(LoadFromImageUID)
        void LoadFromImage(UID imageId, bool setSizeByImage = true);

        // Draws sprite @SCRIPTABLE
        void Draw() override;

        // Returns image size
        Vec2F GetOriginalSize() const;

        // Sets sprite drawing mode @SCRIPTABLE
        void SetMode(SpriteMode mode);

        // Returns sprite drawing mode @SCRIPTABLE
        SpriteMode GetMode() const;

        // Sets sprite slice border
        void SetSliceBorder(const BorderI& border);

        // Returns sprite slice border
        BorderI GetSliceBorder() const;

        // Sets asset @SCRIPTABLE
        void SetImageAsset(const AssetRef<VectorImageAsset>& asset);

        // Returns asset @SCRIPTABLE
        AssetRef<VectorImageAsset> GetImageAsset() const;

        // Returns image asset name @SCRIPTABLE
        const String& GetImageName() const;

        // Sets size by image size @SCRIPTABLE
        void NormalizeSize();

        // Sets size with equal aspect as image by width @SCRIPTABLE
        void NormalizeAspectByWidth();

        // Sets size with equal aspect as image by height @SCRIPTABLE
        void NormalizeAspectByHeight();

        // Sets size with equal aspect as image by nearest value @SCRIPTABLE
        void NormalizeAspect();

        // Returns count of triangles built at the last drawing
        UInt GetTrianglesCount() const;

        // Returns count of vertices built at the last drawing
        UInt GetVerticesCount() const;

        // Returns pixel scale of the mesh used at the last drawing: screen pixels per image unit
        Vec2F GetMeshPixelScale() const;

        // Returns true when the mesh of the last drawing keeps edges on pixel bounds hard: the sprite stood on whole pixels
        bool IsMeshPixelSnapped() const;

        // Returns how many times positions of vertices were written: rebuilt from the image or shifted
        UInt GetMeshRebuildsCount() const;

        // Returns how many times colors of vertices were written, with a rebuild or alone
        UInt GetColorUpdatesCount() const;

        // Returns how many triangles all vector sprites have sent to render since the start
        static UInt64 GetTotalDrawnTriangles();

        // Returns counters of all vector sprites since the start
        static const Statistics& GetStatistics();

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        SERIALIZABLE(VectorSprite);
        CLONEABLE_REF(VectorSprite);

    protected:
        AssetRef<VectorImageAsset> mImageAsset; // Image asset @SERIALIZABLE

        SpriteMode mMode = SpriteMode::Default; // Drawing mode @SERIALIZABLE
        BorderI    mSlices;                     // Slice borders @SERIALIZABLE

        Ref<VectorImageMesh> mSourceMesh; // Shared triangles of the image, owner of indexes
        Ref<RenderGeometry>  mGeometry;   // Transformed and colored vertices, retained by render between frames
        RectF                mBounds;     // Bounds of the vertices

        bool  mMeshDirty = true;      // Are vertices required to rebuild before drawing
        bool  mMeshPostponed = false; // Is the mesh of the current scale left to be built at one of the next frames
        bool  mBasisDirty = false;    // Was the basis set since the last drawing: vertices are rebuilt if it has changed
        bool  mColorDirty = false;    // Are only colors of vertices required to update before drawing
        Vec2F mMeshViewScale;         // Render view pixel scale the vertices are built for
        Basis mMeshBasis;             // Basis of the image rectangle the vertices are built for
        Vec2F mMeshSlicedSize;        // Size of the sliced image the vertices are built for
        float mMeshDepth = 0.0f;      // Depth the vertices are built for
        int   mMeshShiftsCount = 0;   // Count of the shifts of the vertices since they were built from the image
        UInt  mMeshImageVersion = 0;  // Image version the vertices are built for
        UInt  mMeshRebuildsCount = 0; // Count of vertices positions rebuilds and shifts
        UInt  mColorUpdatesCount = 0; // Count of vertices colors updates

        static Statistics mStatistics; // Counters of all vector sprites

    protected:
        // Called when basis was changed
        void BasisChanged() override;

        // Called when color was changed
        void OnColorChanged() override;

        // Rebuilds vertices for the render view pixel scale
        void UpdateMesh(const Vec2F& viewPixelScale);

        // Writes colors of the image multiplied by the sprite color, positions stay
        void UpdateColors();

        // Moves the vertices and their bounds
        void ShiftVertices(const Vec2F& shift);

        // Returns the geometry to write vertices to: another one when the current is drawn and not sent to render yet
        RenderGeometry& GetChangingGeometry();

        // Returns true when the not rotated basis starts and ends on whole screen pixels
        static bool IsOnWholePixels(const Basis& basis, const Vec2F& viewPixelScale);

        // Returns basis of the image rectangle for the mode and the size the sliced image is stretched to
        Basis GetImageBasis(Vec2F& slicedSize) const;
    };
}
// --- META ---

CLASS_BASES_META(o2::VectorSprite)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(o2::VectorSprite)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(image);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(imageName);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(mode);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(sliceBorder);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mImageAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(SpriteMode::Default).NAME(mMode);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSlices);
    FIELD().PROTECTED().NAME(mSourceMesh);
    FIELD().PROTECTED().NAME(mGeometry);
    FIELD().PROTECTED().NAME(mBounds);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mMeshDirty);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mMeshPostponed);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBasisDirty);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mColorDirty);
    FIELD().PROTECTED().NAME(mMeshViewScale);
    FIELD().PROTECTED().NAME(mMeshBasis);
    FIELD().PROTECTED().NAME(mMeshSlicedSize);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mMeshDepth);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mMeshShiftsCount);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mMeshImageVersion);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mMeshRebuildsCount);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mColorUpdatesCount);
}
END_META;
CLASS_METHODS_META(o2::VectorSprite)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetRef<VectorImageAsset>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(const String&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const VectorSprite&);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(LoadFromImageRef).SIGNATURE(void, LoadFromImage, const AssetRef<VectorImageAsset>&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, LoadFromImage, const String&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(LoadFromImageUID).SIGNATURE(void, LoadFromImage, UID, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetOriginalSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMode, SpriteMode);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(SpriteMode, GetMode);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSliceBorder, const BorderI&);
    FUNCTION().PUBLIC().SIGNATURE(BorderI, GetSliceBorder);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetImageAsset, const AssetRef<VectorImageAsset>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(AssetRef<VectorImageAsset>, GetImageAsset);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const String&, GetImageName);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeAspectByWidth);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeAspectByHeight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeAspect);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetTrianglesCount);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetVerticesCount);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetMeshPixelScale);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMeshPixelSnapped);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetMeshRebuildsCount);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetColorUpdatesCount);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(UInt64, GetTotalDrawnTriangles);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Statistics&, GetStatistics);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, BasisChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnColorChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateColors);
    FUNCTION().PROTECTED().SIGNATURE(void, ShiftVertices, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(RenderGeometry&, GetChangingGeometry);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(bool, IsOnWholePixels, const Basis&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetImageBasis, Vec2F&);
}
END_META;
// --- END META ---
