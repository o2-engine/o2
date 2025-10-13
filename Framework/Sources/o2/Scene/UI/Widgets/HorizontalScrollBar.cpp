#include "o2/stdafx.h"
#include "HorizontalScrollBar.h"

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    HorizontalScrollBar::HorizontalScrollBar(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this)
    {}

    HorizontalScrollBar::HorizontalScrollBar(RefCounter* refCounter, const HorizontalScrollBar& other) :
        Widget(refCounter, other), DrawableCursorEventsListener(this), mValue(other.mValue), mMinValue(other.mMinValue),
        mMaxValue(other.mMaxValue), mScrollSense(other.mScrollSense), mScrollHandleSize(other.mScrollHandleSize),
        mHandlePressed(false), mScrollhandleMinPxSize(other.mScrollhandleMinPxSize), mSmoothValue(other.mValue)
    {
        mHandleLayer = FindLayer("handle");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
    }

    HorizontalScrollBar::~HorizontalScrollBar()
    {}

    HorizontalScrollBar& HorizontalScrollBar::operator=(const HorizontalScrollBar& other)
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

    void HorizontalScrollBar::Update(float dt)
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

    void HorizontalScrollBar::SetValue(float value)
    {
        mValue = Math::Clamp(value, mMinValue, mMaxValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    void HorizontalScrollBar::SetValueForcible(float value)
    {
        mValue = Math::Clamp(value, mMinValue, mMaxValue);
        mSmoothValue = mValue;
        UpdateProgressLayersLayouts();

        onChange(value);
        onSmoothChange(mSmoothValue);
    }

    float HorizontalScrollBar::GetValue() const
    {
        return mValue;
    }

    float HorizontalScrollBar::GetSmoothValue() const
    {
        return mSmoothValue;
    }

    void HorizontalScrollBar::SetMinValue(float minValue)
    {
        mMinValue = minValue;
        mValue = Math::Max(mMinValue, mValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    float HorizontalScrollBar::GetMinValue() const
    {
        return mMinValue;
    }

    void HorizontalScrollBar::SetMaxValue(float maxValue)
    {
        mMaxValue = maxValue;
        mValue = Math::Min(mMaxValue, mValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    float HorizontalScrollBar::GetMaxValue() const
    {
        return mMaxValue;
    }

    void HorizontalScrollBar::SetValueRange(float minValue, float maxValue)
    {
        mMinValue = Math::Min(maxValue, minValue);
        mMaxValue = Math::Max(maxValue, minValue);
        mValue = Math::Clamp(mValue, mMinValue, mMaxValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    void HorizontalScrollBar::SetScrollSense(float coef)
    {
        mScrollSense = coef;
    }

    float HorizontalScrollBar::GetScrollSense() const
    {
        return mScrollSense;
    }

    void HorizontalScrollBar::SetScrollHandleSize(float size)
    {
        mScrollHandleSize = size;
        UpdateProgressLayersLayouts();
    }

    float HorizontalScrollBar::GetScrollHandleSize() const
    {
        return mScrollHandleSize;
    }

    bool HorizontalScrollBar::IsUnderPoint(const Vec2F& point)
    {
        if (!mDrawingScissorRect.IsInside(point))
            return false;

        if (mHandleLayer && mHandleLayer->IsUnderPoint(point))
            return true;

        if (mBackLayer && mBackLayer->IsUnderPoint(point))
            return true;

        return false;
    }

    bool HorizontalScrollBar::IsScrollable() const
    {
        return !Math::Equals(mMinValue, mMaxValue);
    }

    void HorizontalScrollBar::OnCursorPressed(const Input::Cursor& cursor)
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

    void HorizontalScrollBar::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        mHandlePressed = false;
    }

    void HorizontalScrollBar::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        mHandlePressed = false;
    }

    void HorizontalScrollBar::OnCursorStillDown(const Input::Cursor& cursor)
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

    float HorizontalScrollBar::GetValueFromCursor(const Input::Cursor &cursor)
    {
        float range = mMaxValue - mMinValue;

        if (Math::Equals(range, 0.0f))
            return mValue;

        float width = Math::Max(layout->GetWidth(), 1.0f);
        float minScrollhandleSize = Math::Max(mScrollhandleMinPxSize/width*range, mScrollHandleSize);
        float rangleWithHandle = range + minScrollhandleSize;
        float szCoef = minScrollhandleSize/rangleWithHandle;

        return (cursor.position.x - layout->worldLeft)/width*range/(1.0f - szCoef) + mMinValue;
    }

    void HorizontalScrollBar::SetValueFromUser(float value)
    {
        SetValue(value);
        onChangeByUser(mValue);
    }

    void HorizontalScrollBar::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;
    }

    void HorizontalScrollBar::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;
    }

    void HorizontalScrollBar::OnScrolled(float scroll)
    {
        SetValue(mValue - scroll*mScrollSense);
    }

    void HorizontalScrollBar::OnDeserialized(const DataValue& node)
    {
        Widget::OnDeserialized(node);

        mHandleLayer = FindLayer("handle");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
    }

    void HorizontalScrollBar::OnEnabled()
    {
        Widget::OnEnabled();

        interactable = true;
    }

    void HorizontalScrollBar::OnDisabled()
    {
        Widget::OnDisabled();

        interactable = false;
    }

    void HorizontalScrollBar::UpdateLayersLayouts()
    {
        UpdateProgressLayersLayouts();
    }

    void HorizontalScrollBar::SetMinimalScrollHandleSize(float pixelSize)
    {
        mScrollhandleMinPxSize = pixelSize;
        UpdateProgressLayersLayouts();
    }

    void HorizontalScrollBar::UpdateProgressLayersLayouts()
    {
        if (mHandleLayer)
        {
            float range = mMaxValue - mMinValue;
            float width = Math::Max(layout->GetWidth(), 1.0f);
            float minScrollhandleSize = Math::Max(mScrollhandleMinPxSize/width*range, mScrollHandleSize);
            float rangeWithHandle = range + minScrollhandleSize;
            float szCoef = minScrollhandleSize/rangeWithHandle;
            float szPos = (mSmoothValue - mMinValue)/range*(1.0f - szCoef);

            if (Math::Equals(range, 0.0f))
            {
                szPos = 0;
                szCoef = 1;
            }

            mHandleLayer->layout.offsetMin = Vec2F();
            mHandleLayer->layout.offsetMax = Vec2F();
            mHandleLayer->layout.anchorMin = Vec2F(Math::Clamp01(szPos), 0);
            mHandleLayer->layout.anchorMax = Vec2F(Math::Clamp01(szPos + szCoef), 1);
        }

        if (mBackLayer)
            mBackLayer->layout = Layout::BothStretch();

        Widget::UpdateLayersLayouts();
    }

    void HorizontalScrollBar::OnLayerAdded(const Ref<WidgetLayer>& layer)
    {
        if (layer->name == "back")
            mBackLayer = layer;
        else if (layer->name == "handle")
            mHandleLayer = layer;

        UpdateProgressLayersLayouts();
    }

    String HorizontalScrollBar::GetCreateMenuGroup()
    {
        return "Progress";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::HorizontalScrollBar>);
// --- META ---

DECLARE_CLASS(o2::HorizontalScrollBar, o2__HorizontalScrollBar);
// --- END META ---
