#pragma once

#include "o2/Scene/Physics/ICollider.h"
#include "o2/Utils/Math/Spline.h"

class b2ChainShape;

namespace o2
{
    // ----------------------------------------
    // Spline-based curve collider
    // Builds a Box2D b2ChainShape from a Bezier
    // spline edited in the scene SplineTool.
    // ----------------------------------------
    class SplineCollider: public ICollider
    {
    public:
        PROPERTIES(SplineCollider);
        PROPERTY(bool, isLoop, SetIsLoop, IsLoop); // Closed loop chain property

        Ref<Spline> spline = mmake<Spline>(); // Collider spline @SERIALIZABLE

    public:
        // Default constructor
        SplineCollider();

        // Copy-constructor
        SplineCollider(const SplineCollider& other);

        // Destructor
        ~SplineCollider();

        // Copy operator
        SplineCollider& operator=(const SplineCollider& other);

        // Returns spline
        Ref<Spline> GetSpline() const;

        // Sets loop (closed chain) mode
        void SetIsLoop(bool loop);

        // Returns true if collider is a closed loop
        bool IsLoop() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

#if IS_EDITOR
        // Called when component added from editor
        void OnAddedFromEditor() override;
#endif

        SERIALIZABLE(SplineCollider);
        CLONEABLE_REF(SplineCollider);

    protected:
        bool mIsLoop = false; // Loop / closed chain @SERIALIZABLE

        b2ChainShape* mShape = nullptr; // Physics shape (re-allocated on every shape change)

    protected:
        // Initializes spline with two default keys and wires onKeysChanged
        void InitSpline();

        // Called by spline's onKeysChanged to rebuild physics fixture; override in
        // derived classes to also rebuild secondary state (e.g. drawable mesh).
        virtual void OnSplineChanged();

        // Returns shape with relative position and angle
        b2Shape* GetShape(const Basis& transform) override;

        // Called when actor was included to scene
        void OnAddToScene() override;
    };
}
// --- META ---

CLASS_BASES_META(o2::SplineCollider)
{
    BASE_CLASS(o2::ICollider);
}
END_META;
CLASS_FIELDS_META(o2::SplineCollider)
{
    FIELD().PUBLIC().NAME(isLoop);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Spline>()).NAME(spline);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsLoop);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mShape);
}
END_META;
CLASS_METHODS_META(o2::SplineCollider)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SplineCollider&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Spline>, GetSpline);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIsLoop, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLoop);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, OnAddedFromEditor);
#endif
    FUNCTION().PROTECTED().SIGNATURE(void, InitSpline);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSplineChanged);
    FUNCTION().PROTECTED().SIGNATURE(b2Shape*, GetShape, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
}
END_META;
// --- END META ---
