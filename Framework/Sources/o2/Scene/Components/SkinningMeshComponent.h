#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Meshes/SkinningMesh.h"
#include "o2/Render/Resources/TextureRef.h"
#include "o2/Scene/Component.h"
#include "o2/Utils/Math/Spline.h"
#include "o2/Utils/Types/UID.h"

namespace o2
{
    class SkinningMeshBoneComponent;

    // ------------------------
    // Skinning mesh component
    // ------------------------
    class SkinningMeshComponent: public Component
    {
    public:
        PROPERTIES(SkinningMeshComponent);
        PROPERTY(AssetRef<ImageAsset>, image, SetImage, GetImage);                   // Image property
        PROPERTY(RectF, mappingFrame, SetMappingFrame, GetMappingFrame);      // Image mapping frame
        PROPERTY(Color4, color, SetColor, GetColor);                          // Color property
        PROPERTY(Vector<Vec2F>, extraPoints, SetExtraPoints, GetExtraPoints); // extra points property

        Ref<Spline> spline = mmake<Spline>(); // Shell spline @SERIALIZABLE

    public:
        // Default constructor
        SkinningMeshComponent();

        // Copy-constructor
        SkinningMeshComponent(const SkinningMeshComponent& other);

        // Destructor
        ~SkinningMeshComponent();

        // Assign operator
        SkinningMeshComponent& operator=(const SkinningMeshComponent& other);

        // Updates mesh bones and reskins
        void OnUpdate(float dt) override;

        // Updates bones transformations
        void UpdateBonesTransforms();

        // Returns mesh
        const SkinningMesh& GetMesh() const;

        // Sets extra points
        void SetExtraPoints(const Vector<Vec2F>& points);

        // Returns extra points list
        const Vector<Vec2F>& GetExtraPoints() const;

        // Sets extra point position
        void SetExtraPoint(int idx, const Vec2F& pos);

        // Adds one extra point
        void AddExtraPoint(const Vec2F& point);

        // Removes extra point by index
        void RemoveExtraPoint(int idx);

        // Sets image
        void SetImage(const AssetRef<ImageAsset>& image);

        // Returns image
        const AssetRef<ImageAsset>& GetImage() const;

        // Sets image mapping frame
        void SetMappingFrame(const RectF& frame);

        // Returns image mapping frame
        const RectF& GetMappingFrame() const;

        // Sets color
        void SetColor(const Color4& color);

        // Returns color
        const Color4& GetColor() const;

        // Marks bones outdated and needs to update
        void NeedUpdateBones(bool force = false);

        // Returns list of bones
        const Vector<Pair<WeakRef<SkinningMeshBoneComponent>, SkinningMesh::Bone*>>& GetBones() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(SkinningMeshComponent);
        CLONEABLE_REF(SkinningMeshComponent);

    protected:
        SkinningMesh mMesh;      // Drawing mesh, built from spline
        Basis        mTransform; // Transform where mesh was built

        Vector<Pair<WeakRef<SkinningMeshBoneComponent>, SkinningMesh::Bone*>> mBonesMapping; // Map of bones to components. Updates in UpdateBones, used in updating mesh

        AssetRef<ImageAsset> mImageAsset;                         // Image asset @SERIALIZABLE
        RectF         mImageMapping = RectF(0, 0, 10, 10); // Image mapping rectangle @SERIALIZABLE

        Vector<Vec2F> mExtraPoints; // Extra topology points @SERIALIZABLE

        Color4 mColor = Color4::White(); // Mesh color @SERIALIZABLE

        bool mNeedUpdateMesh = false;  // True, when mesh data is dirty and need to rebuild
        bool mNeedUpdateBones = false; // True, when bones need to be updated

    protected:
        // Draws sprite 
        void OnDraw() override;

        // Called when component starts, updates bones
        void OnStart() override;

        // Called when actor's transform was changed
        void OnTransformUpdated() override;

        // Calculates mesh from spline
        void UpdateMesh();

        // Checks bones hierarchy and rebuilds hierarchy for skinning mesh
        void UpdateBones();

        // Sets owner actor
        void SetOwnerActor(const Ref<Actor>& actor) override;

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        // Draws origin not skinned mesh wire
        void DrawMeshWire() const;
    };
}
// --- META ---

CLASS_BASES_META(o2::SkinningMeshComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::SkinningMeshComponent)
{
    FIELD().PUBLIC().NAME(image);
    FIELD().PUBLIC().NAME(mappingFrame);
    FIELD().PUBLIC().NAME(color);
    FIELD().PUBLIC().NAME(extraPoints);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Spline>()).NAME(spline);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().NAME(mTransform);
    FIELD().PROTECTED().NAME(mBonesMapping);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mImageAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(RectF(0, 0, 10, 10)).NAME(mImageMapping);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mExtraPoints);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedUpdateMesh);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedUpdateBones);
}
END_META;
CLASS_METHODS_META(o2::SkinningMeshComponent)
{

    typedef const Vector<Pair<WeakRef<SkinningMeshBoneComponent>, SkinningMesh::Bone*>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SkinningMeshComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateBonesTransforms);
    FUNCTION().PUBLIC().SIGNATURE(const SkinningMesh&, GetMesh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExtraPoints, const Vector<Vec2F>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Vec2F>&, GetExtraPoints);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExtraPoint, int, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddExtraPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveExtraPoint, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetImage, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<ImageAsset>&, GetImage);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMappingFrame, const RectF&);
    FUNCTION().PUBLIC().SIGNATURE(const RectF&, GetMappingFrame);
    FUNCTION().PUBLIC().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(const Color4&, GetColor);
    FUNCTION().PUBLIC().SIGNATURE(void, NeedUpdateBones, bool);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, GetBones);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateBones);
    FUNCTION().PROTECTED().SIGNATURE(void, SetOwnerActor, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawMeshWire);
}
END_META;
// --- END META ---
