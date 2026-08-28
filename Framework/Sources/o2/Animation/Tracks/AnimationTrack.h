#pragma once

#include "o2/Animation/IAnimation.h"
#include "o2/Animation/Tracks/IAnimationTrack.h"
#include "o2/Utils/Math/Curve.h"
#include "o2/Utils/Math/Interpolation.h"
#include "o2/Utils/Memory/MemoryManager.h"
#include "o2/Utils/Tools/KeySearch.h"

namespace o2
{
    FORWARD_CLASS_REF(AnimationState);
    
    // ------------------------
    // Template Animation track
    // ------------------------
    template<typename _type>
    class AnimationTrack: public IAnimationTrack
    {
    public:
        typedef _type ValueType;
        class Key;

    public:
        PROPERTIES(AnimationTrack<_type>);
        PROPERTY(Vector<Key>, keys, SetKeys, GetKeysNonContant); // Keys property

    public:
        // Default constructor
        AnimationTrack();

        // Copy-constructor
        AnimationTrack(const AnimationTrack<_type>& other);

        // Assign operator
        AnimationTrack<_type>& operator=(const AnimationTrack<_type>& other);

        // Returns value at time
        _type GetValue(float position) const;

        // Returns value at time, with cached state
        _type GetValue(float position, bool direction, int& cacheKey, int& cacheKeyApprox) const;

        // Called when beginning keys batch change. After this call all keys modifications will not be update approximation
        // Used for optimizing many keys change
        void BeginKeysBatchChange() override;

        // Called when keys batch change completed. Updates approximation
        void CompleteKeysBatchingChange() override;

        // Returns track duration
        float GetDuration() const override;

        // Creates track-type specific player
        Ref<IPlayer> CreatePlayer() const override;

        // Adds key with smoothing
        void AddKeys(Vector<Key> keys, float smooth = 1.0f);

        // Adds single key
        int AddKey(const Key& key);

        // Adds key at position
        int AddKey(const Key& key, float position);

        // Adds and smooths key
        int AddSmoothKey(const Key& key, float smooth);

        // Adds key
        int AddKey(float position, const _type& value, float leftCoef, float leftCoefPosition,
                   float rightCoef, float rightCoefPosition);

        // Adds key at position with value and smoothing
        int AddKey(float position, const _type& value, float smooth = 1.0f);

        // Removes key at position
        bool RemoveKey(float position);

        // Removes key by index
        bool RemoveKeyAt(int idx);

        // Removes all keys
        void RemoveAllKeys();

        // Returns true if animation contains key at position
        bool ContainsKey(float position) const;

        // Returns keys array
        const Vector<Key>& GetKeys() const;

        // Sets key at position
        void SetKey(int idx, const Key& key);

        // Returns key at position
        Key GetKey(float position) const;

        // Returns key at index
        Key GetKeyAt(int idx) const;

        // Returns key by uid
        Key FindKey(UInt64 uid) const;

        // Returns key index by uid
        int FindKeyIdx(UInt64 uid) const;

        // Sets keys
        void SetKeys(const Vector<Key>& keys);

        // Smooths key at position
        void SmoothKey(float position, float smooth);

        // Returns key by position
        Key operator[](float position) const;

        // Returns parametric specified Animation track
        // Sample: Parametric(someBegin, someEnd, 1.0f, 0.0f, 0.4f, 1.0f, 0.6f) 
        static AnimationTrack<_type> Parametric(const _type& begin, const _type& end, float duration,
                                                float beginCoef, float beginCoefPosition,
                                                float endCoef, float endCoefPosition);

        // Returns tween animation from begin to end in duration with ease in
        static AnimationTrack<_type> EaseIn(const _type& begin, const _type& end, float duration = 1.0f);

        // Returns tween animation from begin to end in duration with ease out
        static AnimationTrack<_type> EaseOut(const _type& begin, const _type& end, float duration = 1.0f);

        // Returns tween animation from begin to end in duration with ease in-out
        static AnimationTrack<_type> EaseInOut(const _type& begin, const _type& end, float duration = 1.0f);

        // Returns tween animation from begin to end in duration with linear transition
        static AnimationTrack<_type> Linear(const _type& begin, const _type& end, float duration = 1.0f);

        SERIALIZABLE(AnimationTrack<_type>);
        CLONEABLE_REF(AnimationTrack<_type>);

    public:
        // ----------------------
        // Animation track player
        // ----------------------
        class Player: public IPlayer
        {
        public:
            PROPERTIES(Player);
            GETTER(_type, value, GetValue);    // Current value getter
            SETTER(_type*, target, SetTarget); // Bind target setter

