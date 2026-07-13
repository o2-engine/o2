#include "o2/stdafx.h"
#include "AnimationVec3FTrack.h"

#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Math/Interpolation.h"

namespace o2
{
    AnimationTrack<Vec3F>::AnimationTrack()
    {}

    AnimationTrack<Vec3F>::AnimationTrack(const AnimationTrack<Vec3F>& other) :
        IAnimationTrack(other), mKeys(other.mKeys)
    {}

    AnimationTrack<Vec3F>& AnimationTrack<Vec3F>::operator=(const AnimationTrack<Vec3F>& other)
    {
        IAnimationTrack::operator =(other);
        mKeys = other.mKeys;

        onKeysChanged();

        return *this;
    }

    Vec3F AnimationTrack<Vec3F>::GetValue(float position) const
    {
        int cacheKey = 0, cacheKeyApporx = 0;
        return GetValue(position, true, cacheKey, cacheKeyApporx);
    }

    Vec3F AnimationTrack<Vec3F>::GetValue(float position, bool direction, int& cacheKey, int& cacheKeyApprox) const
    {
        int count = mKeys.Count();

        if (count == 1)
            return mKeys[0].value;
        else if (count == 0)
            return Vec3F();

        int keyLeftIdx = -1, rightKeyIdx = -1;
        SearchKey(mKeys, count, position, keyLeftIdx, rightKeyIdx, direction, cacheKey);

        if (keyLeftIdx < 0)
            return Vec3F();

        const Key& leftKey = mKeys[keyLeftIdx];
        const Key& rightKey = mKeys[rightKeyIdx];

        float coef = (position - leftKey.position)/(rightKey.position - leftKey.position);
        return Math::Lerp(leftKey.value, rightKey.value, coef);
    }

    void AnimationTrack<Vec3F>::BeginKeysBatchChange()
    {
        mBatchChange = true;
    }

    void AnimationTrack<Vec3F>::CompleteKeysBatchingChange()
    {
        mBatchChange = false;
        mChangedKeys = false;
    }

    float AnimationTrack<Vec3F>::GetDuration() const
    {
        return mKeys.IsEmpty() ? 0.0f : mKeys.Last().position;
    }

    Ref<IAnimationTrack::IPlayer> AnimationTrack<Vec3F>::CreatePlayer() const
    {
        return mmake<Player>();
    }

    void AnimationTrack<Vec3F>::AddKeys(const Vector<Key>& keys)
    {
        for (auto& key : keys)
            AddKey(key);

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();
    }

    int AnimationTrack<Vec3F>::AddKey(const Key& key)
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

    int AnimationTrack<Vec3F>::AddKey(const Key& key, float position)
    {
        Key newkey = key;
        newkey.position = position;
        return AddKey(newkey);
    }

    int AnimationTrack<Vec3F>::AddKey(float position, const Vec3F& value)
    {
        return AddKey(Key(position, value));
    }

    bool AnimationTrack<Vec3F>::RemoveKey(float position)
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

    bool AnimationTrack<Vec3F>::RemoveKeyAt(int idx)
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

    void AnimationTrack<Vec3F>::RemoveAllKeys()
    {
        mKeys.Clear();
        onKeysChanged();
    }

