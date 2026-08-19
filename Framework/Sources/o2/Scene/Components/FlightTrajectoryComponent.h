#pragma once

#include "o2/Scene/Component.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Spline.h"

namespace o2
{
    // ------------------------------------------------------------------------------------------
    // Flies the actor along a spline trajectory. The spline (key ranges give a corridor for the
    // random offset) is geometrically mapped between the start and finish points: a transform
    // from the actual spline ends to the target points is computed and cached as Basis (rotation,
    // scale, translation). Flight progress is driven externally by the position parameter
    // (0..1, animatable): each update the trajectory point is written into the actor transform
    // (widgets get layout offsets). The random offset within the corridor is rerolled by
    // ResetRandomOffset() or when position returns to 0
    // ------------------------------------------------------------------------------------------
    class FlightTrajectoryComponent: public Component
    {
    public:
        float position = 0.0f;  // Flight progress: 0 - start, 1 - finish @SERIALIZABLE @EDITOR_PROPERTY @ANIMATABLE @RANGE(0, 1)
        Ref<Spline> spline;     // Trajectory; key ranges give the random offset corridor @SERIALIZABLE @EDITOR_PROPERTY
        Vec2F startPoint;       // Flight start point @SERIALIZABLE @EDITOR_PROPERTY
        Vec2F finishPoint;      // Flight finish point @SERIALIZABLE @EDITOR_PROPERTY

    public:
        // Default constructor
        FlightTrajectoryComponent();

        // Copy-constructor
        FlightTrajectoryComponent(const FlightTrajectoryComponent& other);

        // Sets flight start and finish points @SCRIPTABLE
        void SetPoints(float startX, float startY, float finishX, float finishY);

        // Picks a new random offset within the spline width corridor @SCRIPTABLE
        void ResetRandomOffset();

        // Sets flight progress; 0 rerolls the random offset @SCRIPTABLE
        void SetPosition(float value);

        // Returns flight progress @SCRIPTABLE
        float GetPosition() const;

        // Returns trajectory point for progress t (0..1) with offset and transform applied
        Vec2F EvaluatePoint(float t);

        // Returns name of component in create menu
        static String GetName();

        // Returns category of component in create menu
        static String GetCategory();

        // Returns component icon in create menu
        static String GetIcon();

        SERIALIZABLE(FlightTrajectoryComponent);
        CLONEABLE_REF(FlightTrajectoryComponent);

    private:
        float mRandomCoef = 0.5f;    // Picked offset within the width corridor (0..1)
        float mPrevPosition = 0.0f;  // For rerolling the offset when position returns to 0

        Basis mBasis;                // Transform from spline space to start/finish points
        Vec2F mBasisStart;           // Points the cached transform was computed for
        Vec2F mBasisFinish;
        bool  mBasisDirty = true;

        void OnStart() override;
        void OnUpdate(float dt) override;

        // Recomputes the transform when points or spline changed
        void UpdateBasis();

        // Applies the trajectory position to the actor
        void ApplyToActor();

        REF_COUNTERABLE_IMPL(Component);
    };
}
// --- META ---

CLASS_BASES_META(o2::FlightTrajectoryComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::FlightTrajectoryComponent)
{
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().EDITOR_PROPERTY_ATTRIBUTE().RANGE_ATTRIBUTE(0, 1).SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(position);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(spline);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(startPoint);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(finishPoint);
    FIELD().PRIVATE().DEFAULT_VALUE(0.5f).NAME(mRandomCoef);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mPrevPosition);
    FIELD().PRIVATE().NAME(mBasis);
    FIELD().PRIVATE().NAME(mBasisStart);
    FIELD().PRIVATE().NAME(mBasisFinish);
    FIELD().PRIVATE().DEFAULT_VALUE(true).NAME(mBasisDirty);
}
END_META;
CLASS_METHODS_META(o2::FlightTrajectoryComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const FlightTrajectoryComponent&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPoints, float, float, float, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, ResetRandomOffset);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPosition, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetPosition);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, EvaluatePoint, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PRIVATE().SIGNATURE(void, OnStart);
    FUNCTION().PRIVATE().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateBasis);
    FUNCTION().PRIVATE().SIGNATURE(void, ApplyToActor);
}
END_META;
// --- END META ---
