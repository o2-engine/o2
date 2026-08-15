#include "o2/stdafx.h"
#include "FlightTrajectoryComponent.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Math.h"

namespace o2
{
    FlightTrajectoryComponent::FlightTrajectoryComponent()
    {}

    FlightTrajectoryComponent::FlightTrajectoryComponent(const FlightTrajectoryComponent& other):
        Component(other), position(other.position), startPoint(other.startPoint),
        finishPoint(other.finishPoint), mRandomCoef(other.mRandomCoef)
    {
        if (other.spline)
            spline = mmake<Spline>(*other.spline);
    }

    void FlightTrajectoryComponent::OnStart()
    {
        // default arc, tuned in the editor; middle key width is the random spread corridor
        if (!spline || spline->GetKeys().IsEmpty())
        {
            spline = mmake<Spline>();
            spline->AppendKey(Vec2F(0, 0), 0.0f);
            spline->AppendKey(Vec2F(200, 130), 90.0f);
            spline->AppendKey(Vec2F(400, 0), 0.0f);
        }
    }

    void FlightTrajectoryComponent::SetPoints(float startX, float startY, float finishX, float finishY)
    {
        startPoint = Vec2F(startX, startY);
        finishPoint = Vec2F(finishX, finishY);
        mBasisDirty = true;
    }

    void FlightTrajectoryComponent::ResetRandomOffset()
    {
        mRandomCoef = Math::Random(0.0f, 1.0f);
    }

    void FlightTrajectoryComponent::SetPosition(float value)
    {
        if (value <= 0.0f && mPrevPosition > 0.0f)
            ResetRandomOffset();

        position = value;
        mPrevPosition = value;
        ApplyToActor();
    }

    float FlightTrajectoryComponent::GetPosition() const
    {
        return position;
    }

    Vec2F FlightTrajectoryComponent::EvaluatePoint(float t)
    {
        t = Math::Clamp(t, 0.0f, 1.0f);

        if (!spline || spline->GetKeys().Count() < 2)
            return Math::Lerp(startPoint, finishPoint, t);

        if (mBasisDirty || mBasisStart != startPoint || mBasisFinish != finishPoint)
            UpdateBasis();

        return spline->Evaluate(t*spline->Length(), mRandomCoef)*mBasis;
    }

    String FlightTrajectoryComponent::GetName()
    {
        return "Flight trajectory";
    }

    String FlightTrajectoryComponent::GetCategory()
    {
        return "Animation";
    }

    String FlightTrajectoryComponent::GetIcon()
    {
        return "ui/UI4_trajectory_component.png";
    }

    void FlightTrajectoryComponent::UpdateBasis()
    {
        Vec2F splineStart = spline->Evaluate(0.0f);
        Vec2F splineFinish = spline->Evaluate(spline->Length());

        Vec2F splineDir = splineFinish - splineStart;
        Vec2F targetDir = finishPoint - startPoint;

        if (splineDir.Length() < 0.001f)
        {
            // degenerate spline: move its start to the start point without rotation
            mBasis = Basis(startPoint - splineStart);
        }
        else
        {
            float angle = splineDir.SignedAngle(targetDir);
            float scale = targetDir.Length()/splineDir.Length();

            float sn = Math::Sin(angle), cs = Math::Cos(angle);
            Vec2F xv(cs*scale, sn*scale);
            Vec2F yv(-sn*scale, cs*scale);
            mBasis = Basis(startPoint - Vec2F(splineStart.x*xv.x + splineStart.y*yv.x,
                                              splineStart.x*xv.y + splineStart.y*yv.y), xv, yv);
        }

        mBasisStart = startPoint;
        mBasisFinish = finishPoint;
        mBasisDirty = false;
    }

    void FlightTrajectoryComponent::OnUpdate(float dt)
    {
        // position returning to 0 (e.g. animation rewind) picks a new offset
        if (position <= 0.0f && mPrevPosition > 0.0f)
            ResetRandomOffset();
        mPrevPosition = position;

        ApplyToActor();
    }

    void FlightTrajectoryComponent::ApplyToActor()
    {
        auto actor = GetActor();
        if (!actor)
            return;

        Vec2F point = EvaluatePoint(position);

        // widgets are positioned by layout - move offsets around the anchor point
        if (auto widget = DynamicCast<Widget>(actor))
        {
            Vec2F size = widget->layout->GetSize2D();
            widget->layout->SetOffsetMin(point - size*0.5f);
            widget->layout->SetOffsetMax(point + size*0.5f);
        }
        else
            actor->transform->SetPosition2D(point);
    }
}
// --- META ---

DECLARE_CLASS(o2::FlightTrajectoryComponent, o2__FlightTrajectoryComponent);
// --- END META ---
