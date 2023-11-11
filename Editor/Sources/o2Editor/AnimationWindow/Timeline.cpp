#include "o2Editor/stdafx.h"
#include "Timeline.h"

#include "o2/Application/Input.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2Editor/AnimationWindow/AnimationWindow.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
	AnimationTimeline::AnimationTimeline() :
		Widget()
	{
		mTextFont = FontRef("stdFont.ttf");
		mTextFont->CheckCharacters("0123456789.,+-", 10);

		mText = mnew Text(mTextFont);
		mText->horAlign = HorAlign::Middle;
		mText->verAlign = VerAlign::Bottom;
		mText->height = 8;
		mText->color = Color4(44, 62, 80);

		AddLayer("back", mnew Sprite("ui/UI4_dopesheet_back.png"), Layout::BothStretch(-3, -3, -3, -14))->transparency = 0.5f;

		mBeginMark = mnew Sprite("ui/UI4_time_line_left.png");
		mEndMark = mnew Sprite("ui/UI4_time_line_right.png");

		mTimeLine = mnew Sprite("ui/UI4_time_line.png");

		mTimeLineEventsArea.isUnderPoint = [&](const Vec2F& pos) {
			auto rect = layout->GetWorldRect();
			rect.bottom = rect.top - 20.0f;

			return rect.IsInside(pos);
		};

		mTimeLineEventsArea.onMoved = THIS_FUNC(SetAnimationTimeByCursor);
		mTimeLineEventsArea.onCursorPressed = THIS_FUNC(SetAnimationTimeByCursor);
	}

	AnimationTimeline::AnimationTimeline(const AnimationTimeline& other) :
		Widget(other), mTextFont(other.mTextFont), mText(other.mText->CloneAs<Text>()),
		mBeginMark(other.mBeginMark->CloneAs<Sprite>()), mEndMark(other.mEndMark->CloneAs<Sprite>()),
		mTimeLine(other.mTimeLine->CloneAs<Sprite>())
	{ }

	AnimationTimeline::~AnimationTimeline()
	{
		delete mText;
		delete mTimeLine;
	}

	AnimationTimeline& AnimationTimeline::operator=(const AnimationTimeline& other)
	{
		delete mText;
		delete mBeginMark;
		delete mEndMark;

		Widget::operator=(other);

		mTextFont = other.mTextFont;
		mText = other.mText->CloneAs<Text>();

		mBeginMark = other.mBeginMark->CloneAs<Sprite>();
		mEndMark = other.mEndMark->CloneAs<Sprite>();

		mTimeLine = other.mTimeLine->CloneAs<Sprite>();

		return *this;
	}

	void AnimationTimeline::Draw()
	{
		Widget::Draw();

		o2Render.EnableScissorTest(GetChildrenWorldRect());

		DrawTimeScale();

		mTimeLine->SetPosition(Vec2F(LocalToWorld(mTimeCursor), layout->GetWorldTop()));
		mTimeLine->SetSize(Vec2F(10.0f, layout->GetHeight() + 5.0f));
		mTimeLine->SetSizePivot(Vec2F(7.5f, layout->GetHeight() + 4.0f));
		mTimeLine->Draw();

		o2Render.DisableScissorTest();

		CursorAreaEventsListener::OnDrawn();

		mTimeLineEventsArea.OnDrawn();
	}

	void AnimationTimeline::DrawTimeScale()
	{
		double beginPos = (double)(mScaleOffset - mSmoothViewScroll*mOneSecondDefaultSize*mSmoothViewZoom);
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
			mSmoothViewZoom = Math::Lerp(mSmoothViewZoom, mViewZoom, dt*mScaleSmoothLerpCoef);
			UpdateScrollBarHandleSize();

			onViewChanged();
		}

		mViewHasZoomed = false;
	}

	void AnimationTimeline::UpdateScrollBarHandleSize()
	{
		if (mScrollBar)
			mScrollBar->SetScrollHandleSize(layout->GetWidth()/mOneSecondDefaultSize/mSmoothViewZoom);
	}

	float AnimationTimeline::LocalToWorld(float pos) const
	{
		return (pos - mSmoothViewScroll)*(mOneSecondDefaultSize*mSmoothViewZoom) + mScaleOffset + layout->GetWorldLeft();
	}

	float AnimationTimeline::WorldToLocal(float pos) const
	{
		return (pos - layout->GetWorldLeft() - mScaleOffset)/(mOneSecondDefaultSize*mSmoothViewZoom) + mSmoothViewScroll;
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
		mBeginDragViewScrollOffset = WorldToLocal(cursor.position.x);
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
			mViewScroll -= WorldToLocal(cursor.position.x) - mBeginDragViewScrollOffset;

			if (mViewScroll < 0.0f)
				mViewScroll = mViewScroll/2.0f;

			if (mViewScroll > mDuration)
				mViewScroll = Math::Lerp(mDuration, mViewScroll, 0.5f);

			mSmoothViewScroll = mViewScroll;

			if (mScrollBar)
				mScrollBar->SetValueForcible(mSmoothViewScroll);
		}
	}

	void AnimationTimeline::OnCursorRightMouseReleased(const Input::Cursor& cursor)
	{
		if (mDragViewScroll)
		{
			mViewScrollSpeed = -cursor.delta.x/o2Time.GetDeltaTime()/(mOneSecondDefaultSize*mSmoothViewZoom);
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

		Cfg configs[] = { { 10, 0.1 }, { 5, 0.1 }, { 2, 0.1 }, { 5, 0.5 }, { 10, 1.0 }, { 5, 1.0 }, { 2, 1.0 }, { 5, 5.0 }, { 6, 30.0 } };

		Cfg nearestCfg;
		float nearestCfgScreenChunkSegmentSizeDiff = FLT_MAX;
		for (auto cfg : configs)
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

			if (mViewScroll > mDuration)
				mViewScroll = Math::Lerp(mSmoothViewScroll, mDuration, dt*mScrollBorderBounceCoef);
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

	void AnimationTimeline::SetAnimation(AnimationClip* animation, AnimationPlayer* player /*= nullptr*/)
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
			mScrollBar->maxValue = mDuration;
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
		mViewScroll = left + mScaleOffset/mOneSecondDefaultSize/mViewZoom;

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

	Text* AnimationTimeline::GetText() const
	{
		return mText;
	}

	void AnimationTimeline::SetScrollBar(HorizontalScrollBar* scrollBar)
	{
		mScrollBar = scrollBar;
		mScrollBar->onChangeByUser = [&](float value) { mViewScroll = value; };

		AddChild(scrollBar);
	}

	HorizontalScrollBar* AnimationTimeline::GetScrollBar() const
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
// --- META ---

DECLARE_CLASS(Editor::AnimationTimeline, Editor__AnimationTimeline);
// --- END META ---
