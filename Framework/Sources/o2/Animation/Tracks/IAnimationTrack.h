#pragma once

#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    FORWARD_CLASS_REF(AnimationClip);
    FORWARD_CLASS_REF(AnimationPlayer);
    FORWARD_CLASS_REF(AnimationState);

    // -------------------------
    // Animation track interface
    // -------------------------
    class IAnimationTrack: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        // --------------------------------
        // Animation track player interface
        // --------------------------------
        class IPlayer: public IAnimation
        {
        public:
			// Called when animation started playing
            virtual void OnPlay() {}

            // Sets target by void pointer
            virtual void SetTargetVoid(void* target) {}

            // Sets target property by void pointer
            virtual void SetTargetProxy(const Ref<IAbstractValueProxy>& targetProxy) {}

            // Sets animation track
            virtual void SetTrack(const Ref<IAnimationTrack>& track);

            // Adjusts target type to correct one
            virtual void* AdjustTargetType(void* target, const Type& type) { return target; }

            // Returns animation track
            virtual Ref<IAnimationTrack> GetTrack() const { return nullptr; }

            // Registering this in animation track agent
            virtual void RegMixer(const Ref<AnimationState>& state, const String& path) {}

            // Force setting time (using in Animation): works same as update, but by hard setting time
            void ForceSetTime(float time, float duration);

            //Returns owner player
            const WeakRef<AnimationPlayer>& GetOwnerPlayer() const;

            IOBJECT(IPlayer);

        protected:
            WeakRef<AnimationPlayer> mOwnerPlayer;

            friend class AnimationPlayer;
        };

    public:
        PROPERTIES(IAnimationTrack);
        GETTER(float, duration, GetDuration);   // Animation duration getter

        Loop loop = Loop::None; // Animation loop type @SERIALIZABLE

        String path; // Animated property path @SERIALIZABLE

    public:
        Function<void()> onKeysChanged; // Called when keys has changed

    public:
        // Default constructor
        IAnimationTrack() {}

        // Copy-constructor
        IAnimationTrack(const IAnimationTrack& other): duration(this), loop(other.loop), path(other.path) {}

        // Copy operator
        IAnimationTrack& operator=(const IAnimationTrack& other);

        // Called when beginning keys batch change. After this call all keys modifications will not be update approximation
        // Used for optimizing many keys change
        virtual void BeginKeysBatchChange() {}

        // Called when keys batch change completed. Updates approximation
        virtual void CompleteKeysBatchingChange() {}

        // Returns track duration
        virtual float GetDuration() const { return 0; }

        // Creates track-type specific player
        virtual Ref<IPlayer> CreatePlayer() const { return nullptr; }

        // Returns owner clip
        const WeakRef<AnimationClip>& GetOwnerClip() const;

        SERIALIZABLE(IAnimationTrack);

    protected:
        WeakRef<AnimationClip> mOwnerClip;

        friend class AnimationClip;
    };
};
// --- META ---

CLASS_BASES_META(o2::IAnimationTrack)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::IAnimationTrack)
{
    FIELD().PUBLIC().NAME(duration);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Loop::None).NAME(loop);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(path);
    FIELD().PUBLIC().NAME(onKeysChanged);
    FIELD().PROTECTED().NAME(mOwnerClip);
}
END_META;
CLASS_METHODS_META(o2::IAnimationTrack)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const IAnimationTrack&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysBatchChange);
    FUNCTION().PUBLIC().SIGNATURE(void, CompleteKeysBatchingChange);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IPlayer>, CreatePlayer);
    FUNCTION().PUBLIC().SIGNATURE(const WeakRef<AnimationClip>&, GetOwnerClip);
}
END_META;

CLASS_BASES_META(o2::IAnimationTrack::IPlayer)
{
    BASE_CLASS(o2::IAnimation);
}
END_META;
CLASS_FIELDS_META(o2::IAnimationTrack::IPlayer)
{
    FIELD().PROTECTED().NAME(mOwnerPlayer);
}
END_META;
CLASS_METHODS_META(o2::IAnimationTrack::IPlayer)
{

    FUNCTION().PUBLIC().SIGNATURE(void, OnPlay);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetVoid, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTrack, const Ref<IAnimationTrack>&);
    FUNCTION().PUBLIC().SIGNATURE(void*, AdjustTargetType, void*, const Type&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationTrack>, GetTrack);
    FUNCTION().PUBLIC().SIGNATURE(void, RegMixer, const Ref<AnimationState>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, ForceSetTime, float, float);
    FUNCTION().PUBLIC().SIGNATURE(const WeakRef<AnimationPlayer>&, GetOwnerPlayer);
}
END_META;
// --- END META ---
