#include "o2/stdafx.h"
#include "VideoComponent.h"

#include "o2/Scene/Actor.h"

namespace o2
{
    VideoComponent::VideoComponent()
    {
        mSerializeEnabled = false;
    }

    VideoComponent::VideoComponent(const AssetRef<VideoAsset>& video):
        Video(video)
    {
        mSerializeEnabled = false;
    }

    VideoComponent::VideoComponent(const VideoComponent& other):
        Component(other), Video(other)
    {
        mSerializeEnabled = false;
    }

    VideoComponent::~VideoComponent()
    {}

    VideoComponent& VideoComponent::operator=(const VideoComponent& other)
    {
        Component::operator=(other);
        Video::operator=(other);
        return *this;
    }

    void VideoComponent::OnDraw()
    {
        Video::Draw();
    }

    void VideoComponent::OnStart()
    {
        if (mPlayOnAwake)
            Play();
    }

    void VideoComponent::OnUpdate(float dt)
    {
        Video::Update(dt);
    }

    void VideoComponent::FitActorByVideo() const
    {
        if (mVideoSize.x > 0 && mVideoSize.y > 0)
            mOwner.Lock()->transform->size2D = (Vec2F)mVideoSize;
    }

    bool VideoComponent::IsUnderPoint(const Vec2F& point)
    {
        return Video::IsUnderPoint(point);
    }

    String VideoComponent::GetName()
    {
        return "Video";
    }

    String VideoComponent::GetCategory()
    {
        return "Render";
    }

    String VideoComponent::GetIcon()
    {
        return "ui/UI4_image_component.png";
    }

    Ref<o2::RefCounterable> VideoComponent::CastToRefCounterable(const Ref<VideoComponent>& ref)
    {
        return DynamicCast<Component>(ref);
    }

    void VideoComponent::OnTransformUpdated()
    {
        SetBasis(mOwner.Lock()->transform->GetWorldBasis());
    }

    void VideoComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        Video::OnDeserialized(node);
    }

    void VideoComponent::OnSerialize(DataValue& node) const
    {
        Component::OnSerialize(node);
        Video::OnSerialize(node);
    }

    void VideoComponent::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        Component::OnSerializeDelta(node, origin);
        Video::OnSerialize(node);
    }

    void VideoComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        Video::OnDeserialized(node);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::VideoComponent>);
// --- META ---

DECLARE_CLASS(o2::VideoComponent, o2__VideoComponent);
// --- END META ---
