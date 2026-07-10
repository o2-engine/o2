#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Types/MaterialAsset.h"
#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Render/Mesh.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Scene/Component.h"

namespace o2
{
    // ------------------------------------------------------------------------------
    // Skinned mesh component. Draws a skinned model (SkinnedModelAsset) posed either
    // by bone actors (children created by CreateBoneActors, animated by the standard
    // AnimationComponent) or by the built-in clip player. Skinning runs on GPU with
    // the skinned builtin shaders (bind pose vertex buffer + bones palette uniform);
    // the CPU path remains as a fallback for headless mode and reference tests
    // ------------------------------------------------------------------------------
    class SkinnedMeshComponent: public Component
    {
    public:
        static constexpr int maxBones = 64; // GPU palette limit, see O2_MAX_BONES in the Metal shaders preamble

        PROPERTIES(SkinnedMeshComponent);
        PROPERTY(AssetRef<SkinnedModelAsset>, model, SetModelAsset, GetModelAsset);      // Model asset property
        PROPERTY(Color4, color, SetColor, GetColor);                                     // Color property
        PROPERTY(AssetRef<ImageAsset>, texture, SetTexture, GetTexture);                 // Texture property
        PROPERTY(bool, shaded, SetShaded, IsShaded);                                     // Baked lambert shading property
        PROPERTY(AssetRef<MaterialAsset>, material, SetMaterialAsset, GetMaterialAsset); // Material (asset) for rendering
        PROPERTY(String, animation, SetAnimation, GetAnimation);                         // Playing animation clip name
        PROPERTY(bool, playing, SetPlaying, IsPlaying);                                  // Is animation playing
        PROPERTY(bool, looped, SetLooped, IsLooped);                                     // Is animation looped
        PROPERTY(float, speed, SetSpeed, GetSpeed);                                      // Animation playback speed
        PROPERTY(bool, useBoneActors, SetUseBoneActors, IsUsingBoneActors);              // Pose from bone actors property
        PROPERTY(bool, gpuSkinning, SetGPUSkinningEnabled, IsGPUSkinningEnabled);        // GPU skinning property

    public:
        // Default constructor
        SkinnedMeshComponent();

        // Copy-constructor
        SkinnedMeshComponent(const SkinnedMeshComponent& other);

        // Destructor
        ~SkinnedMeshComponent();

        // Assign operator
        SkinnedMeshComponent& operator=(const SkinnedMeshComponent& other);

        // Returns CPU skinned drawing mesh, rebuilds it if the pose is dirty
        const Mesh& GetMesh();

        // Sets model asset
        void SetModelAsset(const AssetRef<SkinnedModelAsset>& model);

        // Returns model asset
        const AssetRef<SkinnedModelAsset>& GetModelAsset() const;

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

        // Sets playing animation clip by name; empty name means the bind pose
        void SetAnimation(const String& name);

        // Returns playing animation clip name
        const String& GetAnimation() const;

        // Starts or stops animation playback
        void SetPlaying(bool playing);

        // Returns is animation playing
        bool IsPlaying() const;

        // Sets animation looping
        void SetLooped(bool looped);

        // Returns is animation looped
        bool IsLooped() const;

        // Sets animation playback speed
        void SetSpeed(float speed);

        // Returns animation playback speed
        float GetSpeed() const;

        // Sets current animation time in seconds
        void SetAnimationTime(float time);

        // Returns current animation time in seconds
        float GetAnimationTime() const;

        // Returns current animation clip duration; 0 when no clip is selected
        float GetAnimationDuration() const;

        // Creates bone actors hierarchy under the owner and switches the pose source to them
        void CreateBoneActors();

        // Sets pose source: true - bone actors (child actors named as skeleton nodes), false - built-in player
        void SetUseBoneActors(bool use);

        // Returns is the pose read from bone actors
        bool IsUsingBoneActors() const;

        // Enables or disables GPU skinning; when disabled or unavailable the CPU path is used
        void SetGPUSkinningEnabled(bool enabled);

        // Returns is GPU skinning enabled
        bool IsGPUSkinningEnabled() const;

        // Evaluates model space skinning palette (from bone actors or the built-in clip player)
        void EvaluateModelPalette(Vector<Mat4>& outPalette);

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        // Returns 3D render category
        SceneDrawableCategory GetSceneDrawableCategory() const override;

