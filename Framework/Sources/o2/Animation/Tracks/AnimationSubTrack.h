#pragma once

#include "o2/Animation/Tracks/AnimationTrack.h"

namespace o2
{
    // -----------------------------------------------------
	// Animation sub track. Controls nested animation tracks
    // -----------------------------------------------------
    class AnimationSubTrack: public IAnimationTrack
    {
    public:
        PROPERTIES(AnimationSubTrack);
		PROPERTY(float, beginTime, SetBeginTime, GetBeginTime);                               // Begin time
		PROPERTY(float, subTrackDuration, SetSubTrackDuration, GetSubTrackDuration)           // Sub track duration    
		PROPERTY(float, subTrackBeginOffset, SetSubTrackBeginOffset, GetSubTrackBeginOffset); // Sub track begin offset
		PROPERTY(float, subTrackEndOffset, SetSubTrackEndOffset, GetSubTrackEndOffset);       // Sub track end offset

    public:
        // Default constructor
        AnimationSubTrack();

        // Copy-constructor
        AnimationSubTrack(const AnimationSubTrack& other);

        // Assign operator
        AnimationSubTrack& operator=(const AnimationSubTrack& other);

        // Returns track duration
        float GetDuration() const override;

        // Creates track-type specific player
        Ref<IPlayer> CreatePlayer() const override;

		// Sets track begin time
        void SetBeginTime(float beginTime);

		// Returns track begin time
		float GetBeginTime() const;

		// Sets sub track duration
		void SetSubTrackDuration(float duration);

		// Returns sub track duration
		float GetSubTrackDuration() const;

		// Sets sub track begin offset
		void SetSubTrackBeginOffset(float offset);

		// Returns sub track begin offset
		float GetSubTrackBeginOffset() const;

		// Sets sub track end offset
		void SetSubTrackEndOffset(float offset);

		// Returns sub track end offset
		float GetSubTrackEndOffset() const;

        SERIALIZABLE(AnimationSubTrack);
        CLONEABLE_REF(AnimationSubTrack);

    public:
        class Player: public IPlayer
        {
        public:
            PROPERTIES(Player);
            SETTER(IAnimation*, target, SetTarget);                      // Bind target setter
            SETTER(Function<void()>, targetDelegate, SetTargetDelegate); // Bind target change event setter

        public:
            // Default constructor
            Player();

            // Destructor
            ~Player();

            // Sets target pointer
            void SetTarget(IAnimation* value);

            // Sets target pointer and change event
            void SetTarget(IAnimation* value, const Function<void()>& changeEvent);

            // Sets target change event
            void SetTargetDelegate(const Function<void()>& changeEvent) override;

            // Sets animation track
            void SetTrack(const Ref<AnimationSubTrack>& track);

            // Returns animation track
            const Ref<AnimationSubTrack>& GetTrackT() const;

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

            IOBJECT(Player);

        protected:
            Ref<AnimationSubTrack> mTrack = nullptr; // Animation track

            float mPrevInDurationTime = 0.0f; // Previous evaluation in duration time

            IAnimation*      mTarget = nullptr; // Animation target value pointer
            Function<void()> mTargetDelegate;   // Animation target value change event

        protected:
            // Evaluates value
            void Evaluate() override;

            // Registering this in value mixer
            void RegMixer(const Ref<AnimationState>& state, const String& path) override;
        };

    protected:
		float mBeginTime = 0.0f;           // Begin time @SERIALIZABLE
		float mDuration = 1.0f;            // Sub track duration @SERIALIZABLE
		float mSubTrackBeginOffset = 0.0f; // Sub track begin offset @SERIALIZABLE
		float mSubTrackEndOffset = 0.0f;   // Sub track end offset @SERIALIZABLE

    protected:
        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationSubTrack)
{
    BASE_CLASS(o2::IAnimationTrack);
}
END_META;
CLASS_FIELDS_META(o2::AnimationSubTrack)
{
    FIELD().PUBLIC().NAME(beginTime);
    FIELD().PUBLIC().NAME(subTrackDuration);
    FIELD().PUBLIC().NAME(subTrackEndOffset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mBeginTime);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mDuration);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mSubTrackBeginOffset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mSubTrackEndOffset);
}
END_META;
CLASS_METHODS_META(o2::AnimationSubTrack)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationSubTrack&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IPlayer>, CreatePlayer);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBeginTime, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBeginTime);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSubTrackDuration, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSubTrackDuration);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSubTrackBeginOffset, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSubTrackBeginOffset);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSubTrackEndOffset, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSubTrackEndOffset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::AnimationSubTrack::Player)
{
    BASE_CLASS(o2::IAnimationTrack::IPlayer);
}
END_META;
CLASS_FIELDS_META(o2::AnimationSubTrack::Player)
{
    FIELD().PUBLIC().NAME(target);
    FIELD().PUBLIC().NAME(targetDelegate);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mTrack);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPrevInDurationTime);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mTarget);
    FIELD().PROTECTED().NAME(mTargetDelegate);
}
END_META;
CLASS_METHODS_META(o2::AnimationSubTrack::Player)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, IAnimation*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, IAnimation*, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetDelegate, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<AnimationSubTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationSubTrack>&, GetTrackT);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, GetTrack);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, RegMixer, const Ref<AnimationState>&, const String&);
}
END_META;
// --- END META ---
