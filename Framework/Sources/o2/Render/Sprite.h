#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/RectDrawable.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Math/Border.h"

namespace o2
{
    class Bitmap;

    // -----------------
    // Quad image sprite
    // -----------------
    class Sprite: public IRectDrawable
    {
    public:
        PROPERTIES(Sprite);
        PROPERTY(TextureRef, texture, SetTexture, GetTexture);                       // Texture property @SCRIPTABLE
        PROPERTY(RectI, textureSrcRect, SetTextureSrcRect, GetTextureSrcRect);         // Texture source rectangle property @SCRIPTABLE
        PROPERTY(AssetRef<ImageAsset>, image, SetImageAsset, GetImageAsset);                  // Sets image asset @SCRIPTABLE
        PROPERTY(String, imageName, LoadFromImage, GetImageName);                      // Sets image asset path @SCRIPTABLE
        PROPERTY(Color4, leftTopColor, SetLeftTopColor, GetLeftTopCorner);             // Color of left top corner property @SCRIPTABLE
        PROPERTY(Color4, rightTopColor, SetRightTopColor, GetRightTopCorner);          // Color of right top corner property @SCRIPTABLE
        PROPERTY(Color4, leftBottomColor, SetLeftBottomColor, GetLeftBottomCorner);    // Color of left bottom corner property @SCRIPTABLE
        PROPERTY(Color4, rightBottomColor, SetRightBottomColor, GetRightBottomCorner); // Color of right bottom corner property @SCRIPTABLE
        PROPERTY(SpriteMode, mode, SetMode, GetMode);                                  // Sprite drawing mode property @SCRIPTABLE
        PROPERTY(float, fill, SetFill, GetFill);                                       // Sprite fill property @SCRIPTABLE
        PROPERTY(float, tileScale, SetTileScale, GetTileScale);                        // Sprite tile scale property, 1.0f is default @SCRIPTABLE
        PROPERTY(BorderI, sliceBorder, SetSliceBorder, GetSliceBorder);                // Slice border property @SCRIPTABLE

    public:
        // Default constructor
        Sprite();

        // Constructor from image asset
        explicit Sprite(const AssetRef<ImageAsset>& image);

        // Constructor from image asset by path @SCRIPTABLE
        explicit Sprite(const String& imagePath);

        // Constructor from image asset by id
        explicit Sprite(UID imageId);

        // Constructor from color
        explicit Sprite(const Color4& color);

        // Constructor from bitmap
        explicit Sprite(const Bitmap& bitmap);

        // Constructor from texture and source rectangle
        Sprite(TextureRef texture, const RectI& srcRect = RectI());

        // Copy-constructor
        Sprite(const Sprite& other);

        // Destructor
        ~Sprite();

        // Assign operator
        Sprite& operator=(const Sprite& other);

        // Equals operator
        bool operator==(const Sprite& other) const;

        // Not equals operator
        bool operator!=(const Sprite& other) const;

        // Loads sprite from image asset @SCRIPTABLE_NAME(LoadFromImageRef)
        void LoadFromImage(const AssetRef<ImageAsset>& image, bool setSizeByImage = true);

        // Loads sprite from image asset by path @SCRIPTABLE
        void LoadFromImage(const String& imagePath, bool setSizeByImage = true);

        // Loads sprite from image asset by id @SCRIPTABLE_NAME(LoadFromImageUID)
        void LoadFromImage(UID imageId, bool setSizeByImage = true);

        // Loads sprite from mono color @SCRIPTABLE
        void LoadMonoColor(const Color4& color);

        // Loads sprite from bitmap
        void LoadFromBitmap(const Bitmap& bitmap, bool setSizeByImage = true);

        // Draws sprite @SCRIPTABLE
        void Draw() override;

        // Sets using texture @SCRIPTABLE
        void SetTexture(const TextureRef& texture);

        // Returns using texture @SCRIPTABLE
        const TextureRef& GetTexture() const;