        public:
            // Default constructor
            Player();

            // Destructor
            ~Player();

            // Value type cast operator
            operator _type() const;

            // Sets target pointer
            void SetTarget(_type* value);

            // Sets target property pointer
            void SetTargetProxy(const Ref<IValueProxy<_type>>& proxy);

            // Sets animation track
            void SetTrack(const Ref<AnimationTrack<_type>>& track);

            // Returns animation track
            const Ref<AnimationTrack<_type>>& GetTrackT() const;

            // Sets target by void pointer
            void SetTargetVoid(void* target) override;

            // Sets target property by void pointer
            void SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy) override;

            // Sets animation track
            void SetTrack(const Ref<IAnimationTrack>& track) override;

            // Returns animation track
            Ref<IAnimationTrack> GetTrack() const override;

            // Returns current value
            _type GetValue() const;

            IOBJECT(Player);

        protected:
            Ref<AnimationTrack<_type>> mTrack; // Animation track

            _type mCurrentValue; // Current animation track

            float mPrevInDurationTime = 0.0f; // Previous evaluation in duration time
            int   mPrevKey = 0;               // Previous evaluation key index
            int   mPrevKeyApproximation = 0;  // Previous evaluation key approximation index

            _type*                  mTarget = nullptr; // Animation target value pointer
            Ref<IValueProxy<_type>> mTargetProxy;      // Animation target proxy pointer

        protected:
            // Evaluates value
            void Evaluate() override;

            // Registering this in animation component values mixer
            void RegMixer(const Ref<AnimationState>& state, const String& path) override;
        };

        // -------------
        // Animation key
        // -------------
        class Key: public ISerializable
        {
        public:
            UInt64 uid;                  // Random unique id @SERIALIZABLE
            float  position;             // Position on time line, in seconds @SERIALIZABLE
            _type  value = _type();      // Value @SERIALIZABLE
            float  leftSupportValue;     // Transition curve coefficient for previous animation segment @SERIALIZABLE
            float  leftSupportPosition;  // Transition curve coefficient position for previous animation segment (must be in 0...1) @SERIALIZABLE
            float  rightSupportValue;    // Transition curve coefficient for next animation segment @SERIALIZABLE
            float  rightSupportPosition; // Transition curve coefficient position for next animation segment (must be in 0...1)@SERIALIZABLE

        public:
            // Default constructor
            Key();

            // Constructor from value
            Key(const _type& value);

            // Constructor from position and value
            Key(float position, const _type& value);

            // Constructor
            Key(float position, const _type& value, float leftSupportValue, float leftSupportPosition,
                float rightSupportValue, float rightSupportPosition);

            // Copy-constructor
            Key(const Key& other);

            // Assign operator from other key
            Key& operator=(const Key& other);

            // Assign operator from value
            Key& operator=(const _type& value);

            // Value type cast operator
            operator _type() const;

            // Equals operator
            bool operator==(const Key& other) const;

            SERIALIZABLE(Key);

        public:
            static const int   mApproxValuesCount = 10;                // Transition curve approximation values count
            ApproximationValue mCurveApproxValues[mApproxValuesCount]; // Transition curve approximation values
        };

    protected:
        bool mBatchChange = false; // It is true when began batch change
        bool mChangedKeys = false; // It is true when some keys changed during batch change

        Vector<Key> mKeys; // Animation keys @SERIALIZABLE

    protected:
        // Returns keys (for property)
        Vector<Key> GetKeysNonContant();

        // Updates keys approximation
        void UpdateApproximation();

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;
    };
}

namespace o2
{
    // ------------------------------
    // Animation track implementation 
    // ------------------------------

    template<typename _type>
    AnimationTrack<_type>::AnimationTrack()
    {}

    template<typename _type>
    AnimationTrack<_type>::AnimationTrack(const AnimationTrack<_type>& other):
        IAnimationTrack(other), mKeys(other.mKeys)
    {}

    template<typename _type>
    AnimationTrack<_type>& AnimationTrack<_type>::operator=(const AnimationTrack<_type>& other)
    {
        IAnimationTrack::operator=(other);
        mKeys = other.mKeys;

        onKeysChanged();

        return *this;
    }

    template<typename _type>
    _type AnimationTrack<_type>::GetValue(float position) const
    {
        int cacheKey = 0, cacheKeyApprox = 0;
        return GetValue(position, true, cacheKey, cacheKeyApprox);
    }

