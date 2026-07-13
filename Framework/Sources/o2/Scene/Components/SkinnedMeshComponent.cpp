#include "o2/stdafx.h"
#include "SkinnedMeshComponent.h"

#include "o2/Animation/SkinnedModelAnimation.h"
#include "o2/Render/Material.h"
#include "o2/Render/Mesh3DFill.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    SkinnedMeshComponent::SkinnedMeshComponent()
    {}

    SkinnedMeshComponent::SkinnedMeshComponent(const SkinnedMeshComponent& other):
        Component(other), mModel(other.mModel), mTexture(other.mTexture), mMaterialAsset(other.mMaterialAsset),
        mMaterial(other.mMaterial), mColor(other.mColor), mShaded(other.mShaded),
        mAnimationName(other.mAnimationName), mPlaying(other.mPlaying), mLooped(other.mLooped),
        mSpeed(other.mSpeed), mTime(other.mTime), mUseBoneActors(other.mUseBoneActors),
        mGPUSkinning(other.mGPUSkinning)
    {}

    SkinnedMeshComponent::~SkinnedMeshComponent()
    {}

    SkinnedMeshComponent& SkinnedMeshComponent::operator=(const SkinnedMeshComponent& other)
    {
        Component::operator=(other);

        mModel = other.mModel;
        mTexture = other.mTexture;
        mMaterialAsset = other.mMaterialAsset;
        mMaterial = other.mMaterial;
        mColor = other.mColor;
        mShaded = other.mShaded;
        mAnimationName = other.mAnimationName;
        mPlaying = other.mPlaying;
        mLooped = other.mLooped;
        mSpeed = other.mSpeed;
        mTime = other.mTime;
        mUseBoneActors = other.mUseBoneActors;
        mGPUSkinning = other.mGPUSkinning;
        mNeedRebuildPose = true;
        mGPUMeshDirty = true;
        mBoneActors.Clear();

        return *this;
    }

    const Mesh& SkinnedMeshComponent::GetMesh()
    {
        if (mNeedRebuildPose || IsBonePoseChanged())
            RebuildPose();

        return mMesh;
    }

    void SkinnedMeshComponent::SetModelAsset(const AssetRef<SkinnedModelAsset>& model)
    {
        mModel = model;
        mTime = 0.0f;
        mNeedRebuildPose = true;
        mGPUMeshDirty = true;
        mBoneActors.Clear();
    }

    const AssetRef<SkinnedModelAsset>& SkinnedMeshComponent::GetModelAsset() const
    {
        return mModel;
    }

    void SkinnedMeshComponent::SetColor(const Color4& color)
    {
        mColor = color;
        mNeedRebuildPose = true;
        mGPUMeshDirty = true;
    }

    const Color4& SkinnedMeshComponent::GetColor() const
    {
        return mColor;
    }

    void SkinnedMeshComponent::SetTexture(const AssetRef<ImageAsset>& texture)
    {
        mTexture = texture;
        mNeedRebuildPose = true;
        mGPUMeshDirty = true;
    }

    const AssetRef<ImageAsset>& SkinnedMeshComponent::GetTexture() const
    {
        return mTexture;
    }

    void SkinnedMeshComponent::SetShaded(bool shaded)
    {
        mShaded = shaded;
        mNeedRebuildPose = true;
    }

    bool SkinnedMeshComponent::IsShaded() const
    {
        return mShaded;
    }

    void SkinnedMeshComponent::SetMaterialAsset(const AssetRef<MaterialAsset>& asset)
    {
        mMaterialAsset = asset;
        mMaterial = nullptr;
    }

    const AssetRef<MaterialAsset>& SkinnedMeshComponent::GetMaterialAsset() const
    {
        return mMaterialAsset;
    }

    void SkinnedMeshComponent::SetMaterial(const Ref<Material>& material)
    {
        mMaterial = material;
        mMaterialAsset = AssetRef<MaterialAsset>();
    }

    Ref<Material> SkinnedMeshComponent::GetMaterial() const
    {
        if (mMaterialAsset)
            return mMaterialAsset.GetRef();

        return mMaterial;
    }

    void SkinnedMeshComponent::SetAnimation(const String& name)
    {
        mAnimationName = name;
        mTime = 0.0f;
        mNeedRebuildPose = true;
    }

    const String& SkinnedMeshComponent::GetAnimation() const
    {
        return mAnimationName;
    }

    void SkinnedMeshComponent::SetPlaying(bool playing)
    {
        mPlaying = playing;
    }

    bool SkinnedMeshComponent::IsPlaying() const
    {
        return mPlaying;
    }

    void SkinnedMeshComponent::SetLooped(bool looped)
    {
        mLooped = looped;
    }

    bool SkinnedMeshComponent::IsLooped() const
    {
        return mLooped;
    }

    void SkinnedMeshComponent::SetSpeed(float speed)
    {
        mSpeed = speed;
    }

    float SkinnedMeshComponent::GetSpeed() const
    {
        return mSpeed;
    }

    void SkinnedMeshComponent::SetAnimationTime(float time)
    {
        mTime = time;
        mNeedRebuildPose = true;
    }

    float SkinnedMeshComponent::GetAnimationTime() const
    {
        return mTime;
    }

    float SkinnedMeshComponent::GetAnimationDuration() const
    {
        if (!mModel)
            return 0.0f;

        int animation = mModel->GetModelData().FindAnimation(mAnimationName);
        return animation >= 0 ? mModel->GetModelData().animations[animation].duration : 0.0f;
    }

    void SkinnedMeshComponent::CreateBoneActors()
    {
        auto owner = mOwner.Lock();
        if (!owner || !mModel)
            return;

        SkinnedModelAnimation::CreateBoneActors(owner, mModel->GetModelData());
        SetUseBoneActors(true);
    }

    void SkinnedMeshComponent::SetUseBoneActors(bool use)
    {
        mUseBoneActors = use;
        mBoneActors.Clear();
        mNeedRebuildPose = true;
    }

    bool SkinnedMeshComponent::IsUsingBoneActors() const
    {
        return mUseBoneActors;
    }

    void SkinnedMeshComponent::SetGPUSkinningEnabled(bool enabled)
    {
        mGPUSkinning = enabled;
        mNeedRebuildPose = true;
    }

    bool SkinnedMeshComponent::IsGPUSkinningEnabled() const
    {
        return mGPUSkinning;
    }

    void SkinnedMeshComponent::EvaluateModelPalette(Vector<Mat4>& outPalette)
    {
        outPalette.Clear();

        if (!mModel)
            return;

        const SkinnedModelData& modelData = mModel->GetModelData();

        if (mUseBoneActors && ResolveBoneActors())
        {
            Mat4 worldInverted;
            if (auto owner = mOwner.Lock())
                worldInverted = owner->transform->GetWorldTransform3D().Inverted();

            for (int i = 0; i < modelData.joints.Count(); i++)
            {
                Mat4 inverseBind = i < modelData.inverseBindMatrices.Count()
                    ? modelData.inverseBindMatrices[i]
                    : Mat4::Identity();

                auto boneActor = mBoneActors[i].Lock();
                outPalette.Add(worldInverted*boneActor->transform->GetWorldTransform3D()*inverseBind);
            }
        }
        else
        {
            int animation = modelData.FindAnimation(mAnimationName);
            modelData.EvaluateJointsPalette(animation, mTime, outPalette);
        }
    }

    String SkinnedMeshComponent::GetName()
    {
        return "Skinned mesh";
    }

    String SkinnedMeshComponent::GetCategory()
    {
        return "Render";
    }

    String SkinnedMeshComponent::GetIcon()
    {
        return "ui/UI4_image_component.png";
    }

    SceneDrawableCategory SkinnedMeshComponent::GetSceneDrawableCategory() const
    {
        return SceneDrawableCategory::Scene3D;
    }

    bool SkinnedMeshComponent::Get3DDrawableBounds(AABB& bounds)
    {
        if (IsGPUSkinningActive())
        {
            EvaluateWorldPalette(mWorldPaletteCache);
            return GetPaletteBounds(mWorldPaletteCache, bounds);
        }

        return Mesh3DPrimitives::GetMeshBounds(GetMesh(), bounds);
    }

    bool SkinnedMeshComponent::Get3DDrawableLocalBounds(AABB& bounds)
    {
        if (IsGPUSkinningActive())
        {
            EvaluateModelPalette(mPaletteCache);
            mPaletteCache.Add(Mat4::Identity());
            return GetPaletteBounds(mPaletteCache, bounds);
        }

        GetMesh();
        return mSkinnedData.GetBounds(bounds);
    }

    void SkinnedMeshComponent::OnUpdate(float dt)
    {
        if (mUseBoneActors)
        {
            // Bone actors are animated externally: the CPU pose is refreshed every frame
            mNeedRebuildPose = true;
            return;
        }

        if (!mPlaying || !mModel)
            return;

        float duration = GetAnimationDuration();
        if (duration <= 0.0f)
            return;

        mTime += dt*mSpeed;

        if (mLooped)
            mTime -= Math::Floor(mTime/duration)*duration;
        else
            mTime = Math::Clamp(mTime, 0.0f, duration);

        mNeedRebuildPose = true;
    }

    void SkinnedMeshComponent::OnDraw()
    {
        if (IsGPUSkinningActive())
        {
            DrawGPU();
            return;
        }

        // Bone actors can be moved outside of scene updates (editor drag): the pose follows them
        if (mNeedRebuildPose || IsBonePoseChanged())
            RebuildPose();
        else if (ScenePassFilters::IsRawAlbedoMode() != mMeshRawAlbedo)
            ApplyTransform();

        mMesh.SetMaterial(GetMaterial());
        mMesh.Draw();
    }

    void SkinnedMeshComponent::OnTransformUpdated()
    {
        if (IsGPUSkinningActive())
            return;

        if (mNeedRebuildPose)
            RebuildPose();
        else
            ApplyTransform();
    }

    void SkinnedMeshComponent::RebuildPose()
    {
        mNeedRebuildPose = false;

        if (!mModel)
        {
            mSkinnedData = Mesh3DData();
            ApplyTransform();
            return;
        }

        const SkinnedModelData& modelData = mModel->GetModelData();

        EvaluateModelPalette(mPaletteCache);

        if (modelData.influences.IsEmpty() || modelData.joints.IsEmpty())
        {
            // Static model without a skin
            mSkinnedData.positions = modelData.positions;
            if (!modelData.normals.IsEmpty())
                mSkinnedData.normals = modelData.normals;
            else
                SkinnedModelData::ComputeSmoothNormals(modelData.positions, modelData.indices, mSkinnedData.normals);
        }
        else
            modelData.SkinVertices(mPaletteCache, mSkinnedData.positions, mSkinnedData.normals);

        mSkinnedData.uvs = modelData.uvs;
        mSkinnedData.indices = modelData.indices;

        if (mSkinnedData.uvs.Count() != mSkinnedData.positions.Count())
        {
            mSkinnedData.uvs.Resize(mSkinnedData.positions.Count());
            for (int i = 0; i < mSkinnedData.uvs.Count(); i++)
                mSkinnedData.uvs[i] = Vec2F();
        }

        ApplyTransform();
    }

    void SkinnedMeshComponent::ApplyTransform()
    {
        Mat4 worldTransform;
        if (auto owner = mOwner.Lock())
            worldTransform = owner->transform->GetWorldTransform3D();

        mMeshRawAlbedo = ScenePassFilters::IsRawAlbedoMode();

        TextureSource textureSource = mTexture ? mTexture->GetTextureSource() : TextureSource();
        Mesh3DPrimitives::FillMesh(mMesh, mSkinnedData, worldTransform, mColor, textureSource,
                                   mShaded && !mMeshRawAlbedo);
    }

    bool SkinnedMeshComponent::IsBonePoseChanged()
    {
        if (!mUseBoneActors || !mModel)
            return false;

        EvaluateModelPalette(mPoseCheckPalette);
        return mPoseCheckPalette != mPaletteCache;
    }

    bool SkinnedMeshComponent::IsGPUSkinningActive()
    {
        return mGPUSkinning && EnsureGPUMaterials();
    }

    bool SkinnedMeshComponent::EnsureGPUMaterials()
    {
#ifndef PLATFORM_MAC
        // Skinned shaders are Metal only yet, GL platforms use the CPU path
        return false;
#endif

        if (mGPUMaterialsFailed)
            return false;

        if (mSkinnedForwardMaterial)
            return true;

        if (!Render::IsSingletonInitialzed())
            return false;

        auto createMaterial = [](const String& shadersName, const Vector<TextureFormat>& formats)
        {
            Ref<Material> material = Material::CreateFromBuiltinShaders(shadersName);
            if (!material)
                return material;

            if (!formats.IsEmpty())
                material->SetColorAttachmentFormats(formats);

            material->SetVertexLayoutSkinned(true);
            material->AddParam(mmake<ShaderParamFloatVector>("u_bones", Vector<float>()));
            material->AddParam(mmake<ShaderParamFloat>("u_shaded", 0.0f));

            if (!material->Build())
                material = nullptr;

            return material;
        };

        mSkinnedForwardMaterial = createMaterial("SkinnedDefault", {});
        mSkinnedGBufferMaterial = createMaterial("SkinnedGBuffer",
                                                 { TextureFormat::R8G8B8A8, TextureFormat::R16G16B16A16F,
                                                   TextureFormat::R16G16B16A16F });
        mSkinnedShadowMaterial = createMaterial("SkinnedShadowDepth", { TextureFormat::R16G16B16A16F });

        if (!mSkinnedForwardMaterial || !mSkinnedGBufferMaterial || !mSkinnedShadowMaterial)
        {
            mSkinnedForwardMaterial = nullptr;
            mSkinnedGBufferMaterial = nullptr;
            mSkinnedShadowMaterial = nullptr;
            mGPUMaterialsFailed = true;

            o2Debug.LogWarning("Skinned materials build failed, falling back to CPU skinning");
            return false;
        }

        return true;
    }

    int SkinnedMeshComponent::GetUsableJointsCount() const
    {
        if (!mModel)
            return 0;

        int joints = mModel->GetModelData().joints.Count();
        if (joints > maxBones - 1)
        {
            if (!mBonesLimitWarned)
            {
                o2Debug.LogError("Skinned model has " + (String)joints + " bones, over the limit " +
                                 (String)(maxBones - 1) + "; extra bones are replaced with the identity bone");
                const_cast<SkinnedMeshComponent*>(this)->mBonesLimitWarned = true;
            }

            return maxBones - 1;
        }

        return joints;
    }

    void SkinnedMeshComponent::FillGPUMesh()
    {
        mGPUMeshDirty = false;

        if (!mModel)
        {
            mGPUMesh.vertexCount = 0;
            mGPUMesh.polyCount = 0;
            return;
        }

        const SkinnedModelData& modelData = mModel->GetModelData();
        UInt vertexCount = modelData.positions.Count();
        UInt polyCount = modelData.indices.Count()/3;

        if (vertexCount == 0 || polyCount == 0)
        {
            mGPUMesh.vertexCount = 0;
            mGPUMesh.polyCount = 0;
            return;
        }

        mGPUMesh.Resize<SkinnedVertex>(vertexCount, polyCount);

        TextureSource textureSource = mTexture ? mTexture->GetTextureSource() : TextureSource();

        RectF uvRect(0.0f, 1.0f, 1.0f, 0.0f);
        auto texture = textureSource.texture;
        if (texture)
        {
            Vec2F invTexSize(1.0f/texture->GetSize().x, 1.0f/texture->GetSize().y);
            RectF rect = textureSource.sourceRect;
            uvRect = RectF(rect.left*invTexSize.x, 1.0f - rect.top*invTexSize.y,
                           rect.right*invTexSize.x, 1.0f - rect.bottom*invTexSize.y);
        }

        const Vector<Vec3F>* normals = &modelData.normals;
        Vector<Vec3F> computedNormals;
        if (modelData.normals.Count() != modelData.positions.Count())
        {
            SkinnedModelData::ComputeSmoothNormals(modelData.positions, modelData.indices, computedNormals);
            normals = &computedNormals;
        }

        int usableJoints = GetUsableJointsCount();
        int identityBone = usableJoints; // The last palette slot is the identity (world) transform
        Color32Bit color = mColor.ABGR();

        SkinnedVertex* vertices = mGPUMesh.GetVertices<SkinnedVertex>();
        for (UInt i = 0; i < vertexCount; i++)
        {
            SkinnedVertex& vertex = vertices[i];

            const Vec3F& position = modelData.positions[i];
            vertex.x = position.x; vertex.y = position.y; vertex.z = position.z;

            const Vec3F& normal = (*normals)[i];
            vertex.nx = normal.x; vertex.ny = normal.y; vertex.nz = normal.z;

            vertex.color = color;

            Vec2F uv = i < (UInt)modelData.uvs.Count() ? modelData.uvs[i] : Vec2F();
            vertex.tu = uvRect.left + uv.x*uvRect.Width();
            vertex.tv = uvRect.bottom + uv.y*uvRect.Height();

            for (int j = 0; j < SkinnedModelData::influencesPerVertex; j++)
            {
                vertex.boneIndices[j] = (float)identityBone;
                vertex.boneWeights[j] = 0.0f;
            }

            float weightsSum = 0.0f;
            if (i < (UInt)modelData.influences.Count())
            {
                const SkinnedModelData::VertexInfluence& influence = modelData.influences[i];
                for (int j = 0; j < SkinnedModelData::influencesPerVertex; j++)
                {
                    int joint = influence.joints[j];
                    float weight = influence.weights[j];
                    if (weight <= 0.0f)
                        continue;

                    // Influences of out-of-limit joints fall to the identity bone
                    vertex.boneIndices[j] = joint >= 0 && joint < usableJoints ? (float)joint : (float)identityBone;
                    vertex.boneWeights[j] = weight;
                    weightsSum += weight;
                }
            }

            if (weightsSum <= FLT_EPSILON)
                vertex.boneWeights[0] = 1.0f;
            else
            {
                for (int j = 0; j < SkinnedModelData::influencesPerVertex; j++)
                    vertex.boneWeights[j] /= weightsSum;
            }
        }

        VertexIndex* indexes = mGPUMesh.GetIndexes();
        for (UInt i = 0; i < polyCount*3; i++)
            indexes[i] = modelData.indices[i];

        mGPUMesh.SetTexture(texture);
        mGPUMesh.vertexCount = vertexCount;
        mGPUMesh.polyCount = polyCount;
    }

    bool SkinnedMeshComponent::ResolveBoneActors()
    {
        if (!mModel)
            return false;

        auto owner = mOwner.Lock();
        if (!owner)
            return false;

        const SkinnedModelData& modelData = mModel->GetModelData();

        bool resolved = mBoneActors.Count() == modelData.joints.Count() && !modelData.joints.IsEmpty();
        if (resolved)
        {
            for (auto& boneActor : mBoneActors)
            {
                if (!boneActor.Lock())
                {
                    resolved = false;
                    break;
                }
            }
        }

        if (resolved)
            return true;

        mBoneActors.Clear();
        for (int joint : modelData.joints)
        {
            auto boneActor = SkinnedModelAnimation::FindBoneActor(owner, modelData, joint);
            if (!boneActor)
            {
                mBoneActors.Clear();
                return false;
            }

            mBoneActors.Add(boneActor);
        }

        return !mBoneActors.IsEmpty();
    }

    void SkinnedMeshComponent::EvaluateWorldPalette(Vector<Mat4>& outPalette)
    {
        outPalette.Clear();

        Mat4 world;
        if (auto owner = mOwner.Lock())
            world = owner->transform->GetWorldTransform3D();

        if (!mModel)
        {
            outPalette.Add(world);
            return;
        }

        const SkinnedModelData& modelData = mModel->GetModelData();
        int usableJoints = GetUsableJointsCount();

        if (mUseBoneActors && ResolveBoneActors())
        {
            for (int i = 0; i < usableJoints; i++)
            {
                Mat4 inverseBind = i < modelData.inverseBindMatrices.Count()
                    ? modelData.inverseBindMatrices[i]
                    : Mat4::Identity();

                auto boneActor = mBoneActors[i].Lock();
                outPalette.Add(boneActor->transform->GetWorldTransform3D()*inverseBind);
            }
        }
        else
        {
            int animation = modelData.FindAnimation(mAnimationName);
            modelData.EvaluateJointsPalette(animation, mTime, mPaletteCache);

            for (int i = 0; i < usableJoints && i < mPaletteCache.Count(); i++)
                outPalette.Add(world*mPaletteCache[i]);
        }

        outPalette.Add(world); // Identity bone slot
    }

    void SkinnedMeshComponent::DrawGPU()
    {
        if (mGPUMeshDirty)
            FillGPUMesh();

        if (mGPUMesh.polyCount == 0)
            return;

        EvaluateWorldPalette(mWorldPaletteCache);

        // Palette rows packing: three transposed rows (float4) per bone, see O2SkinnedParams
        mPaletteFloats.Resize(mWorldPaletteCache.Count()*12);
        for (int i = 0; i < mWorldPaletteCache.Count(); i++)
        {
            const float* m = mWorldPaletteCache[i].m;
            float* rows = mPaletteFloats.Data() + i*12;
            rows[0] = m[0]; rows[1] = m[4]; rows[2] = m[8];  rows[3] = m[12];
            rows[4] = m[1]; rows[5] = m[5]; rows[6] = m[9];  rows[7] = m[13];
            rows[8] = m[2]; rows[9] = m[6]; rows[10] = m[10]; rows[11] = m[14];
        }

        Ref<Material> material = GetMaterial();
        bool customMaterial = material && material->IsVertexLayoutSkinned() &&
            o2Render.IsMaterialCompatibleWithCurrentTargets(material);

        if (!customMaterial)
        {
            if (o2Render.IsMaterialCompatibleWithCurrentTargets(mSkinnedGBufferMaterial))
                material = mSkinnedGBufferMaterial;
            else if (o2Render.IsMaterialCompatibleWithCurrentTargets(mSkinnedShadowMaterial))
                material = mSkinnedShadowMaterial;
            else
                material = mSkinnedForwardMaterial;
        }

        if (auto bonesParam = DynamicCast<ShaderParamFloatVector>(material->GetShaderParam("u_bones")))
            bonesParam->SetValue(mPaletteFloats);

        if (auto shadedParam = DynamicCast<ShaderParamFloat>(material->GetShaderParam("u_shaded")))
            shadedParam->SetValue(mShaded ? 1.0f : 0.0f);

        material->InvalidateHash();

        mGPUMesh.SetMaterial(material);
        mGPUMesh.Draw();
    }

    bool SkinnedMeshComponent::GetPaletteBounds(const Vector<Mat4>& palette, AABB& bounds) const
    {
        if (!mModel)
            return false;

        AABB bindBounds;
        if (!mModel->GetModelData().GetBounds(bindBounds))
            return false;

        Vec3F corners[8] =
        {
            Vec3F(bindBounds.min.x, bindBounds.min.y, bindBounds.min.z),
            Vec3F(bindBounds.max.x, bindBounds.min.y, bindBounds.min.z),
            Vec3F(bindBounds.min.x, bindBounds.max.y, bindBounds.min.z),
            Vec3F(bindBounds.max.x, bindBounds.max.y, bindBounds.min.z),
            Vec3F(bindBounds.min.x, bindBounds.min.y, bindBounds.max.z),
            Vec3F(bindBounds.max.x, bindBounds.min.y, bindBounds.max.z),
            Vec3F(bindBounds.min.x, bindBounds.max.y, bindBounds.max.z),
            Vec3F(bindBounds.max.x, bindBounds.max.y, bindBounds.max.z)
        };

        Vec3F boundsMin(FLT_MAX, FLT_MAX, FLT_MAX);
        Vec3F boundsMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (auto& matrix : palette)
        {
            for (auto& corner : corners)
            {
                Vec3F point = matrix.TransformPoint(corner);
                boundsMin.x = Math::Min(boundsMin.x, point.x); boundsMax.x = Math::Max(boundsMax.x, point.x);
                boundsMin.y = Math::Min(boundsMin.y, point.y); boundsMax.y = Math::Max(boundsMax.y, point.y);
                boundsMin.z = Math::Min(boundsMin.z, point.z); boundsMax.z = Math::Max(boundsMax.z, point.z);
            }
        }

        if (palette.IsEmpty())
            return false;

        bounds = AABB(boundsMin, boundsMax);
        return true;
    }

    void SkinnedMeshComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        mNeedRebuildPose = true;
        mGPUMeshDirty = true;
        mBoneActors.Clear();
    }

    void SkinnedMeshComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        mNeedRebuildPose = true;
        mGPUMeshDirty = true;
        mBoneActors.Clear();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SkinnedMeshComponent>);
// --- META ---

DECLARE_CLASS(o2::SkinnedMeshComponent, o2__SkinnedMeshComponent);
// --- END META ---
