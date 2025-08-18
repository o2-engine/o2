#pragma once
#include "AnimationTrack.h"
#include "o2/Utils/Math/Curve.h"
#include "o2/Utils/Math/Spline.h"
#include "o2/Utils/Math/Vector2.h"

namespace o2
{
    // --------------------
    // Animated Vec2F value
    // --------------------
    template<>
    class AnimationTrack<o2::Vec2F>: public IAnimationTrack
    {
    public:
        typedef o2::Vec2F ValueType;

    public:
        Ref<Curve>  timeCurve = mmake<Curve>(); // Time curve. represents speed on spline @SERIALIZABLE
        Ref<Spline> spline = mmake<Spline>();   // Movement trajectory spline @SERIALIZABLE

    public:
        // Default constructor
        AnimationTrack();

        // Copy-constructor
        AnimationTrack(const AnimationTrack<Vec2F>& other);

        // Assign operator
        AnimationTrack<Vec2F>& operator=(const AnimationTrack<Vec2F>& other);

        // Returns value at time
        Vec2F GetValue(float position) const;

        // Returns value at time with caching
        Vec2F GetValue(float position, float randomRangeCoef, bool direction, int& cacheTimeKey, int& cacheTimeKeyApprox,
                       int& cacheSplineKey, int& cacheSplineKeyApprox) const;

        // Called when beginning keys batch change. After this call all keys modifications will not be update pproximation
        // Used for optimizing many keys change
        void BeginKeysBatchChange() override;

        // Called when keys batch change completed. Updates approximation
        void CompleteKeysBatchingChange() override;

        // Returns track duration
        float GetDuration() const override;

        // Creates track-type specific player
        Ref<IPlayer> CreatePlayer() const override;

        // Returns parametric specified Animation track
        // Sample: Parametric(someBegin, someEnd, 1.0f, 0.0f, 0.4f, 1.0f, 0.6f) 
        static AnimationTrack<Vec2F> Parametric(const Vec2F& begin, const Vec2F& end, float duration,
                                                float beginCoef, float beginCoefPosition,
                                                float endCoef, float endCoefPosition);

        // Returns tween animation from begin to end in duration with ease in
        static AnimationTrack<Vec2F> EaseIn(const Vec2F& begin, const Vec2F& end, float duration = 1.0f, float strongness = 0.4f);

        // Returns tween animation from begin to end in duration with ease out
        static AnimationTrack<Vec2F> EaseOut(const Vec2F& begin, const Vec2F& end, float duration = 1.0f, float strongness = 0.4f);

        // Returns tween animation from begin to end in duration with ease in-out
        static AnimationTrack<Vec2F> EaseInOut(const Vec2F& begin, const Vec2F& end, float duration = 1.0f, float strongness = 0.4f);

        // Returns tween animation from begin to end in duration with linear transition
        static AnimationTrack<Vec2F> Linear(const Vec2F& begin, const Vec2F& end, float duration = 1.0f);

        SERIALIZABLE(AnimationTrack<Vec2F>);
        CLONEABLE_REF(AnimationTrack<Vec2F>);

    public:
        // ----------------------
        // Animation track player
        // ----------------------
        class Player: public IPlayer
        {
        public:
            PROPERTIES(Player);
            GETTER(o2::Vec2F, value, GetValue);    // Current value getter
            SETTER(o2::Vec2F*, target, SetTarget); // Bind target setter

        public:
            // Default constructor
            Player();

            // Destructor
            ~Player();

            // Value type cast operator
            operator Vec2F() const;

            // Sets target pointer
            void SetTarget(Vec2F* value);

            // Sets target property pointer
            void SetTargetProxy(const Ref<IValueProxy<Vec2F>>& setter);

            // Sets animation track
            void SetTrack(const Ref<AnimationTrack<Vec2F>>& track);

            // Returns animation track
            const Ref<AnimationTrack<Vec2F>>& GetTrackT() const;

            // Sets target by void pointer
            void SetTargetVoid(void* target) override;

            // Sets target property by void pointer
            void SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy) override;

            // Sets animation track
            void SetTrack(const Ref<IAnimationTrack>& track) override;

            // Returns animation track
            Ref<IAnimationTrack> GetTrack() const override;

            // Returns current value
            Vec2F GetValue() const;

            IOBJECT(Player);

        protected:
            Ref<AnimationTrack<Vec2F>> mTrack; // Animation track

            Vec2F mCurrentValue; // Current animation track value

            float mPrevInDurationTime = 0.0f;       // Previous evaluation in duration time
            int   mPrevTimeKey = 0;                 // Previous evaluation time key index
            int   mPrevTimeKeyApproximation = 0;    // Previous evaluation time key approximation index
            int   mPrevSplineKey = 0;               // Previous evaluation spline key index
            int   mPrevSplineKeyApproximation = 0;  // Previous evaluation spline key approximation index

			float mRandomRangeCoef = 0.0f; // Random range coefficient for value approximation

            Vec2F*                  mTarget = nullptr; // Animation target value pointer
            Ref<IValueProxy<Vec2F>> mTargetProxy;      // Animation target proxy pointer

		protected:
			// Called when animation started playing, updates random range coefficient`
            void OnPlay() override;

            // Evaluates value
            void Evaluate() override;

            // Registering this in animatable value agent
            void RegMixer(const Ref<AnimationState>& state, const String& path) override;
        };

    protected:
        // Called when curve updated keys and calculated duration
        void OnCurveChanged();
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationTrack<o2::Vec2F>)
{
    BASE_CLASS(o2::IAnimationTrack);
}
END_META;
CLASS_FIELDS_META(o2::AnimationTrack<o2::Vec2F>)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Curve>()).NAME(timeCurve);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<Spline>()).NAME(spline);
}
END_META;
CLASS_METHODS_META(o2::AnimationTrack<o2::Vec2F>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationTrack<Vec2F>&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetValue, float);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetValue, float, float, bool, int&, int&, int&, int&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysBatchChange);
    FUNCTION().PUBLIC().SIGNATURE(void, CompleteKeysBatchingChange);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IPlayer>, CreatePlayer);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<Vec2F>, Parametric, const Vec2F&, const Vec2F&, float, float, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<Vec2F>, EaseIn, const Vec2F&, const Vec2F&, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<Vec2F>, EaseOut, const Vec2F&, const Vec2F&, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<Vec2F>, EaseInOut, const Vec2F&, const Vec2F&, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(AnimationTrack<Vec2F>, Linear, const Vec2F&, const Vec2F&, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveChanged);
}
END_META;

CLASS_BASES_META(o2::AnimationTrack<o2::Vec2F>::Player)
{
    BASE_CLASS(IPlayer);
}
END_META;
CLASS_FIELDS_META(o2::AnimationTrack<o2::Vec2F>::Player)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(target);
    FIELD().PROTECTED().NAME(mTrack);
    FIELD().PROTECTED().NAME(mCurrentValue);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPrevInDurationTime);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevTimeKey);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevTimeKeyApproximation);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevSplineKey);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPrevSplineKeyApproximation);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mRandomRangeCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mTarget);
    FIELD().PROTECTED().NAME(mTargetProxy);
}
END_META;
CLASS_METHODS_META(o2::AnimationTrack<o2::Vec2F>::Player)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, Vec2F*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IValueProxy<Vec2F>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<AnimationTrack<Vec2F>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationTrack<Vec2F>>&, GetTrackT);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, GetTrack);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetValue);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlay);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, RegMixer, const Ref<AnimationState>&, const String&);
}
END_META;
// --- END META ---
