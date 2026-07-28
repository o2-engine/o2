#pragma once
#include "o2/Scene/Physics3D/ICollider3D.h"

namespace o2
{
    // -----------------------
    // Box 3D physics collider
    // -----------------------
    class BoxCollider3D: public ICollider3D
    {
    public:
        PROPERTIES(BoxCollider3D);
        PROPERTY(Vec3F, size, SetSize, GetSize); // Collider size property

    public:
        // Default constructor
        BoxCollider3D();

        // Copy-constructor
        BoxCollider3D(const BoxCollider3D& other);

        // Copy operator
        BoxCollider3D& operator=(const BoxCollider3D& other);

        // Sets collider size
        void SetSize(const Vec3F& size);

        // Returns collider size
        Vec3F GetSize() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(BoxCollider3D);
        CLONEABLE_REF(BoxCollider3D);

    private:
        Vec3F mSize = Vec3F(1, 1, 1); // Size of the box collider @SERIALIZABLE

    private:
        // Returns box hull shape placed by the relative transform
        b3ShapeId CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale) override;

#if IS_EDITOR
        // Draws collider box wireframe
        void OnDrawGizmos() override;
#endif
    };
}
// --- META ---

CLASS_BASES_META(o2::BoxCollider3D)
{
    BASE_CLASS(o2::ICollider3D);
}
END_META;
CLASS_FIELDS_META(o2::BoxCollider3D)
{
    FIELD().PUBLIC().NAME(size);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec3F(1, 1, 1)).NAME(mSize);
}
END_META;
CLASS_METHODS_META(o2::BoxCollider3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const BoxCollider3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSize, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetSize);
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
