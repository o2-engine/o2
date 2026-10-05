#include "o2/stdafx.h"
#include "Image.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/WidgetLayer.h"

namespace o2
{
    Image::Image(RefCounter* refCounter):
        Widget(refCounter)
    {
        mImage = AddLayer("image", mmake<Sprite>())->GetDrawable();
    }

    Image::Image(RefCounter* refCounter, const Image& other):
        Widget(refCounter, other)
    {
        GetImageDrawable();
    }

    Image& Image::operator=(const Image& other)
    {
        Widget::operator=(other);

        mImage = nullptr;
        GetImageDrawable();

        return *this;
    }

    void Image::SetImage(const Ref<Sprite>& sprite)
    {
        SetImageDrawable(sprite);
    }

    Ref<Sprite> Image::GetImage()
    {
        return DynamicCast<Sprite>(GetImageDrawable());
    }

    void Image::SetImageDrawable(const Ref<IRectDrawable>& drawable)
    {
        if (auto layer = FindLayer("image"))
        {
            layer->SetDrawable(drawable);
            mImage = drawable;
        }
    }

    Ref<IRectDrawable> Image::GetImageDrawable()
    {
        auto layer = FindLayer("image");
        if (!layer)
            layer = AddLayer("image", nullptr);

        if (!layer->GetDrawable())
            layer->SetDrawable(mmake<Sprite>());

        mImage = layer->GetDrawable();
        return layer->GetDrawable();
    }

    void Image::SetImageSource(const AssetRef<Asset>& asset)
    {
        GetImageDrawable();

        auto layer = FindLayer("image");
        if (layer && layer->GetImage() != asset)
            mImage = layer->SetImage(asset);
    }

    AssetRef<Asset> Image::GetImageSource() const
    {
        if (auto layer = FindLayer("image"))
            return layer->GetImage();

        return AssetRef<Asset>();
    }

    void Image::SetImageAsset(const AssetRef<ImageAsset>& asset)
    {
        SetImageSource(asset);
    }

    AssetRef<ImageAsset> Image::GetImageAsset() const
    {
        return DynamicCast<ImageAsset>(GetImageSource().GetRef());
    }

    void Image::SetImageName(const String& name)
    {
        if (name.IsEmpty())
            SetImageSource(AssetRef<Asset>());
        else
            SetImageSource(o2Assets.GetAssetRef(name));
    }

    String Image::GetImageName() const
    {
        AssetRef<Asset> image = GetImageSource();
        return image ? image->GetPath() : String();
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
