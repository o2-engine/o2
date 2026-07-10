#pragma once

#include "o2/Render/SkinnedModelFormat.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Actor;
    class AnimationClip;

    // ------------------------------------------------------------------------------
    // Converts skinned model skeletons into scene actors and model animation clips
    // into engine AnimationClip with tracks on the bone actors transforms, so models
    // are animated by the standard AnimationComponent
    // ------------------------------------------------------------------------------
    namespace SkinnedModelAnimation
    {
        // Bone actor name of the node: node name with path separators sanitized, indexed when unnamed
        String GetBoneActorName(const SkinnedModelData& model, int nodeIndex);

        // Animation path from the model root actor to the node bone actor, like "child/Root/child/Spine"
        String GetBoneActorPath(const SkinnedModelData& model, int nodeIndex);

        // Creates bone actors under the root for skin joints, animated nodes and their ancestors.
        // Existing children with matching names are reused; local TRS is set to the model bind pose
        void CreateBoneActors(const Ref<Actor>& root, const SkinnedModelData& model);

        // Returns the node bone actor below the root, nullptr when missing
        Ref<Actor> FindBoneActor(const Ref<Actor>& root, const SkinnedModelData& model, int nodeIndex);

        // Converts quaternion keys to euler keys, unwrapping ±180 degrees discontinuities so
        // per-component interpolation follows the quaternion arc
        Vector<Vec3F> ConvertRotationKeysToEuler(const Vector<Quat>& rotations);

        // Converts the model clip to an engine animation clip with linear tracks
        // on bone actors transforms (position, eulerAngles, scale)
        Ref<AnimationClip> ConvertClip(const SkinnedModelData& model, int animationIndex);

        // Converts the model clip by name, nullptr when not found
        Ref<AnimationClip> ConvertClip(const SkinnedModelData& model, const String& animationName);
    }
}
