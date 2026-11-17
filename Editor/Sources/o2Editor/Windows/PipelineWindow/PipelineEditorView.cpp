#include "o2Editor/stdafx.h"
#include "PipelineEditor.h"

#include <cmath>

#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

// "Show the whole pipeline" (AssetsLine fitView.ts): the camera flies so every card fits the view
namespace Editor
{
    namespace
    {
        const float flightTime = 0.42f; // Seconds
        const float roomPad = 56.0f;    // Screen pixels kept free on every side
    }

    void PipelineEditor::FitView(bool fly /*= true*/)
    {
        mFlightTime = -1.0f;
        Vec2F center;
        float scale = 1.0f;
        if (!mNodeWidgets.IsEmpty())
        {
            RectF bounds = mNodeWidgets[0]->GetCardRect();
            for (auto& widget : mNodeWidgets)
                bounds = bounds.Expand(widget->GetCardRect());

            // A small pipeline is not blown up past its natural size
            Vec2F view = layout->GetSize2D();
            Vec2F room(Math::Max(80.0f, view.x - roomPad*2.0f), Math::Max(80.0f, view.y - roomPad*2.0f));
            scale = Math::Max(1.0f, Math::Max(bounds.Width()/room.x, bounds.Height()/room.y));
            center = bounds.Center();
        }
        scale = Math::Clamp(scale, mViewCameraMinScale, mViewCameraMaxScale);

        if (!fly)
        {
            SetView(center, scale);
            mCameraDirty = true;
            return;
        }

        mFlightFrom = mViewCamera.GetPosition2D();
        mFlightFromScale = mViewCamera.GetScale2D().x;
        mFlightTo = center;
        mFlightToScale = scale;
        mFlightTime = 0.0f;
        mViewCameraTargetPos = mFlightShownPos = mFlightFrom;
        mViewCameraTargetScale = Vec2F(mFlightFromScale, mFlightFromScale);
        mFlightShownScale = mFlightFromScale;
    }

    void PipelineEditor::UpdateFlight(float dt)
    {
        if (mFlightTime < 0.0f)
            return;

        // Anything else moving the camera (the wheel, a pan, another fit) ends the flight where it is
        if (mViewCameraTargetPos != mFlightShownPos || mViewCameraTargetScale.x != mFlightShownScale)
        {
            mFlightTime = -1.0f;
            return;
        }

        mFlightTime = Math::Min(flightTime, mFlightTime + dt);
        float k = mFlightTime/flightTime;
        float eased = 1.0f - std::pow(1.0f - k, 3.0f);

        // The zoom goes in log space and the point being centred straight to its place: a straight lerp of offsets swings
        float scale = std::exp(std::log(mFlightFromScale) + (std::log(mFlightToScale) - std::log(mFlightFromScale))*eased);
        SetView(Math::Lerp(mFlightFrom, mFlightTo, eased), scale);
        mFlightShownPos = mViewCameraTargetPos;
        mFlightShownScale = mViewCameraTargetScale.x;
        mCameraDirty = true;
        if (k >= 1.0f)
            mFlightTime = -1.0f;
    }
}