    template<typename _type>
    _type AnimationTrack<_type>::GetValue(float position, bool direction, int& cacheKey, int& cacheKeyApprox) const
    {
        int count = mKeys.Count();

        if (count == 1)
            return mKeys[0].value;
        else if (count == 0)
            return _type();

        // outside the keys the track holds its end values instead of extrapolating the last segment
        position = Math::Clamp(position, mKeys[0].position, mKeys[count - 1].position);

        int prevCacheKey = cacheKey;
        int keyLeftIdx = -1, keyRightIdx = -1;
        SearchKey(mKeys, count, position, keyLeftIdx, keyRightIdx, direction, cacheKey);

        if (keyLeftIdx < 0)
            return _type();

        const Key& leftKey = mKeys[keyLeftIdx];
        const Key& rightKey = mKeys[keyRightIdx];

        int segLeftIdx = 0;
        int segRightIdx = 1;

        if (keyLeftIdx != prevCacheKey)
            cacheKeyApprox = 0;

        SearchKey(rightKey.mCurveApproxValues, Key::mApproxValuesCount, position, segLeftIdx, segRightIdx, direction, cacheKeyApprox);

        const ApproximationValue& segLeft = rightKey.mCurveApproxValues[segLeftIdx];
        const ApproximationValue& segRight = rightKey.mCurveApproxValues[segRightIdx];

        float dist = segRight.position - segLeft.position;
        float coef = (position - segLeft.position) / dist;
        float curveCoef = Math::Lerp(segLeft.value, segRight.value, coef);

        return Math::Lerp(leftKey.value, rightKey.value, curveCoef);
    }

    template<typename _type>
    void AnimationTrack<_type>::BeginKeysBatchChange()
    {
        mBatchChange = true;
    }

    template<typename _type>
    void AnimationTrack<_type>::CompleteKeysBatchingChange()
    {
        UpdateApproximation();
        mBatchChange = false;
        mChangedKeys = false;
    }

    template<typename _type>
    float AnimationTrack<_type>::GetDuration() const
    {
        return mKeys.IsEmpty() ? 0.0f : mKeys.Last().position;
    }

    template<typename _type>
    Ref<IAnimationTrack::IPlayer> AnimationTrack<_type>::CreatePlayer() const
    {
        return mmake<Player>();
    }

    template<typename _type>
    void AnimationTrack<_type>::AddKeys(Vector<Key> keys, float smooth /*= 1.0f*/)
    {
        for (auto& key : keys)
            AddKey(key, smooth);

        for (auto& key : keys)
            SmoothKey(key.position, smooth);

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    template<typename _type>
    int AnimationTrack<_type>::AddKey(const Key& key)
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
            UpdateApproximation();

        return pos;
    }

    template<typename _type>
    int AnimationTrack<_type>::AddKey(const Key& key, float position)
    {
        Key newkey = key;
        newkey.position = position;
        return AddKey(newkey);
    }

    template<typename _type>
    int AnimationTrack<_type>::AddSmoothKey(const Key& key, float smooth)
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

        SmoothKey(key.position, smooth);

