#include "o2Editor/stdafx.h"
#include "AssetsLineStatusDot.h"

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    static const float kPulseScale = 2.3f;
    static const float kPulsePeriod = 1.3f;

    Color4 AssetsLineStatusDot::GetStatusColor(AssetsLineStatus status)
    {
        switch (status)
        {
            case AssetsLineStatus::Synced:
            case AssetsLineStatus::Syncing: return Color4(56, 176, 88, 255);
            case AssetsLineStatus::Connecting: return Color4(48, 132, 226, 255);
            case AssetsLineStatus::NotConnected:
            case AssetsLineStatus::Offline:
            case AssetsLineStatus::Error: return Color4(228, 74, 60, 255);
            default: return Color4(158, 170, 176, 255);
        }
    }

    bool AssetsLineStatusDot::IsStatusPulsing(AssetsLineStatus status)
    {
        return status == AssetsLineStatus::Connecting || status == AssetsLineStatus::Syncing;
    }

    void AssetsLineStatusDot::Attach(const Ref<Widget>& widget, const Vec2F& anchor, const Vec2F& offset, float size)
    {
        mAnchor = anchor;
        mOffset = offset;
        mSize = size;
        mPulse = widget->AddLayer("status pulse", mmake<VectorSprite>("ui/pipeline/status_dot.svg"));
        mDot = widget->AddLayer("status dot", mmake<VectorSprite>("ui/pipeline/status_dot.svg"));
        Place(mPulse, mSize);
        Place(mDot, mSize);
        Update(0.0f, mStatus);
    }

    void AssetsLineStatusDot::Place(const Ref<WidgetLayer>& layer, float size)
    {
        layer->layout = Layout(mAnchor, mAnchor, mOffset - Vec2F(size, size)*0.5f, mOffset + Vec2F(size, size)*0.5f);
    }

    void AssetsLineStatusDot::Update(float dt, AssetsLineStatus status)
    {
        if (!mDot)
            return;

        if (status != mStatus)
            mPhase = 0.0f;

        mStatus = status;
        Color4 color = GetStatusColor(status);
        bool pulsing = IsStatusPulsing(status);

        if (pulsing)
            mPhase = Math::Mod(mPhase + dt/kPulsePeriod, 1.0f);

        Color4 dotColor = color;
        if (pulsing)
            dotColor.a = (int)(255*(0.8f + 0.2f*Math::Cos(mPhase*Math::PI()*2.0f)));

        if (hideWhenOff && status == AssetsLineStatus::Off)
            dotColor.a = 0;

        mDot->GetDrawable()->SetColor(dotColor);

        Color4 pulseColor = color;
        float fade = 1.0f - mPhase;
        pulseColor.a = pulsing ? (int)(110*fade*fade) : 0;
        mPulse->GetDrawable()->SetColor(pulseColor);
        Place(mPulse, Math::Lerp(mSize, mSize*kPulseScale, 1.0f - fade*fade));
    }
}