        // Returns world bounds of the posed mesh
        bool Get3DDrawableBounds(AABB& bounds) override;

        // Returns local space bounds of the posed geometry
        bool Get3DDrawableLocalBounds(AABB& bounds) override;

        SERIALIZABLE(SkinnedMeshComponent);
        CLONEABLE_REF(SkinnedMeshComponent);

    protected:
        Mesh       mMesh;        // CPU path drawing mesh in world space
        Mesh3DData mSkinnedData; // CPU path posed geometry in model space, refilled from the skinning

        Mesh mGPUMesh; // GPU path mesh: bind pose skinned vertices, filled once and posed by the palette

        AssetRef<SkinnedModelAsset> mModel;   // Model asset @SERIALIZABLE @EDITOR_PROPERTY
        AssetRef<ImageAsset>        mTexture; // Texture asset @SERIALIZABLE @EDITOR_PROPERTY

        AssetRef<MaterialAsset> mMaterialAsset; // Material asset; when set, the mesh is drawn with it @SERIALIZABLE @EDITOR_PROPERTY
        Ref<Material>           mMaterial;      // Direct material override, not serialized

        Color4 mColor = Color4::White(); // Mesh color @SERIALIZABLE @EDITOR_PROPERTY
        bool   mShaded = true;           // Bake lambert shading into vertex colors @SERIALIZABLE

        String mAnimationName;    // Playing clip name; empty - bind pose @SERIALIZABLE @EDITOR_PROPERTY
        bool   mPlaying = true;   // Is animation playing @SERIALIZABLE @EDITOR_PROPERTY
        bool   mLooped = true;    // Is animation looped @SERIALIZABLE @EDITOR_PROPERTY
        float  mSpeed = 1.0f;     // Playback speed @SERIALIZABLE @EDITOR_PROPERTY
        float  mTime = 0.0f;      // Current animation time in seconds

        bool mUseBoneActors = false; // Pose from bone actors instead of the built-in player @SERIALIZABLE @EDITOR_PROPERTY
        bool mGPUSkinning = true;    // GPU skinning enabled @SERIALIZABLE

        Vector<Mat4>  mPaletteCache;      // Skinning palette, reused between rebuilds
        Vector<Mat4>  mWorldPaletteCache; // World space palette for the GPU path, reused between frames
        Vector<Mat4>  mPoseCheckPalette;  // Scratch palette for detecting bone actors pose changes
        Vector<float> mPaletteFloats;     // Packed palette rows for the bones shader parameter

        Vector<WeakRef<Actor>> mBoneActors; // Resolved bone actors per skin joint, cached

        Ref<Material> mSkinnedForwardMaterial; // GPU skinning material for the forward pass
        Ref<Material> mSkinnedGBufferMaterial; // GPU skinning material for the G-buffer pass
        Ref<Material> mSkinnedShadowMaterial;  // GPU skinning material for the shadow depth pass

        bool mGPUMaterialsFailed = false; // True when skinned materials failed to build, GPU path disabled

        bool mNeedRebuildPose = true;    // True, when the pose is dirty and CPU skinning must be recomputed
        bool mGPUMeshDirty = true;       // True, when the GPU bind pose vertex buffer must be refilled
        bool mMeshRawAlbedo = false;     // True, when mesh was filled without baked shading (G-buffer mode)
        bool mBonesLimitWarned = false;  // One-time warning flag for models above the bones limit

    protected:
        // Called each frame: advances animation time
        void OnUpdate(float dt) override;

        // Draws mesh
        void OnDraw() override;

        // Called when actor's transform was changed
        void OnTransformUpdated() override;

        // Recomputes the CPU skinning into model space geometry and refills the world space mesh
        void RebuildPose();

        // Fills drawing mesh from posed geometry with owner's world 3D transform
        void ApplyTransform();

        // Returns true when the bone actors pose differs from the last applied palette
        bool IsBonePoseChanged();

        // Returns true when GPU skinning is enabled and skinned materials are ready (builds them lazily)
        bool IsGPUSkinningActive();

        // Builds skinned materials once; returns false when unavailable (headless or failed)
        bool EnsureGPUMaterials();

        // Fills the GPU mesh with bind pose skinned vertices
        void FillGPUMesh();

