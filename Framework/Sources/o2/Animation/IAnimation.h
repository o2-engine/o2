#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Map.h"

namespace o2
{
    // -------------------------
    // Basic animation interface
    // -------------------------
    class IAnimation : virtual public ISerializable, public RefCounterable
    {
    public:
        PROPERTIES(IAnimation);
        PROPERTY(bool, playing, SetPlaying, IsPlaying);            // Is animation playing property
        PROPERTY(bool, reversed, SetReverse, IsReversed);          // Is animation reversed property @EDITOR_IGNORE
        PROPERTY(float, speed, SetSpeed, GetSpeed);                // Animation speed. 1.0 is default @EDITOR_IGNORE
        PROPERTY(float, time, SetTime, GetTime);                   // Animation time @EDITOR_IGNORE
        PROPERTY(float, relTime, SetRelTime, GetRelativeTime);          // Animation relative time. 0 - is start of animation, 1 - time at duration @EDITOR_IGNORE
        PROPERTY(float, beginBound, SetBeginBound, GetBeginBound); // Begin playing time bound property @EDITOR_IGNORE
        PROPERTY(float, endBound, SetEndBound, GetEndBound);       // End playing time bound property @EDITOR_IGNORE
        PROPERTY(Loop, loop, SetLoop, GetLoop);                    // Loop animation property
        GETTER(float, duration, GetDuration);                      // Animation duration property

    public:
        Function<void()>      onPlay;   // Event calling when starting playing
        Function<void()>      onStop;   // Event calling when stopping playing
        Function<void()>      onPlayed; // Event calling when finishing playing
        Function<void(float)> onUpdate; // Event calling on animation update

    public:
        // Default constructor
        IAnimation();

        // Default constructor with ref counter
        explicit IAnimation(RefCounter* refCounter);

        // Copy-constructor
        IAnimation(const IAnimation& other);

        // Copy-constructor with ref counter
        IAnimation(RefCounter* refCounter, const IAnimation& other);

        // Virtual destructor
        virtual ~IAnimation();

        // Assign operator
        IAnimation& operator=(const IAnimation& other);

        // Updates animation mTime and mInDurationTime, stops and calls events
        virtual void Update(float dt);

        // Sets sub controlled flag
        virtual void SetSubControlled(bool subControlled);

        // Returns true if animation is sub controlled
        virtual bool IsSubControlled() const;

        // Starting playing animation 
        virtual void Play();

        // Starting playing animation in bounds
        virtual void PlayInBounds(float beginTime, float endTime);

        // Starting playing animation back in bounds
        virtual void PlayBackInBounds(float beginTime, float endTime);

        // Toggles animation direction
        virtual void TogglePlay();

        // Rewinds to starts and play animation
        virtual void RewindAndPlay();

        // Stops animation
        virtual void Stop();

        // Sets begin time bound
        virtual void SetBeginBound(float time);

        // Returns begin time bound
        virtual float GetBeginBound() const;

        // Sets end time bound
        virtual void SetEndBound(float time);

        // Returns end time bound
        virtual float GetEndBound() const;

        // Sets animation time bounds
        virtual void SetBounds(float beginTime, float endTime);

        // Resets bound on range from 0 to duration
        virtual void ResetBounds();

        // Sets playing flag
        virtual void SetPlaying(bool playing);

        // Returns true if animation playing
        virtual bool IsPlaying() const;

        // Sets time
        virtual void SetTime(float time);

        // Returns time
        virtual float GetTime() const;

        // Returns time in loop
        virtual float GetLoopTime() const;

        // Returns duration
        virtual float GetDuration() const;

        // Sets relative time: 0 - is start of animation, 1 - time at duration
        virtual void SetRelTime(float relTime);

        // Returns relative time: 0 - is start of animation, 1 - time at duration
        virtual float GetRelativeTime() const;

        // Returns in duration time. When animtions is looped, in duration time is time from 0 to duration
        virtual float GetInDurationTime() const;

        // Sets animation time to begin
        virtual void GoToBegin();

        // Sets animatio time to end
        virtual void GoToEnd();

        // Starts playing animation forward
        virtual void PlayForward();

        // Starts animation playing reversed
        virtual void PlayBack();

        // Sets reverse animation flag
        virtual void SetReverse(bool reverse);

        // Returns true if animation reversed
        virtual bool IsReversed() const;

