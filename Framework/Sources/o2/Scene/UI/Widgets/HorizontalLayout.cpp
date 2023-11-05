#include "o2/stdafx.h"
#include "HorizontalLayout.h"

#include "o2/Scene/UI/WidgetLayout.h"

namespace o2
{
    HorizontalLayout::HorizontalLayout(): Widget()
    {
        SetLayoutDirty();
    }

    HorizontalLayout::HorizontalLayout(const HorizontalLayout& other) :
        mBaseCorner(other.mBaseCorner), mSpacing(other.mSpacing), mBorder(other.mBorder), mExpandWidth(other.mExpandWidth),
        mExpandHeight(other.mExpandHeight), Widget(other), mFitByChildren(other.mFitByChildren), baseCorner(this),
        spacing(this), border(this), borderLeft(this), borderRight(this), expandWidth(this), expandHeight(this),
        borderTop(this), borderBottom(this), fitByChildren(this)
    {
        for (auto child : mChildWidgets)
            child->GetLayoutData().drivenByParent = true;

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    HorizontalLayout::~HorizontalLayout()
    { }

    HorizontalLayout& HorizontalLayout::operator=(const HorizontalLayout& other)
    {
        Widget::operator=(other);

        mBaseCorner = other.mBaseCorner;
        mSpacing = other.mSpacing;
        mBorder = other.mBorder;
        mExpandWidth = other.mExpandWidth;
        mExpandHeight = other.mExpandHeight;

        for (auto child : mChildWidgets)
            child->GetLayoutData().drivenByParent = true;

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void HorizontalLayout::SetBaseCorner(BaseCorner baseCorner)
    {
        mBaseCorner = baseCorner;
        layout->SetDirty();
    }

    BaseCorner HorizontalLayout::GetBaseCorner() const
    {
        return mBaseCorner;
    }

    void HorizontalLayout::SetSpacing(float spacing)
    {
        mSpacing = spacing;
        layout->SetDirty();
    }

    float HorizontalLayout::GetSpacing() const
    {
        return mSpacing;
    }

    void HorizontalLayout::SetBorder(const BorderF& border)
    {
        mBorder = border;
        layout->SetDirty();
    }

    BorderF HorizontalLayout::GetBorder() const
    {
        return mBorder;
    }

    void HorizontalLayout::SetBorderLeft(float value)
    {
        mBorder.left = value;
        layout->SetDirty();
    }

    float HorizontalLayout::GetBorderLeft() const
    {
        return mBorder.left;
    }

    void HorizontalLayout::SetBorderRight(float value)
    {
        mBorder.right = value;
        layout->SetDirty();
    }

    float HorizontalLayout::GetBorderRight() const
    {
        return mBorder.right;
    }

    void HorizontalLayout::SetBorderTop(float value)
    {
        mBorder.top = value;
        layout->SetDirty();
    }

    float HorizontalLayout::GetBorderTop() const
    {
        return mBorder.top;
    }

    void HorizontalLayout::SetBorderBottom(float value)
    {
        mBorder.bottom = value;
        layout->SetDirty();
    }

    float HorizontalLayout::GetBorderBottom() const
    {
        return mBorder.bottom;
    }

    void HorizontalLayout::SetWidthExpand(bool expand)
    {
        mExpandWidth = expand;
        layout->SetDirty();
    }

    bool HorizontalLayout::IsWidthExpand() const
    {
        return mExpandWidth;
    }

    void HorizontalLayout::SetHeightExpand(bool expand)
    {
        mExpandHeight = expand;
        layout->SetDirty();
    }

    bool HorizontalLayout::IsHeightExpand() const
    {
        return mExpandHeight;
    }

    void HorizontalLayout::SetFitByChildren(bool fit)
    {
        mFitByChildren = fit;
        layout->SetDirty();
    }

    bool HorizontalLayout::IsFittingByChildren() const
    {
        return mFitByChildren;
    }

    void HorizontalLayout::UpdateSelfTransform()
    {
        UpdateLayoutParametres();

        if (mFitByChildren)
            ExpandSizeByChilds();

        Widget::UpdateSelfTransform();

        RearrangeChilds();
    }

    String HorizontalLayout::GetCreateMenuGroup()
    {
        return "Layout";
    }

    float HorizontalLayout::GetMinWidthWithChildren() const
    {
        if (!mFitByChildren)
            return Widget::GetMinWidthWithChildren();

        float res = mBorder.left + mBorder.right + Math::Max(mChildWidgets.Count() - 1, 0)*mSpacing;
        for (auto child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                res += child->GetMinWidthWithChildren();
        }

        res = Math::Max(res, GetLayoutData().minSize.x);

        return res;
    }

    float HorizontalLayout::GetMinHeightWithChildren() const
    {
        if (!mFitByChildren)
            return Widget::GetMinHeightWithChildren();

        float res = 0;
        for (auto child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                res = Math::Max(res, child->GetMinHeightWithChildren() + mBorder.top + mBorder.bottom);
        }

        res = Math::Max(res, GetLayoutData().minSize.y);

        return res;
    }

    float HorizontalLayout::GetWidthWeightWithChildren() const
    {
        float res = 0;
        for (auto child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                res += child->GetWidthWeightWithChildren();
        }

        return res;
    }

    void HorizontalLayout::OnChildAdded(Widget* child)
    {
        child->GetLayoutData().drivenByParent = true;
        Widget::OnChildAdded(child);
    }

    void HorizontalLayout::OnChildRemoved(Widget* child)
    {
        child->GetLayoutData().drivenByParent = false;
        Widget::OnChildRemoved(child);
    }

    void HorizontalLayout::RearrangeChilds()
    {
        UpdateLayoutParametres();

        switch (mBaseCorner)
        {
            case BaseCorner::LeftBottom:
            case BaseCorner::Left:
            case BaseCorner::LeftTop:
            ArrangeFromLeftToRight();
            break;

            case BaseCorner::RightBottom:
            case BaseCorner::Right:
            case BaseCorner::RightTop:
            ArrangeFromRightToLeft();
            break;

            case BaseCorner::Bottom:
            case BaseCorner::Center:
            case BaseCorner::Top:
            ArrangeFromCenter();
            break;
        }

    }

    void HorizontalLayout::UpdateLayoutParametres()
    {
        GetLayoutData().weight.x = 0;

        for (auto child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                GetLayoutData().weight.x += child->GetWidthWeightWithChildren();
        }

        if (GetLayoutData().weight.x < FLT_EPSILON)
            GetLayoutData().weight.x = 1.0f;

        layout->EnableSizeChecks();
    }

    void HorizontalLayout::ArrangeFromCenter()
    {
        if (mExpandWidth)
        {
            float availableWidth = GetChildrenWorldRect().Width() - mBorder.left - mBorder.right;
            float totalWidth = availableWidth;
            float position = -totalWidth*0.5f;
            auto widgets = mChildWidgets;
            auto widths = CalculateExpandedSize(widgets, true, GetChildrenWorldRect().Width() - mBorder.left - mBorder.right,
                mSpacing);

            int i = 0;
            for (auto child : widgets)
            {
                child->GetLayoutData().offsetMin.x = position;
                position += widths[i++];

                child->GetLayoutData().offsetMax.x = position;
                position += mSpacing;

                AlignWidgetByHeight(child, 0.5f);
            }
        }
        else
        {
            float totalWidth = mChildWidgets.Sum<float>([&](Widget* child) { return child->GetMinWidthWithChildren(); });
            totalWidth += (mChildWidgets.Count() - 1)*mSpacing;
            float position = -totalWidth*0.5f;
            for (auto child : mChildWidgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMin.x = position;
                position += Math::Abs(Math::Max(child->GetLayoutData().minSize.x, child->GetMinWidthWithChildren()));

                child->GetLayoutData().offsetMax.x = position;
                position += mSpacing;

                AlignWidgetByHeight(child, 0.5f);
            }
        }
    }

    void HorizontalLayout::ArrangeFromLeftToRight()
    {
        if (mExpandWidth)
        {
            float position = mBorder.left;
            auto widgets = mChildWidgets;
            auto widths = CalculateExpandedSize(widgets, true, GetChildrenWorldRect().Width() - mBorder.left - mBorder.right,
                mSpacing);

            int i = 0;
            for (auto child : widgets)
            {
                child->GetLayoutData().offsetMin.x = position;
                position += widths[i++];

                child->GetLayoutData().offsetMax.x = position;
                position += mSpacing;

                AlignWidgetByHeight(child, 0.0f);
            }
        }
        else
        {
            float position = mBorder.left;
            for (auto child : mChildWidgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMin.x = position;
                position += Math::Abs(Math::Max(child->GetLayoutData().minSize.x, child->GetMinWidthWithChildren()));

                child->GetLayoutData().offsetMax.x = position;
                position += mSpacing;

                AlignWidgetByHeight(child, 0.0f);
            }
        }
    }

    void HorizontalLayout::ArrangeFromRightToLeft()
    {
        if (mExpandWidth)
        {
            float position = mBorder.right;
            auto widgets = mChildWidgets;
            auto widths = CalculateExpandedSize(widgets, true, GetChildrenWorldRect().Width() - mBorder.left - mBorder.right,
                mSpacing);

            int i = 0;
            for (auto child : widgets)
            {
                child->GetLayoutData().offsetMax.x = -position;
                position += widths[i++];

                child->GetLayoutData().offsetMin.x = -position;
                position += mSpacing;

                AlignWidgetByHeight(child, 1.0f);
            }
        }
        else
        {
            float position = mBorder.right;
            for (auto child : mChildWidgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMax.x = -position;
                position += Math::Abs(Math::Max(child->GetLayoutData().minSize.x, child->GetMinWidthWithChildren()));

                child->GetLayoutData().offsetMin.x = -position;
                position += mSpacing;

                AlignWidgetByHeight(child, 1.0f);
            }
        }
    }

    void HorizontalLayout::AlignWidgetByHeight(Widget* child, float widthAnchor)
    {
        if (mExpandHeight)
        {
            child->GetLayoutData().anchorMin = Vec2F(widthAnchor, 0);
            child->GetLayoutData().anchorMax = Vec2F(widthAnchor, 1);
            child->GetLayoutData().offsetMin.y = mBorder.bottom;
            child->GetLayoutData().offsetMax.y = -mBorder.top;
        }
        else
        {
            float height = child->layout->height;
            if (mBaseCorner == BaseCorner::Bottom || mBaseCorner == BaseCorner::LeftBottom || mBaseCorner == BaseCorner::RightBottom)
            {
                child->GetLayoutData().anchorMin = Vec2F(widthAnchor, 0);
                child->GetLayoutData().anchorMax = Vec2F(widthAnchor, 0);
                child->GetLayoutData().offsetMin.y = mBorder.bottom;
                child->GetLayoutData().offsetMax.y = mBorder.bottom + height;
            }
            if (mBaseCorner == BaseCorner::Center || mBaseCorner == BaseCorner::Left || mBaseCorner == BaseCorner::Right)
            {
                child->GetLayoutData().anchorMin = Vec2F(widthAnchor, 0.5f);
                child->GetLayoutData().anchorMax = Vec2F(widthAnchor, 0.5f);
                child->GetLayoutData().offsetMin.y = -height*0.5f;
                child->GetLayoutData().offsetMax.y = height*0.5f;
            }
            if (mBaseCorner == BaseCorner::Top || mBaseCorner == BaseCorner::LeftTop || mBaseCorner == BaseCorner::RightTop)
            {
                child->GetLayoutData().anchorMin = Vec2F(widthAnchor, 1);
                child->GetLayoutData().anchorMax = Vec2F(widthAnchor, 1);
                child->GetLayoutData().offsetMin.y = -mBorder.top - height;
                child->GetLayoutData().offsetMax.y = -mBorder.top;
            }
        }
    }

    void HorizontalLayout::ExpandSizeByChilds()
    {
        const static Vec2F relativePivots[] = {
            Vec2F(0.0f, 0.5f), // Left
            Vec2F(1.0f, 0.5f), // Right
            Vec2F(0.5f, 1.0f), // Top
            Vec2F(0.5f, 0.0f), // Bottom
            Vec2F(0.5f, 0.5f), // Center
            Vec2F(0.0f, 0.0f), // LeftBottom
            Vec2F(0.0f, 1.0f), // LeftTop
            Vec2F(1.0f, 0.0f), // RightBottom
            Vec2F(1.0f, 1.0f)  // RightTop
        };

        Vec2F relativePivot = relativePivots[(int)mBaseCorner];
        Vec2F size(GetMinWidthWithChildren(), GetMinHeightWithChildren());

        Vec2F parentSize = mParent ? mParent->transform->size : Vec2F();
        Vec2F szDelta = size - (GetLayoutData().offsetMax - GetLayoutData().offsetMin + (GetLayoutData().anchorMax - GetLayoutData().anchorMin)*parentSize);

        if (mExpandWidth)
            szDelta.x = 0;

        if (mExpandHeight)
            szDelta.y = 0;

        GetLayoutData().offsetMax += szDelta*(Vec2F::One() - relativePivot);
        GetLayoutData().offsetMin -= szDelta*relativePivot;
    }
}
// --- META ---

DECLARE_CLASS(o2::HorizontalLayout);
// --- END META ---
