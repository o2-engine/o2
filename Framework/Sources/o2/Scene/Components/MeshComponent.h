#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Meshes/Mesh.h"
#include "o2/Render/Resources/TextureRef.h"
#include "o2/Scene/Component.h"
#include "o2/Utils/Math/Spline.h"
#include "o2/Utils/Types/UID.h"

namespace o2
{
    // --------------
    // Mesh component
    // --------------
    class MeshComponent: public Component
    {
    public:
        PROPERTIES(MeshComponent);
        PROPERTY(AssetRef<ImageAsset>, image, SetImage, GetImage);                   // Image property
        PROPERTY(RectF, mappingFrame, SetMappingFrame, GetMappingFrame);      // Image mapping frame
        PROPERTY(Color4, color, SetColor, GetColor);                          // Color property
        PROPERTY(Vector<Vec2F>, extraPoints, SetExtraPoints, GetExtraPoints); // extra points property

        Ref<Spline> spline = mmake<Spline>(); // Shell spline @SERIALIZABLE

    public:
        // Default constructor
        MeshComponent();

        // Copy-constructor
        MeshComponent(const MeshComponent& other);

        // Destructor
        ~MeshComponent();

        // Assign operator
        MeshComponent& operator=(const MeshComponent& other);

        // Returns mesh
        const Mesh& GetMesh() const;

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

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(MeshComponent);
        CLONEABLE_REF(MeshComponent);

    protected:
        Mesh   mMesh;      // Drawing mesh, built from spline
        Basis  mTransform; // Transform where mesh was built

        AssetRef<ImageAsset> mImageAsset;                         // Image asset @SERIALIZABLE
        RectF         mImageMapping = RectF(0, 0, 10, 10); // Image mapping rectangle @SERIALIZABLE

        Vector<Vec2F> mExtraPoints; // Extra topology points @SERIALIZABLE

        Color4 mColor = Color4::White(); // Mesh color @SERIALIZABLE

        bool mNeedUpdateMesh = false; // True, when mesh data is dirty and need to rebuild

    protected:
        // Draws sprite 
        void OnDraw() override;

        // Called when actor's transform was changed
        void OnTransformUpdated() override;

        // Calculates mesh from spline
        void UpdateMesh();

        // Sets owner actor
        void SetOwnerActor(const Ref<Actor>& actor) override;

        // Calling when deserializing
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::MeshComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::MeshComponent)
{
    FIELD().PUBLIC().NAME(image);
    FIELD().PUBLIC().NAME(mappingFrame);
    FIELD().PUBLIC().NAME(color);
    FIELD().PUBLIC().NAME(extraPoints);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Spline>()).NAME(spline);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().NAME(mTransform);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mImageAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(RectF(0, 0, 10, 10)).NAME(mImageMapping);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mExtraPoints);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedUpdateMesh);
}
END_META;
CLASS_METHODS_META(o2::MeshComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const MeshComponent&);
    FUNCTION().PUBLIC().SIGNATURE(const Mesh&, GetMesh);
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
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMesh);
    FUNCTION().PROTECTED().SIGNATURE(void, SetOwnerActor, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
