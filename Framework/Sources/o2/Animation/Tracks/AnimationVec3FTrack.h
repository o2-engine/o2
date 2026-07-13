#pragma once

#include "AnimationTrack.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    // --------------------
    // Animated Vec3F value
    // --------------------
    template<>
    class AnimationTrack<o2::Vec3F>: public IAnimationTrack
    {
    public:
        typedef o2::Vec3F ValueType;
        class Key;

    public:
        PROPERTIES(AnimationTrack<o2::Vec3F>);
        PROPERTY(Vector<Key>, keys, SetKeys, GetKeysNonContant); // Keys property

    public:
        // Default constructor
        AnimationTrack();

        // Copy-constructor
        AnimationTrack(const AnimationTrack<Vec3F>& other);

        // Assign operator
        AnimationTrack<Vec3F>& operator=(const AnimationTrack<Vec3F>& other);

        // Returns value at time
        Vec3F GetValue(float position) const;

        // Returns value at time
        Vec3F GetValue(float position, bool direction, int& cacheKey, int& cacheKeyApprox) const;

        // Called when beginning keys batch change. After this call all keys modifications will not be update approximation
        // Used for optimizing many keys change
        void BeginKeysBatchChange() override;

        // Called when keys batch change completed. Updates approximation
        void CompleteKeysBatchingChange() override;

        // Returns track duration
        float GetDuration() const override;

        // Creates track-type specific player
        Ref<IPlayer> CreatePlayer() const override;

        // Adds keys
        void AddKeys(const Vector<Key>& keys);

        // Adds single key
        int AddKey(const Key& key);

        // Adds key at position
        int AddKey(const Key& key, float position);

        // Adds key
        int AddKey(float position, const Vec3F& value);

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

        // Returns key by position
        Key operator[](float position) const;

        // Returns tween animation from begin to end in duration with linear transition
        static AnimationTrack<Vec3F> Linear(const Vec3F& begin, const Vec3F& end, float duration = 1.0f);

        SERIALIZABLE(AnimationTrack<Vec3F>);
        CLONEABLE_REF(AnimationTrack<Vec3F>);

    public:
        // ----------------------
        // Animation track player
        // ----------------------
        class Player: public IPlayer
        {
        public:
            PROPERTIES(Player);
            GETTER(o2::Vec3F, value, GetValue);    // Current value getter
            SETTER(o2::Vec3F*, target, SetTarget); // Bind target setter

        public:
            // Default constructor
            Player();

            // Destructor
            ~Player();

            // Value type cast operator
            operator Vec3F() const;

            // Sets target pointer
            void SetTarget(Vec3F* value);

            // Sets target property pointer
            void SetTargetProxy(const Ref<IValueProxy<Vec3F>>& setter);

            // Sets animation track
            void SetTrack(const Ref<AnimationTrack<Vec3F>>& track);

            // Returns animation track
            const Ref<AnimationTrack<Vec3F>>& GetTrackT() const;

            // Sets target by void pointer
            void SetTargetVoid(void* target) override;

            // Sets target property by void pointer
            void SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy) override;

            // Sets animation track
            void SetTrack(const Ref<IAnimationTrack>& track) override;

            // Returns animation track
            Ref<IAnimationTrack> GetTrack() const override;

            // Returns current value
            Vec3F GetValue() const;

            IOBJECT(Player);

        protected:
            Ref<AnimationTrack<Vec3F>> mTrack; // Animation track

            Vec3F mCurrentValue; // Current animation track

            float mPrevInDurationTime = 0.0f; // Previous evaluation in duration time
            int   mPrevKey = 0;               // Previous evaluation key index
            int   mPrevKeyApproximation = 0;  // Previous evaluation key approximation index

            Vec3F*                  mTarget = nullptr; // Animation target value pointer
            Ref<IValueProxy<Vec3F>> mTargetProxy;      // Animation target proxy pointer

        protected:
            // Evaluates value
            void Evaluate() override;

            // Registering this in animatable value agent
            void RegMixer(const Ref<AnimationState>& state, const String& path) override;
        };

    public:
        // -------------
        // Animation key
        // -------------
        class Key: public ISerializable
        {
        public:
            UInt64 uid;      // Random unique id @SERIALIZABLE
            float  position; // Position on time line, in seconds @SERIALIZABLE
            Vec3F  value;    // Value @SERIALIZABLE

        public:
            // Default constructor
            Key();

            // Constructor from value
            Key(const Vec3F& value);

            // Constructor from position and value
            Key(float position, const Vec3F& value);

            // Copy-constructor
            Key(const Key& other);

            // Assign operator from other key
            Key& operator=(const Key& other);

            // Assign operator from value
            Key& operator=(const Vec3F& value);

            // Value type cast operator
            operator Vec3F() const;

            // Equals operator
            bool operator==(const Key& other) const;

            SERIALIZABLE(Key);
        };

    protected:
        bool mBatchChange = false; // It is true when began batch change
        bool mChangedKeys = false; // It is true when some keys changed during batch change

        Vector<Key> mKeys; // Animation keys @SERIALIZABLE

    protected:
        // Returns keys (for property)
        Vector<Key> GetKeysNonContant();
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationTrack<o2::Vec3F>)
{
    BASE_CLASS(o2::IAnimationTrack);
}
END_META;
CLASS_FIELDS_META(o2::AnimationTrack<o2::Vec3F>)
{
    FIELD().PUBLIC().NAME(keys);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBatchChange);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mChangedKeys);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mKeys);
}
END_META;
CLASS_METHODS_META(o2::AnimationTrack<o2::Vec3F>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationTrack<Vec3F>&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetValue, float);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetValue, float, bool, int&, int&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysBatchChange);
    FUNCTION().PUBLIC().SIGNATURE(void, CompleteKeysBatchingChange);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IPlayer>, CreatePlayer);
    FUNCTION().PUBLIC().SIGNATURE(void, AddKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, const Key&);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, const Key&, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AddKey, float, const Vec3F&);
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
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<Vec3F>, Linear, const Vec3F&, const Vec3F&, float);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Key>, GetKeysNonContant);
}
END_META;

CLASS_BASES_META(o2::AnimationTrack<o2::Vec3F>::Player)
{
    BASE_CLASS(IPlayer);
}
END_META;
CLASS_FIELDS_META(o2::AnimationTrack<o2::Vec3F>::Player)
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
CLASS_METHODS_META(o2::AnimationTrack<o2::Vec3F>::Player)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, Vec3F*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IValueProxy<Vec3F>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<AnimationTrack<Vec3F>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationTrack<Vec3F>>&, GetTrackT);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, GetTrack);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetValue);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, RegMixer, const Ref<AnimationState>&, const String&);
}
END_META;

CLASS_BASES_META(o2::AnimationTrack<o2::Vec3F>::Key)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::AnimationTrack<o2::Vec3F>::Key)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(uid);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(position);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(value);
}
END_META;
CLASS_METHODS_META(o2::AnimationTrack<o2::Vec3F>::Key)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec3F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(float, const Vec3F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Key&);
}
END_META;
// --- END META ---
