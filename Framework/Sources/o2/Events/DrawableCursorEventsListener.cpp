#include "o2/stdafx.h"
#include "DrawableCursorEventsListener.h"

#include "o2/Render/IDrawable.h"
#include "o2/Utils/Function/Function.h"

namespace o2
{

    DrawableCursorEventsListener::DrawableCursorEventsListener(IDrawable* drawable /*= nullptr*/):
        mEventHandleDrawable(nullptr)
    {
        SetEventHandleDrawable(drawable);
    }

    DrawableCursorEventsListener::DrawableCursorEventsListener(const DrawableCursorEventsListener& other):
        CursorAreaEventsListener(other), mEventHandleDrawable(nullptr)
    {
        SetEventHandleDrawable(other.mEventHandleDrawable);
    }

    DrawableCursorEventsListener::~DrawableCursorEventsListener()
    {
        if (mEventHandleDrawable)
            mEventHandleDrawable->onDraw -= MakeFunction(this, &DrawableCursorEventsListener::OnDrawn);
    }

    void DrawableCursorEventsListener::SetEventHandleDrawable(IDrawable* drawable)
    {
        if (mEventHandleDrawable)
            mEventHandleDrawable->onDraw -= MakeFunction(this, &DrawableCursorEventsListener::OnDrawn);

        mEventHandleDrawable = drawable;

        if (mEventHandleDrawable)
            mEventHandleDrawable->onDraw += MakeFunction(this, &DrawableCursorEventsListener::OnDrawn);
    }

    IDrawable* DrawableCursorEventsListener::GetEventHandleDrawable() const
    {
        return mEventHandleDrawable;
    }

    bool DrawableCursorEventsListener::IsUnderPoint(const Vec2F& point)
    {
        return mEventHandleDrawable ? mEventHandleDrawable->IsUnderPoint(point) : false;
    }

    void DrawableCursorEventsListener::OnDrawn()
    {
        CursorAreaEventsListener::OnDrawn();
    }
}
