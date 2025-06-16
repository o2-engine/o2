#include "o2/stdafx.h"
#include "Image.h"

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/WidgetLayer.h"

namespace o2
{
    Image::Image(RefCounter* refCounter):
        Widget(refCounter)
    {
        mImage = DynamicCast<Sprite>(AddLayer("image", mmake<Sprite>())->GetDrawable());
    }

    Image::Image(RefCounter* refCounter, const Image& other):
        Widget(refCounter, other), image(this), imageAsset(this), imageName(this)
    {
        mImage = GetLayerDrawable<Sprite>("image");
        if (!mImage)
            mImage = DynamicCast<Sprite>(AddLayer("image", mmake<Sprite>())->GetDrawable());
    }

    Image& Image::operator=(const Image& other)
    {
        Widget::operator=(other);

        mImage = GetLayerDrawable<Sprite>("image");
        if (!mImage)
            mImage = DynamicCast<Sprite>(AddLayer("image", mmake<Sprite>())->GetDrawable());

        return *this;
    }

    void Image::SetImage(const Ref<Sprite>& sprite)
    {
        if (auto layer = FindLayer("image"))
        {
            layer->SetDrawable(sprite);
            mImage = sprite;
        }
    }

    Ref<Sprite> Image::GetImage()
    {
        return mImage.Lock();
    }

    void Image::SetImageAsset(const AssetRef<ImageAsset>& asset)
    {
        if (!mImage)
            mImage = GetLayerDrawable<Sprite>("image");

        if (!mImage)
            mImage = DynamicCast<Sprite>(AddLayer("image", mmake<Sprite>())->GetDrawable());

        auto image = mImage.Lock();
        if (image->GetImageAsset() == asset)
            return;

        image->LoadFromImage(asset);
    }

    AssetRef<ImageAsset> Image::GetImageAsset() const
    {
        if (mImage)
            return mImage.Lock()->GetImageAsset();

        return AssetRef<ImageAsset>();
    }

    void Image::SetImageName(const String& name)
    {
        SetImageAsset(AssetRef<ImageAsset>(name));
    }

    String Image::GetImageName() const
    {
        auto imageAsset = GetImageAsset();
        if (imageAsset.IsValid())
            return imageAsset->GetPath();

        return "";
    }

    String Image::GetCreateMenuGroup()
    {
        return "Basic";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Image>);
// --- META ---

DECLARE_CLASS(o2::Image, o2__Image);
// --- END META ---
