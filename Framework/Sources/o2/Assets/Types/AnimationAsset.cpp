#include "o2/stdafx.h"
#include "AnimationAsset.h"

#include "o2/Assets/Assets.h"

namespace o2
{
    AnimationAsset::AnimationAsset(const AnimationAsset& other):
        AssetWithDefaultMeta<AnimationAsset>(other), animation(other.animation)
    {}

    AnimationAsset::AnimationAsset(const AnimationClip& clip):
        AssetWithDefaultMeta<AnimationAsset>(), animation(clip)
    {}

    AnimationAsset& AnimationAsset::operator=(const AnimationAsset& other)
    {
        Asset::operator=(other);
        animation = other.animation;

        return *this;
    }

    Vector<String> AnimationAsset::GetFileExtensions()
    {
        return { "anim" };
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::AnimationAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::AnimationAsset>);
DECLARE_TEMPLATE_CLASS(o2::Ref<o2::AnimationAsset>);
// --- META ---

DECLARE_CLASS(o2::AnimationAsset);
// --- END META ---
