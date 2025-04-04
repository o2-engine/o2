#include "o2/stdafx.h"
#include "IAnimation.h"

#include "o2/Utils/Math/Math.h"

namespace o2
{
    IAnimation::IAnimation()
    {}

    IAnimation::IAnimation(const IAnimation& other) :
        IAnimation(nullptr, other)
    {}

    IAnimation::IAnimation(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    IAnimation::IAnimation(RefCounter* refCounter, const IAnimation& other):
        RefCounterable(refCounter), mTime(other.mTime), mDuration(other.mDuration), mBeginTime(other.mBeginTime), mEndTime(other.mEndTime),
        mDirection(other.mDirection), mSpeed(other.mSpeed), mLoop(other.mLoop),
        mPlaying(other.mPlaying), mInDurationTime(other.mInDurationTime),
        playing(this), reversed(this), speed(this), time(this), relTime(this),
        beginBound(this), endBound(this), loop(this), duration(this)
    {}

    IAnimation::~IAnimation()
    {}

    IAnimation& IAnimation::operator=(const IAnimation& other)
    {
        mTime = other.mTime;
        mDuration = other.mDuration;
        mBeginTime = other.mBeginTime;
        mEndTime = other.mEndTime;
        mDirection = other.mDirection;
        mSpeed = other.mSpeed;
        mLoop = other.mLoop;
        mPlaying = other.mPlaying;
        mInDurationTime = other.mInDurationTime;

        return *this;
    }

    void IAnimation::Update(float dt)
    {
        if (!mPlaying || mSubControlled)
            return;

        mTime += mDirection*dt*mSpeed;

        float lastInDurationTime = mInDurationTime;

        UpdateTime();

        float eventCheckBeg = Math::Min(lastInDurationTime, mInDurationTime);
        float eventCheckEnd = Math::Max(lastInDurationTime, mInDurationTime);
        for (auto& kv : mTimeEvents)
        {
            if (kv.first > eventCheckBeg && kv.first <= eventCheckEnd)
                kv.second.Invoke();
        }
    }

    void IAnimation::SetSubControlled(bool subControlled)
    {
        mSubControlled = subControlled;
    }

    bool IAnimation::IsSubControlled() const
    {
        return mSubControlled;
    }

    void IAnimation::UpdateTime()
    {
        if (mLoop == Loop::None)
        {
            if (mTime > mEndTime)
            {
                mTime = mEndTime;
                mInDurationTime = mTime;
                mPlaying = false;
                Evaluate();

                onStop();
                onPlayed();
                OnStop();
            }
            else if (mTime < mBeginTime)
            {
                mTime = mBeginTime;
                mInDurationTime = mTime;
                mPlaying = false;
                Evaluate();

                onStop();
                onPlayed();
                OnStop();
            }
            else
            {
                mInDurationTime = mTime;
                Evaluate();
            }
        }
        else if (mLoop == Loop::Repeat)
        {
            float x;
            if (mTime > 0)
                mInDurationTime = modff(mTime/mDuration, &x)*mDuration;
            else
                mInDurationTime = (1.0f - modff(-mTime/mDuration, &x))*mDuration;

            Evaluate();
        }
        else //if (mLoop == Loop::PingPong)
        {
            float x;
            if (mTime > 0)
            {
                mInDurationTime = modff(mTime/mDuration, &x)*mDuration;
                if ((int)x%2 == 1)
                    mInDurationTime = mDuration - mInDurationTime;
            }
            else
            {
                mInDurationTime = (1.0f - modff(-mTime/mDuration, &x))*mDuration;
                if ((int)x%2 == 0)
                    mInDurationTime = mDuration - mInDurationTime;
            }

            Evaluate();
        }

        onUpdate(mTime);
    }

    void IAnimation::Play()
    {
        if (mPlaying)
            return;

        Evaluate();

        mPlaying = true;

        onPlay();
        OnPlay();
    }

    void IAnimation::PlayInBounds(float beginTime, float endTime)
    {
        mBeginTime = beginTime;
        mEndTime = endTime;

        Play();
    }

    void IAnimation::PlayBackInBounds(float beginTime, float endTime)
    {
        mBeginTime = beginTime;
        mEndTime = endTime;

        PlayBack();
    }

    void IAnimation::TogglePlay()
    {
        SetReverse(!IsReversed());
        Play();
    }

    void IAnimation::RewindAndPlay()
    {
        GoToBegin();
        Play();
    }

    void IAnimation::Stop()
    {
        if (!mPlaying)
            return;

        Evaluate();

        mPlaying = false;

        onStop();
        OnStop();
    }

    void IAnimation::SetBeginBound(float time)
    {
        SetBounds(time, mEndTime);
    }

    float IAnimation::GetBeginBound() const
    {
        return mBeginTime;
    }

    void IAnimation::SetEndBound(float time)
    {
        SetBounds(mBeginTime, time);
    }

    float IAnimation::GetEndBound() const
    {
        return mEndTime;
    }

    void IAnimation::SetBounds(float beginTime, float endTime)
    {
        mBeginTime = Math::Min(beginTime, endTime);
        mEndTime = Math::Max(beginTime, endTime);

        SetReverse(beginTime > endTime);
        mTime = Math::Clamp(mTime, mBeginTime, mEndTime);

        Evaluate();
    }

    void IAnimation::ResetBounds()
    {
        mBeginTime = 0.0f;
        mEndTime = mDuration;
    }

    void IAnimation::SetPlaying(bool playing)
    {
        if (playing)
            Play();
        else
            Stop();
    }

    bool IAnimation::IsPlaying() const
    {
        return mPlaying;
    }

    void IAnimation::SetTime(float time)
    {
        mTime = time;
        UpdateTime();
    }

    float IAnimation::GetTime() const
    {
        return mTime;
    }

    float IAnimation::GetLoopTime() const
    {
        float duration = GetDuration();

        switch (mLoop) 
        {
            case Loop::None:
            return Math::Clamp(mTime, 0.0f, duration);

            case Loop::PingPong:
            return Math::Mod(mTime, duration)*((int)(mTime / duration) % 2 == 0 ? 1.0f : -1.0f);

            case Loop::Repeat:
            return Math::Mod(mTime, duration);
        }

        return mTime;
    }

    float IAnimation::GetDuration() const
    {
        return mEndTime - mBeginTime;
    }

    void IAnimation::SetRelTime(float relTime)
    {
        SetTime(Math::Lerp(mBeginTime, mEndTime, relTime));
    }

    float IAnimation::GetRelativeTime() const
    {
        return mTime/(mEndTime - mBeginTime);
    }

    float IAnimation::GetInDurationTime() const
    {
        return mInDurationTime;
    }

    void IAnimation::GoToBegin()
    {
        SetTime(mBeginTime);
    }

    void IAnimation::GoToEnd()
    {
        SetTime(mEndTime);
    }

    void IAnimation::PlayForward()
    {
        SetReverse(false);
        Play();
    }

    void IAnimation::PlayBack()
    {
        SetReverse(true);
        Play();
    }

    void IAnimation::SetReverse(bool reverse)
    {
        mDirection = reverse ? -1.0f : 1.0f;
    }

    bool IAnimation::IsReversed() const
    {
        return mDirection < 0.0f;
    }

    void IAnimation::SetSpeed(float speed)
    {
        mSpeed = speed;
    }

    float IAnimation::GetSpeed() const
    {
        return mSpeed;
    }

    void IAnimation::SetLoop(Loop loop /*= Loop::Repeat*/)
    {
        mLoop = loop;
        OnLoopChanged();
    }

    Loop IAnimation::GetLoop() const
    {
        return mLoop;
    }

    void IAnimation::AddTimeEvent(float time, const Function<void()>& eventFunc)
    {
        mTimeEvents.Add({ time, eventFunc });
    }

    void IAnimation::RemoveTimeEvent(float time)
    {
        mTimeEvents.RemoveAll([&](auto& x) { return Math::Equals(x.first, time); });
    }

    void IAnimation::RemoveTimeEvent(const Function<void()>& eventFunc)
    {
        mTimeEvents.RemoveAll([=](auto& x) { return x.second == eventFunc; });
    }

    void IAnimation::RemoveAllTimeEvents()
    {
        mTimeEvents.Clear();
    }
}
// --- META ---

DECLARE_CLASS(o2::IAnimation, o2__IAnimation);
// --- END META ---
