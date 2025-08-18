#include "o2/stdafx.h"
#include "AnimationVec2FTrack.h"

#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Math/Interpolation.h"
#include "o2/Utils/Math/Vector2.h"

namespace o2
{
    AnimationTrack<Vec2F>::AnimationTrack()
    {
        timeCurve->onKeysChanged.Add(this, &AnimationTrack<Vec2F>::OnCurveChanged);
        spline->onKeysChanged.Add(this, &AnimationTrack<Vec2F>::OnCurveChanged);
    }

    AnimationTrack<Vec2F>::AnimationTrack(const AnimationTrack<Vec2F>& other) :
        IAnimationTrack(other), timeCurve(other.timeCurve), spline(other.spline)
    {
        timeCurve->onKeysChanged.Add(this, &AnimationTrack<Vec2F>::OnCurveChanged);
        spline->onKeysChanged.Add(this, &AnimationTrack<Vec2F>::OnCurveChanged);
    }

    AnimationTrack<Vec2F>& AnimationTrack<Vec2F>::operator=(const AnimationTrack<Vec2F>& other)
    {
        IAnimationTrack::operator =(other);
        timeCurve = other.timeCurve;
        spline = other.spline;

        onKeysChanged();

        return *this;
    }

    Vec2F AnimationTrack<Vec2F>::GetValue(float position) const
    {
        int cacheKey = 0, cacheKeyApporx = 0;
        return GetValue(position, 0.0f, true, cacheKey, cacheKeyApporx, cacheKey, cacheKeyApporx);
    }

    Vec2F AnimationTrack<Vec2F>::GetValue(float position, float randomRangeCoef, bool direction, int& cacheTimeKey, int& cacheTimeKeyApprox,
                                          int& cacheSplineKey, int& cacheSplineKeyApprox) const
    {
        float timePos = timeCurve->Evaluate(position, randomRangeCoef, direction, cacheTimeKey, cacheTimeKeyApprox);
        return spline->Evaluate(timePos*spline->Length(), randomRangeCoef, direction, cacheSplineKey, cacheTimeKeyApprox);
    }

    void AnimationTrack<Vec2F>::BeginKeysBatchChange()
    {
        timeCurve->BeginKeysBatchChange();
        spline->BeginKeysBatchChange();
    }

    void AnimationTrack<Vec2F>::CompleteKeysBatchingChange()
    {
        timeCurve->CompleteKeysBatchingChange();
        spline->CompleteKeysBatchingChange();
    }

    float AnimationTrack<Vec2F>::GetDuration() const
    {
        return timeCurve->Length();
    }

    Ref<IAnimationTrack::IPlayer> AnimationTrack<Vec2F>::CreatePlayer() const
    {
        return mmake<Player>();
    }

    void AnimationTrack<Vec2F>::OnCurveChanged()
    {
        onKeysChanged();
    }

    AnimationTrack<Vec2F> AnimationTrack<Vec2F>::Parametric(const Vec2F& begin, const Vec2F& end, float duration,
                                                          float beginCoef, float beginCoefPosition,
                                                          float endCoef, float endCoefPosition)
    {
        AnimationTrack<Vec2F> res;
        res.spline->SetKeys({ Spline::Key(begin, 0.0f, Vec2F(), Vec2F()), Spline::Key(end, 0.0f, Vec2F(), Vec2F()) });
        res.timeCurve = mmake<Curve>(Curve::Parametric(0.0f, 1.0f, duration, beginCoef, beginCoefPosition, endCoef, endCoefPosition));
        return res;
    }

    AnimationTrack<Vec2F> AnimationTrack<Vec2F>::EaseIn(const Vec2F& begin, const Vec2F& end, float duration /*= 1.0f*/,
                                                        float strongness /*= 0.4f*/)
    {
        return Parametric(begin, end, duration, 0.0f, strongness, 1.0f, 1.0f);
    }

