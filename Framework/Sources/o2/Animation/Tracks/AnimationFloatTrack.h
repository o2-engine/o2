#pragma once

#include "o2/Animation/Tracks/AnimationTrack.h"

namespace o2
{
    // ------------------------------
    // Animated float value interface
    // ------------------------------
    template<>
    class AnimationTrack<float>: public IAnimationTrack
    {
    public:
        typedef float ValueType;
        typedef Curve::Key Key;

    public:
        PROPERTIES(AnimationTrack<float>);
        PROPERTY(Vector<Key>, keys, SetKeys, GetKeysNonContant); // Keys property

    public:
        Ref<Curve> curve = mmake<Curve>(); // Animation curve @SERIALIZABLE

    public:
        // Default constructor
        AnimationTrack();

        // Copy-constructor
        AnimationTrack(const AnimationTrack<float>& other);

        // Assign operator
        AnimationTrack<float>& operator=(const AnimationTrack<float>& other);

        // Returns value at time
        float GetValue(float position) const;

        // Returns value at time
        float GetValue(float position, bool direction, int& cacheKey, int& cacheKeyApprox) const;

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
        void AddKeys(Vector<Vec2F> values, float smooth = 1.0f);

        // Adds single key
        int AddKey(const Key& key);

        // Adds key at position
        int AddKey(const Key& key, float position);

        // Adds and smooths key
        int AddSmoothKey(const Key& key, float smooth);

        // Adds key
        int AddKey(float position, float value, float leftCoef, float leftCoefPosition,
                   float rightCoef, float rightCoefPosition);

        // Adds key at position with value and smoothing
        int AddKey(float position, float value, float smooth = 1.0f);

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
        static AnimationTrack<float> Parametric(float begin, float end, float duration,
                                                float beginCoef, float beginCoefPosition,
                                                float endCoef, float endCoefPosition);

        // Returns tween animation from begin to end in duration with ease in
        static AnimationTrack<float> EaseIn(float begin = 0.0f, float end = 1.0f, float duration = 1.0f, float strongness = 0.4f);

        // Returns tween animation from begin to end in duration with ease out
        static AnimationTrack<float> EaseOut(float begin = 0.0f, float end = 1.0f, float duration = 1.0f, float strongness = 0.4f);

        // Returns tween animation from begin to end in duration with ease in-out
        static AnimationTrack<float> EaseInOut(float begin = 0.0f, float end = 1.0f, float duration = 1.0f, float strongness = 0.4f);

        // Returns tween animation from begin to end in duration with linear transition
        static AnimationTrack<float> Linear(float begin = 0.0f, float end = 1.0f, float duration = 1.0f);

        SERIALIZABLE(AnimationTrack<float>);
        CLONEABLE_REF(AnimationTrack<float>);

    public:
        class Player: public IPlayer
        {
        public:
            PROPERTIES(Player);
            GETTER(float, value, GetValue);                              // Current value getter 
            SETTER(float*, target, SetTarget);                           // Bind target setter
            SETTER(Function<void()>, targetDelegate, SetTargetDelegate); // Bind target change event setter

        public:
            // Default constructor
            Player();

            // Destructor
            ~Player();

            // Value type cast operator
            operator float() const;

            // Sets target pointer
            void SetTarget(float* value);

            // Sets target pointer and change event
            void SetTarget(float* value, const Function<void()>& changeEvent);

            // Sets target change event
            void SetTargetDelegate(const Function<void()>& changeEvent) override;

            // Sets target property pointer
            void SetTargetProxy(const Ref<IValueProxy<float>>& proxy);

            // Sets animation track
            void SetTrack(const Ref<AnimationTrack<float>>& track);

            // Returns animation track
            const Ref<AnimationTrack<float>>& GetTrackT() const;

            // Sets target by void pointer
            void SetTargetVoid(void* target) override;

            // Sets target by void pointer and change event
            void SetTargetVoid(void* target, const Function<void()>& changeEvent) override;

            // Sets target property by void pointer
            void SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy) override;

            // Sets animation track
            void SetTrack(const Ref<IAnimationTrack>& track) override;

            // Returns animation track
            Ref<IAnimationTrack> GetTrack() const override;

            // Returns current value
            float GetValue() const;

            IOBJECT(Player);

        protected:
            Ref<AnimationTrack<float>> mTrack = nullptr; // Animation track

            float mCurrentValue; // Current animation track

            float mPrevInDurationTime = 0.0f; // Previous evaluation in duration time
            int   mPrevKey = 0;               // Previous evaluation key index
			int   mPrevKeyApproximation = 0;  // Previous evaluation key approximation index

			float mRandomRangeCoef = 0.0f; // Random range coefficient for value approximation

            float*                  mTarget = nullptr; // Animation target value pointer
            Function<void()>        mTargetDelegate;   // Animation target value change event
            Ref<IValueProxy<float>> mTargetProxy;      // Animation target proxy pointer

		protected:
			// Called when animation started playing, updates random range coefficient`
			void OnPlay() override;

            // Evaluates value
            void Evaluate() override;

            // Registering this in value mixer
            void RegMixer(const Ref<AnimationState>& state, const String& path) override;
        };

    protected:
        // Returns keys (for property)
        Vector<Key> GetKeysNonContant();

        // Called when curve updated keys and calculated duration
        void OnCurveChanged();

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationTrack<float>)
{
    BASE_CLASS(o2::IAnimationTrack);
}
END_META;
CLASS_FIELDS_META(o2::AnimationTrack<float>)
{
    FIELD().PUBLIC().NAME(keys);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Curve>()).NAME(curve);
}
END_META;
CLASS_METHODS_META(o2::AnimationTrack<float>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationTrack<float>&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetValue, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetValue, float, bool, int&, int&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysBatchChange);
    FUNCTION().PUBLIC().SIGNATURE(void, CompleteKeysBatchingChange);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IPlayer>, CreatePlayer);
    FUNCTION().PUBLIC().SIGNATURE(void, AddKeys, Vector<Vec2F>, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, const Key&);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, const Key&, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddSmoothKey, const Key&, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, float, float, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, float, float, float);
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
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<float>, Parametric, float, float, float, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<float>, EaseIn, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<float>, EaseOut, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<float>, EaseInOut, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<float>, Linear, float, float, float);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Key>, GetKeysNonContant);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::AnimationTrack<float>::Player)
{
    BASE_CLASS(IPlayer);
}
END_META;
CLASS_FIELDS_META(o2::AnimationTrack<float>::Player)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(target);
    FIELD().PUBLIC().NAME(targetDelegate);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mTrack);
    FIELD().PROTECTED().NAME(mCurrentValue);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPrevInDurationTime);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevKey);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevKeyApproximation);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mRandomRangeCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mTarget);
    FIELD().PROTECTED().NAME(mTargetDelegate);
    FIELD().PROTECTED().NAME(mTargetProxy);
}
END_META;
CLASS_METHODS_META(o2::AnimationTrack<float>::Player)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, float*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, float*, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetDelegate, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IValueProxy<float>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<AnimationTrack<float>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationTrack<float>>&, GetTrackT);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, GetTrack);
    FUNCTION().PUBLIC().SIGNATURE(float, GetValue);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlay);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, RegMixer, const Ref<AnimationState>&, const String&);
}
END_META;
// --- END META ---
