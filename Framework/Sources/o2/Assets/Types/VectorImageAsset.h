#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Render/VectorGraphics/VectorMesh.h"
#include "o2/Utils/Math/Border.h"
#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    // ------------------------------------------------------------------------------
    // Triangles of a vector image built for one pixel scale, shared between drawables
    // ------------------------------------------------------------------------------
    struct VectorImageMesh: public RefCounterable
    {
        VectorMesh mesh;           // Triangles in image space, mesh.pixelScale is the scale they are built for
        BorderF    slices;         // Slice lines the triangles are split by
        bool       sliced = false; // Are triangles split by slice lines
        bool       pixelSnapped = true; // Are edges on pixel bounds left hard, for drawing from a whole screen pixel
    };

    // -----------------------------------------------------------------
    // Vector image asset: SVG file parsed into shapes, drawn as triangles
    // -----------------------------------------------------------------
    class VectorImageAsset: public Asset
    {
    public:
        class Meta;

    public:
        PROPERTIES(VectorImageAsset);
        PROPERTY(BorderI, sliceBorder, SetSliceBorder, GetSliceBorder);    // Slice border property
        PROPERTY(SpriteMode, defaultMode, SetDefaultMode, GetDefaultMode); // Sprite default mode property

        GETTER(Vec2F, size, GetSize);     // Image size getter
        GETTER(float, width, GetWidth);   // Image width getter
        GETTER(float, height, GetHeight); // Image height getter

        GETTER(Ref<Meta>, meta, GetMeta); // Meta information getter

    public:
        // Default constructor
        VectorImageAsset();

        // Copy-constructor
        VectorImageAsset(const VectorImageAsset& other);

        // Destructor
        ~VectorImageAsset() override;

        // Assign operator
        VectorImageAsset& operator=(const VectorImageAsset& other);

        // Parses SVG text and replaces the image; returns false when the text is not a valid SVG
        bool SetSource(const String& svg);

        // Returns SVG text
        const String& GetSource() const;

        // Returns parsed image
        const VectorImage& GetImage() const;

        // Returns true when the SVG was parsed without error
        bool IsValid() const;

        // Returns parsing error, empty when the image is valid
        const String& GetError() const;

        // Returns parsing warnings: unsupported elements and attributes
        const Vector<String>& GetWarnings() const;

        // Returns image size
        Vec2F GetSize() const;

        // Returns image width
        float GetWidth() const;

        // Returns image height
        float GetHeight() const;

        // Sets slice border
        void SetSliceBorder(const BorderI& border);

        // Returns slice border
        BorderI GetSliceBorder() const;

        // Sets default sprite mode
        void SetDefaultMode(SpriteMode mode);

        // Returns default sprite mode
        SpriteMode GetDefaultMode() const;

        // Returns cached triangles for the pixel scale: screen pixels per image unit by axes. Null for invalid image.
        // Not pixel snapped mesh has the fringe along every edge, for drawing between screen pixels
        Ref<VectorImageMesh> GetMesh(const Vec2F& pixelScale, bool pixelSnapped = true);

        // Returns cached triangles split by slice lines, for VectorMesh::FillSlicedVertices
        Ref<VectorImageMesh> GetSlicedMesh(const Vec2F& pixelScale, const BorderF& slices, bool pixelSnapped = true);

        // Returns is the mesh of the scale and slices built already, getting it is cheap then
        bool IsMeshBuilt(const Vec2F& pixelScale, const BorderF& slices = BorderF(), bool pixelSnapped = true) const;

        // Returns the scale the mesh is actually built for: nearest power of two, axes differ 4 times at most
        static Vec2F QuantizePixelScale(const Vec2F& pixelScale);

        // Returns count of cached meshes
        int GetCachedMeshesCount() const;

        // Removes cached meshes; drawables keep the ones they use
        void ClearMeshCache();

        // Returns how many meshes all vector images have tessellated since the start
        static UInt GetTessellationsCount();

        // Returns seconds all vector images have spent in tessellation since the start
        static double GetTessellationTime();

        // Returns image version: changes on every load of the image, never equal for two loaded assets
        UInt GetVersion() const;

        // Returns meta information
        Ref<Meta> GetMeta() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_file_icon.svg"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 97; }

        ASSET_TYPE(VectorImageAsset, Meta);

    public:
        // ----------------
        // Meta information
        // ----------------
        class Meta: public DefaultAssetMeta<VectorImageAsset>
        {
        public:
            BorderI    sliceBorder;                        // Default slice border, in image units @SERIALIZABLE
            SpriteMode defaultMode = SpriteMode::Default;  // Default sprite mode @SERIALIZABLE

        public:
            // Returns true if other meta is equal to this
            bool IsEqual(AssetMeta* other) const override;

            SERIALIZABLE(Meta);
            CLONEABLE_REF(Meta);
        };

    protected:
        static constexpr int mMaxCachedMeshes = 32; // Cache is dropped when it grows over this count

        String         mSource;        // SVG text
        VectorImage    mImage;         // Parsed image
        bool           mValid = false; // Is image parsed without error
        String         mError;         // Parsing error
        Vector<String> mWarnings;      // Parsing warnings

        UInt mVersion = 0; // Image version, new on every load, 0 for a never loaded image

        Vector<Ref<VectorImageMesh>> mMeshes; // Cached meshes by pixel scale and slices

        bool mSubscribedOnRebuild = false; // Is subscribed on assets rebuilding

        static UInt   mTessellationsCount; // Meshes tessellated by all vector images
        static double mTessellationTime;   // Seconds spent in tessellation by all vector images

    protected:
        // Loads and parses SVG file
        void LoadData(const String& path) override;

        // Saves SVG text
        void SaveData(const String& path) const override;

        // Returns cached or builds new mesh
        Ref<VectorImageMesh> GetMesh(const Vec2F& pixelScale, const BorderF& slices, bool sliced, bool pixelSnapped);

        // Reloads image when its file was rebuilt
        void OnAssetsRebuilt(const Vector<UID>& changedAssets);

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::VectorImageAsset)
{
    BASE_CLASS(o2::Asset);
}
END_META;
CLASS_FIELDS_META(o2::VectorImageAsset)
{
    FIELD().PUBLIC().NAME(sliceBorder);
    FIELD().PUBLIC().NAME(defaultMode);
    FIELD().PUBLIC().NAME(size);
    FIELD().PUBLIC().NAME(width);
    FIELD().PUBLIC().NAME(height);
    FIELD().PUBLIC().NAME(meta);
    FIELD().PROTECTED().NAME(mSource);
    FIELD().PROTECTED().NAME(mImage);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mValid);
    FIELD().PROTECTED().NAME(mError);
    FIELD().PROTECTED().NAME(mWarnings);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mVersion);
    FIELD().PROTECTED().NAME(mMeshes);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSubscribedOnRebuild);
}
END_META;
CLASS_METHODS_META(o2::VectorImageAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const VectorImageAsset&);
    FUNCTION().PUBLIC().SIGNATURE(bool, SetSource, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetSource);
    FUNCTION().PUBLIC().SIGNATURE(const VectorImage&, GetImage);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsValid);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetError);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<String>&, GetWarnings);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetSize);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWidth);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSliceBorder, const BorderI&);
    FUNCTION().PUBLIC().SIGNATURE(BorderI, GetSliceBorder);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDefaultMode, SpriteMode);
    FUNCTION().PUBLIC().SIGNATURE(SpriteMode, GetDefaultMode);
    FUNCTION().PUBLIC().SIGNATURE(Ref<VectorImageMesh>, GetMesh, const Vec2F&, bool);
    FUNCTION().PUBLIC().SIGNATURE(Ref<VectorImageMesh>, GetSlicedMesh, const Vec2F&, const BorderF&, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMeshBuilt, const Vec2F&, const BorderF&, bool);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vec2F, QuantizePixelScale, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(int, GetCachedMeshesCount);
    FUNCTION().PUBLIC().SIGNATURE(void, ClearMeshCache);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(UInt, GetTessellationsCount);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(double, GetTessellationTime);
    FUNCTION().PUBLIC().SIGNATURE(UInt, GetVersion);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Meta>, GetMeta);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<VectorImageMesh>, GetMesh, const Vec2F&, const BorderF&, bool, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetsRebuilt, const Vector<UID>&);
}
END_META;

CLASS_BASES_META(o2::VectorImageAsset::Meta)
{
    BASE_CLASS(o2::DefaultAssetMeta<VectorImageAsset>);
}
END_META;
CLASS_FIELDS_META(o2::VectorImageAsset::Meta)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(sliceBorder);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(SpriteMode::Default).NAME(defaultMode);
}
END_META;
CLASS_METHODS_META(o2::VectorImageAsset::Meta)
{

    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, AssetMeta*);
}
END_META;
// --- END META ---
