#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Types/MaterialAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Scene/Component.h"

namespace o2
{
    // 3D primitive shape type
    enum class PrimitiveType3D { Box, Sphere, Plane, Cylinder };

    // -------------------------------------------------------------------------
    // 3D primitive mesh component. Builds parametric geometry (box, sphere,
    // plane, cylinder) and draws it transformed by owner actor's 3D transform
    // -------------------------------------------------------------------------
    class MeshPrimitiveComponent: public Component
    {
    public:
        PROPERTIES(MeshPrimitiveComponent);
        PROPERTY(PrimitiveType3D, primitiveType, SetPrimitiveType, GetPrimitiveType); // Primitive type property
        PROPERTY(Vec3F, size, SetSize, GetSize);                                      // Size property
        PROPERTY(int, segments, SetSegments, GetSegments);                            // Segments count property
        PROPERTY(Color4, color, SetColor, GetColor);                                  // Color property
        PROPERTY(AssetRef<ImageAsset>, texture, SetTexture, GetTexture);              // Texture property
        PROPERTY(bool, shaded, SetShaded, IsShaded);                                  // Baked lambert shading property
        PROPERTY(AssetRef<MaterialAsset>, material, SetMaterialAsset, GetMaterialAsset); // Material (asset) for rendering

    public:
        // Default constructor
        MeshPrimitiveComponent();

        // Copy-constructor
        MeshPrimitiveComponent(const MeshPrimitiveComponent& other);

        // Destructor
        ~MeshPrimitiveComponent();

        // Assign operator
        MeshPrimitiveComponent& operator=(const MeshPrimitiveComponent& other);

        // Returns drawing mesh, rebuilds it if parameters were changed
        const Mesh& GetMesh();

        // Sets primitive type
        void SetPrimitiveType(PrimitiveType3D type);

        // Returns primitive type
        PrimitiveType3D GetPrimitiveType() const;

        // Sets size; sphere and cylinder take radius from x, cylinder height from y
        void SetSize(const Vec3F& size);

        // Returns size
        const Vec3F& GetSize() const;

        // Sets segments count for sphere and cylinder
        void SetSegments(int segments);

        // Returns segments count
        int GetSegments() const;

        // Sets color
        void SetColor(const Color4& color);

        // Returns color
        const Color4& GetColor() const;

        // Sets texture
        void SetTexture(const AssetRef<ImageAsset>& texture);

        // Returns texture
        const AssetRef<ImageAsset>& GetTexture() const;

        // Sets baked lambert shading enabled
        void SetShaded(bool shaded);

        // Returns is baked lambert shading enabled
        bool IsShaded() const;

        // Sets material by asset reference. GetMaterial() will return asset's material
        void SetMaterialAsset(const AssetRef<MaterialAsset>& asset);

        // Returns material asset reference, or null if not set
        const AssetRef<MaterialAsset>& GetMaterialAsset() const;

        // Sets material for rendering. Pass nullptr for the pass default material. Clears material asset
        void SetMaterial(const Ref<Material>& material);

        // Returns current material (from material asset if set, else direct override; may be null)
        Ref<Material> GetMaterial() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        // Returns 3D render category
        SceneDrawableCategory GetSceneDrawableCategory() const override;

        // Returns world bounds of the mesh
        bool Get3DDrawableBounds(AABB& bounds) override;

        // Returns local space bounds of the mesh geometry
        bool Get3DDrawableLocalBounds(AABB& bounds) override;

        SERIALIZABLE(MeshPrimitiveComponent);
        CLONEABLE_REF(MeshPrimitiveComponent);

    protected:
        Mesh       mMesh;      // Drawing mesh in world space
        Mesh3DData mLocalData; // Local space geometry, built from parameters

        PrimitiveType3D mPrimitiveType = PrimitiveType3D::Box; // Primitive type @SERIALIZABLE
        Vec3F           mSize = Vec3F(100, 100, 100);          // Primitive size @SERIALIZABLE
        int             mSegments = 24;                        // Sphere and cylinder segments @SERIALIZABLE
        Color4          mColor = Color4::White();              // Mesh color @SERIALIZABLE
        bool            mShaded = true;                        // Bake lambert shading into vertex colors @SERIALIZABLE

        AssetRef<ImageAsset> mTexture; // Texture asset @SERIALIZABLE

        AssetRef<MaterialAsset> mMaterialAsset; // Material asset; when set, the mesh is drawn with it @SERIALIZABLE @EDITOR_PROPERTY
        Ref<Material>           mMaterial;      // Direct material override, not serialized

        bool mNeedRebuildMesh = true; // True, when local geometry is dirty and need to rebuild
        bool mMeshRawAlbedo = false;  // True, when mesh was filled without baked shading (G-buffer mode)

    protected:
        // Draws mesh
        void OnDraw() override;

        // Called when actor's transform was changed
        void OnTransformUpdated() override;

        // Rebuilds local geometry and applies world transform
        void RebuildMesh();

        // Fills drawing mesh from local geometry with owner's world 3D transform
        void ApplyTransform();

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;
    };
}
// --- META ---

PRE_ENUM_META(o2::PrimitiveType3D);

CLASS_BASES_META(o2::MeshPrimitiveComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::MeshPrimitiveComponent)
{
    FIELD().PUBLIC().NAME(primitiveType);
    FIELD().PUBLIC().NAME(size);
    FIELD().PUBLIC().NAME(segments);
    FIELD().PUBLIC().NAME(color);
    FIELD().PUBLIC().NAME(texture);
    FIELD().PUBLIC().NAME(shaded);
    FIELD().PUBLIC().NAME(material);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().NAME(mLocalData);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(PrimitiveType3D::Box).NAME(mPrimitiveType);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec3F(100, 100, 100)).NAME(mSize);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(24).NAME(mSegments);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mShaded);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mTexture);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mMaterialAsset);
    FIELD().PROTECTED().NAME(mMaterial);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mNeedRebuildMesh);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mMeshRawAlbedo);
}
END_META;
CLASS_METHODS_META(o2::MeshPrimitiveComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const MeshPrimitiveComponent&);
    FUNCTION().PUBLIC().SIGNATURE(const Mesh&, GetMesh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPrimitiveType, PrimitiveType3D);
    FUNCTION().PUBLIC().SIGNATURE(PrimitiveType3D, GetPrimitiveType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSize, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(const Vec3F&, GetSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSegments, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSegments);
    FUNCTION().PUBLIC().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(const Color4&, GetColor);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTexture, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<ImageAsset>&, GetTexture);
    FUNCTION().PUBLIC().SIGNATURE(void, SetShaded, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsShaded);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaterialAsset, const AssetRef<MaterialAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<MaterialAsset>&, GetMaterialAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaterial, const Ref<Material>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Material>, GetMaterial);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE(SceneDrawableCategory, GetSceneDrawableCategory);
    FUNCTION().PUBLIC().SIGNATURE(bool, Get3DDrawableBounds, AABB&);
    FUNCTION().PUBLIC().SIGNATURE(bool, Get3DDrawableLocalBounds, AABB&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, ApplyTransform);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
