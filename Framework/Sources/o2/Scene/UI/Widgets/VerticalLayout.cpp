#include "o2/stdafx.h"
#include "VerticalLayout.h"

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"

namespace o2
{
    VerticalLayout::VerticalLayout(RefCounter* refCounter):
        Widget(refCounter)
    {
        SetLayoutDirty();
    }

    VerticalLayout::VerticalLayout(RefCounter* refCounter, const VerticalLayout& other):
        Widget(refCounter, other), mBaseCorner(other.mBaseCorner), mSpacing(other.mSpacing), mBorder(other.mBorder), mExpandWidth(other.mExpandWidth),
        mExpandHeight(other.mExpandHeight), mFitByChildren(other.mFitByChildren)
    {
        for (auto& child : mChildWidgets)
            child->GetLayoutData().drivenByParent = true;

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    VerticalLayout::~VerticalLayout()
    {
        mFitByChildren = false;
    }

    VerticalLayout& VerticalLayout::operator=(const VerticalLayout& other)
    {
        mBaseCorner = other.mBaseCorner;
        mSpacing = other.mSpacing;
        mBorder = other.mBorder;
        mExpandWidth = other.mExpandWidth;
        mExpandHeight = other.mExpandHeight;
        mFitByChildren = other.mFitByChildren;

        Widget::operator=(other);

        for (auto& child : mChildWidgets)
            child->GetLayoutData().drivenByParent = true;

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void VerticalLayout::SetBaseCorner(BaseCorner baseCorner)
    {
        mBaseCorner = baseCorner;
        layout->SetDirty();
    }

    BaseCorner VerticalLayout::GetBaseCorner() const
    {
        return mBaseCorner;
    }

    void VerticalLayout::SetSpacing(float spacing)
    {
        mSpacing = spacing;
        layout->SetDirty();
    }

    float VerticalLayout::GetSpacing() const
    {
        return mSpacing;
    }

    void VerticalLayout::SetBorder(const BorderF& border)
    {
        mBorder = border;
        layout->SetDirty();
    }

    BorderF VerticalLayout::GetBorder() const
    {
        return mBorder;
    }

    void VerticalLayout::SetBorderLeft(float value)
    {
        mBorder.left = value;
        layout->SetDirty();
    }

    float VerticalLayout::GetBorderLeft() const
    {
        return mBorder.left;
    }

    void VerticalLayout::SetBorderRight(float value)
    {
        mBorder.right = value;
        layout->SetDirty();
    }

    float VerticalLayout::GetBorderRight() const
    {
        return mBorder.right;
    }

    void VerticalLayout::SetBorderTop(float value)
    {
        mBorder.top = value;
        layout->SetDirty();
    }

    float VerticalLayout::GetBorderTop() const
    {
        return mBorder.top;
    }

    void VerticalLayout::SetBorderBottom(float value)
    {
        mBorder.bottom = value;
        layout->SetDirty();
    }

    float VerticalLayout::GetBorderBottom() const
    {
        return mBorder.bottom;
    }

    void VerticalLayout::SetWidthExpand(bool expand)
    {
        mExpandWidth = expand;
        layout->SetDirty();
    }

    bool VerticalLayout::IsWidthExpand() const
    {
        return mExpandWidth;
    }

    void VerticalLayout::SetHeightExpand(bool expand)
    {
        mExpandHeight = expand;
        layout->SetDirty();
    }

    bool VerticalLayout::IsHeightExpand() const
    {
        return mExpandHeight;
    }

    void VerticalLayout::SetFitByChildren(bool fit)
    {
        mFitByChildren = fit;
        layout->SetDirty();
    }

    bool VerticalLayout::IsFittingByChildren() const
    {
        return mFitByChildren;
    }

    void VerticalLayout::UpdateSelfTransform()
{
        UpdateLayoutParametres();

        if (mFitByChildren)
            ExpandSizeByChilds();

        Widget::UpdateSelfTransform();

        RearrangeChilds();
    }

    String VerticalLayout::GetCreateMenuGroup()
    {
        return "Layout";
    }

    float VerticalLayout::GetMinWidthWithChildren() const
    {
        if (!mFitByChildren)
            return Widget::GetMinWidthWithChildren();

        float res = 0;
        for (auto& child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                res = Math::Max(res, child->GetMinWidthWithChildren() + mBorder.left + mBorder.right);
        }

        res = Math::Max(res, GetLayoutData().minSize.x);

        return res;
    }

    float VerticalLayout::GetMinHeightWithChildren() const
    {
        if (!mFitByChildren)
            return Widget::GetMinHeightWithChildren();

        float res = mBorder.top + mBorder.bottom + Math::Max(mChildWidgets.Count() - 1, 0)*mSpacing;
        for (auto& child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                res += child->GetMinHeightWithChildren();
        }

        res = Math::Max(res, GetLayoutData().minSize.y);

        return res;
    }

    float VerticalLayout::GetHeightWeightWithChildren() const
    {
        float res = 0;
        for (auto& child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                res += child->GetHeightWeightWithChildren();
        }

        return res;
    }

    void VerticalLayout::OnChildAdded(const Ref<Widget>& child)
    {
        child->GetLayoutData().drivenByParent = true;
    }

    void VerticalLayout::OnChildRemoved(const Ref<Widget>& child)
    {
        child->GetLayoutData().drivenByParent = false;
    }

    void VerticalLayout::RearrangeChilds()
    {
        switch (mBaseCorner)
        {
            case BaseCorner::LeftTop:
            case BaseCorner::Top:
            case BaseCorner::RightTop:
            ArrangeFromTopToBottom();
            break;

            case BaseCorner::Left:
            case BaseCorner::Center:
            case BaseCorner::Right:
            ArrangeFromCenter();
            break;

            case BaseCorner::LeftBottom:
            case BaseCorner::Bottom:
            case BaseCorner::RightBottom:
            ArrangeFromBottomToTop();
            break;
        }

    }

    void VerticalLayout::UpdateLayoutParametres()
    {
        GetLayoutData().weight.y = 0;
        for (auto& child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                GetLayoutData().weight.y += child->GetHeightWeightWithChildren();
        }

        if (GetLayoutData().weight.y < FLT_EPSILON)
            GetLayoutData().weight.y = 1.0f;

        layout->EnableSizeChecks();
    }

    void VerticalLayout::ArrangeFromCenter()
    {
        if (mExpandHeight)
        {
            float availableHeight = GetChildrenWorldRect().Height() - mBorder.bottom - mBorder.top;
            float totalHeight = availableHeight;
            float position = -totalHeight*0.5f;
            auto widgets = mChildWidgets;
            auto heights = CalculateExpandedSize(widgets, false, GetChildrenWorldRect().Height() - mBorder.top - mBorder.bottom,
                                                 mSpacing);

            int i = 0;
            for (auto& child : widgets)
            {
                child->GetLayoutData().offsetMin.y = position;
                position += heights[i++];

                child->GetLayoutData().offsetMax.y = position;
                position += mSpacing;

                AlignWidgetByWidth(child, 0.5f);
            }
        }
        else
        {
            float totalHeight = mChildWidgets.Sum<float>([&](auto& child) { return child->GetMinHeightWithChildren(); });
            totalHeight += (mChildWidgets.Count() - 1)*mSpacing;
            float position = -totalHeight*0.5f;
            for (auto& child : mChildWidgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMin.y = position;
                position += Math::Abs(Math::Max(child->GetLayoutData().minSize.y, child->GetMinHeightWithChildren()));

                child->GetLayoutData().offsetMax.y = position;
                position += mSpacing;

                AlignWidgetByWidth(child, 0.5f);
            }
        }
    }

    void VerticalLayout::ArrangeFromBottomToTop()
    {
        if (mExpandHeight)
        {
            float position = mBorder.bottom;
            auto widgets = mChildWidgets;
            auto heights = CalculateExpandedSize(widgets, false, GetChildrenWorldRect().Height() - mBorder.top - mBorder.bottom,
                                                 mSpacing);

            int i = 0;
            for (auto& child : widgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMin.y = position;
                position += heights[i++];

                child->GetLayoutData().offsetMax.y = position;
                position += mSpacing;

                AlignWidgetByWidth(child, 0.0f);
            }
        }
        else
        {
            float position = mBorder.bottom;
            for (auto& child : mChildWidgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMin.y = position;
                position += Math::Abs(Math::Max(child->GetLayoutData().minSize.y, child->GetMinHeightWithChildren()));

                child->GetLayoutData().offsetMax.y = position;
                position += mSpacing;

                AlignWidgetByWidth(child, 0.0f);
            }
        }
    }

    void VerticalLayout::ArrangeFromTopToBottom()
    {
        if (mExpandHeight)
        {
            float position = mBorder.top;
            auto widgets = mChildWidgets;
            auto heights = CalculateExpandedSize(widgets, false, GetChildrenWorldRect().Height() - mBorder.top - mBorder.bottom,
                                                 mSpacing);

            int i = 0;
            for (auto& child : widgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMax.y = -position;
                position += heights[i++];

                child->GetLayoutData().offsetMin.y = -position;
                position += mSpacing;

                AlignWidgetByWidth(child, 1.0f);
            }
        }
        else
        {
            float position = mBorder.top;
            for (auto& child : mChildWidgets)
            {
                if (!child->mResEnabledInHierarchy)
                    continue;

                child->GetLayoutData().offsetMax.y = -position;
                position += Math::Abs(Math::Max(child->GetLayoutData().minSize.y, child->GetMinHeightWithChildren()));

                child->GetLayoutData().offsetMin.y = -position;
                position += mSpacing;

                AlignWidgetByWidth(child, 1.0f);
            }
        }
    }

    void VerticalLayout::AlignWidgetByWidth(const Ref<Widget>& child, float heightAnchor)
    {
        if (mExpandWidth)
        {
            child->GetLayoutData().anchorMin = Vec2F(0, heightAnchor);
            child->GetLayoutData().anchorMax = Vec2F(1, heightAnchor);
            child->GetLayoutData().offsetMin.x = mBorder.left;
            child->GetLayoutData().offsetMax.x = -mBorder.right;
        }
        else
        {
            float width = child->layout->GetWidth();
            if (mBaseCorner == BaseCorner::LeftBottom || mBaseCorner == BaseCorner::Left || mBaseCorner == BaseCorner::LeftTop)
            {
                child->GetLayoutData().anchorMin = Vec2F(0, heightAnchor);
                child->GetLayoutData().anchorMax = Vec2F(0, heightAnchor);
                child->GetLayoutData().offsetMin.x = mBorder.left;
                child->GetLayoutData().offsetMax.x = mBorder.left + width;
            }
            if (mBaseCorner == BaseCorner::Bottom || mBaseCorner == BaseCorner::Center || mBaseCorner == BaseCorner::Top)
            {
                child->GetLayoutData().anchorMin = Vec2F(0.5f, heightAnchor);
                child->GetLayoutData().anchorMax = Vec2F(0.5f, heightAnchor);
                child->GetLayoutData().offsetMin.x = -width*0.5f;
                child->GetLayoutData().offsetMax.x = width*0.5f;
            }
            if (mBaseCorner == BaseCorner::RightBottom || mBaseCorner == BaseCorner::Right || mBaseCorner == BaseCorner::RightTop)
            {
                child->GetLayoutData().anchorMin = Vec2F(1, heightAnchor);
                child->GetLayoutData().anchorMax = Vec2F(1, heightAnchor);
                child->GetLayoutData().offsetMin.x = -mBorder.right - width;
                child->GetLayoutData().offsetMax.x = -mBorder.right;
            }
        }
    }

    void VerticalLayout::ExpandSizeByChilds()
    {
        const static Vec2F relativePivots[] ={
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

        Vec2F parentSize = mParentWidget ? mParentWidget.Lock()->GetChildrenWorldRect().Size() : Vec2F();
        Vec2F szDelta = size - (GetLayoutData().offsetMax - GetLayoutData().offsetMin + (GetLayoutData().anchorMax - GetLayoutData().anchorMin)*parentSize);

        if (mExpandWidth)
            szDelta.x = 0;

        if (mExpandHeight)
            szDelta.y = 0;

        GetLayoutData().offsetMax += szDelta*(Vec2F::One() - relativePivot);
        GetLayoutData().offsetMin -= szDelta*relativePivot;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::VerticalLayout>);
// --- META ---

DECLARE_CLASS(o2::VerticalLayout, o2__VerticalLayout);
// --- END META ---
