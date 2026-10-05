#include "o2Editor/stdafx.h"
#include "PipelineComposerFrame.h"

#include "o2/Render/Render.h"

namespace Editor
{
    PipelineComposerFrame::PipelineComposerFrame()
    {
        mResizeHandles = { mLeftTopHandle, mLeftHandle, mLeftBottomHandle, mTopHandle, mBottomHandle, mRightTopHandle, mRightHandle,
                           mRightBottomHandle };
        mRotateHandles = { mLeftTopRotateHandle, mLeftBottomRotateHandle, mRightTopRotateHandle, mRightBottomRotateHandle };
        mHandleSize = mLeftTopHandle->GetRegularDrawable()->GetSize2D();
        if (mHandleSize.x <= 0.0f || mHandleSize.y <= 0.0f)
            mHandleSize = Vec2F(mFrameHandlesSize, mFrameHandlesSize);

        for (auto& handle : mResizeHandles)
        {
            WeakRef<DragHandle> weakHandle(handle);
            handle->isPointInside = [this, weakHandle](const Vec2F& point)
            {
                auto handle = weakHandle.Lock();
                return handle && IsHandleUnderPoint(*handle, point);
            };
        }

        SetScreenScale(1.0f);
    }

    void PipelineComposerFrame::SetScreenScale(float scale)
    {
        if (Math::Equals(scale, mScale))
            return;

        mScale = scale;
        for (auto& handle : mResizeHandles)
            handle->SetDrawablesSize(mHandleSize*scale);

        for (auto& handle : mRotateHandles)
        {
            handle->SetDrawablesSize(Vec2F(rotateSize, rotateSize)*scale);
            handle->SetDrawablesSizePivot(Vec2F(mFrameHandlesSize, mFrameHandlesSize)*(0.5f*scale));
        }
    }

    bool PipelineComposerFrame::IsHandleUnderPoint(const DragHandle& handle, const Vec2F& point) const
    {
        Vec2F delta = point - handle.GetScreenPosition();
        Vec2F half = mHandleSize*(0.5f*mScale) + Vec2F(grabMargin, grabMargin)*mScale;
        return Math::Abs(delta.Dot(mFrame.xv.Normalized())) <= half.x && Math::Abs(delta.Dot(mFrame.yv.Normalized())) <= half.y;
    }

    void PipelineComposerFrame::Draw(float pixelSize)
    {
        // The line stands outside the layer's box, so a small layer is not covered by its own frame
        float out = lineWidth*0.5f*pixelSize;
        Vec2F xu = mFrame.xv.Normalized()*out, yu = mFrame.yv.Normalized()*out;
        Vec2F origin = mFrame.origin - xu - yu, xv = mFrame.xv + xu*2.0f, yv = mFrame.yv + yu*2.0f;
        Vector<Vec2F> line = { origin, origin + xv, origin + xv + yv, origin + yv, origin };
        // A faint light halo keeps the dark line visible on a dark picture
        o2Render.DrawAALine(line, Color4(255, 255, 255, 60), lineWidth + haloWidth);
        o2Render.DrawAALine(line, mFrameColor, lineWidth);
        CursorAreaEventsListener::OnDrawn();

        for (auto& handle : mRotateHandles)
            handle->Draw();

        for (auto& handle : mResizeHandles)
            handle->Draw();
    }
}
