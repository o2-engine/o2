#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Scene/Physics/SplineCollider.h"
#include "o2/Utils/Math/Color.h"

namespace o2
{
    // ----------------------------------------------------------------
    // Spline collider with a tiling drawable strip along the curve.
    // Generates a mesh that follows the spline at a given perpendicular
    // offset, with configurable strip width. The sprite tiles
    // horizontally along the curve and may be part of an atlas — at
    // each tile boundary the mesh splits (duplicate vertices) so that
    // atlas UVs do not bleed between tiles.
    // ----------------------------------------------------------------
    class SplineMeshCollider: public SplineCollider
    {
    public:
        PROPERTIES(SplineMeshCollider);
        PROPERTY(AssetRef<ImageAsset>, image, SetImage, GetImage); // Sprite image
        PROPERTY(float, width, SetWidth, GetWidth);                // Strip width
        PROPERTY(float, offset, SetOffset, GetOffset);             // Perpendicular offset from spline center
        PROPERTY(Color4, color, SetColor, GetColor);               // Mesh tint color

    public:
        SplineMeshCollider();
        SplineMeshCollider(const SplineMeshCollider& other);
        ~SplineMeshCollider();

        SplineMeshCollider& operator=(const SplineMeshCollider& other);

        void SetImage(const AssetRef<ImageAsset>& image);
        const AssetRef<ImageAsset>& GetImage() const;

        void SetWidth(float w);
        float GetWidth() const;

        void SetOffset(float o);
        float GetOffset() const;

        void SetColor(const Color4& c);
        const Color4& GetColor() const;

        static String GetName();
        static String GetCategory();
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(SplineMeshCollider);
        CLONEABLE_REF(SplineMeshCollider);

    protected:
        AssetRef<ImageAsset> mImage;          // Sprite image @SERIALIZABLE
        float                mWidth = 50.0f;  // Strip width @SERIALIZABLE
        float                mOffset = 0.0f;  // Perpendicular offset @SERIALIZABLE
        Color4               mColor = Color4::White(); // Tint color @SERIALIZABLE

        Mesh mMesh;                  // Drawing mesh, built from spline
        bool mNeedUpdateMesh = true; // True when mesh must be rebuilt before next draw

    protected:
        // Override: also marks the mesh dirty when spline keys change
        void OnSplineChanged() override;

        // Component lifecycle
        void OnDraw() override;
        void OnTransformUpdated() override;
        void OnAddToScene() override;

        // Builds mMesh from the current spline + sprite + width + offset
        void UpdateMesh();
    };
}
// --- META ---

CLASS_BASES_META(o2::SplineMeshCollider)
{
    BASE_CLASS(o2::SplineCollider);
}
END_META;
CLASS_FIELDS_META(o2::SplineMeshCollider)
{
    FIELD().PUBLIC().NAME(image);
    FIELD().PUBLIC().NAME(width);
    FIELD().PUBLIC().NAME(offset);
    FIELD().PUBLIC().NAME(color);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mImage);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(50.0f).NAME(mWidth);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mOffset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mNeedUpdateMesh);
}
END_META;
CLASS_METHODS_META(o2::SplineMeshCollider)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SplineMeshCollider&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetImage, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<ImageAsset>&, GetImage);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, SetOffset, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetOffset);
    FUNCTION().PUBLIC().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(const Color4&, GetColor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSplineChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh);
}
END_META;
// --- END META ---
