#include "o2/stdafx.h"
#include "ScrollArea.h"

#include "o2/Events/EventSystem.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    ScrollArea::ScrollArea(RefCounter* refCounter) :
        Widget(refCounter)
    {}

    ScrollArea::ScrollArea(RefCounter* refCounter, const ScrollArea& other) :
        Widget(refCounter, other), mOwnHorScrollBar(other.mOwnHorScrollBar), mOwnVerScrollBar(other.mOwnVerScrollBar),
        mClipAreaLayout(other.mClipAreaLayout), mScrollPos(other.mScrollPos), mScrollSpeedDamp(other.mScrollSpeedDamp),
        mViewAreaLayout(other.mViewAreaLayout), mEnableScrollsHiding(other.mEnableScrollsHiding), scroll(this),
        horScroll(this), verScroll(this)
    {
        if (mOwnHorScrollBar)
        {
            mHorScrollBar = GetInternalWidgetByType<HorizontalScrollBar>("horScrollBar");
            mHorScrollBar->GetLayoutData().drivenByParent = true;
            mHorScrollBar->onSmoothChange += THIS_FUNC(OnHorScrollChanged);
            mHorScrollBar->SetEnabledForcible(false);

            mEnableHorScroll = false;
        }
        else 
            mHorScrollBar = nullptr;

        if (mOwnVerScrollBar)
        {
            mVerScrollBar = GetInternalWidgetByType<VerticalScrollBar>("verScrollBar");
            mVerScrollBar->GetLayoutData().drivenByParent = true;
            mVerScrollBar->onSmoothChange += THIS_FUNC(OnVerScrollChanged);
            mVerScrollBar->SetEnabledForcible(false);

            mEnableVerScroll = false;
        }
        else 
            mVerScrollBar = nullptr;

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    ScrollArea::~ScrollArea()
    {
        if (mHorScrollBar)
        {
            if (!mOwnHorScrollBar)
                mHorScrollBar->onSmoothChange -= THIS_FUNC(OnHorScrollChanged);
        }

        if (mVerScrollBar)
        {
            if (!mOwnVerScrollBar)
                mVerScrollBar->onSmoothChange -= THIS_FUNC(OnVerScrollChanged);
        }
    }

    ScrollArea& ScrollArea::operator=(const ScrollArea& other)
    {
        if (mHorScrollBar)
        {
            if (!mOwnHorScrollBar)
                mHorScrollBar->onSmoothChange -= THIS_FUNC(OnHorScrollChanged);
        }

        if (mVerScrollBar)
        {
            if (!mOwnVerScrollBar)
                mVerScrollBar->onSmoothChange -= THIS_FUNC(OnVerScrollChanged);
        }

        mClipAreaLayout = other.mClipAreaLayout;
        mViewAreaLayout = other.mViewAreaLayout;
        mScrollPos = other.mScrollPos;
        mOwnHorScrollBar = other.mOwnHorScrollBar;
        mOwnVerScrollBar = other.mOwnVerScrollBar;
        mScrollSpeedDamp = other.mScrollSpeedDamp;
        mEnableScrollsHiding = other.mEnableScrollsHiding;

        Widget::operator=(other);

        if (mOwnHorScrollBar)
        {
            mHorScrollBar = GetInternalWidgetByType<HorizontalScrollBar>("horScrollBar");
            mHorScrollBar->onSmoothChange += THIS_FUNC(OnHorScrollChanged);
            mHorScrollBar->GetLayoutData().drivenByParent = true;
            mHorScrollBar->SetEnabledForcible(false);

            mEnableHorScroll = false;
        }
        else mHorScrollBar = nullptr;

        if (mOwnVerScrollBar)
        {
            mVerScrollBar = GetInternalWidgetByType<VerticalScrollBar>("verScrollBar");
            mVerScrollBar->onSmoothChange += THIS_FUNC(OnVerScrollChanged);
            mVerScrollBar->GetLayoutData().drivenByParent = true;
            mVerScrollBar->SetEnabledForcible(false);
            
            mEnableVerScroll = false;
        }
        else mVerScrollBar = nullptr;

        RetargetStatesAnimations();
        UpdateScrollParams();
        SetLayoutDirty();
        return *this;
    }

    void ScrollArea::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy || mIsClipped)
        {
            if (mIsClipped)
            {
                for (auto& child : mChildrenInheritedDepth)
                    child->Draw();
            }

            return;
        }

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        IDrawable::OnDrawn();

        o2Render.EnableScissorTest(mAbsoluteClipArea);

        for (auto& child : mChildrenInheritedDepth)
            child->Draw();

        o2Render.DisableScissorTest();

        CursorAreaEventsListener::OnDrawn();

        for (auto& child : mInternalWidgets)
            child->Draw();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        DrawDebugFrame();
    }

    void ScrollArea::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        mHasScrolled = false;

        mLayoutUpdated = layout->IsDirty();

        Widget::Update(dt);

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        CheckScrollBarsVisibility();

        if (!mPressedCursor && mScrollSpeed != Vec2F())
        {
            SetScroll(mScrollPos + mScrollSpeed*dt);

            if (Math::Abs(mScrollSpeed.x) > FLT_EPSILON)
                mScrollSpeed.x -= Math::Sign(mScrollSpeed.x)*
                Math::Min(Math::Abs(mScrollSpeed.x), Math::Max(Math::Abs(mScrollSpeed.x)*dt*mScrollSpeedDamp, 0.05f));

            if (Math::Abs(mScrollSpeed.y) > FLT_EPSILON)
                mScrollSpeed.y -= Math::Sign(mScrollSpeed.y)*
                Math::Min(Math::Abs(mScrollSpeed.y), Math::Max(Math::Abs(mScrollSpeed.y)*dt*mScrollSpeedDamp, 0.05f));
        }

        if (mEnableScrollsHiding)
        {
            const float barsHideDelay = 2;
            float curTime = o2Time.GetApplicationTime();

            if (curTime - mLastHorScrollChangeTime > barsHideDelay && mHorScrollBar && mHorScrollBar->IsEnabled() &&
                mEnableHorScroll)
            {
                auto enableHorBarState = state["enableHorBar"];
                if (enableHorBarState)
                    *enableHorBarState = false;

                mHorScrollBar->Hide();
            }

            if (curTime - mLastVerScrollChangeTime > barsHideDelay && mVerScrollBar && mVerScrollBar->IsEnabled() &&
                mEnableVerScroll)
            {
                auto enableVerBarState = state["enableVerBar"];
                if (enableVerBarState)
                    *enableVerBarState = false;

                mVerScrollBar->Hide();
            }
        }
    }

    void ScrollArea::UpdateChildren(float dt)
    {
        Widget::UpdateChildren(dt);

        if (mLayoutUpdated && !!mResEnabledInHierarchy)
        {
            CheckChildrenClipping();
            UpdateScrollParams();
        }
    }

    void ScrollArea::SetScroll(const Vec2F& scroll)
    {
        Vec2F newScrollPos(Math::Clamp(scroll.x, mScrollRange.left, mScrollRange.right),
                           Math::Clamp(scroll.y, mScrollRange.bottom, mScrollRange.top));

        Vec2F scrollDelta;

        if (mHorScrollBar)
            mHorScrollBar->SetValue(newScrollPos.x);
        else
            scrollDelta.x = newScrollPos.x - mScrollPos.x;

        if (mVerScrollBar)
            mVerScrollBar->SetValue(newScrollPos.y);
        else
            scrollDelta.y = newScrollPos.y - mScrollPos.y;

        if (!mVerScrollBar || !mHorScrollBar)
            MoveScrollPosition(scrollDelta);

        onScrolled(newScrollPos);
        OnScrolled();
    }

    void ScrollArea::SetScrollForcible(const Vec2F& scroll)
    {
        Vec2F newScrollPos(Math::Clamp(scroll.x, mScrollRange.left, mScrollRange.right),
                           Math::Clamp(scroll.y, mScrollRange.bottom, mScrollRange.top));

        Vec2F scrollDelta;

        if (mHorScrollBar)
            mHorScrollBar->SetValueForcible(newScrollPos.x);
        else
            scrollDelta.x = newScrollPos.x - mScrollPos.x;

        if (mVerScrollBar)
            mVerScrollBar->SetValueForcible(newScrollPos.y);
        else
            scrollDelta.y = newScrollPos.y - mScrollPos.y;

        if (!mVerScrollBar || !mHorScrollBar)
            MoveScrollPosition(scrollDelta);

        onScrolled(newScrollPos);
        OnScrolled();
    }

    Vec2F ScrollArea::GetScroll() const
    {
        return mScrollPos;
    }

    RectF ScrollArea::GetScrollRange() const
    {
        return mScrollRange;
    }

    void ScrollArea::ResetScroll()
    {
        SetScrollForcible(mScrollArea.LeftBottom());
    }

    void ScrollArea::SetHorizontalScroll(float scroll)
    {
        SetScroll(Vec2F(scroll, mScrollPos.y));
    }

    float ScrollArea::GetHorizontalScroll() const
    {
        return mScrollPos.x;
    }

    void ScrollArea::SetVerticalScroll(float scroll)
    {
        SetScroll(Vec2F(mScrollPos.x, scroll));
    }

    float ScrollArea::GetVerticalScroll() const
    {
        return mScrollPos.y;
    }

    void ScrollArea::SetHorizontalScrollBar(const Ref<HorizontalScrollBar>& scrollbar, bool owner /*= true*/)
    {
        if (mHorScrollBar)
        {
            if (!mOwnHorScrollBar)
                mHorScrollBar->onSmoothChange -= THIS_FUNC(OnHorScrollChanged);
        }

        mHorScrollBar = scrollbar;
        mOwnHorScrollBar = owner;

        if (mHorScrollBar)
        {
            mHorScrollBar->name = "horScrollBar";
            mHorScrollBar->SetInternalParent(Ref(this), false);
            mHorScrollBar->GetLayoutData().drivenByParent = true;
            mHorScrollBar->onSmoothChange += THIS_FUNC(OnHorScrollChanged);
        }

        UpdateScrollParams();
        SetLayoutDirty();
    }

    const Ref<HorizontalScrollBar>& ScrollArea::GetHorizontalScrollbar() const
    {
        return mHorScrollBar;
    }

    void ScrollArea::SetVerticalScrollBar(const Ref<VerticalScrollBar>& scrollbar, bool owner /*= true*/)
    {
        if (mVerScrollBar)
        {
            if (!mOwnVerScrollBar)
                mVerScrollBar->onSmoothChange -= THIS_FUNC(OnVerScrollChanged);
        }

        mVerScrollBar = scrollbar;
        mOwnVerScrollBar = owner;

        if (mVerScrollBar)
        {
            mVerScrollBar->name = "verScrollBar";
            mVerScrollBar->SetInternalParent(Ref(this), false);
            mVerScrollBar->GetLayoutData().drivenByParent = true;
            mVerScrollBar->onSmoothChange += THIS_FUNC(OnVerScrollChanged);
        }

        UpdateScrollParams();
        SetLayoutDirty();
    }

    const Ref<VerticalScrollBar>& ScrollArea::GetVerticalScrollbar() const
    {
        return mVerScrollBar;
    }

    void ScrollArea::SetEnableScrollsHiding(bool hideScrolls)
    {
        mEnableScrollsHiding = hideScrolls;

        if (!mEnableScrollsHiding)
        {
            if (mHorScrollBar && !mHorScrollBar->IsEnabled())
            {
                auto enableHorBarState = state["enableHorBar"];
                if (enableHorBarState)
                    *enableHorBarState = true;

                mHorScrollBar->Show(true);
            }

            if (mVerScrollBar && !mVerScrollBar->IsEnabled())
            {
                auto enableHorBarState = state["enableHorBar"];
                if (enableHorBarState)
                    *enableHorBarState = true;

                mVerScrollBar->Show(true);
            }
        }
    }

    bool ScrollArea::IsScrollsHiding()
    {
        return mEnableScrollsHiding;
    }

    void ScrollArea::SetScrollBarsShowingByCursor(bool showByCursor)
    {
        mShowScrollBarsByCursor = showByCursor;
    }

    bool ScrollArea::IsScrollBarsShowingByCursor()
    {
        return mShowScrollBarsByCursor;
    }

    void ScrollArea::SetClippingLayout(const Layout& clipLayout)
    {
        mClipAreaLayout = clipLayout;
        SetLayoutDirty();
    }

    Layout ScrollArea::GetClippingLayout() const
    {
        return mClipAreaLayout;
    }

    void ScrollArea::SetViewLayout(const Layout& viewLayout)
    {
        mViewAreaLayout = viewLayout;
        SetLayoutDirty();
    }

    Layout ScrollArea::GetViewLayout() const
    {
        return mViewAreaLayout;
    }

    void ScrollArea::SetChildrenWorldRect(const RectF& childrenWorldRect)
    {
        Vec2F roundedScrollPos(-Math::Round(mScrollPos.x), Math::Round(mScrollPos.y));

        mAbsoluteViewArea = mViewAreaLayout.Calculate(GetLayoutData().worldRectangle);
        mAbsoluteClipArea = mClipAreaLayout.Calculate(GetLayoutData().worldRectangle);

        GetLayoutData().childrenWorldRect = mAbsoluteViewArea + roundedScrollPos;
    }

    void ScrollArea::OnChildAdded(const Ref<Widget>& child)
    {
        child->GetLayoutData().drivenByParent = true;
    }

    void ScrollArea::OnChildRemoved(const Ref<Widget>& child)
    {
        child->GetLayoutData().drivenByParent = false;
    }

    void ScrollArea::CheckScrollBarsVisibility()
    {
        if (mEnableScrollsHiding && mShowScrollBarsByCursor)
        {
            auto cursor = o2Input.GetCursor(0);

            if (cursor)
            {
                if (mHorScrollBar && mHorScrollBar->IsUnderPoint(cursor->position) &&
                    mEnableHorScroll)
                {
                    mLastHorScrollChangeTime = o2Time.GetApplicationTime();

                    if (!mHorScrollBar->IsEnabled())
                    {
                        auto enableHorBarState = state["enableHorBar"];
                        if (enableHorBarState)
                            *enableHorBarState = true;

                        mHorScrollBar->Show();
                    }
                }

                if (mVerScrollBar && mVerScrollBar->IsUnderPoint(cursor->position) &&
                    mEnableVerScroll)
                {
                    mLastVerScrollChangeTime = o2Time.GetApplicationTime();

                    if (!mVerScrollBar->IsEnabled())
                    {
                        auto enableVerBarState = state["enableVerBar"];
                        if (enableVerBarState)
                            *enableVerBarState = true;

                        mVerScrollBar->Show();
                    }
                }
            }
        }
    }

    void ScrollArea::UpdateChildrenTransforms()
    {
        Widget::UpdateChildrenTransforms();

        CheckChildrenClipping();
        UpdateScrollParams();
    }

    bool ScrollArea::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

    bool ScrollArea::IsScrollable() const
    {
        return true;
    }

    bool ScrollArea::IsInputTransparent() const
    {
        return !mHasScrolled;
    }

    void ScrollArea::MoveScrollPosition(const Vec2F& delta)
    {
        mScrollPos += delta;
        SetChildrenWorldRect(GetLayoutData().worldRectangle);

        Vec2F widgetsMove(-delta.x, delta.y);
        for (auto& child : mChildWidgets)
            child->MoveAndCheckClipping(widgetsMove, mAbsoluteClipArea);

        UpdateLayersLayouts();
        UpdateScrollParams();
    }

    void ScrollArea::CheckChildrenClipping()
    {
        for (auto& child : mChildWidgets)
            child->CheckClipping(mAbsoluteClipArea);
    }

    void ScrollArea::CheckClipping(const RectF& clipArea)
    {
        mIsClipped = !mBoundsWithChilds.IsIntersects(clipArea);

        RectF newClipArea = clipArea.GetIntersection(mAbsoluteClipArea);

        for (auto& child : mChildWidgets)
            child->CheckClipping(newClipArea);
    }

    void ScrollArea::RecalculateScrollAreaRect(const RectF &childRect, const Vec2F &offset)
    {
        mScrollArea.left = Math::Min(mScrollArea.left, childRect.left - offset.x);
        mScrollArea.bottom = Math::Min(mScrollArea.bottom, childRect.bottom - offset.y);
        mScrollArea.right = Math::Max(mScrollArea.right, childRect.right - offset.x);
        mScrollArea.top = Math::Max(mScrollArea.top, childRect.top - offset.y);
    }

    void ScrollArea::InitializeScrollAreaRectCalculation(Vec2F& offset)
    {
        mScrollArea = RectF(0.0f, 0.0f, mAbsoluteViewArea.Width(), mAbsoluteViewArea.Height());

        offset = mAbsoluteViewArea.LeftBottom() - GetLayoutData().worldRectangle.LeftBottom() -
            mAbsoluteViewArea.Size()*layout->pivot + Vec2F(Math::Round(mScrollPos.x), Math::Round(mScrollPos.y));
    }

    void ScrollArea::CalculateScrollArea()
    {
        Vec2F offset;
        InitializeScrollAreaRectCalculation(offset);

        for (auto& child : mChildWidgets)
        {
            if (!child->mResEnabledInHierarchy || child->GetType() == TypeOf(ContextMenu))
                continue;

            RecalculateScrollAreaRect(child->GetLayoutData().rectangle, offset);
        }
    }

    void ScrollArea::UpdateScrollParams()
    {
        mAbsoluteViewArea = mViewAreaLayout.Calculate(GetLayoutData().worldRectangle);
        RectF localViewArea(0.0f, 0.0f, mAbsoluteViewArea.Width(), mAbsoluteViewArea.Height());

        CalculateScrollArea();

        mScrollRange.left = localViewArea.left + mScrollArea.left;
        mScrollRange.right = mScrollArea.right - localViewArea.right;

        mScrollRange.bottom = localViewArea.top - mScrollArea.top;
        mScrollRange.top = localViewArea.bottom - mScrollArea.bottom;

        //         o2Debug.Log(mName + " area: " + (String)mScrollArea + ", range: " + (String)mScrollRange + 
        //                     ", scroll: " + (String)mScrollPos);

        //         mScrollPos.x = Math::Clamp(mScrollPos.x, mScrollRange.left, mScrollRange.right);
        //         mScrollPos.y = Math::Clamp(mScrollPos.y, mScrollRange.bottom, mScrollRange.top);

        if (mHorScrollBar)
        {
            if (Math::Equals(mScrollRange.left, mScrollRange.right, 1.2f))
            {
                if (mHorScrollBar->IsEnabled())
                {
                    auto enableHorBarState = state["enableHorBar"];
                    if (enableHorBarState)
                        *enableHorBarState = false;

                    mHorScrollBar->Hide();
                }

                mHorScrollBar->SetValueRange(0, 0);

                mEnableHorScroll = false;
            }
            else
            {
                if (!mEnableHorScroll)
                {
                    auto enableHorBarState = state["enableHorBar"];
                    if (enableHorBarState)
                        *enableHorBarState = true;

                    mHorScrollBar->Show();
                }

                mEnableHorScroll = true;

                mHorScrollBar->SetValueRange(mScrollRange.left, mScrollRange.right);
                mHorScrollBar->SetScrollHandleSize(localViewArea.Width());
            }
        }

        if (mVerScrollBar)
        {
            if (Math::Equals(mScrollRange.bottom, mScrollRange.top, 1.2f))
            {
                if (mVerScrollBar->IsEnabled())
                {
                    auto enableVerBarState = state["enableVerBar"];
                    if (enableVerBarState)
                        *enableVerBarState = false;

                    mVerScrollBar->Hide();
                }

                mVerScrollBar->SetValueRange(0, 0);

                mEnableVerScroll = false;
            }
            else
            {
                if (!mEnableVerScroll)
                {
                    auto enableVerBarState = state["enableVerBar"];
                    if (enableVerBarState)
                        *enableVerBarState = true;

                    mVerScrollBar->Show();
                }

                mEnableVerScroll = true;

                mVerScrollBar->SetValueRange(mScrollRange.bottom, mScrollRange.top);
                mVerScrollBar->SetScrollHandleSize(localViewArea.Height());
            }
        }
    }

    void ScrollArea::OnHorScrollChanged(float value)
    {
        if (!mHorScrollBar->IsEnabled() && mEnableHorScroll)
        {
            auto enableHorBarState = state["enableHorBar"];
            if (enableHorBarState)
                *enableHorBarState = true;

            mHorScrollBar->Show();
        }

        mLastHorScrollChangeTime = o2Time.GetApplicationTime();

        Vec2F delta(Math::Clamp(value, mScrollRange.left, mScrollRange.right) - mScrollPos.x, 0.0f);
        MoveScrollPosition(delta);

        onScrolled(mScrollPos);
        OnScrolled();
    }

    void ScrollArea::OnVerScrollChanged(float value)
    {
        if (!mVerScrollBar->IsEnabled() && mEnableVerScroll)
        {
            auto enableVerBarState = state["enableVerBar"];
            if (enableVerBarState)
                *enableVerBarState = true;

            mVerScrollBar->Show();
        }

        mLastVerScrollChangeTime = o2Time.GetApplicationTime();

        Vec2F delta(0.0f, Math::Clamp(value, mScrollRange.bottom, mScrollRange.top) - mScrollPos.y);
        MoveScrollPosition(delta);

        onScrolled(mScrollPos);
        OnScrolled();
    }

    void ScrollArea::OnDrawn()
    {
        CursorAreaEventsListener::OnDrawn();
        Widget::OnDrawn();
    }

    void ScrollArea::OnDeserialized(const DataValue& node)
    {
        Widget::OnDeserialized(node);

        if (mHorScrollBar)
        {
            mEnableHorScroll = mHorScrollBar->IsEnabled();
            mHorScrollBar->onSmoothChange -= THIS_FUNC(OnHorScrollChanged);
        }

        if (mVerScrollBar)
        {
            mEnableVerScroll = mVerScrollBar->IsEnabled();
            mVerScrollBar->onSmoothChange -= THIS_FUNC(OnVerScrollChanged);
        }

        mHorScrollBar = GetInternalWidgetByType<HorizontalScrollBar>("horScrollBar");
        mOwnHorScrollBar = mHorScrollBar != nullptr;
        if (mOwnHorScrollBar)
            mHorScrollBar->onSmoothChange += THIS_FUNC(OnHorScrollChanged);

        mVerScrollBar = GetInternalWidgetByType<VerticalScrollBar>("verScrollBar");
        mOwnVerScrollBar = mVerScrollBar != nullptr;
        if (mOwnVerScrollBar)
            mVerScrollBar->onSmoothChange += THIS_FUNC(OnVerScrollChanged);

        for (auto& child : mChildWidgets)
            child->GetLayoutData().drivenByParent = true;

        RetargetStatesAnimations();
    }

    void ScrollArea::OnScrolled()
    {}

    void ScrollArea::OnScrolled(float scroll)
    {
        mScrollSpeed = Vec2F();

        if (mVerScrollBar && mEnableVerScroll)
        {
            mVerScrollBar->OnScrolled(o2Input.GetMouseWheelDelta());
            mHasScrolled = !Math::Equals(mVerScrollBar->GetMinValue(), mVerScrollBar->GetMaxValue());
        }
        else if (mHorScrollBar && mEnableHorScroll)
        {
            mHorScrollBar->OnScrolled(o2Input.GetMouseWheelDelta());
            mHasScrolled = !Math::Equals(mHorScrollBar->GetMinValue(), mHorScrollBar->GetMaxValue());
        }
    }

    String ScrollArea::GetCreateMenuGroup()
    {
        return "Basic";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::ScrollArea>);
// --- META ---

DECLARE_CLASS(o2::ScrollArea, o2__ScrollArea);
// --- END META ---
