#include "o2/stdafx.h"
#include "MeshPrimitiveComponent.h"
#include "o2/Render/Mesh3DFill.h"

#include "o2/Scene/Actor.h"

namespace o2
{
    MeshPrimitiveComponent::MeshPrimitiveComponent()
    {}

    MeshPrimitiveComponent::MeshPrimitiveComponent(const MeshPrimitiveComponent& other):
        Component(other), mPrimitiveType(other.mPrimitiveType), mSize(other.mSize), mSegments(other.mSegments),
        mColor(other.mColor), mShaded(other.mShaded), mTexture(other.mTexture), mMaterialAsset(other.mMaterialAsset),
        mMaterial(other.mMaterial)
    {}

    MeshPrimitiveComponent::~MeshPrimitiveComponent()
    {}

    MeshPrimitiveComponent& MeshPrimitiveComponent::operator=(const MeshPrimitiveComponent& other)
    {
        Component::operator=(other);

        mPrimitiveType = other.mPrimitiveType;
        mSize = other.mSize;
        mSegments = other.mSegments;
        mColor = other.mColor;
        mShaded = other.mShaded;
        mTexture = other.mTexture;
        mMaterialAsset = other.mMaterialAsset;
        mMaterial = other.mMaterial;
        mNeedRebuildMesh = true;

        return *this;
    }

    const Mesh& MeshPrimitiveComponent::GetMesh()
    {
        EnsureMesh();
        return mMesh;
    }

    void MeshPrimitiveComponent::SetPrimitiveType(PrimitiveType3D type)
    {
        mPrimitiveType = type;
        mNeedRebuildMesh = true;
    }

    PrimitiveType3D MeshPrimitiveComponent::GetPrimitiveType() const
    {
        return mPrimitiveType;
    }

    void MeshPrimitiveComponent::SetSize(const Vec3F& size)
    {
        mSize = size;
        mNeedRebuildMesh = true;
    }

    const Vec3F& MeshPrimitiveComponent::GetSize() const
    {
        return mSize;
    }

    void MeshPrimitiveComponent::SetSegments(int segments)
    {
        mSegments = segments;
        mNeedRebuildMesh = true;
    }

    int MeshPrimitiveComponent::GetSegments() const
    {
        return mSegments;
    }

    void MeshPrimitiveComponent::SetColor(const Color4& color)
    {
        mColor = color;
        mNeedRebuildMesh = true;
    }

    const Color4& MeshPrimitiveComponent::GetColor() const
    {
        return mColor;
    }

    void MeshPrimitiveComponent::SetTexture(const AssetRef<ImageAsset>& texture)
    {
        mTexture = texture;
        mNeedRebuildMesh = true;
    }

    const AssetRef<ImageAsset>& MeshPrimitiveComponent::GetTexture() const
    {
        return mTexture;
    }

    void MeshPrimitiveComponent::SetShaded(bool shaded)
    {
        mShaded = shaded;
        mNeedRebuildMesh = true;
    }

    bool MeshPrimitiveComponent::IsShaded() const
    {
        return mShaded;
    }

    void MeshPrimitiveComponent::SetMaterialAsset(const AssetRef<MaterialAsset>& asset)
    {
        mMaterialAsset = asset;
        mMaterial = nullptr;
    }

    const AssetRef<MaterialAsset>& MeshPrimitiveComponent::GetMaterialAsset() const
    {
        return mMaterialAsset;
    }

    void MeshPrimitiveComponent::SetMaterial(const Ref<Material>& material)
    {
        mMaterial = material;
        mMaterialAsset = AssetRef<MaterialAsset>();
    }

    Ref<Material> MeshPrimitiveComponent::GetMaterial() const
    {
        if (mMaterialAsset)
            return mMaterialAsset.GetRef();

        return mMaterial;
    }

    String MeshPrimitiveComponent::GetName()
    {
        return "Mesh primitive 3D";
    }

    String MeshPrimitiveComponent::GetCategory()
    {
        return "Render";
    }

    String MeshPrimitiveComponent::GetIcon()
    {
        return "ui/UI4_image_component.png";
    }

    SceneDrawableCategory MeshPrimitiveComponent::GetSceneDrawableCategory() const
    {
        return SceneDrawableCategory::Scene3D;
    }

