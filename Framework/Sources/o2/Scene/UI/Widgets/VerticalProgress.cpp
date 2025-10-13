#include "o2/stdafx.h"
#include "o2/Scene/UI/Widgets/VerticalProgress.h"

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"

namespace o2
{
    VerticalProgress::VerticalProgress(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this)
    {}

    VerticalProgress::VerticalProgress(RefCounter* refCounter, const VerticalProgress& other) :
        Widget(refCounter, other), DrawableCursorEventsListener(this), mValue(other.mValue), mMinValue(other.mMinValue),
        mMaxValue(other.mMaxValue), mOrientation(other.mOrientation), mScrollSense(other.mScrollSense)
    {
        mBarLayer = FindLayer("bar");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
    }

    VerticalProgress::~VerticalProgress()
    {}

    VerticalProgress& VerticalProgress::operator=(const VerticalProgress& other)
    {
        Widget::operator=(other);

        mValue = other.mValue;
        mMinValue = other.mMinValue;
        mMaxValue = other.mMaxValue;
        mOrientation = other.mOrientation;
        mScrollSense = other.mScrollSense;
        mBarLayer = FindLayer("bar");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void VerticalProgress::Update(float dt)
    {
        Widget::Update(dt);

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        const float threshold = 0.01f;
        const float smoothCoef = 30.0f;

        if (!Math::Equals(mValue, mSmoothValue, threshold))
        {
            mSmoothValue = Math::Clamp(Math::Lerp(mSmoothValue, mValue, dt*smoothCoef), mMinValue, mMaxValue);

            if (Math::Abs(mValue - mSmoothValue) < threshold)
                mSmoothValue = mValue;

            UpdateProgressLayersLayouts();
        }
    }

    void VerticalProgress::SetValue(float value, bool byUser /*= false*/)
    {
        mValue = Math::Clamp(value, mMinValue, mMaxValue);
        UpdateProgressLayersLayouts();

        onChange(mValue);

        if (byUser)
            onChangeByUser(mValue);
    }

    void VerticalProgress::SetValueForcible(float value)
    {
        mValue = Math::Clamp(value, mMinValue, mMaxValue);
        mSmoothValue = mValue;
        UpdateProgressLayersLayouts();

        onChange(mValue);
    }

    float VerticalProgress::GetValue() const
    {
        return mValue;
    }

    void VerticalProgress::SetMinValue(float minValue)
    {
        mMinValue = minValue;
        mValue = Math::Max(mMinValue, mValue);
        UpdateProgressLayersLayouts();

        onChange(mValue);
    }

    float VerticalProgress::GetMinValue() const
    {
        return mMinValue;
    }

    void VerticalProgress::SetMaxValue(float maxValue)
    {
        mMaxValue = maxValue;
        mValue = Math::Min(mMaxValue, mValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    float VerticalProgress::GetMaxValue() const
    {
        return mMaxValue;
    }

    void VerticalProgress::SetValueRange(float minValue, float maxValue)
    {
        mMaxValue = maxValue;
        mValue = Math::Clamp(mValue, mMinValue, mMaxValue);
        UpdateProgressLayersLayouts();

        onChange(value);
    }

    void VerticalProgress::SetScrollSense(float coef)
    {
        mScrollSense = coef;
    }

    float VerticalProgress::GetScrollSense() const
    {
        return mScrollSense;
    }

    void VerticalProgress::SetOrientation(Orientation orientation)
    {
        mOrientation = orientation;
        UpdateLayersLayouts();
    }

    VerticalProgress::Orientation VerticalProgress::GetOrientation() const
    {
        return mOrientation;
    }

    bool VerticalProgress::IsUnderPoint(const Vec2F& point)
    {
        if (mBackLayer)
            return mDrawingScissorRect.IsInside(point) && mBackLayer->IsUnderPoint(point);

        return false;
    }

    bool VerticalProgress::IsScrollable() const
    {
        return true;
    }

    void VerticalProgress::OnCursorPressed(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = true;

        GetValueFromCursor(cursor);
    }

    void VerticalProgress::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;
    }

    void VerticalProgress::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;
    }

    void VerticalProgress::OnCursorStillDown(const Input::Cursor& cursor)
    {
        GetValueFromCursor(cursor);
    }

    void VerticalProgress::GetValueFromCursor(const Input::Cursor &cursor)
    {
        float height = layout->height;
        float d = mMaxValue - mMinValue;
        if (mOrientation == VerticalProgress::Orientation::Up)

            SetValue((cursor.position.y - layout->worldBottom)/height*d + mMinValue, true);
        else
            SetValue((height - (cursor.position.y - layout->worldBottom))/height*d + mMinValue, true);
    }

    void VerticalProgress::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;
    }

    void VerticalProgress::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;
    }

    void VerticalProgress::OnScrolled(float scroll)
    {
        SetValue(mValue + scroll*mScrollSense);
    }

    void VerticalProgress::OnDeserialized(const DataValue& node)
    {
        Widget::OnDeserialized(node);

        mBarLayer = FindLayer("bar");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
    }

    void VerticalProgress::OnEnabled()
    {
        Widget::OnEnabled();

        interactable = true;
    }

    void VerticalProgress::OnDisabled()
    {
        Widget::OnDisabled();

        interactable = false;
    }

    void VerticalProgress::UpdateLayersLayouts()
    {
        UpdateProgressLayersLayouts();
    }

    void VerticalProgress::UpdateProgressLayersLayouts()
    {
        if (mBarLayer)
        {
            mBarLayer->layout.offsetMin = Vec2F();
            mBarLayer->layout.offsetMax = Vec2F();

            if (mOrientation == Orientation::Down)
            {
                mBarLayer->layout.anchorMin = Vec2F(0, 1.0f - (mSmoothValue - mMinValue)/(mMaxValue - mMinValue));
                mBarLayer->layout.anchorMax = Vec2F(1, 1);
            }
            else if (mOrientation == Orientation::Up)
            {
                mBarLayer->layout.anchorMin = Vec2F(0, 0);
                mBarLayer->layout.anchorMax = Vec2F(1, (mSmoothValue - mMinValue)/(mMaxValue - mMinValue));
            }
        }

        if (mBackLayer)
            mBackLayer->layout = Layout::BothStretch();

        Widget::UpdateLayersLayouts();
    }

    void VerticalProgress::OnLayerAdded(const Ref<WidgetLayer>& layer)
    {
        if (layer->name == "back")
            mBackLayer = layer;
        else if (layer->name == "bar")
            mBarLayer = layer;

        UpdateProgressLayersLayouts();
    }

    String VerticalProgress::GetCreateMenuGroup()
    {
        return "Progress";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::VerticalProgress>);
// --- META ---

ENUM_META(o2::VerticalProgress::Orientation)
{
    ENUM_ENTRY(Down);
    ENUM_ENTRY(Up);
}
END_ENUM_META;

DECLARE_CLASS(o2::VerticalProgress, o2__VerticalProgress);
// --- END META ---
