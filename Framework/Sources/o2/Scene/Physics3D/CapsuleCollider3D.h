#pragma once
#include "o2/Scene/Physics3D/ICollider3D.h"

namespace o2
{
    // ---------------------------
    // Capsule 3D physics collider
    // ---------------------------
    // The capsule is aligned with the collider's local Y axis. Height is the distance between the
    // two hemisphere centers (i.e. the cylindrical part length), radius is the hemisphere radius.
    class CapsuleCollider3D: public ICollider3D
    {
    public:
        PROPERTIES(CapsuleCollider3D);
        PROPERTY(float, radius, SetRadius, GetRadius); // Hemisphere radius property
        PROPERTY(float, height, SetHeight, GetHeight); // Distance between hemisphere centers property

    public:
        // Default constructor
        CapsuleCollider3D();

        // Copy-constructor
        CapsuleCollider3D(const CapsuleCollider3D& other);

        // Copy operator
        CapsuleCollider3D& operator=(const CapsuleCollider3D& other);

        // Sets hemisphere radius
        void SetRadius(float radius);

        // Returns hemisphere radius
        float GetRadius() const;

        // Sets distance between hemisphere centers
        void SetHeight(float height);

        // Returns distance between hemisphere centers
        float GetHeight() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(CapsuleCollider3D);
        CLONEABLE_REF(CapsuleCollider3D);

    private:
        float mRadius = 0.5f; // Hemisphere radius @SERIALIZABLE
        float mHeight = 1.0f; // Distance between the two hemisphere centers @SERIALIZABLE

    private:
        // Returns capsule shape placed by the relative transform
        b3ShapeId CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale) override;

#if IS_EDITOR
        // Draws collider capsule wireframe
        void OnDrawGizmos() override;
#endif
    };
}
// --- META ---

CLASS_BASES_META(o2::CapsuleCollider3D)
{
    BASE_CLASS(o2::ICollider3D);
}
END_META;
CLASS_FIELDS_META(o2::CapsuleCollider3D)
{
    FIELD().PUBLIC().NAME(radius);
    FIELD().PUBLIC().NAME(height);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.5f).NAME(mRadius);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mHeight);
}
END_META;
CLASS_METHODS_META(o2::CapsuleCollider3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const CapsuleCollider3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRadius, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRadius);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHeight);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PRIVATE().SIGNATURE(b3ShapeId, CreateShape, b3BodyId, const b3ShapeDef&, const b3Transform&, float);
#if  IS_EDITOR
    FUNCTION().PRIVATE().SIGNATURE(void, OnDrawGizmos);
#endif
}
END_META;
// --- END META ---