    bool MeshPrimitiveComponent::Get3DDrawableBounds(AABB& bounds)
    {
        AABB local;
        if (!Get3DDrawableLocalBounds(local))
            return false;

        Mat4 worldTransform;
        if (auto owner = mOwner.Lock())
            worldTransform = owner->transform->GetWorldTransform3D();

        // Bound of the transformed local box, so asking for bounds doesn't have to fill the whole mesh
        Vec3F corners[8];
        for (int i = 0; i < 8; i++)
        {
            corners[i] = worldTransform.TransformPoint(Vec3F((i & 1) ? local.max.x : local.min.x,
                                                             (i & 2) ? local.max.y : local.min.y,
                                                             (i & 4) ? local.max.z : local.min.z));
        }

        bounds = AABB::Bound(corners, 8);
        return true;
    }

    bool MeshPrimitiveComponent::Get3DDrawableLocalBounds(AABB& bounds)
    {
        if (mNeedRebuildMesh)
            RebuildMesh();

        bounds = mLocalBounds;
        return mHasLocalBounds;
    }

    void MeshPrimitiveComponent::OnDraw()
    {
        // The shading mode is picked up here and not in EnsureMesh, so that filling the mesh for
        // something else (bounds queries) doesn't flip it and cost the drawing pass a second fill.
        // The raw albedo mode only changes baked shading, an unshaded mesh fills identically
        bool rawAlbedo = ScenePassFilters::IsRawAlbedoMode();
        if (mShaded && rawAlbedo != mMeshRawAlbedo)
        {
            mMeshRawAlbedo = rawAlbedo;
            mNeedApplyTransform = true;
        }

        EnsureMesh();

        mMesh.SetMaterial(GetMaterial());
        mMesh.Draw();
    }

    void MeshPrimitiveComponent::OnTransformUpdated()
    {
        // Filling the mesh is deferred to the first use: transforming every vertex here would be redone
        // anyway by the first pass that draws with a different shading mode
        mNeedApplyTransform = true;
    }

    void MeshPrimitiveComponent::EnsureMesh()
    {
        if (mNeedRebuildMesh)
            RebuildMesh();

        if (mNeedApplyTransform)
            ApplyTransform();
    }

    void MeshPrimitiveComponent::RebuildMesh()
    {
        switch (mPrimitiveType)
        {
            case PrimitiveType3D::Box:
                mLocalData = Mesh3DPrimitives::BuildBox(mSize);
                break;

            case PrimitiveType3D::Sphere:
                mLocalData = Mesh3DPrimitives::BuildSphere(mSize.x*0.5f, mSegments, Math::Max(mSegments/2, 2));
                break;

            case PrimitiveType3D::Plane:
                mLocalData = Mesh3DPrimitives::BuildPlane(Vec2F(mSize.x, mSize.y));
                break;

            case PrimitiveType3D::Cylinder:
                mLocalData = Mesh3DPrimitives::BuildCylinder(mSize.x*0.5f, mSize.y, mSegments);
                break;
        }

        mNeedRebuildMesh = false;
        mNeedApplyTransform = true;
        mHasLocalBounds = mLocalData.GetBounds(mLocalBounds);
    }

    void MeshPrimitiveComponent::ApplyTransform()
    {
        Mat4 worldTransform;
        if (auto owner = mOwner.Lock())
            worldTransform = owner->transform->GetWorldTransform3D();

        mNeedApplyTransform = false;

        TextureSource textureSource = mTexture ? mTexture->GetTextureSource() : TextureSource();
        Mesh3DPrimitives::FillMesh(mMesh, mLocalData, worldTransform, mColor, textureSource, mShaded && !mMeshRawAlbedo);
    }

    void MeshPrimitiveComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        mNeedRebuildMesh = true;
    }

    void MeshPrimitiveComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        mNeedRebuildMesh = true;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::MeshPrimitiveComponent>);
// --- META ---

ENUM_META(o2::PrimitiveType3D, o2__PrimitiveType3D)
{
    ENUM_ENTRY(Box);
    ENUM_ENTRY(Cylinder);
    ENUM_ENTRY(Plane);
    ENUM_ENTRY(Sphere);
}
END_ENUM_META;

DECLARE_CLASS(o2::MeshPrimitiveComponent, o2__MeshPrimitiveComponent);
// --- END META ---
