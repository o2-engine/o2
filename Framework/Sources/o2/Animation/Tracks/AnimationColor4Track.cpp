#include "o2/stdafx.h"
#include "AnimationColor4Track.h"

#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Math/Interpolation.h"

namespace o2
{
    AnimationTrack<Color4>::AnimationTrack()
    {}

    AnimationTrack<Color4>::AnimationTrack(const AnimationTrack<Color4>& other) :
        IAnimationTrack(other), mKeys(other.mKeys)
    {}

    AnimationTrack<Color4>& AnimationTrack<Color4>::operator=(const AnimationTrack<Color4>& other)
    {
        IAnimationTrack::operator =(other);
        mKeys = other.mKeys;

        onKeysChanged();

        return *this;
    }

    Color4 AnimationTrack<Color4>::GetValue(float position) const
    {
        int cacheKey = 0, cacheKeyApporx = 0;
        return GetValue(position, true, cacheKey, cacheKeyApporx);
    }

    Color4 AnimationTrack<Color4>::GetValue(float position, bool direction, int& cacheKey, int& cacheKeyApprox) const
    {
        int count = mKeys.Count();

        if (count == 1)
            return mKeys[0].value;
        else if (count == 0)
            return Color4();

        int prevCacheKey = cacheKey;
        int keyLeftIdx = -1, rightKeyIdx = -1;
        SearchKey(mKeys, count, position, keyLeftIdx, rightKeyIdx, direction, cacheKey);

        if (keyLeftIdx < 0)
            return Color4();

        const Key& leftKey = mKeys[keyLeftIdx];
        const Key& rightKey = mKeys[rightKeyIdx];

        float coef = (position - leftKey.position)/(rightKey.position - leftKey.position);
        return Math::Lerp(leftKey.value, rightKey.value, coef);
    }

    void AnimationTrack<Color4>::BeginKeysBatchChange()
    {
        mBatchChange = true;
    }

    void AnimationTrack<Color4>::CompleteKeysBatchingChange()
    {
        mBatchChange = false;
        mChangedKeys = false;
    }

    float AnimationTrack<Color4>::GetDuration() const
    {
        return mKeys.IsEmpty() ? 0.0f : mKeys.Last().position;
    }

    Ref<IAnimationTrack::IPlayer> AnimationTrack<Color4>::CreatePlayer() const
    {
        return mmake<Player>();
    }

    void AnimationTrack<Color4>::AddKeys(const Vector<Key>& keys)
    {
        for (auto& key : keys)
            AddKey(key);

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();
    }

