#pragma once
#include "o2/Scene/Physics/ICollider.h"
#include "Box2D/Collision/Shapes/b2PolygonShape.h"

namespace o2
{
    // --------------------
    // Box physics collider
    // --------------------
    class BoxCollider: public ICollider
    {
    public:
        PROPERTIES(BoxCollider);
        PROPERTY(Vec2F, size, SetSize, GetSize);                 // Collider size property
        PROPERTY(bool, fitByActor, SetFitByActor, IsFitByActor); // Fit by actor's size property

    public:
        // Default constructor
        BoxCollider();

        // Copy-constructor
        BoxCollider(const BoxCollider& other);

        // Copy operator
        BoxCollider& operator=(const BoxCollider& other);

        // Sets collider size
        void SetSize(const Vec2F& size);

        // Returns collider size
        Vec2F GetSize() const;

        // Sets collider fit to actor's size
        void SetFitByActor(bool fit);

        // Is collider fits to actor's size
        bool IsFitByActor() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

#if IS_EDITOR
        // Called when component added from editor; fits collider by actor size
        void OnAddedFromEditor() override;
#endif

        SERIALIZABLE(BoxCollider);
        CLONEABLE_REF(BoxCollider);

    private:
        Vec2F mSize = Vec2F(1, 1); // Size of collider @SERIALIZABLE

        bool mFitByActor = true; // Is box size is fitting by actor's size @SERIALIZABLE

        b2PolygonShape mShape; // Physics shape

    private:
        // Returns shape with relative position and angle
        b2Shape* GetShape(const Basis& transform) override;

        // Called when transformation was changed; fits size by actor size when required
        void OnTransformUpdated() override;

        void FitSize();

        // Called when actor was included to scene; fits size if required
        void OnAddToScene() override;

#if IS_EDITOR
        // Draws collider box wireframe
        void OnDrawGizmos() override;
#endif
    };
}
// --- META ---

CLASS_BASES_META(o2::BoxCollider)
{
    BASE_CLASS(o2::ICollider);
}
END_META;
CLASS_FIELDS_META(o2::BoxCollider)
{
    FIELD().PUBLIC().NAME(size);
    FIELD().PUBLIC().NAME(fitByActor);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2F(1, 1)).NAME(mSize);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mFitByActor);
    FIELD().PRIVATE().NAME(mShape);
}
END_META;
CLASS_METHODS_META(o2::BoxCollider)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const BoxCollider&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSize, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFitByActor, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFitByActor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, OnAddedFromEditor);
#endif
    FUNCTION().PRIVATE().SIGNATURE(b2Shape*, GetShape, const Basis&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PRIVATE().SIGNATURE(void, FitSize);
    FUNCTION().PRIVATE().SIGNATURE(void, OnAddToScene);
#if  IS_EDITOR
    FUNCTION().PRIVATE().SIGNATURE(void, OnDrawGizmos);
#endif
}
END_META;
// --- END META ---
