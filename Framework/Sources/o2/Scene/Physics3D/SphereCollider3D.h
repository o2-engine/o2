#pragma once
#include "o2/Scene/Physics3D/ICollider3D.h"

namespace o2
{
    // --------------------------
    // Sphere 3D physics collider
    // --------------------------
    class SphereCollider3D: public ICollider3D
    {
    public:
        PROPERTIES(SphereCollider3D);
        PROPERTY(float, radius, SetRadius, GetRadius); // Collider radius property

    public:
        // Default constructor
        SphereCollider3D();

        // Copy-constructor
        SphereCollider3D(const SphereCollider3D& other);

        // Copy operator
        SphereCollider3D& operator=(const SphereCollider3D& other);

        // Sets collider radius
        void SetRadius(float radius);

        // Returns collider radius
        float GetRadius() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(SphereCollider3D);
        CLONEABLE_REF(SphereCollider3D);

    private:
        float mRadius = 0.5f; // Radius of the sphere collider @SERIALIZABLE

    private:
        // Returns sphere shape placed by the relative transform
        b3ShapeId CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale) override;

#if IS_EDITOR
        // Draws collider sphere wireframe
        void OnDrawGizmos() override;
#endif
    };
}
// --- META ---

CLASS_BASES_META(o2::SphereCollider3D)
{
    BASE_CLASS(o2::ICollider3D);
}
END_META;
CLASS_FIELDS_META(o2::SphereCollider3D)
{
    FIELD().PUBLIC().NAME(radius);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.5f).NAME(mRadius);
}
END_META;
CLASS_METHODS_META(o2::SphereCollider3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SphereCollider3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRadius, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRadius);
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