    AnimationTrack<Vec2F> AnimationTrack<Vec2F>::EaseOut(const Vec2F& begin, const Vec2F& end, float duration /*= 1.0f*/,
                                                         float strongness /*= 0.4f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.0f, 1.0f, strongness);
    }

    AnimationTrack<Vec2F> AnimationTrack<Vec2F>::EaseInOut(const Vec2F& begin, const Vec2F& end, float duration /*= 1.0f*/,
                                                           float strongness /*= 0.4f*/)
    {
        return Parametric(begin, end, duration, 0.0f, strongness, 1.0f, 1.0f);
    }

    AnimationTrack<Vec2F> AnimationTrack<Vec2F>::Linear(const Vec2F& begin, const Vec2F& end, float duration /*= 1.0f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    AnimationTrack<Vec2F>::Player::Player():
        IPlayer()
    {}

    AnimationTrack<Vec2F>::Player::~Player()
    {}

    AnimationTrack<Vec2F>::Player::operator Vec2F() const
    {
        return mCurrentValue;
    }

    void AnimationTrack<Vec2F>::Player::SetTarget(Vec2F* value)
    {
        mTargetProxy = nullptr;
        mTarget = value;
    }

    void AnimationTrack<Vec2F>::Player::SetTargetProxy(const Ref<IValueProxy<Vec2F>>& proxy)
    {
        mTarget = nullptr;
        mTargetProxy = proxy;
    }

    void AnimationTrack<Vec2F>::Player::SetTrack(const Ref<AnimationTrack<Vec2F>>& track)
    {
        mTrack = track;
        IPlayer::SetTrack(track);
    }

    void AnimationTrack<Vec2F>::Player::SetTargetVoid(void* target)
    {
        SetTarget((Vec2F*)target);
    }

    void AnimationTrack<Vec2F>::Player::SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy)
    {
        SetTargetProxy(DynamicCast<IValueProxy<Vec2F>>(targetProxy));
    }

    void AnimationTrack<Vec2F>::Player::SetTrack(const Ref<IAnimationTrack>& track)
    {
        SetTrack(DynamicCast<AnimationTrack<Vec2F>>(track));
    }

    Vec2F AnimationTrack<Vec2F>::Player::GetValue() const
    {
        return mCurrentValue;
    }

    const Ref<AnimationTrack<Vec2F>>& AnimationTrack<Vec2F>::Player::GetTrackT() const
    {
        return mTrack;
    }

    Ref<IAnimationTrack> AnimationTrack<Vec2F>::Player::GetTrack() const
    {
        return mTrack;
	}

	void AnimationTrack<Vec2F>::Player::OnPlay()
	{
        mRandomRangeCoef = Math::Random(0.0f, 1.0f);
	}

    void AnimationTrack<Vec2F>::Player::Evaluate()
    {
        if (mTrack->timeCurve->IsEmpty())
        {
            mCurrentValue = mTarget ? *mTarget : mTargetProxy ? mTargetProxy->GetValue() : Vec2F();
            return;
        }

        mCurrentValue = mTrack->GetValue(mInDurationTime, mRandomRangeCoef, mInDurationTime > mPrevInDurationTime, 
                          mPrevTimeKey, mPrevTimeKeyApproximation,
                        mPrevSplineKey, mPrevSplineKeyApproximation);

        mPrevInDurationTime = mInDurationTime;

        if (mTarget)
            *mTarget = mCurrentValue;
        else if (mTargetProxy)
            mTargetProxy->SetValue(mCurrentValue);
    }

    void AnimationTrack<Vec2F>::Player::RegMixer(const Ref<AnimationState>& state, const String& path)
    {
        state->mOwner.Lock()->RegValueTrack<Vec2F>(Ref(this), path, state);
    }
}
// --- META ---

DECLARE_CLASS(o2::AnimationTrack<o2::Vec2F>, o2__AnimationTrack_o2__Vec2F_);

DECLARE_CLASS(o2::AnimationTrack<o2::Vec2F>::Player, o2__AnimationTrack_o2__Vec2F___Player);
// --- END META ---
