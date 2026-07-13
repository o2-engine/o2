#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Types/MaterialAsset.h"
#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Scene/Component.h"

namespace o2
{
    // ------------------------------------------------------------------------
    // 3D mesh component. Draws geometry from Mesh3DAsset transformed by owner
    // actor's 3D transform
    // ------------------------------------------------------------------------
    class Mesh3DComponent: public Component
    {
    public:
        PROPERTIES(Mesh3DComponent);
        PROPERTY(AssetRef<Mesh3DAsset>, mesh, SetMeshAsset, GetMeshAsset);   // Mesh asset property
        PROPERTY(Color4, color, SetColor, GetColor);                         // Color property
        PROPERTY(AssetRef<ImageAsset>, texture, SetTexture, GetTexture);    // Texture property
        PROPERTY(bool, shaded, SetShaded, IsShaded);                        // Baked lambert shading property
        PROPERTY(AssetRef<MaterialAsset>, material, SetMaterialAsset, GetMaterialAsset); // Material (asset) for rendering

    public:
        // Default constructor
        Mesh3DComponent();

        // Copy-constructor
        Mesh3DComponent(const Mesh3DComponent& other);

        // Destructor
        ~Mesh3DComponent();

        // Assign operator
        Mesh3DComponent& operator=(const Mesh3DComponent& other);

        // Returns drawing mesh, rebuilds it if parameters were changed
        const Mesh& GetMesh();

        // Sets mesh asset
        void SetMeshAsset(const AssetRef<Mesh3DAsset>& mesh);

        // Returns mesh asset
        const AssetRef<Mesh3DAsset>& GetMeshAsset() const;

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

        SERIALIZABLE(Mesh3DComponent);
        CLONEABLE_REF(Mesh3DComponent);

    protected:
        Mesh mMesh; // Drawing mesh in world space

        AssetRef<Mesh3DAsset> mMeshAsset; // Mesh asset @SERIALIZABLE
        AssetRef<ImageAsset>  mTexture;   // Texture asset @SERIALIZABLE

        AssetRef<MaterialAsset> mMaterialAsset; // Material asset; when set, the mesh is drawn with it @SERIALIZABLE @EDITOR_PROPERTY
        Ref<Material>           mMaterial;      // Direct material override, not serialized

        Color4 mColor = Color4::White(); // Mesh color @SERIALIZABLE
        bool   mShaded = true;           // Bake lambert shading into vertex colors @SERIALIZABLE

        bool mNeedRebuildMesh = true; // True, when mesh is dirty and need to rebuild
        bool mMeshRawAlbedo = false;  // True, when mesh was filled without baked shading (G-buffer mode)

    protected:
        // Draws mesh
        void OnDraw() override;

        // Called when actor's transform was changed
        void OnTransformUpdated() override;

        // Fills drawing mesh from asset geometry with owner's world 3D transform
        void RebuildMesh();

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::Mesh3DComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::Mesh3DComponent)
{
    FIELD().PUBLIC().NAME(mesh);
    FIELD().PUBLIC().NAME(color);
    FIELD().PUBLIC().NAME(texture);
    FIELD().PUBLIC().NAME(shaded);
    FIELD().PUBLIC().NAME(material);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mMeshAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mTexture);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mMaterialAsset);
    FIELD().PROTECTED().NAME(mMaterial);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mShaded);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mNeedRebuildMesh);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mMeshRawAlbedo);
}
END_META;
CLASS_METHODS_META(o2::Mesh3DComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Mesh3DComponent&);
    FUNCTION().PUBLIC().SIGNATURE(const Mesh&, GetMesh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMeshAsset, const AssetRef<Mesh3DAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<Mesh3DAsset>&, GetMeshAsset);
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
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