        // Sets texture source rectangle
        void SetTextureSrcRect(const RectI& rect);

        // Returns texture source rectangle
        RectI GetTextureSrcRect() const;

        // Returns original or texture source size
        Vec2I GetOriginalSize() const;

        // Sets corner color
        void SetCornerColor(Corner corner, const Color4& color);

		// Sets colors of all corners
		void SetCornerColors(const Color4& leftTop, const Color4& rightTop,
							 const Color4& rightBottom, const Color4& leftBottom);

        // Returns corner color
        Color4 GetCornerColor(Corner corner) const;

        // Sets left top corner color
        void SetLeftTopColor(const Color4& color);

        // Returns left top corner color
        Color4 GetLeftTopCorner() const;

        // Sets right top corner color
        void SetRightTopColor(const Color4& color);

        // Returns right top corner color
        Color4 GetRightTopCorner() const;

        // Sets right bottom corner color
        void SetRightBottomColor(const Color4& color);

        // Returns right bottom corner color
        Color4 GetRightBottomCorner() const;

        // Sets left bottom corner color
        void SetLeftBottomColor(const Color4& color);

        // Returns left bottom corner color
        Color4 GetLeftBottomCorner() const;

        // Sets sprite fill value (0 ... 1) @SCRIPTABLE
        void SetFill(float fill);

        // Returns sprite fill @SCRIPTABLE
        float GetFill() const;

        // Sets tile scale. 1.0f is default
        void SetTileScale(float scale);

        // Returns tile scale
        float GetTileScale() const;

        // Sets sprite drawing mode @SCRIPTABLE
        void SetMode(SpriteMode mode);

        // Returns sprite drawing mode @SCRIPTABLE
        SpriteMode GetMode() const;

        // Sets sprite slice border
        void SetSliceBorder(const BorderI& border);

        // Returns sprite slice border
        BorderI GetSliceBorder() const;

        // Sets asset @SCRIPTABLE
        void SetImageAsset(const AssetRef<ImageAsset>& asset);

        // Returns asset @SCRIPTABLE
        AssetRef<ImageAsset> GetImageAsset() const;

        // Returns image asset name @SCRIPTABLE
        const String& GetImageName() const;

        // Returns atlas asset id (returns 0 when sprite is not from atlas) @SCRIPTABLE
        UID GetAtlasAssetId() const;

        // Sets size by texture source rectangle size @SCRIPTABLE
        void NormalizeSize();

        // Sets size with equal aspect as texture source rectangle by width @SCRIPTABLE
        void NormalizeAspectByWidth();

        // Sets size with equal aspect as texture source rectangle by width @SCRIPTABLE
        void NormalizeAspectByHeight();

        // Sets size with equal aspect as texture source rectangle by nearest value @SCRIPTABLE
        void NormalizeAspect();

        // Calling when serializing
        void OnSerialize(DataValue& node) const override;

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        SERIALIZABLE(Sprite);
        CLONEABLE_REF(Sprite);

    protected:
        Color4 mCornersColors[4]; // Corners colors

        RectI                mTextureSrcRect; // Texture source rectangle
        AssetRef<ImageAsset> mImageAsset;     // Image asset @SERIALIZABLE

        SpriteMode           mMode = SpriteMode::Default; // Drawing mode @SERIALIZABLE
        BorderI              mSlices;                     // Slice borders @SERIALIZABLE
        float                mFill = 1.0f;                // Sprite fillness @SERIALIZABLE
        float                mTileScale = 1.0f;           // Scale of tiles in tiled mode. 1.0f is default and equals to default image size @SERIALIZABLE
        
        Mesh mMesh; // Drawing mesh

        void(Sprite::*mMeshBuildFunc)(); // Mesh building function pointer (by mode)

    protected:
        // Called when basis was changed
        void BasisChanged() override;

        // Called when color was changed
        void OnColorChanged() override;

