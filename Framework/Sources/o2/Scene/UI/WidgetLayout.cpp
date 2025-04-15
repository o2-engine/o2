#include "o2/stdafx.h"
#include "WidgetLayout.h"

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"

namespace o2
{
    WidgetLayout::WidgetLayout() :
        ActorTransform(mnew WidgetLayoutData())
    {
        mData = static_cast<WidgetLayoutData*>(ActorTransform::mData);
        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
    }

    WidgetLayout::WidgetLayout(const WidgetLayout& other) :
        ActorTransform(mnew WidgetLayoutData()), anchorMin(this), anchorMax(this), offsetMin(this), offsetMax(this),
        anchorLeft(this), anchorRight(this), anchorBottom(this), anchorTop(this), offsetLeft(this),
        offsetBottom(this), offsetTop(this), minSize(this), minWidth(this), minHeight(this),
        maxSize(this), maxWidth(this), maxHeight(this), weight(this), widthWeight(this),
        heigthWeight(this), offsetRight(this)
    {
        mData = static_cast<WidgetLayoutData*>(ActorTransform::mData);

        CopyFrom(other);
        mCheckMinMaxFunc = other.mCheckMinMaxFunc;
    }

    WidgetLayout::WidgetLayout(const Vec2F& anchorMin, const Vec2F& anchorMax,
                               const Vec2F& offsetMin, const Vec2F& offsetMax) :
        ActorTransform(mnew WidgetLayoutData())
    {
        mData = static_cast<WidgetLayoutData*>(ActorTransform::mData);

        mData->anchorMin = anchorMin;
        mData->anchorMax = anchorMax;
        mData->offsetMin = offsetMin;
        mData->offsetMax = offsetMax;

        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
    }

    WidgetLayout::WidgetLayout(float anchorLeft, float anchorTop, float anchorRight, float anchorBottom,
                               float offsetLeft, float offsetTop, float offsetRight, float offsetBottom) :
        ActorTransform(mnew WidgetLayoutData())
    {
        mData = static_cast<WidgetLayoutData*>(ActorTransform::mData);

        mData->anchorMin.Set(anchorLeft, anchorBottom);
        mData->anchorMax.Set(anchorRight, anchorTop);
        mData->offsetMin.Set(offsetLeft, offsetBottom);
        mData->offsetMax.Set(offsetRight, offsetTop);

        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
    }

    WidgetLayout& WidgetLayout::operator=(const WidgetLayout& other)
    {
        CopyFrom(other);
        SetDirty();

        return *this;
    }

    bool WidgetLayout::operator==(const WidgetLayout& other) const
    {
        return mData->anchorMin == other.mData->anchorMin &&
            mData->anchorMax == other.mData->anchorMax &&
            mData->offsetMin == other.mData->offsetMin &&
            mData->offsetMax == other.mData->offsetMax;
    }

    void WidgetLayout::SetPosition(const Vec2F& position)
    {
        Vec2F delta = position - GetPosition();
        mData->offsetMin += delta;
        mData->offsetMax += delta;

        SetDirty();
    }

    void WidgetLayout::SetSize(const Vec2F& size)
    {
        RectF parentRect = GetParentRectangle();
        RectF rectangle(mData->offsetMin + mData->anchorMin*parentRect.Size(),
                        mData->offsetMax + mData->anchorMax*parentRect.Size());

        Vec2F szDelta = size - rectangle.Size();
        mData->offsetMax += szDelta*(Vec2F::One() - mData->pivot);
        mData->offsetMin -= szDelta*mData->pivot;

        SetDirty();
    }

    void WidgetLayout::SetWidth(float value)
    {
        RectF parentRect = GetParentRectangle();
        RectF rectangle(mData->offsetMin + mData->anchorMin*parentRect.Size(),
                        mData->offsetMax + mData->anchorMax*parentRect.Size());

        float szDelta = value - rectangle.Width();
        mData->offsetMax.x += szDelta*(1.0f - mData->pivot.x);
        mData->offsetMin.x -= szDelta*mData->pivot.x;

        SetDirty();
    }

