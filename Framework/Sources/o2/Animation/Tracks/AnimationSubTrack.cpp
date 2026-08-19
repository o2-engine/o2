#include "o2/stdafx.h"
#include "AnimationSubTrack.h"

#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Components/AnimationComponent.h"

namespace o2
{
    AnimationSubTrack::AnimationSubTrack()
    {}

    AnimationSubTrack::AnimationSubTrack(const AnimationSubTrack& other):
        IAnimationTrack(other), mSubTrackBeginTime(other.mSubTrackBeginTime), mSubTrackDuration(other.mSubTrackDuration),
        mSubTrackBeginOffset(other.mSubTrackBeginOffset), mSubTrackEndOffset(other.mSubTrackEndOffset)
    {}

    AnimationSubTrack& AnimationSubTrack::operator=(const AnimationSubTrack& other)
    {
        IAnimationTrack::operator=(other);

        mSubTrackBeginTime = other.mSubTrackBeginTime;
        mSubTrackDuration = other.mSubTrackDuration;
        mSubTrackBeginOffset = other.mSubTrackBeginOffset;
        mSubTrackEndOffset = other.mSubTrackEndOffset;

        onKeysChanged();

        return *this;
    }

    float AnimationSubTrack::GetDuration() const
    {
        return mSubTrackDuration + mSubTrackBeginTime;
    }

    Ref<IAnimationTrack::IPlayer> AnimationSubTrack::CreatePlayer() const
    {
        return mmake<Player>();
    }

    void AnimationSubTrack::SetBeginTime(float beginTime)
    {
        mSubTrackBeginTime = beginTime;

        onKeysChanged();
    }

    float AnimationSubTrack::GetBeginTime() const
    {
        return mSubTrackBeginTime;
    }

    float AnimationSubTrack::GetSubTrackDuration() const
    {
        return mSubTrackDuration;
    }

    void AnimationSubTrack::SetSubTrackBeginOffset(float offset)
    {
        mSubTrackBeginOffset = offset;

        onKeysChanged();
    }

    float AnimationSubTrack::GetSubTrackBeginOffset() const
    {
        return mSubTrackBeginOffset;
    }

    void AnimationSubTrack::SetSubTrackEndOffset(float offset)
    {
        mSubTrackEndOffset = offset;

        onKeysChanged();
    }

    float AnimationSubTrack::GetSubTrackEndOffset() const
    {
        return mSubTrackEndOffset;
    }

    void AnimationSubTrack::OnDeserialized(const DataValue& node)
    {}

    AnimationSubTrack::Player::Player():
        IPlayer()
    {}

    AnimationSubTrack::Player::~Player()
    {}

    void AnimationSubTrack::Player::SetTarget(IAnimation* value)
    {
        mTarget = value;

        // target can be null when the track path doesn't resolve to an IAnimation
        if (!mTarget)
            return;

        mTarget->SetSubControlled(true);

        UpdateSubTrackDuration();
    }

    void AnimationSubTrack::Player::SetTrack(const Ref<AnimationSubTrack>& track)
    {
        mTrack = track;
        IPlayer::SetTrack(track);
    }

    void* AnimationSubTrack::Player::AdjustTargetType(void* target, const Type& type)
    {
        if (type == TypeOf(IAnimation))
            return target;

        const Type* originalType = &type;
        if (originalType->GetUsage() == Type::Usage::StringAccessor)
        {
            originalType = dynamic_cast<const StringPointerAccessorType*>(originalType)->GetReturnType();

            if (originalType->GetUsage() == Type::Usage::Reference)
                originalType = dynamic_cast<const ReferenceType*>(originalType)->GetBaseType();
        }
        else if (originalType->GetUsage() == Type::Usage::Reference)
        {
            auto refType = dynamic_cast<const ReferenceType*>(originalType);
            target = refType->GetObjectRawPtr(target);
            originalType = refType->GetBaseType();
        }

        if (auto objectOriginalType = dynamic_cast<const ObjectType*>(originalType))
        {
            IObject* iobject = objectOriginalType->DynamicCastToIObject(target);
            return dynamic_cast<IAnimation*>(iobject);
        }

        return nullptr;
    }

    const Ref<AnimationSubTrack>& AnimationSubTrack::Player::GetTrackT() const
    {
        return mTrack;
    }

    void AnimationSubTrack::Player::SetTargetVoid(void* target)
    {
        SetTarget((IAnimation*)target);
    }

    void AnimationSubTrack::Player::SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy)
    {}

    void AnimationSubTrack::Player::SetTrack(const Ref<IAnimationTrack>& track)
    {
        SetTrack(DynamicCast<AnimationSubTrack>(track));
    }

    Ref<IAnimationTrack> AnimationSubTrack::Player::GetTrack() const
    {
        return mTrack;
    }

    void AnimationSubTrack::Player::Evaluate()
    {
        if (!mTrack)
            return;

        if (mTarget)
        {
            if (mTime >= mTrack->mSubTrackBeginTime + mTrack->mSubTrackBeginOffset &&
                mTime <= mTrack->mSubTrackBeginTime + mTrack->mSubTrackDuration - mTrack->mSubTrackEndOffset)
            {
                float subTrackTime = mTime - mTrack->mSubTrackBeginTime;
                mTarget->SetTime(subTrackTime);
            }
        }
    }

    void AnimationSubTrack::Player::RegMixer(const Ref<AnimationState>& state, const String& path)
    {
        state->mOwner.Lock()->RegSubTrack(Ref(this), path, state);
    }

    void AnimationSubTrack::Player::UpdateSubTrackDuration()
    {
        if (mTrack && mTarget)
        {
            float targetDuration = mTarget->GetDuration();
            if (!Math::Equals(mTrack->mSubTrackDuration, targetDuration))
            {
                // Notify the clip so its duration and bound players pick up the real target length
                mTrack->mSubTrackDuration = targetDuration;
                mTrack->onKeysChanged();
            }
        }
    }

}
// --- META ---

DECLARE_CLASS(o2::AnimationSubTrack, o2__AnimationSubTrack);

DECLARE_CLASS(o2::AnimationSubTrack::Player, o2__AnimationSubTrack__Player);
// --- END META ---