        // Sets animation speed. 1 is default
        virtual void SetSpeed(float speed);

        // Returns animation speed. 1 is default
        virtual float GetSpeed() const;

        // Sets animation loop type
        virtual void SetLoop(Loop loop = Loop::Repeat);

        // Returns animation loop type
        virtual Loop GetLoop() const;

        // Adds event on time line
        virtual void AddTimeEvent(float time, const Function<void()>& eventFunc);

        // Removes event by time
        virtual void RemoveTimeEvent(float time);

        // Removes event
        virtual void RemoveTimeEvent(const Function<void()>& eventFunc);

        // Removes all events
        virtual void RemoveAllTimeEvents();

        SERIALIZABLE(IAnimation);

    protected:
        float mTime = 0.0f;            // Current animation time, can be out of bounds
        float mInDurationTime = 0.0f ; // In duration time
        float mDuration = 0.0f;        // Animation duration @SERIALIZABLE
        float mBeginTime = 0.0f;       // Begin time @SERIALIZABLE
        float mEndTime = 0.0f;         // End time @SERIALIZABLE
        float mDirection = 1.0f;       // Animation direction: 1 - forward, -1 - reversed
        float mSpeed = 1.0f;           // Animation speed, 1 is default @SERIALIZABLE
        Loop  mLoop = Loop::None;      // Loop type @SERIALIZABLE
        bool  mPlaying = false;        // True if animation playing @SERIALIZABLE
        bool  mSubControlled = false;  // True if animation is controlled by parent animation

        Vector<Pair<float, Function<void()>>> mTimeEvents; // Animation time events

    protected:
        // Updates mTime and mInDurationTime
        void UpdateTime();

        // Called for updating animated object, after updating time
        virtual void Evaluate() {}

        // Called when animation starts playing
        virtual void OnPlay() {}

        // Called when animation stops playing
        virtual void OnStop() {}

        // Called when animation loop state changed
        virtual void OnLoopChanged() {}
    };
}
// --- META ---

CLASS_BASES_META(o2::IAnimation)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::IAnimation)
{
    FIELD().PUBLIC().NAME(playing);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(reversed);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(speed);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(time);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(relTime);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(beginBound);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(endBound);
    FIELD().PUBLIC().NAME(loop);
    FIELD().PUBLIC().NAME(duration);
    FIELD().PUBLIC().NAME(onPlay);
    FIELD().PUBLIC().NAME(onStop);
    FIELD().PUBLIC().NAME(onPlayed);
    FIELD().PUBLIC().NAME(onUpdate);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mTime);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mInDurationTime);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mDuration);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mBeginTime);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mEndTime);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mDirection);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mSpeed);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Loop::None).NAME(mLoop);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mPlaying);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSubControlled);
    FIELD().PROTECTED().NAME(mTimeEvents);
}
END_META;
CLASS_METHODS_META(o2::IAnimation)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const IAnimation&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const IAnimation&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSubControlled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSubControlled);
    FUNCTION().PUBLIC().SIGNATURE(void, Play);
    FUNCTION().PUBLIC().SIGNATURE(void, PlayInBounds, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, PlayBackInBounds, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, TogglePlay);
    FUNCTION().PUBLIC().SIGNATURE(void, RewindAndPlay);
    FUNCTION().PUBLIC().SIGNATURE(void, Stop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBeginBound, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBeginBound);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEndBound, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetEndBound);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBounds, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, ResetBounds);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlaying, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTime, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLoopTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRelTime, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRelativeTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInDurationTime);
    FUNCTION().PUBLIC().SIGNATURE(void, GoToBegin);
    FUNCTION().PUBLIC().SIGNATURE(void, GoToEnd);
    FUNCTION().PUBLIC().SIGNATURE(void, PlayForward);
    FUNCTION().PUBLIC().SIGNATURE(void, PlayBack);
    FUNCTION().PUBLIC().SIGNATURE(void, SetReverse, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsReversed);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpeed, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSpeed);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLoop, Loop);
    FUNCTION().PUBLIC().SIGNATURE(Loop, GetLoop);
    FUNCTION().PUBLIC().SIGNATURE(void, AddTimeEvent, float, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveTimeEvent, float);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveTimeEvent, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllTimeEvents);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTime);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlay);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStop);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLoopChanged);
}
END_META;
// --- END META ---
