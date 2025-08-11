#include "o2Editor/stdafx.h"
#include "Timeline.h"

#include "o2/Application/Input.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    AnimationTimeline::AnimationTimeline(RefCounter* refCounter) :
        Widget(refCounter)
    {
        mTextFont = AssetRef<VectorFontAsset>("stdFont.ttf")->GetFont();
        mTextFont->CheckCharacters("0123456789.,+-", 10);

        mText = mmake<Text>(mTextFont);
        mText->horAlign = HorAlign::Middle;
        mText->verAlign = VerAlign::Bottom;
        mText->height = 8;
        mText->color = Color4(44, 62, 80);

        AddLayer("back", mmake<Sprite>("ui/UI4_dopesheet_back.png"), Layout::BothStretch(-3, -3, -3, -14))->transparency = 0.5f;

        mBeginMark = mmake<Sprite>("ui/UI4_time_line_left.png");
        mEndMark = mmake<Sprite>("ui/UI4_time_line_right.png");

        mTimeLineEventsArea = mmake<CursorEventsArea>();
        mTimeLine = mmake<Sprite>("ui/UI4_time_line.png");

        mTimeLineEventsArea->isUnderPoint = [&](const Vec2F& pos) {
            auto rect = layout->GetWorldRect();
            rect.bottom = rect.top - 20.0f;

            return rect.IsInside(pos);
        };

        mTimeLineEventsArea->onMoved = THIS_FUNC(SetAnimationTimeByCursor);
        mTimeLineEventsArea->onCursorPressed = THIS_FUNC(SetAnimationTimeByCursor);
    }

    AnimationTimeline::AnimationTimeline(RefCounter* refCounter, const AnimationTimeline& other) :
        Widget(refCounter, other), mTextFont(other.mTextFont), mText(other.mText->CloneAs<Text>()),
        mBeginMark(other.mBeginMark->CloneAs<Sprite>()), mEndMark(other.mEndMark->CloneAs<Sprite>()),
        mTimeLine(other.mTimeLine->CloneAs<Sprite>())
    { }

    AnimationTimeline::~AnimationTimeline()
    {}

    AnimationTimeline& AnimationTimeline::operator=(const AnimationTimeline& other)
    {
        Widget::operator=(other);

        mTextFont = other.mTextFont;
        mText = other.mText->CloneAsRef<Text>();

        mBeginMark = other.mBeginMark->CloneAsRef<Sprite>();
        mEndMark = other.mEndMark->CloneAsRef<Sprite>();

        mTimeLine = other.mTimeLine->CloneAsRef<Sprite>();

        return *this;
    }

    void AnimationTimeline::Draw()
    {
        Widget::Draw();

        o2Render.EnableScissorTest(GetChildrenWorldRect());

        DrawTimeScale();

        mTimeLine->SetPosition(Vec2F(LocalToWorld(mTimeCursor), layout->GetWorldTop()));
        mTimeLine->SetSize(Vec2F(10.0f, layout->GetHeight() + 5.0f));
        mTimeLine->SetSizePivot(Vec2F(4.5f, layout->GetHeight() + 4.0f));
        mTimeLine->Draw();

        o2Render.DisableScissorTest();

        CursorAreaEventsListener::OnDrawn();

        mTimeLineEventsArea->OnDrawn();
    }

    void AnimationTimeline::DrawTimeScale()
    {
        double beginPos = (double)(mScaleOffset - mSmoothViewScroll);
        double endPos = beginPos + mDuration*mOneSecondDefaultSize*mSmoothViewZoom;

        Layout beginLayout = mBeginMarkLayout;
        beginLayout.offsetMax.x += (float)beginPos;

        Layout endLayout = mEndMarkLayout;
        endLayout.offsetMin.x += (float)endPos;

        mBeginMark->SetRect(beginLayout.Calculate(GetChildrenWorldRect()));
        mBeginMark->Draw();

        mEndMark->SetRect(endLayout.Calculate(GetChildrenWorldRect()));
        mEndMark->Draw();

        int bigLinePeriod;
        double bigLineTimeAmount;
        ChooseScaleParams(bigLinePeriod, bigLineTimeAmount);

        double posDelta = bigLineTimeAmount * mOneSecondDefaultSize*mSmoothViewZoom;

        int lineIdx = 0;
        while (beginPos < 0)
        {
            lineIdx++;
            beginPos += posDelta;
        }

        endPos = (double)layout->GetWidth();

        RectF worldRect = layout->GetWorldRect();
        for (double pos = beginPos; pos < endPos; pos += posDelta, lineIdx++)
        {
            float screenPos = (float)pos + worldRect.left;
            bool isBigLine = lineIdx%bigLinePeriod == 0;

            o2Render.DrawAALine(Vec2F(screenPos, worldRect.top - (isBigLine ? mBigLineOffset : mSmallLineOffset)),
                                Vec2F(screenPos, worldRect.bottom),
                                isBigLine ? mBigLineColor : mSmallLineColor);

            if (isBigLine)
            {
                mText->SetText((String)((double)lineIdx*bigLineTimeAmount));
                mText->SetPosition(Vec2F(screenPos, worldRect.top - mTextOffset));
                mText->Draw();
            }
        }
    }

    void AnimationTimeline::Update(float dt)
    {
        Widget::Update(dt);

        UpdateZooming(dt);
        UpdateScrolling(dt);
    }

    void AnimationTimeline::UpdateZooming(float dt)
    {
        if (!Math::Equals(mSmoothViewZoom, mViewZoom, 0.001f))
		{
			float timeAtCursorBefore = WorldToLocal(o2Input.GetCursorPos().x);
			mSmoothViewZoom = Math::Lerp(mSmoothViewZoom, mViewZoom, dt * mScaleSmoothLerpCoef);
			float timeAtCursorAfter = WorldToLocal(o2Input.GetCursorPos().x);

            if (mViewHasZoomed)
            {
                float scrollDelta = (timeAtCursorAfter - timeAtCursorBefore) * mOneSecondDefaultSize * mSmoothViewZoom;
                mViewScroll -= scrollDelta;
                mSmoothViewScroll -= scrollDelta;
            }

            UpdateScrollBarHandleSize();

            onViewChanged();
        }
        else
            mViewHasZoomed = false;
    }

    void AnimationTimeline::UpdateScrollBarHandleSize()
    {
        if (mScrollBar)
            mScrollBar->SetScrollHandleSize(layout->GetWidth());
    }

    float AnimationTimeline::LocalToWorld(float pos) const
    {
        return pos*(mOneSecondDefaultSize*mSmoothViewZoom) + mScaleOffset + layout->GetWorldLeft() - mSmoothViewScroll;
    }

    float AnimationTimeline::WorldToLocal(float pos) const
    {
        return (pos + mSmoothViewScroll - layout->GetWorldLeft() - mScaleOffset)/(mOneSecondDefaultSize*mSmoothViewZoom);
    }

    void AnimationTimeline::OnTransformUpdated()
    {
        UpdateScrollBarHandleSize();
        Widget::OnTransformUpdated();
    }

    void AnimationTimeline::SetAnimationTimeByCursor(const Input::Cursor& cursor)
    {
        if (cursor.isPressed && mAnimationWindow->mAnimation)
        {
            mAnimationWindow->mPreviewToggle->value = true;
            SetTimeCursor(Math::Max(0.0f, WorldToLocal(cursor.position.x)));
        }
    }

    void AnimationTimeline::OnCursorRightMousePressed(const Input::Cursor& cursor)
    {
        mBeginDragViewScrollOffset = cursor.position.x;
        mBeginDragViewScroll = mViewScroll;
    }

    void AnimationTimeline::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {
        const float beginScrollThreshold = 3.0f;

        if (!mDragViewScroll && !mViewMoveDisabled)
        {
            if (Math::Abs(LocalToWorld(mBeginDragViewScrollOffset) - cursor.position.x) > beginScrollThreshold)
                mDragViewScroll = true;
        }

        if (mDragViewScroll)
        {
            mViewScroll = mBeginDragViewScroll + mBeginDragViewScrollOffset - cursor.position.x;

            if (mViewScroll < 0.0f)
                mViewScroll = mViewScroll/2.0f;

			float maxScroll = mDuration*mOneSecondDefaultSize*mSmoothViewZoom;
            if (mViewScroll > maxScroll)
                mViewScroll = Math::Lerp(mViewScroll, maxScroll, 0.5f);

            mSmoothViewScroll = mViewScroll;

            if (mScrollBar)
                mScrollBar->SetValueForcible(mSmoothViewScroll);
        }
    }

    void AnimationTimeline::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        if (mDragViewScroll)
        {
            mViewScrollSpeed = -cursor.delta.x/o2Time.GetDeltaTime();
            mDragViewScroll = false;
        }
    }

    void AnimationTimeline::OnScrolled(float scroll)
    {
        if (!mViewMoveDisabled)
        {
            mViewZoom = Math::Clamp(mViewZoom/(1.0f - o2Input.GetMouseWheelDelta()*mScaleSense), mMinScale, mMaxScale);
            mViewHasZoomed = true;
        }
    }

    void AnimationTimeline::ChooseScaleParams(int& bigLinePeriod, double& bigLineTimeAmount)
    {
        struct Cfg
        {
            int chunkSegments;
            double chunkDuration;
        };

        Cfg configs[] = { { 5, 0.05 }, { 10, 0.1 }, { 5, 0.1 }, { 2, 0.1 }, { 5, 0.5 }, { 10, 1.0 }, { 5, 1.0 }, { 2, 1.0 }, { 5, 5.0 }, { 6, 30.0 } };

        Cfg nearestCfg;
        float nearestCfgScreenChunkSegmentSizeDiff = FLT_MAX;
        for (auto& cfg : configs)
        {
            float screenChunkSegmentSize = (float)cfg.chunkDuration/(float)cfg.chunkSegments*mOneSecondDefaultSize*mSmoothViewZoom;
            float screenChunkSegmentSizeDiff = mPerfectScaleSegmentSize - screenChunkSegmentSize;
            if (screenChunkSegmentSizeDiff > 0.0f)
                screenChunkSegmentSizeDiff *= 2.0f;

            if (Math::Abs(screenChunkSegmentSizeDiff) < nearestCfgScreenChunkSegmentSizeDiff)
            {
                nearestCfgScreenChunkSegmentSizeDiff = screenChunkSegmentSizeDiff;
                nearestCfg = cfg;
            }
        }

        bigLinePeriod = nearestCfg.chunkSegments;
        bigLineTimeAmount = nearestCfg.chunkDuration/(double)nearestCfg.chunkSegments;
    }

    void AnimationTimeline::UpdateScrolling(float dt)
    {
        float prevViewScroll = mSmoothViewScroll;

        if (!mDragViewScroll && Math::Abs(mViewScrollSpeed) > 0.0001f)
        {
            mViewScroll += mViewScrollSpeed*dt;
            mSmoothViewScroll = mViewScroll;
            mViewScrollSpeed = Math::Lerp(mViewScrollSpeed, 0.0f, dt*mScrollSpeedDecreaseCoef);

            if (mScrollBar)
                mScrollBar->SetValueForcible(mSmoothViewScroll);
        }

        if (!mDragViewScroll && !mViewMoveDisabled)
        {
            if (mViewScroll < 0.0f)
                mViewScroll = Math::Lerp(mViewScroll, 0.0f, dt*mScrollBorderBounceCoef);

			float maxScroll = mDuration * mOneSecondDefaultSize * mSmoothViewZoom;
            if (mViewScroll > maxScroll)
                mViewScroll = Math::Lerp(mViewScroll, maxScroll, dt*mScrollBorderBounceCoef);
        }

        if (!Math::Equals(mViewScroll, mSmoothViewScroll, 0.0001f))
        {
            mSmoothViewScroll = Math::Lerp(mSmoothViewScroll, mViewScroll, dt*mScrollSmoothCoef);

            if (mScrollBar)
                mScrollBar->SetValueForcible(mSmoothViewScroll);
        }

        if (!Math::Equals(prevViewScroll, mSmoothViewScroll))
            onViewChanged();
    }

    void AnimationTimeline::SetAnimation(const Ref<AnimationClip>& animation, const Ref<AnimationPlayer>& player /*= nullptr*/)
    {
        if (mAnimation)
            mAnimation->onDurationChange -= THIS_FUNC(UpdateDuration);

        mAnimation = animation;
        mPlayer = player;

        if (mAnimation)
        {
            mAnimation->onDurationChange += THIS_FUNC(UpdateDuration);
            mViewZoom = Math::Clamp((layout->worldRight - layout->worldLeft)/mOneSecondDefaultSize/mAnimation->GetDuration(), mMinScale, mMaxScale);

            UpdateDuration(mAnimation->GetDuration());
        }        
    }

    void AnimationTimeline::UpdateDuration(float duration)
    {
        mDuration = duration;

        if (mScrollBar)
        {
            mScrollBar->minValue = 0.0f;
            mScrollBar->maxValue = mDuration*(mOneSecondDefaultSize*mViewZoom);
            UpdateScrollBarHandleSize();
        }
    }

    void AnimationTimeline::SetScroll(float scroll)
    {
        mSmoothViewScroll = scroll;
    }

    void AnimationTimeline::SetViewRange(float left, float right, bool force /*= true*/)
    {
        mViewZoom = (layout->worldRight - layout->worldLeft)/mOneSecondDefaultSize/(right - left);
        mViewScroll = left + mScaleOffset;

        if (force)
        {
            mSmoothViewScroll = mViewScroll;
            mSmoothViewZoom = mViewZoom;
        }
    }

    void AnimationTimeline::SetTimeCursor(float time)
    {
        mAnimationWindow->mDisableTimeTracking = true;
        mTimeCursor = time;

        if (mPlayer)
        {
            mPlayer->Stop();
            mPlayer->SetTime(time);

            o2EditorSceneScreen.NeedRedraw();
        }

        mAnimationWindow->mDisableTimeTracking = false;
    }

    float AnimationTimeline::GetTimeCursor() const
    {
        return mTimeCursor;
    }

    float AnimationTimeline::GetScroll() const
    {
        return mSmoothViewScroll;
    }

    void AnimationTimeline::SetScale(float scale)
    {
        mViewZoom = scale;
    }

    float AnimationTimeline::GetScale() const
    {
        return mViewZoom;
    }

    void AnimationTimeline::SetViewMoveDisabled(bool disabled)
    {
        mViewMoveDisabled = disabled;
    }

    const Ref<Text>& AnimationTimeline::GetText() const
    {
        return mText;
    }

    void AnimationTimeline::SetScrollBar(const Ref<HorizontalScrollBar>& scrollBar)
    {
        mScrollBar = scrollBar;
        mScrollBar->onChangeByUser = [&](float value) { mViewScroll = value; };

        AddChild(scrollBar);
    }

    const Ref<HorizontalScrollBar>& AnimationTimeline::GetScrollBar() const
    {
        return mScrollBar;
    }

    bool AnimationTimeline::IsSameTime(float timeA, float timeB, float threshold /*= 3.0f*/) const
    {
        return Math::Abs(LocalToWorld(timeA) - LocalToWorld(timeB)) <= threshold;
    }

    bool AnimationTimeline::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

    bool AnimationTimeline::IsScrollable() const
    {
        return true;
    }

    bool AnimationTimeline::IsInputTransparent() const
    {
        return !mViewHasZoomed && !mDragViewScroll;
    }

    String AnimationTimeline::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AnimationTimeline>);
// --- META ---

DECLARE_CLASS(Editor::AnimationTimeline, Editor__AnimationTimeline);
// --- END META ---
