#pragma once
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class IAnimationTrack;

    template<typename _type>
    class AnimationTrack;

    // -----------------------------------------
    // Animation clip. Contains animation tracks
    // -----------------------------------------
    class AnimationClip : public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        PROPERTIES(AnimationClip);
        PROPERTY(Loop, loop, SetLoop, GetLoop); // Loop type property
        GETTER(float, duration, GetDuration);   // Duration getter

    public:
        Function<void()> onChanged; // Called when some Animation track has changed @EDITOR_IGNORE

        Function<void(float)> onDurationChange; // Called when duration has changed @EDITOR_IGNORE

        Function<void(const Ref<IAnimationTrack>&)> onTrackAdded;  // Called when new track added @EDITOR_IGNORE
        Function<void(const Ref<IAnimationTrack>&)> onTrackRemove; // Called when new track removing @EDITOR_IGNORE

    public:
        // Default constructor
        AnimationClip(RefCounter* refCounter);

        // Copy-constructor
        AnimationClip(RefCounter* refCounter, const AnimationClip& other);

        // Destructor
        ~AnimationClip();

        // Assign operator
        AnimationClip& operator=(const AnimationClip& other);

        // Removes and clears all Animation tracks
        void Clear();

        // Returns clip duration
        float GetDuration() const;

        // Sets loop
        void SetLoop(Loop loop);

        // Returns loop
        Loop GetLoop() const;

        // Returns animation tracks
        Vector<Ref<IAnimationTrack>>& GetTracks();

        // Returns animation tracks
        const Vector<Ref<IAnimationTrack>>& GetTracks() const;

        // Returns is contains Animation track by path (some like "path/abc/cde")
        bool ContainsTrack(const String& path) const;

		// Returns Animation track by path (some like "path/abc/cde")
		Ref<IAnimationTrack> GetTrack(const String& path);

        // Returns Animation track by path (some like "path/abc/cde")
        template<typename _type>
        Ref<AnimationTrack<_type>> GetTrackByType(const String& path);

        // Adds animation track with specified path
        template<typename _type>
        Ref<AnimationTrack<_type>> AddTrack(const String& path);

        // Add animation track with specified path and type
        Ref<IAnimationTrack> AddTrack(const String& path, const Type& type);

        // Removes Animation track by path
        void RemoveTrack(const String& path);

        //insert animation

        // Returns parametric specified animation
        // Sample: Parametric(targetObjectPtr, targetValuePtr, someBegin, someEnd, 1.0f, 0.0f, 0.4f, 1.0f, 0.6f) 
        template<typename _type>
        static Ref<AnimationClip> Parametric(const String& animatingValuePath,
                                             const _type& begin, const _type& end, float duration,
                                             float beginCoef, float beginCoefPosition,
                                             float endCoef, float endCoefPosition);

        // Returns animation from begin to end in duration with ease in
        template<typename _type>
        static Ref<AnimationClip> EaseIn(const String& animatingValuePath,
                                         const _type& begin, const _type& end, float duration = 1.0f);

        // Returns animation from begin to end in duration with ease out
        template<typename _type>
        static Ref<AnimationClip> EaseOut(const String& animatingValuePath,
                                          const _type& begin, const _type& end, float duration = 1.0f);

        // Returns animation from begin to end in duration with ease in-out
        template<typename _type>
        static Ref<AnimationClip> EaseInOut(const String& animatingValuePath,
                                            const _type& begin, const _type& end, float duration = 1.0f);

        // Returns animation from begin to end in duration with linear transition
        template<typename _type>
        static Ref<AnimationClip> Linear(const String& animatingValuePath,
                                         const _type& begin, const _type& end, float duration = 1.0f);

        SERIALIZABLE(AnimationClip);
        CLONEABLE_REF(AnimationClip);

    protected:
        Vector<Ref<IAnimationTrack>> mTracks; // Animation track @SERIALIZABLE

        float mDuration = 0.0f;   // Animation duration @SERIALIZABLE
        Loop  mLoop = Loop::None; // Animation loop type @SERIALIZABLE

    protected:
        // Returns Animation track by path
        template<typename _type>
        Ref<AnimationTrack<_type>> FindTrack(const String& path);

        // Called when some Animation track has changed
        void OnTrackChanged();

        // Recalculates maximum duration by Animation tracks
        void RecalculateDuration();

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        // Called when Animation track was added. Need to register value agent in animation target
        void OnTrackAdded(const Ref<IAnimationTrack>& track);

        friend class Animate;
        friend class AnimationComponent;
        friend class AnimationPlayer;
    };
}