    bool AnimationTrack<Vec3F>::ContainsKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return true;
        }

        return false;
    }

    const Vector<AnimationTrack<Vec3F>::Key>& AnimationTrack<Vec3F>::GetKeys() const
    {
        return mKeys;
    }

    AnimationTrack<Vec3F>::Key AnimationTrack<Vec3F>::GetKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return key;
        }

        return Key();
    }

    AnimationTrack<Vec3F>::Key AnimationTrack<Vec3F>::GetKeyAt(int idx) const
    {
        return mKeys[idx];
    }

    void AnimationTrack<Vec3F>::SetKey(int idx, const Key& key)
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return;

        mKeys[idx] = key;

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();
    }

    AnimationTrack<Vec3F>::Key AnimationTrack<Vec3F>::FindKey(UInt64 uid) const
    {
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return key;
        }

        return Key();
    }

    int AnimationTrack<o2::Vec3F>::FindKeyIdx(UInt64 uid) const
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

    void AnimationTrack<Vec3F>::SetKeys(const Vector<Key>& keys)
    {
        mKeys = keys;

        if (mBatchChange)
            mChangedKeys = true;
        else
            onKeysChanged();
    }

    AnimationTrack<Vec3F>::Key AnimationTrack<Vec3F>::operator[](float position) const
    {
        return GetKey(position);
    }

    Vector<AnimationTrack<Vec3F>::Key> AnimationTrack<Vec3F>::GetKeysNonContant()
    {
        return mKeys;
    }

    AnimationTrack<Vec3F> AnimationTrack<Vec3F>::Linear(const Vec3F& begin, const Vec3F& end, float duration /*= 1.0f*/)
    {
        AnimationTrack<Vec3F> res;
        res.AddKey(0.0f, begin);
        res.AddKey(duration, end);
        return res;
    }

    AnimationTrack<Vec3F>::Key::Key() :
        uid(Math::Random()), position(0)
    {}

    AnimationTrack<Vec3F>::Key::Key(const Vec3F& value) :
        uid(Math::Random()), position(0), value(value)
    {}

    AnimationTrack<Vec3F>::Key& AnimationTrack<Vec3F>::Key::operator=(const Vec3F& value)
    {
        this->value = value;
        return *this;
    }

    AnimationTrack<Vec3F>::Key::operator Vec3F() const
    {
        return value;
    }

    AnimationTrack<Vec3F>::Key::Key(float position, const Vec3F& value) :
        uid(Math::Random()), position(position), value(value)
    {}

    AnimationTrack<Vec3F>::Key::Key(const Key& other) :
        uid(other.uid), position(other.position), value(other.value)
    {}

    AnimationTrack<Vec3F>::Key& AnimationTrack<Vec3F>::Key::operator=(const Key& other)
    {
        uid = other.uid;
        position = other.position;
        value = other.value;

        return *this;
    }

    bool AnimationTrack<Vec3F>::Key::operator==(const Key& other) const
    {
        return position == other.position && value == other.value;
    }

    AnimationTrack<Vec3F>::Player::Player():
        IPlayer()
    {}

    AnimationTrack<Vec3F>::Player::~Player()
    {}

    AnimationTrack<Vec3F>::Player::operator Vec3F() const
    {
        return mCurrentValue;
    }

    void AnimationTrack<Vec3F>::Player::SetTarget(Vec3F* value)
    {
        mTargetProxy = nullptr;
        mTarget = value;
    }

    void AnimationTrack<Vec3F>::Player::SetTargetProxy(const Ref<IValueProxy<Vec3F>>& proxy)
    {
        mTarget = nullptr;
        mTargetProxy = proxy;
    }

    void AnimationTrack<Vec3F>::Player::SetTrack(const Ref<AnimationTrack<Vec3F>>& track)
    {
        mTrack = track;
        IPlayer::SetTrack(track);
    }

    void AnimationTrack<Vec3F>::Player::SetTargetVoid(void* target)
    {
        SetTarget((Vec3F*)target);
    }

    void AnimationTrack<Vec3F>::Player::SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy)
    {
        SetTargetProxy(DynamicCast<IValueProxy<Vec3F>>(targetProxy));
    }

    void AnimationTrack<Vec3F>::Player::SetTrack(const Ref<IAnimationTrack>& track)
    {
        SetTrack(DynamicCast<AnimationTrack<Vec3F>>(track));
    }

    Vec3F AnimationTrack<Vec3F>::Player::GetValue() const
    {
        return mCurrentValue;
    }

    const Ref<AnimationTrack<Vec3F>>& AnimationTrack<Vec3F>::Player::GetTrackT() const
    {
        return mTrack;
    }

    Ref<IAnimationTrack> AnimationTrack<Vec3F>::Player::GetTrack() const
    {
        return mTrack;
    }

    void AnimationTrack<Vec3F>::Player::Evaluate()
    {
        mPrevInDurationTime = mInDurationTime;

        if (mTrack->GetKeys().IsEmpty())
        {
            mCurrentValue = mTarget ? *mTarget : mTargetProxy ? mTargetProxy->GetValue() : Vec3F();
            return;
        }

        mCurrentValue = mTrack->GetValue(mInDurationTime, mInDurationTime > mPrevInDurationTime,
                                         mPrevKey, mPrevKeyApproximation);

        if (mTarget)
            *mTarget = mCurrentValue;
        else if (mTargetProxy)
            mTargetProxy->SetValue(mCurrentValue);
    }

    void AnimationTrack<Vec3F>::Player::RegMixer(const Ref<AnimationState>& state, const String& path)
    {
        state->mOwner.Lock()->RegValueTrack<Vec3F>(Ref(this), path, state);
    }
}
// --- META ---

DECLARE_CLASS(o2::AnimationTrack<o2::Vec3F>, o2__AnimationTrack_o2__Vec3F_);

DECLARE_CLASS(o2::AnimationTrack<o2::Vec3F>::Player, o2__AnimationTrack_o2__Vec3F___Player);

DECLARE_CLASS(o2::AnimationTrack<o2::Vec3F>::Key, o2__AnimationTrack_o2__Vec3F___Key);
// --- END META ---
