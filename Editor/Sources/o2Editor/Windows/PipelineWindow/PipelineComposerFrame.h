#pragma once

#include "o2/Utils/Editor/FrameHandles.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------------------------------------------------
    // Transform frame of the composer's selected layer. The engine frame sized in screen pixels whatever the editor
    // camera's zoom: the line, the handles with their grab areas and the rotation corners all go through one factor
    // ------------------------------------------------------------------------------------------------------------
    class PipelineComposerFrame : public FrameHandles
    {
    public:
        static constexpr float lineWidth = 1.5f;   // Frame line, screen pixels
        static constexpr float haloWidth = 2.5f;   // Faint halo round the line, screen pixels
        static constexpr float grabMargin = 5.0f;  // A resize handle is grabbed this far round its picture
        static constexpr float rotateSize = 22.0f; // Rotation corner, reaching past the grab area of its handle

    public:
        // Default constructor; remembers the sizes the handles are drawn with at scale 1
        PipelineComposerFrame();

        // Sizes the handles by the factor: the size of a screen pixel in canvas units, within the composer's limits
        void SetScreenScale(float scale);

        // Returns the factor the handles are sized by
        float GetScreenScale() const { return mScale; }

        // Draws the line just outside the frame with its halo, then the handles; pixelSize is a screen pixel in canvas units
        void Draw(float pixelSize);

    protected:
        Vector<Ref<DragHandle>> mResizeHandles; // The eight handles of the corners and the sides
        Vector<Ref<DragHandle>> mRotateHandles; // The four rotation corners
        Vec2F                   mHandleSize;    // Size of a resize handle's picture at scale 1
        float                   mScale = 0.0f;  // Factor the handles are sized by, 0 before the first one

    protected:
        // Returns true when the point is on the handle's picture or within the grab margin round it
        bool IsHandleUnderPoint(const DragHandle& handle, const Vec2F& point) const;
    };
}
