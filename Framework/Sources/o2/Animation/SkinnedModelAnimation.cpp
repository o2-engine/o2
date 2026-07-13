#include "o2/stdafx.h"
#include "SkinnedModelAnimation.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/Tracks/AnimationVec3FTrack.h"
#include "o2/Scene/Actor.h"

namespace o2::SkinnedModelAnimation
{
    namespace
    {
        // Marks the node and all its ancestors as bone actors
        void MarkWithAncestors(const SkinnedModelData& model, int nodeIndex, Vector<bool>& included)
        {
            while (nodeIndex >= 0 && nodeIndex < model.nodes.Count() && !included[nodeIndex])
            {
                included[nodeIndex] = true;
                nodeIndex = model.nodes[nodeIndex].parent;
            }
        }

        // Bone actors set: skin joints, animation channels targets and their ancestors
        Vector<bool> CollectBoneNodes(const SkinnedModelData& model)
        {
            Vector<bool> included;
            included.Resize(model.nodes.Count());
            for (int i = 0; i < included.Count(); i++)
                included[i] = false;

            for (int joint : model.joints)
                MarkWithAncestors(model, joint, included);

            for (auto& animation : model.animations)
            {
                for (auto& channel : animation.channels)
                    MarkWithAncestors(model, channel.node, included);
            }

            return included;
        }

        Vec3F UnwrapNear(const Vec3F& value, const Vec3F& previous)
        {
            const float twoPi = Math::PI()*2.0f;
            Vec3F res = value;
            res.x += Math::Round((previous.x - res.x)/twoPi)*twoPi;
            res.y += Math::Round((previous.y - res.y)/twoPi)*twoPi;
            res.z += Math::Round((previous.z - res.z)/twoPi)*twoPi;
            return res;
        }

        void AddLinearKeys(const Ref<AnimationClip>& clip, const String& path,
                           const Vector<float>& times, const Vector<Vec3F>& values, bool step)
        {
            auto track = clip->AddTrack<Vec3F>(path);

            int keysCount = Math::Min(times.Count(), values.Count());
            for (int i = 0; i < keysCount; i++)
            {
                // STEP interpolation holds the previous value until the next key
                if (step && i > 0 && times[i] - times[i - 1] > 0.002f)
                    track->AddKey(times[i] - 0.001f, values[i - 1]);

                track->AddKey(times[i], values[i]);
            }
        }
    }

    String GetBoneActorName(const SkinnedModelData& model, int nodeIndex)
    {
        if (nodeIndex < 0 || nodeIndex >= model.nodes.Count())
            return String();

        String name = model.nodes[nodeIndex].name;
        if (name.IsEmpty())
            return String("node") + (String)nodeIndex;

        return name.ReplacedAll("/", "_");
    }

    String GetBoneActorPath(const SkinnedModelData& model, int nodeIndex)
    {
        if (nodeIndex < 0 || nodeIndex >= model.nodes.Count())
            return String();

        String path = String("child/") + GetBoneActorName(model, nodeIndex);
        int parent = model.nodes[nodeIndex].parent;
        while (parent >= 0 && parent < model.nodes.Count())
        {
            path = String("child/") + GetBoneActorName(model, parent) + "/" + path;
            parent = model.nodes[parent].parent;
        }

        return path;
    }

    void CreateBoneActors(const Ref<Actor>& root, const SkinnedModelData& model)
    {
        if (!root)
            return;

        Vector<bool> included = CollectBoneNodes(model);

        Vector<Ref<Actor>> actors;
        actors.Resize(model.nodes.Count());

        Function<Ref<Actor>(int)> resolve = [&](int nodeIndex) -> Ref<Actor>
        {
            if (actors[nodeIndex])
                return actors[nodeIndex];

            Ref<Actor> parent = root;
            int parentNode = model.nodes[nodeIndex].parent;
            if (parentNode >= 0 && parentNode < model.nodes.Count())
                parent = resolve(parentNode);

            String name = GetBoneActorName(model, nodeIndex);
            Ref<Actor> actor = parent->GetChild(name);
            if (!actor)
            {
                actor = mmake<Actor>();
                actor->SetName(name);
                parent->AddChild(actor);
            }

            const SkinnedModelData::Node& node = model.nodes[nodeIndex];
            actor->transform->SetSize(Vec3F());
            actor->transform->SetPosition(node.position);
            actor->transform->SetRotation(node.rotation);
            actor->transform->SetScale(node.scale);

            actors[nodeIndex] = actor;
            return actor;
        };

        for (int i = 0; i < model.nodes.Count(); i++)
        {
            if (included[i])
                resolve(i);
        }
    }

    Ref<Actor> FindBoneActor(const Ref<Actor>& root, const SkinnedModelData& model, int nodeIndex)
    {
        if (!root || nodeIndex < 0 || nodeIndex >= model.nodes.Count())
            return nullptr;

        String path = GetBoneActorName(model, nodeIndex);
        int parent = model.nodes[nodeIndex].parent;
        while (parent >= 0 && parent < model.nodes.Count())
        {
            path = GetBoneActorName(model, parent) + "/" + path;
            parent = model.nodes[parent].parent;
        }

        return root->GetChild(path);
    }

    Vector<Vec3F> ConvertRotationKeysToEuler(const Vector<Quat>& rotations)
    {
        Vector<Vec3F> result;
        result.Reserve(rotations.Count());

        Vec3F previous;
        for (int i = 0; i < rotations.Count(); i++)
        {
            Vec3F euler = rotations[i].Normalized().ToEuler();
            if (i > 0)
            {
                // The same rotation has a mirrored euler representation; both candidates are
                // unwrapped by 360 degrees steps and the closest to the previous key is taken
                Vec3F mirrored(euler.x + Math::PI(), Math::PI() - euler.y, euler.z + Math::PI());

                Vec3F direct = UnwrapNear(euler, previous);
                Vec3F alternative = UnwrapNear(mirrored, previous);

                float directDistance = (direct - previous).SqrLength();
                float alternativeDistance = (alternative - previous).SqrLength();

                euler = directDistance <= alternativeDistance ? direct : alternative;
            }

            result.Add(euler);
            previous = euler;
        }

        return result;
    }

    Ref<AnimationClip> ConvertClip(const SkinnedModelData& model, int animationIndex)
    {
        if (animationIndex < 0 || animationIndex >= model.animations.Count())
            return nullptr;

        auto clip = mmake<AnimationClip>();
        clip->SetLoop(Loop::Repeat);

        const SkinnedModelData::AnimationClip& animation = model.animations[animationIndex];
        for (auto& channel : animation.channels)
        {
            if (channel.node < 0 || channel.node >= model.nodes.Count() || channel.times.IsEmpty())
                continue;

            String bonePath = GetBoneActorPath(model, channel.node);

            switch (channel.path)
            {
                case SkinnedModelData::AnimationChannel::Path::Translation:
                    AddLinearKeys(clip, bonePath + "/transform/position", channel.times, channel.vectors, channel.step);
                    break;

                case SkinnedModelData::AnimationChannel::Path::Scale:
                    AddLinearKeys(clip, bonePath + "/transform/scale", channel.times, channel.vectors, channel.step);
                    break;

                case SkinnedModelData::AnimationChannel::Path::Rotation:
                    AddLinearKeys(clip, bonePath + "/transform/eulerAngles", channel.times,
                                  ConvertRotationKeysToEuler(channel.rotations), channel.step);
                    break;
            }
        }

        return clip;
    }

    Ref<AnimationClip> ConvertClip(const SkinnedModelData& model, const String& animationName)
    {
        return ConvertClip(model, model.FindAnimation(animationName));
    }
}
