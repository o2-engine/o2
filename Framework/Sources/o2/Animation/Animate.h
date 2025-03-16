#pragma once
#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Animation/Tracks/AnimationColor4Track.h"

namespace o2
{
    // -----------------------
    // Key container interface
    // -----------------------
    struct IKeyContainer: public RefCounterable
    {
        // Virtual destructor
        virtual ~IKeyContainer() {}

        // Applies stored key to animation
        virtual void Apply(float time) = 0;
    };

    // ----------------------
    // Template key container
    // ----------------------
    template <typename T>
    struct KeyContainer : public IKeyContainer
    {
        typename AnimationTrack<T>::Key key;

        Ref<AnimationTrack<T>> animatedValue;

    public:
        void Apply(float time);
    };

    // ---------------
    // Vec2F container
    // ---------------
    template <>
    struct KeyContainer<Vec2F> : public IKeyContainer
    {
        Curve::Key timeKey;

        Ref<AnimationTrack<Vec2F>> animatedValue;

    public:
        void Apply(float time);
    };

    // -------------------
    // Scale key container
    // -------------------
    struct ScaleKeyContainer : public IKeyContainer
    {
        AnimationTrack<float>::Key keyX;
        AnimationTrack<float>::Key keyY;

        Ref<AnimationTrack<float>> animatedValueX;
        Ref<AnimationTrack<float>> animatedValueY;

    public:
        void Apply(float time);
    };

    // --------------------------------------------
    // Class for building a simple animation sequence
    // --------------------------------------------
    class Animate
    {
    public:
        // Constructor. Takes an object as a parameter
        explicit Animate(IObject& object);

        // Animation cast operator. Needed to store as an animation
        operator Ref<AnimationClip>() const;

        // Inserts a delay for seconds
        Animate& Wait(float seconds);

        // Applies stored transformations after seconds
        Animate& For(float seconds);

        // Splits the sequence
        Animate& Then();

        // Moves the object to (x,y)
        Animate& Move(float x, float y);

        // Moves the object into position
        Animate& Move(const Vec2F& point);

        // Moves the object along a path from points
        Animate& Move(const Vector<Vec2F>& points);

        // Changes alpha
        Animate& Alpha(float alpha);

        // Shows the object
        Animate& Show();

        // Hides the object
        Animate& Hide();

        // Sets color
        Animate& Color(const Color4& color);

        // Sets color
        Animate& Color(int r, int g, int b, int a);

        // Sets scale
        Animate& Scale(float scale);

        // Sets scale
        Animate& Scale(const Vec2F& scale);

        // Rotates the object
        Animate& Rotate(float angle);

        // Sets the animation to loop
        Animate& Looped();

        // Sets ping-pong loop
        Animate& PingPong();

        // Changes specified parameter
        template<typename T>
        Animate& Change(T* target, const T& value);

    protected:
        IObject* mTarget = nullptr; // Target object being animated

        Ref<AnimationClip> mAnimation = mmake<AnimationClip>(); // Animation being built

        bool  mKeysApplied = false; // Whether stored keys were applied
        float mTime = 0.0f;         // Current sequence time

        Vector<Ref<IKeyContainer>> mKeyContainers; // Stored keys that are applied in For()

        Function<void()> mFunction; // Stored callback that is applied in For()

        Ref<AnimationTrack<Color4>> mColorAnimatedValue;   // Color Animation track, stored when needed
        Ref<AnimationTrack<Vec2F>> mPositionAnimatedValue; // Position Animation track, stored when needed
        Ref<AnimationTrack<float>> mScaleXAnimatedValue;   // Scale X Animation track, stored when needed
        Ref<AnimationTrack<float>> mScaleYAnimatedValue;   // Scale Y Animation track, stored when needed
        Ref<AnimationTrack<float>> mRotationAnimatedValue; // Rotation Animation track, stored when needed

    protected:
        // Checks color Animation track: creates them if needed
        void CheckColorAnimatedValue();

        // Checks position Animation track: creates them if needed
        void CheckPositionAnimatedvalue();

        // Checks scale Animation track: creates them if needed
        void CheckScaleAnimatedValue();

        // Checks rotation Animation track: creates it if needed
        void CheckRotateAnimatedValue();

        // Checks applied keys: if keys were applied, clears key containers
        void CheckAppliedKeys();

        // Applies keys and function to the animation at the current time
        void ApplyKeys();
    };

    template<typename T>
    Animate& Animate::Change(T* target, const T& value)
    {
        CheckAppliedKeys();

        auto container = mmake<KeyContainer<T>>();
        container->animatedValue = GetAnimatedValue(target);
        container->key.value = value;
        mKeyContainers.Add(container);

        return *this;
    }

    template <typename T>
    void KeyContainer<T>::Apply(float time)
    {
        key.position = time;
        animatedValue->AddKey(key);
    }

}