        return pos;
    }

    template<typename _type>
    int AnimationTrack<_type>::AddKey(float position, const _type& value,
                                      float leftCoef, float leftCoefPosition,
                                      float rightCoef, float rightCoefPosition)
    {
        return AddKey(Key(position, value, leftCoef, leftCoefPosition, rightCoef, rightCoefPosition));
    }

    template<typename _type>
    int AnimationTrack<_type>::AddKey(float position, const _type& value, float smooth /*= 1.0f*/)
    {
        return AddSmoothKey(Key(position, value, 0.0f, 0.0f, 1.0f, 1.0f), smooth);
    }

    template<typename _type>
    typename AnimationTrack<_type>::Key AnimationTrack<_type>::GetKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return key;
        }

        return Key();
    }

    template<typename _type>
    typename AnimationTrack<_type>::Key AnimationTrack<_type>::GetKeyAt(int idx) const
    {
        return mKeys[idx];
    }

    template<typename _type>
    void AnimationTrack<_type>::SetKey(int idx, const Key& key)
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return;

        mKeys[idx] = key;

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    template<typename _type>
    typename AnimationTrack<_type>::Key AnimationTrack<_type>::FindKey(UInt64 uid) const
    {
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return key;
        }

        return Key();
    }

    template<typename _type>
    int AnimationTrack<_type>::FindKeyIdx(UInt64 uid) const
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

    template<typename _type>
    bool AnimationTrack<_type>::RemoveKey(float position)
    {
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (Math::Equals(mKeys[i].position, position))
            {
                mKeys.RemoveAt(i);

                if (mBatchChange)
                    mChangedKeys = true;
                else
                    UpdateApproximation();

                return true;
            }
        }

        return false;
    }

    template<typename _type>
    bool AnimationTrack<_type>::RemoveKeyAt(int idx)
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return false;

        mKeys.RemoveAt(idx);

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return true;
    }


    template<typename _type>
    void AnimationTrack<_type>::RemoveAllKeys()
    {
        mKeys.Clear();
        onKeysChanged();
    }

    template<typename _type>
    bool AnimationTrack<_type>::ContainsKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return true;
        }

        return false;
    }

    template<typename _type>
    const Vector<typename AnimationTrack<_type>::Key>& AnimationTrack<_type>::GetKeys() const
    {
        return mKeys;
    }

    template<typename _type>
    void AnimationTrack<_type>::SetKeys(const Vector<Key>& keys)
    {
        mKeys = keys;

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    template<typename _type>
    void AnimationTrack<_type>::SmoothKey(float position, float smooth)
    {
        int pos = 0;

        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (Math::Equals(mKeys[i].position, position))
            {
                pos = i;
                break;
            }
        }

        float baseSmooth = 0.4f;
        float resSmooth = baseSmooth*smooth;

        mKeys[pos].leftSupportValue = 1.0f;
        mKeys[pos].leftSupportPosition = 1.0f - resSmooth;

        mKeys[pos].rightSupportValue = 0.0f;
        mKeys[pos].rightSupportPosition = resSmooth;

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    template<typename _type>
    typename AnimationTrack<_type>::Key AnimationTrack<_type>::operator[](float position) const
    {
        return GetKey(position);
    }

    template<typename _type>
    Vector<typename AnimationTrack<_type>::Key> AnimationTrack<_type>::GetKeysNonContant()
    {
        return mKeys;
    }

    template<typename _type>
    void AnimationTrack<_type>::UpdateApproximation()
    {
        for (int i = 1; i < mKeys.Count(); i++)
        {
            Key& beginKey = mKeys[i - 1];
            Key& endKey = mKeys[i];

            beginKey.leftSupportPosition = Math::Clamp01(beginKey.leftSupportPosition);
            endKey.rightSupportPosition = Math::Clamp01(endKey.rightSupportPosition);

            Vec2F curvea(beginKey.position, 0.0f);
            Vec2F curveb(Math::Lerp(beginKey.position, endKey.position, beginKey.rightSupportPosition), beginKey.rightSupportValue);
            Vec2F curvec(Math::Lerp(beginKey.position, endKey.position, endKey.leftSupportPosition), endKey.leftSupportValue);
            Vec2F curved(endKey.position, 1.0f);

            for (int j = 0; j < Key::mApproxValuesCount; j++)
            {
                float coef = (float)j/(float)(Key::mApproxValuesCount - 1); \
                    endKey.mCurveApproxValues[j] = Bezier(curvea, curveb, curvec, curved, coef);
            }
        }

        onKeysChanged();
    }

    template<typename _type>
    void AnimationTrack<_type>::OnDeserialized(const DataValue& node)
    {
        UpdateApproximation();
    }

    template<typename _type>
    AnimationTrack<_type> AnimationTrack<_type>::Parametric(const _type& begin, const _type& end, float duration,
                                                            float beginCoef, float beginCoefPosition,
                                                            float endCoef, float endCoefPosition)
    {
        AnimationTrack<_type> res;
        res.AddKey(0.0f, begin, 0.0f, 0.0f, beginCoef, beginCoefPosition*duration);
        res.AddKey(duration*duration, end, endCoef, endCoefPosition, 0.0f, 0.0f);
        return res;
    }

    template<typename _type>
    AnimationTrack<_type> AnimationTrack<_type>::EaseIn(const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.5f, 1.0f, 1.0f);
    }

    template<typename _type>
    AnimationTrack<_type> AnimationTrack<_type>::EaseOut(const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.0f, 1.0f, 0.5f);
    }

    template<typename _type>
    AnimationTrack<_type> AnimationTrack<_type>::EaseInOut(const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.4f, 1.0f, 1.0f);
    }

    template<typename _type>
    AnimationTrack<_type> AnimationTrack<_type>::Linear(const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    // ----------------------------------
    // Animation track key implementation
    // ----------------------------------

    template<typename _type>
    AnimationTrack<_type>::Key::Key():
        uid(Math::Random()), position(0), leftSupportValue(1.0f), leftSupportPosition(1.0f), rightSupportValue(0.0f), rightSupportPosition(0.0f)
    {}

    template<typename _type>
    AnimationTrack<_type>::Key::Key(const _type& value) :
        uid(Math::Random()), position(0), leftSupportValue(1.0f), leftSupportPosition(1.0f), rightSupportValue(0.0f), rightSupportPosition(0.0f),
        value(value)
    {}

    template<typename _type>
    AnimationTrack<_type>::Key::Key(float position, const _type& value):
        uid(Math::Random()), position(position), leftSupportValue(1.0f), leftSupportPosition(1.0f), rightSupportValue(0.0f), rightSupportPosition(0.0f),
        value(value)
    {}

    template<typename _type>
    AnimationTrack<_type>::Key::Key(float position, const _type& value,
                                    float leftSupportValue, float leftSupportPosition,
                                    float rightSupportValue, float rightSupportPosition):
        uid(Math::Random()), position(position), leftSupportValue(leftSupportValue), leftSupportPosition(leftSupportPosition),
        rightSupportValue(rightSupportValue), rightSupportPosition(rightSupportPosition), value(value)
    {}

    template<typename _type>
    AnimationTrack<_type>::Key::Key(const Key& other):
        uid(other.uid), position(other.position), leftSupportValue(other.leftSupportValue), leftSupportPosition(other.leftSupportPosition),
        rightSupportValue(other.rightSupportValue), rightSupportPosition(other.rightSupportPosition), value(other.value)
    {
        memcpy(mCurveApproxValues, other.mCurveApproxValues, mApproxValuesCount*sizeof(Vec2F));
    }

    template<typename _type>
    typename AnimationTrack<_type>::Key& AnimationTrack<_type>::Key::operator=(const _type& value)
    {
        this->value = value;
        return *this;
    }

    template<typename _type>
    AnimationTrack<_type>::Key::operator _type() const
    {
        return value;
    }

    template<typename _type>
    typename AnimationTrack<_type>::Key& AnimationTrack<_type>::Key::operator=(const Key& other)
    {
        uid = other.uid;
        position = other.position;
        value = other.value;
        leftSupportValue = other.leftSupportValue;
        leftSupportPosition = other.leftSupportPosition;
        rightSupportValue = other.rightSupportValue;
        rightSupportPosition = other.rightSupportPosition;

        memcpy(mCurveApproxValues, other.mCurveApproxValues, mApproxValuesCount*sizeof(Vec2F));

        return *this;
    }

    template<typename _type>
    bool AnimationTrack<_type>::Key::operator==(const Key& other) const
    {
        return Math::Equals(position, other.position) && Math::Equals(value, other.value);
    }

    // ---------------------
    // Player implementation
    // ---------------------

    template<typename _type>
    AnimationTrack<_type>::Player::Player()
    {}

    template<typename _type>
    AnimationTrack<_type>::Player::~Player()
    {}

    template<typename _type>
    AnimationTrack<_type>::Player::operator _type() const
    {
        return mCurrentValue;
    }

    template<typename _type>
    void AnimationTrack<_type>::Player::SetTarget(_type* value)
    {
        mTargetProxy = nullptr;
        mTarget = value;
    }

    template<typename _type>
    void AnimationTrack<_type>::Player::SetTargetProxy(const Ref<IValueProxy<_type>>& proxy)
    {
        mTarget = nullptr;
        mTargetProxy = proxy;
    }

    template<typename _type>
    void AnimationTrack<_type>::Player::SetTargetVoid(void* target)
    {
        SetTarget((_type*)target);
    }

    template<typename _type>
    void AnimationTrack<_type>::Player::SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy)
    {
        SetTargetProxy(DynamicCast<IValueProxy<_type>>(targetProxy));
    }

    template<typename _type>
    void AnimationTrack<_type>::Player::SetTrack(const Ref<AnimationTrack<_type>>& track)
    {
        mTrack = track;
        IPlayer::SetTrack(track);
    }

    template<typename _type>
    void AnimationTrack<_type>::Player::SetTrack(const Ref<IAnimationTrack>& track)
    {
        SetTrack(DynamicCast<AnimationTrack<_type>>(track));
    }

    template<typename _type>
    Ref<IAnimationTrack> AnimationTrack<_type>::Player::GetTrack() const
    {
        return mTrack;
    }

    template<typename _type>
    const Ref<AnimationTrack<_type>>& AnimationTrack<_type>::Player::GetTrackT() const
    {
        return mTrack;
    }

    template<typename _type>
    _type AnimationTrack<_type>::Player::GetValue() const
    {
        return mCurrentValue;
    }

    template<typename _type>
    void AnimationTrack<_type>::Player::Evaluate()
    {
        mPrevInDurationTime = mInDurationTime;

        if (mTrack->GetKeys().IsEmpty())
        {
            mCurrentValue = mTarget ? *mTarget : mTargetProxy ? mTargetProxy->GetValue() : _type();
            return;
        }

        mCurrentValue = mTrack->GetValue(mInDurationTime, mInDurationTime > mPrevInDurationTime,
                                         mPrevKey, mPrevKeyApproximation);

        if (mTarget)
            *mTarget = mCurrentValue;
        else if (mTargetProxy)
            mTargetProxy->SetValue(mCurrentValue);
    }
};
// --- META ---

