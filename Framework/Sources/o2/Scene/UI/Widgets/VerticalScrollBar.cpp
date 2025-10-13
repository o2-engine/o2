#include "o2/stdafx.h"
#include "VerticalScrollBar.h"

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    VerticalScrollBar::VerticalScrollBar(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this)
    {}

    VerticalScrollBar::VerticalScrollBar(RefCounter* refCounter, const VerticalScrollBar& other) :
        Widget(refCounter, other), DrawableCursorEventsListener(this), mValue(other.mValue), mMinValue(other.mMinValue),
        mMaxValue(other.mMaxValue), mScrollSense(other.mScrollSense), mScrollHandleSize(other.mScrollHandleSize),
        mHandlePressed(false), mScrollhandleMinPxSize(other.mScrollhandleMinPxSize), mSmoothValue(other.mValue)
    {
        mHandleLayer = FindLayer("handle");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    VerticalScrollBar::~VerticalScrollBar()
    {}

    VerticalScrollBar& VerticalScrollBar::operator=(const VerticalScrollBar& other)
    {
        Widget::operator=(other); 

        mValue = other.mValue;
        mMinValue = other.mMinValue;
        mMaxValue = other.mMaxValue;
        mScrollSense = other.mScrollSense;
        mScrollHandleSize = other.mScrollHandleSize;
        mScrollhandleMinPxSize = other.mScrollhandleMinPxSize;
        mSmoothValue = mValue;
        mHandlePressed = false;
        mHandleLayer = FindLayer("handle");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void VerticalScrollBar::Update(float dt)
    {
        Widget::Update(dt);

        const float threshold = 0.01f;
        const float smoothCoef = 30.0f;

        if (!Math::Equals(mValue, mSmoothValue, threshold))
        {
            mSmoothValue = Math::Clamp(Math::Lerpc(mSmoothValue, mValue, dt*smoothCoef), mMinValue, mMaxValue);

            if (Math::Abs(mValue - mSmoothValue) < threshold)
                mSmoothValue = mValue;

            UpdateProgressLayersLayouts();

            onSmoothChange(mSmoothValue);
        }
    }

    void VerticalScrollBar::SetValue(float value)
    {
        mValue = Math::Clamp(value, mMinValue, mMaxValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    void VerticalScrollBar::SetValueForcible(float value)
    {
        mValue = Math::Clamp(value, mMinValue, mMaxValue);
        mSmoothValue = mValue;
        UpdateProgressLayersLayouts();

        onChange(value);
        onSmoothChange(mSmoothValue);
    }

    float VerticalScrollBar::GetValue() const
    {
        return mValue;
    }

    void VerticalScrollBar::SetMinValue(float minValue)
    {
        mMinValue = minValue;
        mValue = Math::Max(mMinValue, mValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    float VerticalScrollBar::GetMinValue() const
    {
        return mMinValue;
    }

    void VerticalScrollBar::SetMaxValue(float maxValue)
    {
        mMaxValue = maxValue;
        mValue = Math::Min(mMaxValue, mValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    float VerticalScrollBar::GetMaxValue() const
    {
        return mMaxValue;
    }

    void VerticalScrollBar::SetValueRange(float minValue, float maxValue)
    {
        mMinValue = Math::Min(maxValue, minValue);
        mMaxValue = Math::Max(maxValue, minValue);
        mValue = Math::Clamp(mValue, mMinValue, mMaxValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    void VerticalScrollBar::SetScrollSense(float coef)
    {
        mScrollSense = coef;
    }

    float VerticalScrollBar::GetScrollSense() const
    {
        return mScrollSense;
    }

    void VerticalScrollBar::SetScrollHandleSize(float size)
    {
        mScrollHandleSize = size;
        UpdateProgressLayersLayouts();
    }

    float VerticalScrollBar::GetScrollHandleSize() const
    {
        return mScrollHandleSize;
    }

    bool VerticalScrollBar::IsUnderPoint(const Vec2F& point)
    {
        if (!mDrawingScissorRect.IsInside(point))
            return false;

        if (mHandleLayer && mHandleLayer->IsUnderPoint(point))
            return true;

        if (mBackLayer && mBackLayer->IsUnderPoint(point))
            return true;

        return false;
    }

    bool VerticalScrollBar::IsScrollable() const
    {
        return !Math::Equals(mMinValue, mMaxValue);
    }

    void VerticalScrollBar::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (mHandleLayer && mHandleLayer->IsUnderPoint(cursor.position))
        {
            mHandlePressed = true;
            float pressedValue = GetValueFromCursor(cursor);
            mPressHandleOffset = mValue - pressedValue;

            auto pressedState = state["pressed"];
            if (pressedState)
                *pressedState = true;
        }
        else
        {
            float pressedValue = GetValueFromCursor(cursor);
            if (pressedValue > mValue + mScrollHandleSize*0.5f)
                SetValueFromUser(mValue + mScrollHandleSize);
            else
                SetValueFromUser(mValue - mScrollHandleSize);
        }
    }

    void VerticalScrollBar::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        mHandlePressed = false;
    }

    void VerticalScrollBar::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        mHandlePressed = false;
    }

    void VerticalScrollBar::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mHandlePressed)
            SetValueFromUser(GetValueFromCursor(cursor) + mPressHandleOffset);
        else
        {
            const float timeThreshold = 0.5f;
            const float speed = 5.0f;

            if (cursor.pressedTime > timeThreshold && !(mHandleLayer && mHandleLayer->IsUnderPoint(cursor.position)))
            {
                float pressedValue = GetValueFromCursor(cursor);
                if (pressedValue > mValue + mScrollHandleSize*0.5f)
                    SetValueFromUser(mValue + mScrollHandleSize*o2Time.GetDeltaTime()*speed);
                else
                    SetValueFromUser(mValue - mScrollHandleSize*o2Time.GetDeltaTime()*speed);
            }
        }
    }

    float VerticalScrollBar::GetValueFromCursor(const Input::Cursor &cursor)
    {
        float range = mMaxValue - mMinValue;

        if (Math::Equals(range, 0.0f))
            return mValue;

        float height = layout->GetWorldRect().Height();
        float minScrollhandleSize = Math::Max(mScrollhandleMinPxSize/height*range, mScrollHandleSize);
        float rangleWithHandle = range + minScrollhandleSize;
        float szCoef = minScrollhandleSize/rangleWithHandle;

        return (height - (cursor.position.y - layout->GetWorldRect().bottom))/height*range/(1.0f - szCoef) + mMinValue;
    }

    void VerticalScrollBar::SetValueFromUser(float value)
    {
        SetValue(value);
        onChangeByUser(mValue);
    }

    void VerticalScrollBar::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;
    }

    void VerticalScrollBar::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;
    }

    void VerticalScrollBar::OnScrolled(float scroll)
    {
        SetValue(mValue - scroll*mScrollSense);
    }

    void VerticalScrollBar::OnDeserialized(const DataValue& node)
    {
        Widget::OnDeserialized(node);

        mHandleLayer = FindLayer("handle");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
    }

    void VerticalScrollBar::OnEnabled()
    {
        interactable = true;
    }

    void VerticalScrollBar::OnDisabled()
    {
        interactable = false;
    }

    void VerticalScrollBar::SetMinimalScrollHandleSize(float pixelSize)
    {
        mScrollhandleMinPxSize = pixelSize;
        UpdateProgressLayersLayouts();
    }

    void VerticalScrollBar::UpdateSelfTransform()
    {
        layout->Update();
    }

    void VerticalScrollBar::UpdateProgressLayersLayouts()
    {
        if (mHandleLayer)
        {
            float range = mMaxValue - mMinValue;
            float height = Math::Max(layout->GetWorldRect().Height(), 1.0f);
            float minScrollhandleSize = Math::Max(mScrollhandleMinPxSize/height*range, mScrollHandleSize);
            float rangleWithHandle = range + minScrollhandleSize;
            float szCoef = minScrollhandleSize/rangleWithHandle;
            float szPos = (mSmoothValue - mMinValue)/range*(1.0f - szCoef);

            if (Math::Equals(range, 0.0f))
            {
                szPos = 0;
                szCoef = 1;
            }

            mHandleLayer->layout.offsetMin = Vec2F();
            mHandleLayer->layout.offsetMax = Vec2F();
            mHandleLayer->layout.anchorMin = Vec2F(0, Math::Clamp01(1.0f - szPos));
            mHandleLayer->layout.anchorMax = Vec2F(1, Math::Clamp01(1.0f - szPos - szCoef));
        }

        if (mBackLayer)
            mBackLayer->layout = Layout::BothStretch();

        Widget::UpdateLayersLayouts();
    }

    void VerticalScrollBar::UpdateLayersLayouts()
    {
        UpdateProgressLayersLayouts();
    }

    void VerticalScrollBar::OnLayerAdded(const Ref<WidgetLayer>& layer)
    {
        if (layer->name == "back")
            mBackLayer = layer;
        else if (layer->name == "handle")
            mHandleLayer = layer;

        UpdateProgressLayersLayouts();
    }

    float VerticalScrollBar::GetSmoothValue() const
    {
        return mSmoothValue;
    }

    String VerticalScrollBar::GetCreateMenuGroup()
    {
        return "Progress";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::VerticalScrollBar>);
// --- META ---

DECLARE_CLASS(o2::VerticalScrollBar, o2__VerticalScrollBar);
// --- END META ---
