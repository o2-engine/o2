#pragma once

#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Types/Ref.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"

using namespace o2;

namespace o2
{
    class Widget;
    class WidgetLayer;
}

namespace Editor
{
    // ------------------------------------------------------------------------------
    // Dot of the AssetsLine link state drawn on a widget: grey - off, red - no link,
    // blue pulsing - connecting, green - connected, green pulsing - syncing
    // ------------------------------------------------------------------------------
    class AssetsLineStatusDot
    {
    public:
        // Adds the dot and its pulse to the widget, centred at the offset from the anchor (relative point of the widget)
        void Attach(const Ref<Widget>& widget, const Vec2F& anchor, const Vec2F& offset, float size = 9.0f);

        // Hides the dot while the link is off, for a badge that should not draw the eye then
        bool hideWhenOff = false;

        // Shows the state and moves the pulse on
        void Update(float dt, AssetsLineStatus status);

        // Returns the state shown
        AssetsLineStatus GetStatus() const { return mStatus; }

        // Returns true while the dot pulses
        bool IsPulsing() const { return IsStatusPulsing(mStatus); }

        // Returns the colour of the state
        static Color4 GetStatusColor(AssetsLineStatus status);

        // Returns true for the states that are in progress
        static bool IsStatusPulsing(AssetsLineStatus status);

    private:
        Ref<WidgetLayer> mDot;          // The dot itself
        Ref<WidgetLayer> mPulse;        // Ring growing out of the dot while in progress
        Vec2F            mAnchor;       // Relative point of the widget the dot is placed from
        Vec2F            mOffset;       // Dot centre from the anchor
        float            mSize = 9.0f;  // Dot diameter
        float            mPhase = 0.0f; // Pulse position, 0..1
        AssetsLineStatus mStatus = AssetsLineStatus::Off; // State shown

    private:
        // Places a layer as a circle of the size around the dot centre
        void Place(const Ref<WidgetLayer>& layer, float size);
    };
}
