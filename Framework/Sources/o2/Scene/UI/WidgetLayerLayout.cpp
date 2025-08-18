#include "o2/stdafx.h"
#include "WidgetLayerLayout.h"

#include "WidgetLayer.h"

namespace o2
{
    WidgetLayerLayout::WidgetLayerLayout()
    {}

    WidgetLayerLayout::WidgetLayerLayout(const WidgetLayerLayout& other) :
        mAnchorMin(other.mAnchorMin), mAnchorMax(other.mAnchorMax), mOffsetMin(other.mOffsetMin),
        mOffsetMax(other.mOffsetMax)
    {}

    WidgetLayerLayout::operator Layout() const
    {
        return Layout(mAnchorMin, mAnchorMax, mOffsetMin, mOffsetMax);
    }

    WidgetLayerLayout& WidgetLayerLayout::operator=(const WidgetLayerLayout& other)
    {
        mAnchorMax = other.mAnchorMax;
        mAnchorMin = other.mAnchorMin;
        mOffsetMax = other.mOffsetMax;
        mOffsetMin = other.mOffsetMin;

        return *this;
    }

    bool WidgetLayerLayout::operator!=(const Layout& other) const
    {
        return mAnchorMin != other.anchorMin || mAnchorMax != other.anchorMax ||
            mOffsetMin != other.offsetMin || mOffsetMax != other.offsetMax;
    }

    bool WidgetLayerLayout::operator!=(const WidgetLayerLayout& other) const
    {
        return mAnchorMin != other.mAnchorMin || mAnchorMax != other.mAnchorMax ||
            mOffsetMin != other.mOffsetMin || mOffsetMax != other.mOffsetMax;
    }

    bool WidgetLayerLayout::operator==(const Layout& other) const
    {
        return mAnchorMin == other.anchorMin && mAnchorMax == other.anchorMax &&
            mOffsetMin == other.offsetMin && mOffsetMax == other.offsetMax;
    }

    bool WidgetLayerLayout::operator==(const WidgetLayerLayout& other) const
    {
        return mAnchorMin == other.mAnchorMin && mAnchorMax == other.mAnchorMax &&
            mOffsetMin == other.mOffsetMin && mOffsetMax == other.mOffsetMax;
    }

    WidgetLayerLayout& WidgetLayerLayout::operator=(const Layout& other)
    {
        mAnchorMin = other.anchorMin;
        mAnchorMax = other.anchorMax;
        mOffsetMin = other.offsetMin;
        mOffsetMax = other.offsetMax;

        return *this;
    }

    void WidgetLayerLayout::SetOwner(const Ref<WidgetLayer>& widgetLayer)
    {
        mWidgetLayer = widgetLayer;
    }

    RectF WidgetLayerLayout::Calculate(const RectF& source)
    {
        Vec2F srcSize = source.Size();
        return RectF(source.left + srcSize.x*mAnchorMin.x + mOffsetMin.x,
                     source.bottom + srcSize.y*mAnchorMin.y + mOffsetMin.y,
                     source.left + srcSize.x*mAnchorMax.x + mOffsetMax.x,
                     source.bottom + srcSize.y*mAnchorMax.y + mOffsetMax.y);
    }

    void WidgetLayerLayout::SetAnchorMin(const Vec2F& min)
    {
        mAnchorMin = min;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    Vec2F WidgetLayerLayout::GetAnchorMin() const
    {
        return mAnchorMin;
    }

    void WidgetLayerLayout::SetAnchorMax(const Vec2F& max)
    {
        mAnchorMax = max;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    Vec2F WidgetLayerLayout::GetAnchorMax() const
    {
        return mAnchorMax;
    }

    void WidgetLayerLayout::SetAnchorLeft(float value)
    {
        mAnchorMin.x = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetAnchorLeft() const
    {
        return mAnchorMin.x;
    }

    void WidgetLayerLayout::SetAnchorRight(float value)
    {
        mAnchorMax.x = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetAnchorRight() const
    {
        return mAnchorMax.x;
    }

    void WidgetLayerLayout::SetAnchorBottom(float value)
    {
        mAnchorMin.y = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetAnchorBottom() const
    {
        return mAnchorMin.y;
    }

    void WidgetLayerLayout::SetAnchorTop(float value)
    {
        mAnchorMax.y = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetAnchorTop() const
    {
        return mAnchorMax.y;
    }

    void WidgetLayerLayout::SetOffsetMin(const Vec2F& min)
    {
        mOffsetMin = min;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    Vec2F WidgetLayerLayout::GetOffsetMin() const
    {
        return mOffsetMin;
    }

    void WidgetLayerLayout::SetOffsetMax(const Vec2F& max)
    {
        mOffsetMax = max;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    Vec2F WidgetLayerLayout::GetOffsetMax() const
    {
        return mOffsetMax;
    }

    void WidgetLayerLayout::SetOffsetLeft(float value)
    {
        mOffsetMin.x = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetOffsetLeft() const
    {
        return mOffsetMin.x;
    }

    void WidgetLayerLayout::SetoffsetRight(float value)
    {
        mOffsetMax.x = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetoffsetRight() const
    {
        return mOffsetMax.x;
    }

    void WidgetLayerLayout::SetOffsetBottom(float value)
    {
        mOffsetMin.y = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetOffsetBottom() const
    {
        return mOffsetMin.y;
    }

    void WidgetLayerLayout::SetOffsetTop(float value)
    {
        mOffsetMax.y = value;
        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    float WidgetLayerLayout::GetOffsetTop() const
    {
        return mOffsetMax.y;
    }

    void WidgetLayerLayout::SetPosition(const Vec2F& position)
    {
        Vec2F delta = position - mWidgetLayer.Lock()->mAbsolutePosition.LeftBottom();
        mOffsetMin += delta;
        mOffsetMax += delta;

        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    Vec2F WidgetLayerLayout::GetPosition() const
    {
        return mWidgetLayer.Lock()->mAbsolutePosition.LeftBottom();
    }

    void WidgetLayerLayout::SetSize(const Vec2F& size)
    {
        Vec2F delta = size - mWidgetLayer.Lock()->mAbsolutePosition.Size();
        mOffsetMin -= delta*0.5f;
        mOffsetMax += delta*0.5f;

        mWidgetLayer.Lock()->OnLayoutChanged();
    }

    Vec2F WidgetLayerLayout::GetSize() const
    {
        return mWidgetLayer.Lock()->mAbsolutePosition.Size();
    }

}
// --- META ---

DECLARE_CLASS(o2::WidgetLayerLayout, o2__WidgetLayerLayout);
// --- END META ---
