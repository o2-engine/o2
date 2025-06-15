#pragma once
#include "o2/Animation/IAnimation.h"
#include "o2/Animation/Tracks/IAnimationTrack.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    FORWARD_CLASS_REF(AnimationClip);
    FORWARD_CLASS_REF(AnimationState);

    // ---------------------
    // Animation clip player
    // ---------------------
    class AnimationPlayer: public IAnimation, public ICloneableRef
    {
    public:
        Function<void(const Ref<IAnimationTrack::IPlayer>&)> onTrackPlayerAdded;  // Called when new track added
        Function<void(const Ref<IAnimationTrack::IPlayer>&)> onTrackPlayerRemove; // Called when new track removing

    public:
        // Default constructor
        AnimationPlayer(RefCounter* refCounter, IObject* target = nullptr, const Ref<AnimationClip>& clip = nullptr);

        // Destructor
        ~AnimationPlayer();

        // Sets animation target
        // Bind all animation tracks to target's child fields (if it possible)
        void SetTarget(IObject* target, bool errors = true);

        // Returns animation's target
        IObject* GetTarget() const;

        // Sets animation clip
        void SetClip(const Ref<AnimationClip>& clip);

        // Returns animation clip
        const Ref<AnimationClip>& GetClip() const;

        // Returns track players list
        const Vector<Ref<IAnimationTrack::IPlayer>>& GetTrackPlayers() const;

        IOBJECT(AnimationPlayer);
        CLONEABLE_REF(AnimationPlayer);

    protected:
        Ref<AnimationClip> mClip; // Animation clip

        IObject* mTarget = nullptr; // Target object

        Ref<AnimationState> mAnimationState = nullptr; // Animation state owner

        Vector<Ref<IAnimationTrack::IPlayer>> mTrackPlayers; // Animation clip track players

    protected:
        // Called when animation starts playing
        void OnPlay() override;

        // Evaluates all Animation tracks by time
        void Evaluate() override;

        // Creates clip tracks players and bind to properties from target
        void BindTracks(bool errors);

        // Binds animation track
        void BindTrack(const ObjectType* type, void* castedTarget, const Ref<IAnimationTrack>& track, bool errors);

        // Called when added new track in clip
        void OnClipTrackAdded(const Ref<IAnimationTrack>& track);

        // Called when removing track from clip
        void OnClipTrackRemove(const Ref<IAnimationTrack>& track);

        // Called when clip duration changed
        void OnClipDurationChanged(float duration);

        friend class AnimationComponent;
        friend class AnimationState;
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationPlayer)
{
    BASE_CLASS(o2::IAnimation);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::AnimationPlayer)
{
    FIELD().PUBLIC().NAME(onTrackPlayerAdded);
    FIELD().PUBLIC().NAME(onTrackPlayerRemove);
    FIELD().PROTECTED().NAME(mClip);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mTarget);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mAnimationState);
    FIELD().PROTECTED().NAME(mTrackPlayers);
}
END_META;
CLASS_METHODS_META(o2::AnimationPlayer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, IObject*, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, IObject*, bool);
    FUNCTION().PUBLIC().SIGNATURE(IObject*, GetTarget);
    FUNCTION().PUBLIC().SIGNATURE(void, SetClip, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationClip>&, GetClip);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<IAnimationTrack::IPlayer>>&, GetTrackPlayers);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlay);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, BindTracks, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, BindTrack, const ObjectType*, void*, const Ref<IAnimationTrack>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClipTrackAdded, const Ref<IAnimationTrack>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClipTrackRemove, const Ref<IAnimationTrack>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClipDurationChanged, float);
}
END_META;
// --- END META ---