        // Draws the GPU skinned mesh with the pass-compatible skinned material
        void DrawGPU();

        // Returns skin joints count clamped to the palette limit (last slot is the identity bone)
        int GetUsableJointsCount() const;

        // Resolves bone actors for skin joints; returns true when all are found
        bool ResolveBoneActors();

        // Evaluates world space palette with the identity bone in the last slot
        void EvaluateWorldPalette(Vector<Mat4>& outPalette);

        // Computes bounds as the union of the palette transformed bind pose bounds
        bool GetPaletteBounds(const Vector<Mat4>& palette, AABB& bounds) const;

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::SkinnedMeshComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::SkinnedMeshComponent)
{
    FIELD().PUBLIC().NAME(model);
    FIELD().PUBLIC().NAME(color);
    FIELD().PUBLIC().NAME(texture);
    FIELD().PUBLIC().NAME(shaded);
    FIELD().PUBLIC().NAME(material);
    FIELD().PUBLIC().NAME(animation);
    FIELD().PUBLIC().NAME(playing);
    FIELD().PUBLIC().NAME(looped);
    FIELD().PUBLIC().NAME(speed);
    FIELD().PUBLIC().NAME(useBoneActors);
    FIELD().PUBLIC().NAME(gpuSkinning);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().NAME(mSkinnedData);
    FIELD().PROTECTED().NAME(mGPUMesh);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mModel);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mTexture);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mMaterialAsset);
    FIELD().PROTECTED().NAME(mMaterial);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mShaded);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mAnimationName);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mPlaying);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mLooped);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mSpeed);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mTime);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mUseBoneActors);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mGPUSkinning);
    FIELD().PROTECTED().NAME(mPaletteCache);
    FIELD().PROTECTED().NAME(mWorldPaletteCache);
    FIELD().PROTECTED().NAME(mPoseCheckPalette);
    FIELD().PROTECTED().NAME(mPaletteFloats);
    FIELD().PROTECTED().NAME(mBoneActors);
    FIELD().PROTECTED().NAME(mSkinnedForwardMaterial);
    FIELD().PROTECTED().NAME(mSkinnedGBufferMaterial);
    FIELD().PROTECTED().NAME(mSkinnedShadowMaterial);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mGPUMaterialsFailed);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mNeedRebuildPose);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mGPUMeshDirty);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mMeshRawAlbedo);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBonesLimitWarned);
}
END_META;
CLASS_METHODS_META(o2::SkinnedMeshComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SkinnedMeshComponent&);
    FUNCTION().PUBLIC().SIGNATURE(const Mesh&, GetMesh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetModelAsset, const AssetRef<SkinnedModelAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<SkinnedModelAsset>&, GetModelAsset);
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
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetAnimation);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlaying, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLooped, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLooped);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpeed, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSpeed);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimationTime, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAnimationTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAnimationDuration);
    FUNCTION().PUBLIC().SIGNATURE(void, CreateBoneActors);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUseBoneActors, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUsingBoneActors);
    FUNCTION().PUBLIC().SIGNATURE(void, SetGPUSkinningEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsGPUSkinningEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, EvaluateModelPalette, Vector<Mat4>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE(SceneDrawableCategory, GetSceneDrawableCategory);
    FUNCTION().PUBLIC().SIGNATURE(bool, Get3DDrawableBounds, AABB&);
    FUNCTION().PUBLIC().SIGNATURE(bool, Get3DDrawableLocalBounds, AABB&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildPose);
    FUNCTION().PROTECTED().SIGNATURE(void, ApplyTransform);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsBonePoseChanged);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsGPUSkinningActive);
    FUNCTION().PROTECTED().SIGNATURE(bool, EnsureGPUMaterials);
    FUNCTION().PROTECTED().SIGNATURE(void, FillGPUMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawGPU);
    FUNCTION().PROTECTED().SIGNATURE(int, GetUsableJointsCount);
    FUNCTION().PROTECTED().SIGNATURE(bool, ResolveBoneActors);
    FUNCTION().PROTECTED().SIGNATURE(void, EvaluateWorldPalette, Vector<Mat4>&);
    FUNCTION().PROTECTED().SIGNATURE(bool, GetPaletteBounds, const Vector<Mat4>&, AABB&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