    int AnimationTrack<Color4>::AddKey(const Key& key)
    {
        int pos = mKeys.Count();
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].position > key.position)
            {
                pos = i;
                break;
            }
        }

        pos = Math::Clamp(pos, 0, mKeys.Count());
        mKeys.Insert(key, pos);

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();

        return pos;
    }

    int AnimationTrack<Color4>::AddKey(const Key& key, float position)
    {
        Key newkey = key;
        newkey.position = position;
        return AddKey(newkey);
    }

    int AnimationTrack<Color4>::AddKey(float position, const Color4& value)
    {
        return AddKey(Key(position, value));
    }

    bool AnimationTrack<Color4>::RemoveKey(float position)
    {
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (Math::Equals(mKeys[i].position, position))
            {
                mKeys.RemoveAt(i);

                if (mBatchChange)
                    mChangedKeys = true;
                else
                    onKeysChanged();

                return true;
            }
        }

        return false;
    }

    bool AnimationTrack<Color4>::RemoveKeyAt(int idx)
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return false;

        mKeys.RemoveAt(idx);

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();

        return true;
    }

    void AnimationTrack<Color4>::RemoveAllKeys()
    {
        mKeys.Clear();
        onKeysChanged();
    }

    bool AnimationTrack<Color4>::ContainsKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return true;
        }

        return false;
    }

    const Vector<AnimationTrack<Color4>::Key>& AnimationTrack<Color4>::GetKeys() const
    {
        return mKeys;
    }

    AnimationTrack<Color4>::Key AnimationTrack<Color4>::GetKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return key;
        }

        return Key();
    }

    AnimationTrack<Color4>::Key AnimationTrack<Color4>::GetKeyAt(int idx) const
    {
        return mKeys[idx];
    }

    void AnimationTrack<Color4>::SetKey(int idx, const Key& key)
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return;

        mKeys[idx] = key;

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();
    }

    AnimationTrack<Color4>::Key AnimationTrack<Color4>::FindKey(UInt64 uid) const
    {
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return key;
        }

        return Key();
    }

    int AnimationTrack<o2::Color4>::FindKeyIdx(UInt64 uid) const
    {
        int idx = 0;
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return idx;

            idx++;
        }

        return -1;
    }

    void AnimationTrack<Color4>::SetKeys(const Vector<Key>& keys)
    {
        mKeys = keys;

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();
    }

    AnimationTrack<Color4>::Key AnimationTrack<Color4>::operator[](float position) const
    {
        return GetKey(position);
    }

    Vector<AnimationTrack<Color4>::Key> AnimationTrack<Color4>::GetKeysNonContant()
    {
        return mKeys;
    }

    AnimationTrack<Color4> AnimationTrack<Color4>::Linear(const Color4& begin, const Color4& end, float duration /*= 1.0f*/)
    {
        AnimationTrack<Color4> res;
        res.AddKey(0.0f, begin);
        res.AddKey(duration, end);
        return res;
    }

    AnimationTrack<Color4>::Key::Key() :
        uid(Math::Random()), position(0)
    {}

    AnimationTrack<Color4>::Key::Key(const Color4& value) :
        uid(Math::Random()), position(0), value(value)
    {}

    AnimationTrack<Color4>::Key& AnimationTrack<Color4>::Key::operator=(const Color4& value)
    {
        this->value = value;
        return *this;
    }

    AnimationTrack<Color4>::Key::operator Color4() const
    {
        return value;
    }

    AnimationTrack<Color4>::Key::Key(float position, const Color4& value) :
        uid(Math::Random()), position(position), value(value)
    {}

    AnimationTrack<Color4>::Key::Key(const Key& other) :
        uid(other.uid), position(other.position), value(other.value)
    {}

    AnimationTrack<Color4>::Key& AnimationTrack<Color4>::Key::operator=(const Key& other)
    {
        uid = other.uid;
        position = other.position;
        value = other.value;

        return *this;
    }

    bool AnimationTrack<Color4>::Key::operator==(const Key& other) const
    {
        return position == other.position && value == other.value;
    }

    AnimationTrack<Color4>::Player::Player():
        IPlayer()
    {}

    AnimationTrack<Color4>::Player::~Player()
    {}

    AnimationTrack<Color4>::Player::operator Color4() const
    {
        return mCurrentValue;
    }

    void AnimationTrack<Color4>::Player::SetTarget(Color4* value)
    {
        mTargetProxy = nullptr;
        mTarget = value;
    }

    void AnimationTrack<Color4>::Player::SetTargetProxy(const Ref<IValueProxy<Color4>>& proxy)
    {
        mTarget = nullptr;
        mTargetProxy = proxy;
    }

    void AnimationTrack<Color4>::Player::SetTrack(const Ref<AnimationTrack<Color4>>& track)
    {
        mTrack = track;
        IPlayer::SetTrack(track);
    }

    void AnimationTrack<Color4>::Player::SetTargetVoid(void* target)
    {
        SetTarget((Color4*)target);
    }

    void AnimationTrack<Color4>::Player::SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy)
    {
        SetTargetProxy(DynamicCast<IValueProxy<Color4>>(targetProxy));
    }

    void AnimationTrack<Color4>::Player::SetTrack(const Ref<IAnimationTrack>& track)
    {
        SetTrack(DynamicCast<AnimationTrack<Color4>>(track));
    }

    Color4 AnimationTrack<Color4>::Player::GetValue() const
    {
        return mCurrentValue;
    }

    const Ref<AnimationTrack<Color4>>& AnimationTrack<Color4>::Player::GetTrackT() const
    {
        return mTrack;
    }

    Ref<IAnimationTrack> AnimationTrack<Color4>::Player::GetTrack() const
    {
        return mTrack;
    }

    void AnimationTrack<Color4>::Player::Evaluate()
    {

        mPrevInDurationTime = mInDurationTime;

        if (mTrack->GetKeys().IsEmpty())
        {
            mCurrentValue = mTarget ? *mTarget : mTargetProxy ? mTargetProxy->GetValue() : Color4();
            return;
        }

        mCurrentValue = mTrack->GetValue(mInDurationTime, mInDurationTime > mPrevInDurationTime, 
                                         mPrevKey, mPrevKeyApproximation);

        if (mTarget)
            *mTarget = mCurrentValue;
        else if (mTargetProxy)
            mTargetProxy->SetValue(mCurrentValue);
    }

    void AnimationTrack<Color4>::Player::RegMixer(const Ref<AnimationState>& state, const String& path)
    {
        state->mOwner.Lock()->RegValueTrack<Color4>(Ref(this), path, state);
    }
}
// --- META ---

DECLARE_CLASS(o2::AnimationTrack<o2::Color4>, o2__AnimationTrack_o2__Color4_);

DECLARE_CLASS(o2::AnimationTrack<o2::Color4>::Player, o2__AnimationTrack_o2__Color4___Player);

DECLARE_CLASS(o2::AnimationTrack<o2::Color4>::Key, o2__AnimationTrack_o2__Color4___Key);
// --- END META ---