META_TEMPLATES(typename _type)
CLASS_BASES_META(o2::AnimationTrack<_type>)
{
    BASE_CLASS(o2::IAnimationTrack);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_FIELDS_META(o2::AnimationTrack<_type>)
{
    FIELD().PUBLIC().NAME(keys);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBatchChange);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mChangedKeys);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mKeys);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_METHODS_META(o2::AnimationTrack<_type>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationTrack<_type>&);
    FUNCTION().PUBLIC().SIGNATURE(_type, GetValue, float);
    FUNCTION().PUBLIC().SIGNATURE(_type, GetValue, float, bool, int&, int&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysBatchChange);
    FUNCTION().PUBLIC().SIGNATURE(void, CompleteKeysBatchingChange);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IPlayer>, CreatePlayer);
    FUNCTION().PUBLIC().SIGNATURE(void, AddKeys, Vector<Key>, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, const Key&);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, const Key&, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddSmoothKey, const Key&, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, float, const _type&, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, float, const _type&, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, RemoveKey, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, RemoveKeyAt, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllKeys);
    FUNCTION().PUBLIC().SIGNATURE(bool, ContainsKey, float);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Key>&, GetKeys);
    FUNCTION().PUBLIC().SIGNATURE(void, SetKey, int, const Key&);
    FUNCTION().PUBLIC().SIGNATURE(Key, GetKey, float);
    FUNCTION().PUBLIC().SIGNATURE(Key, GetKeyAt, int);
    FUNCTION().PUBLIC().SIGNATURE(Key, FindKey, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(int, FindKeyIdx, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(void, SetKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SmoothKey, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<_type>, Parametric, const _type&, const _type&, float, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<_type>, EaseIn, const _type&, const _type&, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<_type>, EaseOut, const _type&, const _type&, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<_type>, EaseInOut, const _type&, const _type&, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<_type>, Linear, const _type&, const _type&, float);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Key>, GetKeysNonContant);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateApproximation);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