        // Called when material was changed (propagates to mesh)
        void OnMaterialChanged() override;

        // Initialized texture by image: uses atlas part or texture
        void InitializeTextureAndMaterial();

        // Updates mesh geometry
        void UpdateMesh();

        // Builds mesh for default mode
        void BuildDefaultMesh();

        // Builds mesh for sliced mode
        void BuildSlicedMesh();

        // Builds mesh for tiled mode
        void BuildTiledMesh();

        // Builds mesh for fixed aspect mode
        void BuildFixedAspectMesh();

        // Builds mesh for fill left to right mode
        void BuildFillLeftToRightMesh();

        // Builds mesh for fill right to left mode
        void BuildFillRightToLeftMesh();

        // Builds mesh for fill up to down mode
        void BuildFillUpToDownMesh();

        // Builds mesh for fill down to up mode
        void BuildFillDownToUpMesh();

        // Builds mesh for fill 360 clock wise mode
        void BuildFill360CWMesh();

        // Builds mesh for fill 360 counter clock wise mode
        void BuildFill360CCWMesh();

        // Called when assets was rebuilded
        void ReloadImage();

        friend class Render;
    };
}
// --- META ---

CLASS_BASES_META(o2::Sprite)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(o2::Sprite)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(texture);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(textureSrcRect);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(image);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(imageName);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(leftTopColor);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(rightTopColor);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(leftBottomColor);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(rightBottomColor);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(mode);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(fill);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(tileScale);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(sliceBorder);
    FIELD().PROTECTED().NAME(mCornersColors);
    FIELD().PROTECTED().NAME(mTextureSrcRect);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mImageAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(SpriteMode::Default).NAME(mMode);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSlices);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mFill);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mTileScale);
    FIELD().PROTECTED().NAME(mMesh);
}
END_META;
CLASS_METHODS_META(o2::Sprite)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(const String&);
    FUNCTION().PUBLIC().CONSTRUCTOR(UID);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Color4&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Bitmap&);
    FUNCTION().PUBLIC().CONSTRUCTOR(TextureRef, const RectI&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Sprite&);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(LoadFromImageRef).SIGNATURE(void, LoadFromImage, const AssetRef<ImageAsset>&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, LoadFromImage, const String&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_NAME_ATTRIBUTE(LoadFromImageUID).SIGNATURE(void, LoadFromImage, UID, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, LoadMonoColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, LoadFromBitmap, const Bitmap&, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetTexture, const TextureRef&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const TextureRef&, GetTexture);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTextureSrcRect, const RectI&);
    FUNCTION().PUBLIC().SIGNATURE(RectI, GetTextureSrcRect);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetOriginalSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCornerColor, Corner, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCornerColors, const Color4&, const Color4&, const Color4&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetCornerColor, Corner);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLeftTopColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetLeftTopCorner);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRightTopColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetRightTopCorner);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRightBottomColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetRightBottomCorner);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLeftBottomColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetLeftBottomCorner);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetFill, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetFill);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTileScale, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTileScale);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMode, SpriteMode);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(SpriteMode, GetMode);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSliceBorder, const BorderI&);
    FUNCTION().PUBLIC().SIGNATURE(BorderI, GetSliceBorder);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetImageAsset, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(AssetRef<ImageAsset>, GetImageAsset);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const String&, GetImageName);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(UID, GetAtlasAssetId);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeAspectByWidth);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeAspectByHeight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, NormalizeAspect);
    FUNCTION().PUBLIC().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, BasisChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnColorChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMaterialChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTextureAndMaterial);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildDefaultMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildSlicedMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildTiledMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildFixedAspectMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildFillLeftToRightMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildFillRightToLeftMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildFillUpToDownMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildFillDownToUpMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildFill360CWMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildFill360CCWMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, ReloadImage);
}
END_META;
// --- END META ---