    void WidgetLayout::SetHeight(float value)
    {
        RectF parentRect = GetParentRectangle();
        RectF rectangle(mData->offsetMin + mData->anchorMin*parentRect.Size(),
                        mData->offsetMax + mData->anchorMax*parentRect.Size());

        float szDelta = value - rectangle.Height();
        mData->offsetMax.y += szDelta*mData->pivot.y;
        mData->offsetMin.y -= szDelta*(1.0f - mData->pivot.y);

        SetDirty();
    }

    Vec2F WidgetLayout::GetSize() const
    {
        return Vec2F(GetWidth(), GetHeight());
    }

    float WidgetLayout::GetWidth() const
    {
        return Math::Clamp(mData->size.x, mData->minSize.x, mData->maxSize.x);
    }

    float WidgetLayout::GetHeight() const
    {
        return Math::Clamp(mData->size.y, mData->minSize.y, mData->maxSize.y);
    }

    void WidgetLayout::SetRect(const RectF& rect)
    {
        RectF parentRect = GetParentRectangle();
        RectF parentAnchoredRect(parentRect.Size()*mData->anchorMin,
                                 parentRect.Size()*mData->anchorMax);

        RectF localRect = rect;
        if (auto parent = mData->owner->mParent.Lock())
            localRect += parentRect.Size()*parent->transform->mData->pivot;

        mData->offsetMin = localRect.LeftBottom() - parentAnchoredRect.LeftBottom();
        mData->offsetMax = localRect.RightTop() - parentAnchoredRect.RightTop();

        SetDirty();
    }

    RectF WidgetLayout::GetChildrenWorldRect() const
    {
        return mData->childrenWorldRect;
    }

    void WidgetLayout::SetAxisAlignedRect(const RectF& rect)
    {
        ActorTransform::SetAxisAlignedRect(rect);
        UpdateOffsetsByCurrentTransform();
    }

    void WidgetLayout::SetPivot(const Vec2F& pivot)
    {
        mData->pivot = pivot;
        SetDirty();
    }

    void WidgetLayout::SetBasis(const Basis& basis)
    {
        ActorTransform::SetBasis(basis);
        UpdateOffsetsByCurrentTransform();
    }

    void WidgetLayout::SetNonSizedBasis(const Basis& basis)
    {
        ActorTransform::SetNonSizedBasis(basis);
        UpdateOffsetsByCurrentTransform();
    }

    RectF WidgetLayout::GetRect() const
    {
        RectF parentRect = GetParentRectangle();

        RectF rectangle(mData->offsetMin + mData->anchorMin*parentRect.Size(),
                        mData->offsetMax + mData->anchorMax*parentRect.Size());

        return rectangle;
    }

    void WidgetLayout::SetAnchorMin(const Vec2F& min)
    {
        mData->anchorMin = min;
        SetDirty();
    }

    Vec2F WidgetLayout::GetAnchorMin() const
    {
        return mData->anchorMin;
    }

    void WidgetLayout::SetAnchorMax(const Vec2F& max)
    {
        mData->anchorMax = max;
        SetDirty();
    }

    Vec2F WidgetLayout::GetAnchorMax() const
    {
        return mData->anchorMax;
    }

