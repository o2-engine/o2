#include "o2/stdafx.h"
#include "Spine.h"

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"

#include "spine/SkeletonRenderer.h"
#include "spine/Animation.h"

namespace o2
{
    Spine::Spine(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    Spine::Spine(RefCounter* refCounter, const Spine& other):
        Spine(refCounter, other.mSpineAsset)
    {}

    Spine::Spine(RefCounter* refCounter, const AssetRef<SpineAsset>& spine):
        RefCounterable(refCounter)
    {
        Load(spine);
    }

    Spine::~Spine()
    {
        Unload();
    }

    Spine& Spine::operator=(const Spine& other)
    {
        Unload();

        return *this;
    }

    void Spine::Load(const AssetRef<SpineAsset>& spine)
    {
        Unload();

        mSpineAsset = spine;

        if (!mSpineAsset || !mSpineAsset->GetSpineSkeletonData() || !mSpineAsset->GetSpineAnimationStateData())
            return;

        // Load skeleton and animation state
        mSkeleton = mnew spine::Skeleton(mSpineAsset->GetSpineSkeletonData());
        mAnimationState = mnew spine::AnimationState(mSpineAsset->GetSpineAnimationStateData());

        // Setup pose
        mAnimationState->apply(*mSkeleton);
        mSkeleton->updateWorldTransform(spine::Physics::Physics_None);

        // Get animation names
        mAnimationNames.Clear();
        auto animations = mSkeleton->getData()->getAnimations();
        for (int i = 0; i < animations.size(); ++i)
            mAnimationNames.Add(animations[i]->getName().buffer());
    }

    void Spine::Unload()
    {
        if (mSkeleton)
        {
            delete mSkeleton;
            mSkeleton = nullptr;
        }

        if (mAnimationState)
        {
            delete mAnimationState;
            mAnimationState = nullptr;
        }
    }

    void Spine::Draw()
    {
        static spine::SkeletonRenderer skeletonRenderer;
        static Map<spine::BlendMode, BlendMode> blendModes = {
            { spine::BlendMode_Normal, BlendMode::Normal },
            { spine::BlendMode_Additive, BlendMode::Add },
            { spine::BlendMode_Multiply, BlendMode::Normal },
            { spine::BlendMode_Screen, BlendMode::Normal }
        };

        spine::RenderCommand* command = skeletonRenderer.render(*mSkeleton);
        while (command) 
        {
            Vertex vertex;

            float* positions = command->positions;
            float* uvs = command->uvs;
            uint32_t* colors = command->colors;
            uint16_t* indices = command->indices;
            Texture* texture = (Texture*)command->texture;

            mVertices.Clear();
            mIndices.Clear();

            for (int i = 0; i < command->numVertices; ++i)
            {
                int ii = i*2;
                Vec2F position(positions[ii], positions[ii + 1]);
                position = transform * position;

                vertex.x = position.x;
                vertex.y = position.y;
                vertex.tu = uvs[ii];
                vertex.tv = 1.0f - uvs[ii + 1];
                vertex.color = colors[i];
                mVertices.Add(vertex);
            }

            for (int i = 0, n = command->numIndices; i < n; ++i)
                mIndices.Add(indices[i]);

            BlendMode blendMode = blendModes[command->blendMode];
            TextureRef textureRef = TextureRef((Texture*)command->texture);

            o2Render.DrawBuffer(PrimitiveType::Polygon, mVertices.Data(), command->numVertices, mIndices.Data(), command->numIndices/3, textureRef, blendMode);

            if (o2Input.IsKeyDown(VK_F3))
                o2Render.DrawMeshBufferWire(mVertices.Data(), command->numVertices, mIndices.Data(), command->numIndices/3, Color4::Red());
                
            command = command->next;
        }

        OnDrawn();
    }

    void Spine::Update(float dt)
    {
        if (!mSkeleton || !mAnimationState)
            return;

        mAnimationState->update(dt);
        mAnimationState->apply(*mSkeleton);

        mSkeleton->update(dt);
        mSkeleton->updateWorldTransform(spine::Physics::Physics_Update);
    }

    const Vector<String>& Spine::GetAnimationNames() const
    {
        return mAnimationNames;
    }

    Ref<Spine::Track> Spine::GetTrack(const String& name)
    {
        int trackIndex = mAnimationNames.IndexOf(name);
        if (trackIndex == -1)
        {
            o2Debug.LogError("Spine animation %s not found", name.Data());
            return nullptr;
        }

        return mmake<Track>(Ref(this), trackIndex, name);
    }

    Spine::Track::Track(const Ref<Spine>& owner, int trackIndex, const String& name)
    {
        mOwner = owner;
        mAnimationName = name;
        mTrackIndex = trackIndex;
    }

    void Spine::Track::SetTime(float time)
    {
        auto tmpTrackEntry = mTrackEntry;
        mTrackEntry = nullptr;

        IAnimation::SetTime(time);

        mTrackEntry = tmpTrackEntry;
        if (mOwner && mTrackEntry)
            mTrackEntry->setTrackTime(time);
    }

    void Spine::Track::SetWeight(float weight)
    {
        if (mOwner && mTrackEntry)
            mTrackEntry->setAlpha(weight);
    }

    float Spine::Track::GetWeight() const
    {
        if (mOwner && mTrackEntry)
            return mTrackEntry->getAlpha();

        return 0.0f;
    }

    void Spine::Track::Evaluate()
    {
        if (mOwner && mTrackEntry)
        {
            float x;
            mInDurationTime = modff(mTrackEntry->getTrackTime()/mDuration, &x)*mDuration;
        }
    }

    void Spine::Track::OnPlay()
    {
        if (!mOwner)
            return;

        mTrackEntry = mOwner.Lock()->mAnimationState->setAnimation(mTrackIndex, mAnimationName.Data(), mLoop == Loop::Repeat);
        mDuration = mTrackEntry->getAnimation()->getDuration();
        ResetBounds();
    }

    void Spine::Track::OnStop()
    {
        if (!mOwner)
            return;

        mOwner.Lock()->mAnimationState->clearTrack(mTrackIndex);
    }

    void Spine::Track::OnLoopChanged()
    {
        if (mOwner && mTrackEntry)
            mTrackEntry->setLoop(mLoop == Loop::Repeat);
    }

}
// --- META ---

DECLARE_CLASS(o2::Spine::Track, o2__Spine__Track);
// --- END META ---
