#include "o2/stdafx.h"
#include "Spoiler.h"

#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"

namespace o2
{
    Spoiler::Spoiler(RefCounter* refCounter):
        VerticalLayout(refCounter)
    {
        mBaseCorner = BaseCorner::Top;
        mFitByChildren = true;
        mExpandWidth = true;
        mExpandHeight = false;

        CreateExpandAnimation();
        mExpandState->GetAnimationPlayer()->onUpdate = THIS_FUNC(UpdateExpanding);
        mExpandState->SetState(false);
        UpdateExpanding(0);
    }

    Spoiler::Spoiler(RefCounter* refCounter, const Spoiler& other):
        VerticalLayout(refCounter, other), mHeadHeight(other.mHeadHeight)
    {
        mExpandState = GetStateObject("expand");
        if (!mExpandState)
            CreateExpandAnimation();

        mExpandState->GetAnimationPlayer()->onUpdate = THIS_FUNC(UpdateExpanding);
        mExpandState->SetState(false);

        InitializeControls();
        RetargetStatesAnimations();
        UpdateExpanding(0);
    }

    Spoiler& Spoiler::operator=(const Spoiler& other)
    {
        VerticalLayout::operator=(other);

        mExpandState = GetStateObject("expand");
        if (!mExpandState)
            CreateExpandAnimation();

        mExpandState->GetAnimationPlayer()->onUpdate = THIS_FUNC(UpdateExpanding);
        mExpandState->SetState(false);

        mHeadHeight = other.mHeadHeight;

        InitializeControls();
        UpdateExpanding(0);

        return *this;
    }

    void Spoiler::Expand(bool forcible /*= false*/)
    {
        SetExpanded(true, forcible);
    }

    void Spoiler::Collapse(bool forcible /*= false*/)
    {
        SetExpanded(false, forcible);
    }

    void Spoiler::SetExpanded(bool expand, bool forcible /*= false*/)
    {
        if (mExpandState)
        {
            if (forcible)
                mExpandState->SetStateForcible(expand);
            else
                mExpandState->SetState(expand);
        }

        if (expand)
            onExpand();

        if (auto expandBtn = GetExpandButton())
        {
			if (forcible)
				expandBtn->SetStateForcible("expanded", expand);
			else
                expandBtn->SetState("expanded", expand);
        }

        mTargetHeight = VerticalLayout::GetMinHeightWithChildren();
    }

    bool Spoiler::IsExpanded() const
    {
        return mExpandState ? mExpandState->GetState() : false;
    }

#undef DrawText
    void Spoiler::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        OnDrawn();

        if (!IsFullyCollapsed())
        {
            bool clipping = !IsFullyExpanded();
            if (clipping)
                o2Render.EnableScissorTest(mBounds);

            for (auto& child : mChildrenInheritedDepth)
                child->Draw();

            if (clipping)
                o2Render.DisableScissorTest();
        }

        for (auto& child : mInternalWidgets)
            child->Draw();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        DrawDebugFrame();
    }

    void Spoiler::SetCaption(const WString& caption)
    {
        auto textLayer = GetLayerDrawable<Text>("caption");
        if (textLayer)
            textLayer->text = caption;
    }

    const WString& Spoiler::GetCaption() const
    {
        auto textLayer = GetLayerDrawable<Text>("caption");
        if (textLayer)
            return textLayer->GetText();

        return WString::empty;
    }

    void Spoiler::SetHeadHeight(float height)
    {
        mHeadHeight = height;
    }

    float Spoiler::GetHeadHeight() const
    {
        return mHeadHeight;
    }

    void Spoiler::RearrangeChilds()
    {
        float borderTop = mBorder.top;
        mBorder.top = borderTop + mHeadHeight;

        VerticalLayout::RearrangeChilds();

        mBorder.top = borderTop;
    }

    void Spoiler::UpdateExpanding(float dt)
    {
        layout->SetDirty(false);
    }

    void Spoiler::CreateExpandAnimation()
    {
        mExpandState = AddState("expand", AnimationClip::Parametric("mExpandCoef", 0.0f, 1.0f, 0.4f, 0.0f, 0.4f, 1.0f, 1.0f));
    }

    float Spoiler::GetMinHeightWithChildren() const
    {
        if (!mFitByChildren)
            return Widget::GetMinHeightWithChildren();

        float res = Math::Max(mChildWidgets.Count() - 1, 0)*mSpacing + mBorder.top + mBorder.bottom;
        for (auto& child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
                res += child->GetMinHeightWithChildren();
        }

        res = res*Math::Clamp01(mExpandCoef) + mHeadHeight;
        res = Math::Max(res, GetLayoutData().minSize.y);

        return res;
    }

    void Spoiler::UpdateLayoutParametres()
    {
        if (IsFullyExpanded())
            VerticalLayout::UpdateLayoutParametres();
        else
        {
            GetLayoutData().weight.y = 1;
            GetLayoutData().minSize.y = 0;
        }
    }

    void Spoiler::InitializeControls()
    {
        auto textLayer = GetLayerDrawable<Text>("caption");
        auto expandBtn = GetExpandButton();

        if (expandBtn)
        {
            expandBtn->onClick = [&]() { SetExpanded(!IsExpanded()); };
            expandBtn->isPointInside = [=](const Vec2F& point) {
                return expandBtn->layout->IsPointInside(point) || (textLayer && textLayer->IsPointInside(point));
            };
        }
    }

    Ref<Button> Spoiler::GetExpandButton() const
    {
        return FindInternalWidgetByType<Button>("expand");
    }

    bool Spoiler::IsFullyExpanded() const
    {
        if (!mExpandState)
            return true;

        return mExpandState->GetState() && !mExpandState->GetAnimationPlayer()->IsPlaying();
    }

    bool Spoiler::IsFullyCollapsed() const
    {
        if (!mExpandState)
            return false;

        return !mExpandState->GetState() && !mExpandState->GetAnimationPlayer()->IsPlaying();
    }

    String Spoiler::GetCreateMenuGroup()
    {
        return "Dropping";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Spoiler>);
// --- META ---

DECLARE_CLASS(o2::Spoiler, o2__Spoiler);
// --- END META ---
