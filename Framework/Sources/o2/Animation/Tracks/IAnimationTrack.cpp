#include "o2/stdafx.h"
#include "AnimationTrack.h"

#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/AnimationState.h"

namespace o2
{
    //FORWARD_REF_IMPL(AnimationClip);
    FORWARD_REF_IMPL(AnimationPlayer);
    //FORWARD_REF_IMPL(AnimationState);

    void IAnimationTrack::IPlayer::SetTrack(const Ref<IAnimationTrack>& track)
    {
        if (!track)
            return;

        mLoop = track->loop;
        mDuration = track->GetDuration();
        mBeginTime = 0;
        mEndTime = mDuration;

        track->onKeysChanged += [=]()
        {
            mDuration = track->GetDuration();
            mEndTime = mDuration;
        };
    }

    void IAnimationTrack::IPlayer::ForceSetTime(float time, float duration)
    {
        float lastTime = mTime;
        mTime = time;

        float lastInDurationTime = mInDurationTime;

        if (mLoop == Loop::None)
        {
            mInDurationTime = Math::Clamp(mTime, mBeginTime, mEndTime);
            Evaluate();

            if (lastTime < mTime)
            {
                if (mEndTime > Math::Min(mTime, lastTime) && mEndTime <= Math::Max(mTime, lastTime))
                {
                    onStop();
                    onPlayed();
                }
            }
            else
            {
                if (mBeginTime > Math::Min(mTime, lastTime) && mBeginTime <= Math::Max(mTime, lastTime))
                {
                    onStop();
                    onPlayed();
                }
            }
        }
        else if (mLoop == Loop::Repeat)
        {
            float x;
            if (mTime > 0)
                mInDurationTime = modff(mTime/mDuration, &x)*mDuration;
            else
                mInDurationTime = (1.0f - modff(-mTime/mDuration, &x))*mDuration;

            mInDurationTime = Math::Clamp(mInDurationTime, mBeginTime, mEndTime);

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

            mInDurationTime = Math::Clamp(mInDurationTime, mBeginTime, mEndTime);

            Evaluate();
        }

        onUpdate(mTime);

        float eventCheckBeg = Math::Min(lastInDurationTime, mInDurationTime);
        float eventCheckEnd = Math::Max(lastInDurationTime, mInDurationTime);
        for (auto& kv : mTimeEvents)
        {
            if (kv.first > eventCheckBeg && kv.first <= eventCheckEnd)
                kv.second.Invoke();
        }
    }

    const WeakRef<AnimationPlayer>& IAnimationTrack::IPlayer::GetOwnerPlayer() const
    {
        return mOwnerPlayer;
    }

    IAnimationTrack& IAnimationTrack::operator=(const IAnimationTrack& other)
    {
        loop = other.loop;
        return *this;
    }

    const WeakRef<AnimationClip>& IAnimationTrack::GetOwnerClip() const
    {
        return mOwnerClip;
    }

}
// --- META ---

DECLARE_CLASS(o2::IAnimationTrack, o2__IAnimationTrack);

DECLARE_CLASS(o2::IAnimationTrack::IPlayer, o2__IAnimationTrack__IPlayer);
// --- END META ---
