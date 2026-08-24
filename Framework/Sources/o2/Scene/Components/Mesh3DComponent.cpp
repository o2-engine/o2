#include "o2/stdafx.h"
#include "Mesh3DComponent.h"
#include "o2/Render/Mesh3DFill.h"

#include "o2/Scene/Actor.h"

namespace o2
{
    Mesh3DComponent::Mesh3DComponent()
    {}

    Mesh3DComponent::Mesh3DComponent(const Mesh3DComponent& other):
        Component(other), mMeshAsset(other.mMeshAsset), mTexture(other.mTexture), mColor(other.mColor),
        mShaded(other.mShaded), mMaterialAsset(other.mMaterialAsset), mMaterial(other.mMaterial)
    {}

    Mesh3DComponent::~Mesh3DComponent()
    {}

    Mesh3DComponent& Mesh3DComponent::operator=(const Mesh3DComponent& other)
    {
        Component::operator=(other);

        mMeshAsset = other.mMeshAsset;
        mTexture = other.mTexture;
        mColor = other.mColor;
        mShaded = other.mShaded;
        mMaterialAsset = other.mMaterialAsset;
        mMaterial = other.mMaterial;
        mNeedRebuildMesh = true;

        return *this;
    }

    const Mesh& Mesh3DComponent::GetMesh()
    {
        if (mNeedRebuildMesh)
            RebuildMesh();

        return mMesh;
    }

    void Mesh3DComponent::SetMeshAsset(const AssetRef<Mesh3DAsset>& mesh)
    {
        mMeshAsset = mesh;
        mNeedRebuildMesh = true;
    }

    const AssetRef<Mesh3DAsset>& Mesh3DComponent::GetMeshAsset() const
    {
        return mMeshAsset;
    }

    void Mesh3DComponent::SetColor(const Color4& color)
    {
        mColor = color;
        mNeedRebuildMesh = true;
    }

    const Color4& Mesh3DComponent::GetColor() const
    {
        return mColor;
    }

    void Mesh3DComponent::SetTexture(const AssetRef<ImageAsset>& texture)
    {
        mTexture = texture;
        mNeedRebuildMesh = true;
    }

    const AssetRef<ImageAsset>& Mesh3DComponent::GetTexture() const
    {
        return mTexture;
    }

    void Mesh3DComponent::SetShaded(bool shaded)
    {
        mShaded = shaded;
        mNeedRebuildMesh = true;
    }

    bool Mesh3DComponent::IsShaded() const
    {
        return mShaded;
    }

    void Mesh3DComponent::SetMaterialAsset(const AssetRef<MaterialAsset>& asset)
    {
        mMaterialAsset = asset;
        mMaterial = nullptr;
    }

    const AssetRef<MaterialAsset>& Mesh3DComponent::GetMaterialAsset() const
    {
        return mMaterialAsset;
    }

    void Mesh3DComponent::SetMaterial(const Ref<Material>& material)
    {
        mMaterial = material;
        mMaterialAsset = AssetRef<MaterialAsset>();
    }

    Ref<Material> Mesh3DComponent::GetMaterial() const
    {
        if (mMaterialAsset)
            return mMaterialAsset.GetRef();

        return mMaterial;
    }

    String Mesh3DComponent::GetName()
    {
        return "Mesh 3D";
    }

    String Mesh3DComponent::GetCategory()
    {
        return "Render";
    }

    String Mesh3DComponent::GetIcon()
    {
        return "ui/UI4_image_component.png";
    }

    SceneDrawableCategory Mesh3DComponent::GetSceneDrawableCategory() const
    {
        return SceneDrawableCategory::Scene3D;
    }

    bool Mesh3DComponent::Get3DDrawableBounds(AABB& bounds)
    {
        return Mesh3DPrimitives::GetMeshBounds(GetMesh(), bounds);
    }

    bool Mesh3DComponent::Get3DDrawableLocalBounds(AABB& bounds)
    {
        return mMeshAsset ? mMeshAsset->GetMeshData().GetBounds(bounds) : false;
    }

    void Mesh3DComponent::OnDraw()
    {
        // The raw albedo mode only changes baked shading, an unshaded mesh fills identically
        if (mNeedRebuildMesh || (mShaded && ScenePassFilters::IsRawAlbedoMode() != mMeshRawAlbedo))
            RebuildMesh();

        mMesh.SetMaterial(GetMaterial());
        mMesh.Draw();
    }

    void Mesh3DComponent::OnTransformUpdated()
    {
        RebuildMesh();
    }

    void Mesh3DComponent::RebuildMesh()
    {
        mNeedRebuildMesh = false;

        if (!mMeshAsset)
        {
            mMesh.vertexCount = 0;
            mMesh.polyCount = 0;
            return;
        }

        Mat4 worldTransform;
        if (auto owner = mOwner.Lock())
            worldTransform = owner->transform->GetWorldTransform3D();

        mMeshRawAlbedo = ScenePassFilters::IsRawAlbedoMode();

        TextureSource textureSource = mTexture ? mTexture->GetTextureSource() : TextureSource();
        Mesh3DPrimitives::FillMesh(mMesh, mMeshAsset->GetMeshData(), worldTransform, mColor, textureSource,
                                   mShaded && !mMeshRawAlbedo);
    }

    void Mesh3DComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        mNeedRebuildMesh = true;
    }

    void Mesh3DComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        mNeedRebuildMesh = true;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Mesh3DComponent>);
// --- META ---

DECLARE_CLASS(o2::Mesh3DComponent, o2__Mesh3DComponent);
// --- END META ---