    void WidgetLayout::SetAnchorLeft(float value)
    {
        mData->anchorMin.x = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorLeft() const
    {
        return mData->anchorMin.x;
    }

    void WidgetLayout::SetAnchorRight(float value)
    {
        mData->anchorMax.x = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorRight() const
    {
        return mData->anchorMax.x;
    }

    void WidgetLayout::SetAnchorBottom(float value)
    {
        mData->anchorMin.y = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorBottom() const
    {
        return mData->anchorMin.y;
    }

    void WidgetLayout::SetAnchorTop(float value)
    {
        mData->anchorMax.y = value;
        SetDirty();
    }

    float WidgetLayout::GetAnchorTop() const
    {
        return mData->anchorMax.y;
    }

    void WidgetLayout::SetOffsetMin(const Vec2F& min)
    {
        mData->offsetMin = min;
        SetDirty();
    }

    Vec2F WidgetLayout::GetOffsetMin() const
    {
        return mData->offsetMin;
    }

    void WidgetLayout::SetOffsetMax(const Vec2F& max)
    {
        mData->offsetMax = max;
        SetDirty();
    }

    Vec2F WidgetLayout::GetOffsetMax() const
    {
        return mData->offsetMax;
    }

    void WidgetLayout::SetOffsetLeft(float value)
    {
        mData->offsetMin.x = value;
        SetDirty();
    }

    float WidgetLayout::GetOffsetLeft() const
    {
        return mData->offsetMin.x;
    }

    void WidgetLayout::SetoffsetRight(float value)
    {
        mData->offsetMax.x = value;
        SetDirty();
    }

    float WidgetLayout::GetoffsetRight() const
    {
        return mData->offsetMax.x;
    }

    void WidgetLayout::SetOffsetBottom(float value)
    {
        mData->offsetMin.y = value;
        SetDirty();
    }

    float WidgetLayout::GetOffsetBottom() const
    {
        return mData->offsetMin.y;
    }

    void WidgetLayout::SetOffsetTop(float value)
    {
        mData->offsetMax.y = value;
        SetDirty();
    }

    float WidgetLayout::GetOffsetTop() const
    {
        return mData->offsetMax.y;
    }

    void WidgetLayout::SetMinimalSize(const Vec2F& minSize)
    {
        mData->minSize = minSize;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    Vec2F WidgetLayout::GetMinimalSize() const
    {
        return mData->minSize;
    }

    void WidgetLayout::SetMinimalWidth(float value)
    {
        mData->minSize.x = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMinWidth() const
    {
        return mData->minSize.x;
    }

    void WidgetLayout::SetMinimalHeight(float value)
    {
        mData->minSize.y = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMinHeight() const
    {
        return mData->minSize.y;
    }

    void WidgetLayout::SetMaximalSize(const Vec2F& maxSize)
    {
        mData->maxSize = maxSize;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    Vec2F WidgetLayout::GetMaximalSize() const
    {
        return mData->maxSize;
    }

    void WidgetLayout::SetMaximalWidth(float value)
    {
        mData->maxSize.x = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMaxWidth() const
    {
        return mData->maxSize.x;
    }

    void WidgetLayout::SetMaximalHeight(float value)
    {
        mData->maxSize.y = value;
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
        SetDirty();
    }

    float WidgetLayout::GetMaxHeight() const
    {
        return mData->maxSize.y;
    }

    void WidgetLayout::DisableSizeChecks()
    {
        mCheckMinMaxFunc = &WidgetLayout::DontCheckMinMax;
        SetDirty();
    }

    void WidgetLayout::EnableSizeChecks()
    {
        mCheckMinMaxFunc = &WidgetLayout::CheckMinMax;
    }

    void WidgetLayout::SetWeight(const Vec2F& weight)
    {
        mData->weight = weight;
        SetDirty();
    }

    Vec2F WidgetLayout::GetWeight() const
    {
        return mData->weight;
    }

    void WidgetLayout::SetWidthWeight(float widthWeigth)
    {
        mData->weight.x = widthWeigth;
        SetDirty();
    }

    float WidgetLayout::GetWidthWeight()
    {
        return mData->weight.x;
    }

    void WidgetLayout::SetHeightWeight(float heigthWeigth)
    {
        mData->weight.y = heigthWeigth;
        SetDirty();
    }

    float WidgetLayout::GetHeightWeight()
    {
        return mData->weight.y;
    }

    WidgetLayout WidgetLayout::BothStretch(float borderLeft, float borderBottom, float borderRight, float borderTop)
    {
        WidgetLayout res;
        res.mData->anchorMin = Vec2F(0, 0);
        res.mData->anchorMax = Vec2F(1, 1);
        res.mData->offsetMin = Vec2F(borderLeft, borderBottom);
        res.mData->offsetMax = Vec2F(-borderRight, -borderTop);
        return res;
    }

    WidgetLayout WidgetLayout::Based(BaseCorner corner, const Vec2F& size, const Vec2F& offset)
    {
        WidgetLayout res;
        switch (corner)
        {
        case BaseCorner::Left:
        res.mData->anchorMin = Vec2F(0.0f, 0.5f);
        res.mData->anchorMax = Vec2F(0.0f, 0.5f);
        res.mData->offsetMin = Vec2F(0.0f, -size.y*0.5f) + offset;
        res.mData->offsetMax = Vec2F(size.x, size.y*0.5f) + offset;
        break;

        case BaseCorner::Right:
        res.mData->anchorMin = Vec2F(1.0f, 0.5f);
        res.mData->anchorMax = Vec2F(1.0f, 0.5f);
        res.mData->offsetMin = Vec2F(-size.x, -size.y*0.5f) + offset;
        res.mData->offsetMax = Vec2F(0.0f, size.y*0.5f) + offset;
        break;
        case BaseCorner::Top:
        res.mData->anchorMin = Vec2F(0.5f, 1.0f);
        res.mData->anchorMax = Vec2F(0.5f, 1.0f);
        res.mData->offsetMin = Vec2F(-size.x*0.5f, -size.y) + offset;
        res.mData->offsetMax = Vec2F(size.x*0.5f, 0.0f) + offset;
        break;

        case BaseCorner::Bottom:
        res.mData->anchorMin = Vec2F(0.5f, 0.0f);
        res.mData->anchorMax = Vec2F(0.5f, 0.0f);
        res.mData->offsetMin = Vec2F(-size.x*0.5f, 0.0f) + offset;
        res.mData->offsetMax = Vec2F(size.x*0.5f, size.y) + offset;
        break;

        case BaseCorner::Center:
        res.mData->anchorMin = Vec2F(0.5f, 0.5f);
        res.mData->anchorMax = Vec2F(0.5f, 0.5f);
        res.mData->offsetMin = Vec2F(-size.x*0.5f, -size.y*0.5f) + offset;
        res.mData->offsetMax = Vec2F(size.x*0.5f, size.y*0.5f) + offset;
        break;

        case BaseCorner::LeftBottom:
        res.mData->anchorMin = Vec2F(0.0f, 0.0f);
        res.mData->anchorMax = Vec2F(0.0f, 0.0f);
        res.mData->offsetMin = Vec2F(0.0f, 0.0f) + offset;
        res.mData->offsetMax = Vec2F(size.x, size.y) + offset;
        break;

        case BaseCorner::LeftTop:
        res.mData->anchorMin = Vec2F(0.0f, 1.0f);
        res.mData->anchorMax = Vec2F(0.0f, 1.0f);
        res.mData->offsetMin = Vec2F(0.0f, -size.y) + offset;
        res.mData->offsetMax = Vec2F(size.x, 0.0f) + offset;
        break;

        case BaseCorner::RightBottom:
        res.mData->anchorMin = Vec2F(1.0f, 0.0f);
        res.mData->anchorMax = Vec2F(1.0f, 0.0f);
        res.mData->offsetMin = Vec2F(-size.x, 0.0f) + offset;
        res.mData->offsetMax = Vec2F(0.0f, size.y) + offset;
        break;

        case BaseCorner::RightTop:
        res.mData->anchorMin = Vec2F(1.0f, 1.0f);
        res.mData->anchorMax = Vec2F(1.0f, 1.0f);
        res.mData->offsetMin = Vec2F(-size.x, -size.y) + offset;
        res.mData->offsetMax = Vec2F(0.0f, 0.0f) + offset;
        break;
        }

        return res;
    }

    WidgetLayout WidgetLayout::VerStretch(HorAlign align, float top, float bottom, float width, float offsX)
    {
        WidgetLayout res;
        res.mData->anchorMin.y = 0.0f;
        res.mData->anchorMax.y = 1.0f;
        res.mData->offsetMin.y = bottom;
        res.mData->offsetMax.y = -top;

        switch (align)
        {
        case HorAlign::Left:
        res.mData->anchorMin.x = 0.0f;
        res.mData->anchorMax.x = 0.0f;
        res.mData->offsetMin.x = offsX;
        res.mData->offsetMax.x = offsX + width;
        break;

        case HorAlign::Middle:
        res.mData->anchorMin.x = 0.5f;
        res.mData->anchorMax.x = 0.5f;
        res.mData->offsetMin.x = offsX - width*0.5f;
        res.mData->offsetMax.x = offsX + width*0.5f;
        break;

        case HorAlign::Right:
        res.mData->anchorMin.x = 1.0f;
        res.mData->anchorMax.x = 1.0f;
        res.mData->offsetMin.x = -offsX - width;
        res.mData->offsetMax.x = -offsX;
        break;

        default:
        break;
        }

        return res;
    }

    WidgetLayout WidgetLayout::HorStretch(VerAlign align, float left, float right, float height, float offsY)
    {
        WidgetLayout res;
        res.mData->anchorMin.x = 0.0f;
        res.mData->anchorMax.x = 1.0f;
        res.mData->offsetMin.x = left;
        res.mData->offsetMax.x = -right;

        switch (align)
        {
        case VerAlign::Top:
        res.mData->anchorMin.y = 1.0f;
        res.mData->anchorMax.y = 1.0f;
        res.mData->offsetMin.y = -offsY - height;
        res.mData->offsetMax.y = -offsY;
        break;

        case VerAlign::Middle:
        res.mData->anchorMin.y = 0.5f;
        res.mData->anchorMax.y = 0.5f;
        res.mData->offsetMin.y = offsY - height*0.5f;
        res.mData->offsetMax.y = offsY + height*0.5f;
        break;

        case VerAlign::Bottom:
        res.mData->anchorMin.y = 0.0f;
        res.mData->anchorMax.y = 0.0f;
        res.mData->offsetMin.y = offsY;
        res.mData->offsetMax.y = offsY + height;
        break;

        case VerAlign::Both:
        break;
        }

        return res;
    }

    void WidgetLayout::SetOwner(const Ref<Actor>& actor)
    {
        ActorTransform::SetOwner(actor);
        mData->owner = (Widget*)actor.Get();
        SetDirty();
    }

    void WidgetLayout::SetDirty(bool fromParent /*= false*/)
    {
        if (!fromParent && mData->drivenByParent && mData->owner)
        {
            if (auto parent = mData->owner->mParent.Lock())
                parent->transform->SetDirty(fromParent);
        }

        ActorTransform::SetDirty(fromParent);
    }

    RectF WidgetLayout::GetParentRectangle() const
    {
        if (auto parentWidget = mData->owner->mParentWidget.Lock())
            return parentWidget->GetLayoutData().childrenWorldRect;
        else if (auto parent = mData->owner->mParent.Lock())
            return parent->transform->mData->worldRectangle;

        return RectF();
    }

    void WidgetLayout::Update()
    {
        RectF parentWorldRect;
        Vec2F parentWorldPosition;

        if (auto parentWidget = mData->owner->mParentWidget.Lock())
        {
            parentWorldRect = parentWidget->GetLayoutData().childrenWorldRect;

            RectF notWidgetWorldRect = parentWidget->transform->mData->worldRectangle;
            parentWorldPosition = notWidgetWorldRect.LeftBottom() +
                parentWidget->transform->mData->pivot*notWidgetWorldRect.Size();
        }
        else if (auto parent = mData->owner->mParent.Lock())
        {
            parentWorldRect = parent->transform->mData->worldRectangle;

            parentWorldPosition = parentWorldRect.LeftBottom() +
                parent->transform->mData->pivot*parentWorldRect.Size();
        }

        RectF worldRectangle(parentWorldRect.LeftBottom() + mData->offsetMin + mData->anchorMin*parentWorldRect.Size(),
                             parentWorldRect.LeftBottom() + mData->offsetMax + mData->anchorMax*parentWorldRect.Size());

        mData->size = worldRectangle.Size();
        mData->position = worldRectangle.LeftBottom() - parentWorldPosition + mData->size*mData->pivot;

        (this->*mCheckMinMaxFunc)();

        FloorRectangle();
        UpdateRectangle();
        UpdateTransform();
        UpdateWorldRectangleAndTransform();

        mData->updateFrame = mData->dirtyFrame;

        if (mData->owner)
        {
            mData->owner->SetChildrenWorldRect(mData->worldRectangle);
            mData->owner->OnTransformUpdated();
        }
    }

    void WidgetLayout::FloorRectangle()
    {
        mData->size.x = Math::Round(mData->size.x);
        mData->size.y = Math::Round(mData->size.y);
        mData->position.x = Math::Round(mData->position.x);
        mData->position.y = Math::Round(mData->position.y);
    }

    void WidgetLayout::UpdateOffsetsByCurrentTransform()
    {
        Vec2F offs;

        if (auto parentWidget = mData->owner->mParentWidget.Lock())
        {
            offs = parentWidget->GetLayoutData().childrenWorldRect.LeftBottom() -
                parentWidget->GetLayoutData().worldRectangle.LeftBottom();
        }

        SetRect(ActorTransform::GetRect() - offs);
    }

    void WidgetLayout::CopyFrom(const ActorTransform& other)
    {
        const WidgetLayout* otherLayout = dynamic_cast<const WidgetLayout*>(&other);

        if (otherLayout)
        {
            mData->anchorMin = otherLayout->mData->anchorMin;
            mData->anchorMax = otherLayout->mData->anchorMax;
            mData->offsetMin = otherLayout->mData->offsetMin;
            mData->offsetMax = otherLayout->mData->offsetMax;
            mData->minSize = otherLayout->mData->minSize;
            mData->maxSize = otherLayout->mData->maxSize;
            mData->weight = otherLayout->mData->weight;

            mCheckMinMaxFunc = otherLayout->mCheckMinMaxFunc;
        }

        ActorTransform::CopyFrom(other);
    }

    void WidgetLayout::CheckMinMax()
    {
        Vec2F resSize = mData->size;
        Vec2F minSizeWithChildren(mData->owner->GetMinWidthWithChildren(), mData->owner->GetMinHeightWithChildren());

        Vec2F clampSize(Math::Clamp(resSize.x, minSizeWithChildren.x, mData->maxSize.x),
                        Math::Clamp(resSize.y, minSizeWithChildren.y, mData->maxSize.y));

        Vec2F szDelta = clampSize - resSize;

        if (szDelta != Vec2F())
            mData->size += szDelta;
    }

    void WidgetLayout::DontCheckMinMax()
    {}

#if IS_SCRIPTING_SUPPORTED
    void WidgetLayout::Set(const WidgetLayout& other)
    {
        *this = other;
    }
#endif

    Vector<float> CalculateExpandedSize(Vector<Ref<Widget>>& widgets, bool horizontal, float availableWidth, float spacing)
    {
        Vector<float> minSizes; minSizes.Reserve(widgets.Count());
        Vector<float> maxSizes; maxSizes.Reserve(widgets.Count());
        Vector<float> weights; weights.Reserve(widgets.Count());

        float minSizesSum = 0;
        float weightsSum = 0;

        for (auto itChild = widgets.begin(); itChild != widgets.end();)
        {
            if (!(*itChild)->IsEnabledInHierarchy())
                itChild = widgets.erase(itChild);
            else
            {
                auto child = *itChild;

                float minWidth = horizontal ? child->layout->GetMinWidth() : child->layout->GetMinHeight();
                float maxWidth = horizontal ? child->layout->GetMaxWidth() : child->layout->GetMaxHeight();
                float weight = horizontal ? child->layout->GetWidthWeight() : child->layout->GetHeightWeight();

                minSizesSum += minWidth;

                if (minWidth < maxWidth)
                    weightsSum += weight;

                minSizes.Add(minWidth);
                maxSizes.Add(maxWidth);
                weights.Add(weight);

                ++itChild;
            }
        }

    Vector<float> widths = minSizes;

    int childCount = widgets.Count();

    availableWidth -= (spacing*(float)Math::Max(0, widgets.Count() - 1));
    float expandWidth = availableWidth - minSizesSum;
    while (expandWidth > 0)
    {
        float currentExpand = expandWidth;
        float invWeightsSum = 1.0f/weightsSum;

        for (int i = 0; i < childCount; i++)
        {
            if (widths[i] < maxSizes[i])
            {
                float expand = currentExpand*weights[i]*invWeightsSum;
                float maxExpand = maxSizes[i] - widths[i];

                if (expand > maxExpand)
                {
                    float coef = maxExpand/expand;
                    currentExpand *= coef;
                }
            }
        }

        for (int i = 0; i < childCount; i++)
        {
            if (widths[i] < maxSizes[i])
            {
                widths[i] += currentExpand*weights[i]*invWeightsSum;

                if (widths[i] >= maxSizes[i])
                    weightsSum -= weights[i];
            }
        }

        expandWidth -= currentExpand;
    }

    return widths;
}

    bool WidgetLayoutData::IsSerializeEnabled() const
    {
        return false;
    }

}
// --- META ---

DECLARE_CLASS(o2::WidgetLayout, o2__WidgetLayout);

DECLARE_CLASS(o2::WidgetLayoutData, o2__WidgetLayoutData);
// --- END META ---
