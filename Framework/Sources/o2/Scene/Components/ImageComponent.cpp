#include "o2/stdafx.h"
#include "ImageComponent.h"

#include "o2/Scene/Actor.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"

namespace o2
{
    ImageComponent::ImageComponent()
    {
        mSerializeEnabled = false;
    }

    ImageComponent::ImageComponent(const AssetRef<ImageAsset>& image) :
        Sprite(image)
    {
        mSerializeEnabled = false;
    }

    ImageComponent::ImageComponent(const String& imagePath) :
        Sprite(imagePath)
    {
        mSerializeEnabled = false;
    }

    ImageComponent::ImageComponent(TextureRef texture, const RectI& srcRect) :
        Sprite(texture, srcRect)
    {
        mSerializeEnabled = false;
    }

    ImageComponent::ImageComponent(const Color4& color) :
        Sprite(color)
    {
        mSerializeEnabled = false;
    }

    ImageComponent::ImageComponent(const Bitmap& bitmap) :
        Sprite(bitmap)
    {
        mSerializeEnabled = false;
    }

    ImageComponent::ImageComponent(const Sprite& sprite) :
        Sprite(sprite)
    {
        mSerializeEnabled = false;
    }

    ImageComponent::ImageComponent(const ImageComponent& other) :
        Component(other), Sprite(other)
    {
        mSerializeEnabled = false;
    }

    ImageComponent::~ImageComponent()
    {}

    ImageComponent& ImageComponent::operator=(const ImageComponent& other)
    {
        Component::operator=(other);
        Sprite::operator=(other);
        return *this;
    }

    void ImageComponent::OnDraw()
    {
        //PROFILE_SAMPLE_FUNC();

        Sprite::Draw();
    }

    void ImageComponent::FitActorByImage() const
    {
        if (mImageAsset)
            mOwner.Lock()->transform->size = mImageAsset->GetSize();
    }

    bool ImageComponent::IsUnderPoint(const Vec2F& point)
    {
        return Sprite::IsUnderPoint(point);
    }

    String ImageComponent::GetName()
    {
        return "Image";
    }

    String ImageComponent::GetCategory()
    {
        return "Render";
    }

    String ImageComponent::GetIcon()
    {
        return "ui/UI4_image_component.png";
    }

    Ref<o2::RefCounterable> ImageComponent::CastToRefCounterable(const Ref<ImageComponent>& ref)
    {
        return DynamicCast<Component>(ref);
    }

    void ImageComponent::OnTransformUpdated()
    {
        SetBasis(mOwner.Lock()->transform->GetWorldBasis());
    }

    void ImageComponent::SetOwnerActor(const Ref<Actor>& actor)
    {
        Component::SetOwnerActor(actor);
    }

    void ImageComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        Sprite::OnDeserialized(node);
    }

    void ImageComponent::OnSerialize(DataValue& node) const
    {
        Component::OnSerialize(node);
        Sprite::OnSerialize(node);
    }

    void ImageComponent::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        Component::OnSerializeDelta(node, origin);
        Sprite::OnSerializeDelta(node, origin);
    }

    void ImageComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        Sprite::OnDeserializedDelta(node, origin);
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::ImageComponent>);
// --- META ---

DECLARE_CLASS(o2::ImageComponent, o2__ImageComponent);
// --- END META ---