#include "o2/Animation/Tracks/AnimationTrack.h"

namespace o2
{
    template<typename _type>
    Ref<AnimationTrack<_type>> AnimationClip::FindTrack(const String& path)
    {
        for (auto& track : mTracks) {
            if (track->path == path)
                return dynamic_cast<AnimationTrack<_type>*>(track);
        }

        return nullptr;
    }

    template<typename _type>
    Ref<AnimationClip> AnimationClip::Parametric(const String& animatingValuePath,
                                            const _type& begin, const _type& end, float duration,
                                            float beginCoef, float beginCoefPosition, float endCoef, float endCoefPosition)
    {
        auto res = mmake<AnimationClip>();
        *res->AddTrack<_type>(animatingValuePath) = AnimationTrack<_type>::Parametric(begin, end, duration,
                                                                                     beginCoef, beginCoefPosition,
                                                                                     endCoef, endCoefPosition);
        return res;
    }

    template<typename _type>
    Ref<AnimationClip> AnimationClip::EaseIn(const String& animatingValuePath,
                                        const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        auto res = mmake<AnimationClip>();
        *res->AddTrack<_type>(animatingValuePath) = AnimationTrack<_type>::EaseIn(begin, end, duration);
        return res;
    }

    template<typename _type>
    Ref<AnimationClip> AnimationClip::EaseOut(const String& animatingValuePath,
                                         const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        auto res = mmake<AnimationClip>();
        *res->AddTrack<_type>(animatingValuePath) = AnimationTrack<_type>::EaseOut(begin, end, duration);
        return res;
    }

    template<typename _type>
    Ref<AnimationClip> AnimationClip::EaseInOut(const String& animatingValuePath,
                                           const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        auto res = mmake<AnimationClip>();
        *res->AddTrack<_type>(animatingValuePath) = AnimationTrack<_type>::EaseInOut(begin, end, duration);
        return res;
    }

    template<typename _type>
    Ref<AnimationClip> AnimationClip::Linear(const String& animatingValuePath,
                                        const _type& begin, const _type& end, float duration /*= 1.0f*/)
    {
        auto res = mmake<AnimationClip>();
        *res->AddTrack<_type>(animatingValuePath) = AnimationTrack<_type>::Linear(begin, end, duration);
        return res;
    }

    template<typename _type>
    Ref<AnimationTrack<_type>> AnimationClip::AddTrack(const String& path)
    {
        auto track = mmake<AnimationTrack<_type>>();
        track->onKeysChanged += THIS_FUNC(OnTrackChanged);
        track->path = path;

        mTracks.Add(track);
        OnTrackAdded(track);

        return track;
    }

    template<typename _type>
    Ref<AnimationTrack<_type>> AnimationClip::GetTrackByType(const String& path)
    {
        for (auto& track : mTracks) {
            if (track->path == path)
                return DynamicCast<AnimationTrack<_type>>(track);;
        }

        return nullptr;
    }
}
// --- META ---

CLASS_BASES_META(o2::AnimationClip)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::AnimationClip)
{
    FIELD().PUBLIC().NAME(loop);
    FIELD().PUBLIC().NAME(duration);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onChanged);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onDurationChange);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onTrackAdded);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(onTrackRemove);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mTracks);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mDuration);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Loop::None).NAME(mLoop);
}
END_META;
CLASS_METHODS_META(o2::AnimationClip)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AnimationClip&);
    FUNCTION().PUBLIC().SIGNATURE(void, Clear);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLoop, Loop);
    FUNCTION().PUBLIC().SIGNATURE(Loop, GetLoop);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<IAnimationTrack>>&, GetTracks);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<IAnimationTrack>>&, GetTracks);
    FUNCTION().PUBLIC().SIGNATURE(bool, ContainsTrack, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, GetTrack, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, AddTrack, const String&, const Type&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveTrack, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTrackChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, RecalculateDuration);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTrackAdded, const Ref<IAnimationTrack>&);
}
END_META;
// --- END META ---
