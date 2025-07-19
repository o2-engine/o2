#include "o2/stdafx.h"
#include "WidgetState.h"

#include "o2/Scene/UI/Widget.h"

namespace o2
{
    WidgetState::WidgetState()
    {}

    WidgetState::WidgetState(const WidgetState& state):
        name(state.name), mState(state.mState), 
        offStateAnimationSpeed(state.offStateAnimationSpeed), state(this), animationAsset(this), animationClip(this)
    {
        mAnimation = state.mAnimation;
        mPlayer->SetClip(mAnimation ? mAnimation->animation : nullptr);
        mPlayer->relTime = mState ? 1.0f:0.0f;
    }

    WidgetState::~WidgetState()
    {}

    WidgetState::operator bool()
    {
        return GetState();
    }

    bool WidgetState::operator==(const WidgetState& other) const
    {
        return false;
    }

    void WidgetState::SetOwner(const Ref<Widget>& owner, bool errors)
    {
        mOwner = owner;
        mPlayer->SetTarget(const_cast<Widget*>(owner.Get()), errors);
        mPlayer->relTime = mState ? 1.0f : 0.0f;
    }

    const Ref<AnimationPlayer>& WidgetState::GetAnimationPlayer()
    {
        return mPlayer;
    }

    void WidgetState::SetAnimationAsset(const AssetRef<AnimationAsset>& asset)
    {
        mAnimation = asset;
        mPlayer->SetClip(mAnimation ? mAnimation->animation : nullptr);
    }

    const AssetRef<AnimationAsset>& WidgetState::GetAnimationAsset() const
    {
        return mAnimation;
    }

    void WidgetState::SetAnimationClip(const Ref<AnimationClip>& animation)
    {
        if (mAnimation && mAnimation.IsInstance())
            mAnimation->animation = animation;
        else
        {
            mAnimation.SetInstance(mmake<AnimationAsset>(animation).Get());
            mPlayer->SetClip(mAnimation->animation);
        }
    }

    Ref<AnimationClip>& WidgetState::GetAnimationClip()
    {
        if (mAnimation)
            return mAnimation->animation;

        static Ref<AnimationClip> empty;
        return empty;
    }

    void WidgetState::SetState(bool state)
    {
         if (mState == state && !mPlayer->IsPlaying())
             return;

        mState = state;

        if (state)
        {
            mPlayer->speed = 1.0f;
            mPlayer->PlayForward();

            onStateBecomesTrue();
        }
        else
        {
            mPlayer->speed = offStateAnimationSpeed;
            mPlayer->PlayBack();

            onStateBecomesFalse();
        }
    }

    void WidgetState::SetStateForcible(bool state)
    {
        if (mState == state)
        {
            if ((state && Math::Equals(mPlayer->GetRelativeTime(), 1.0f)) ||
                (!state && Math::Equals(mPlayer->GetRelativeTime(), 0.0f)))
            {
                return;
            }
        }

        mState = state;

        if (mState)
        {
            mPlayer->GoToEnd();
            mPlayer->Stop();
            onStateBecomesTrue();
            onStateFullyTrue();
        }
        else
        {
            mPlayer->GoToBegin();
            mPlayer->Stop();
            onStateBecomesFalse();
            onStateFullyFalse();
        }
    }

    bool WidgetState::GetState() const
    {
        return mState;
    }

    void WidgetState::Update(float dt)
    {
        if (mPlayer->IsPlaying())
        {
            mPlayer->Update(dt);

            if (!mPlayer->IsPlaying())
            {
                if (mState) 
                    onStateFullyTrue();
                else 
                    onStateFullyFalse();
            }
        }
    }

    void WidgetState::OnAnimationChanged()
    {
        mPlayer->SetClip(mAnimation ? mAnimation->animation : nullptr);
    }

    void WidgetState::OnDeserialized(const DataValue& node)
    {
        mPlayer->SetClip(mAnimation ? mAnimation->animation : nullptr);
    }

    void WidgetState::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        OnDeserialized(node);
    }

    WidgetState& WidgetState::operator=(bool state)
    {
        SetState(state);
        return *this;
    }

    void WidgetState::BeginPreview()
    {
        mPlayer->Stop();
    }

    void WidgetState::EndPreview()
    {
        mPlayer->Play();
    }

    Ref<Actor> WidgetState::GetPreviewActor() const
    {
        return mOwner.Lock();
    }

    Ref<IAnimation> WidgetState::GetPreviewPlayer() const
    {
        return mPlayer;
    }

    RefCounter* WidgetState::GetRefCounter() const
    {
        return RefCounterable::GetRefCounter();
    }
}
// --- META ---

DECLARE_CLASS(o2::WidgetState, o2__WidgetState);
// --- END META ---