META_TEMPLATES(typename _type)
CLASS_BASES_META(o2::AnimationTrack<_type>::Player)
{
    BASE_CLASS(o2::IAnimationTrack::IPlayer);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_FIELDS_META(o2::AnimationTrack<_type>::Player)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(target);
    FIELD().PROTECTED().NAME(mTrack);
    FIELD().PROTECTED().NAME(mCurrentValue);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPrevInDurationTime);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevKey);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevKeyApproximation);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mTarget);
    FIELD().PROTECTED().NAME(mTargetProxy);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_METHODS_META(o2::AnimationTrack<_type>::Player)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, _type*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IValueProxy<_type>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<AnimationTrack<_type>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationTrack<_type>>&, GetTrackT);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, GetTrack);
    FUNCTION().PUBLIC().SIGNATURE(_type, GetValue);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, RegMixer, const Ref<AnimationState>&, const String&);
}
END_META;

META_TEMPLATES(typename _type)
CLASS_BASES_META(o2::AnimationTrack<_type>::Key)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_FIELDS_META(o2::AnimationTrack<_type>::Key)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(uid);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(position);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(_type()).NAME(value);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(leftSupportValue);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(leftSupportPosition);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(rightSupportValue);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(rightSupportPosition);
    FIELD().PUBLIC().NAME(mCurveApproxValues);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_METHODS_META(o2::AnimationTrack<_type>::Key)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const _type&);
    FUNCTION().PUBLIC().CONSTRUCTOR(float, const _type&);
    FUNCTION().PUBLIC().CONSTRUCTOR(float, const _type&, float, float, float, float);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Key&);
}
END_META;
// --- END META ---
