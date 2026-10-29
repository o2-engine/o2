#include "o2Editor/stdafx.h"
#include "ImageSlicesEditorWidget.h"

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Properties/Basic/BorderIntProperty.h"
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    ImageSlicesEditorWidget::ImageSlicesEditorWidget(RefCounter* refCounter):
        Widget(refCounter)
    {
        InitializeImagePreview();
    }

    ImageSlicesEditorWidget::ImageSlicesEditorWidget(RefCounter* refCounter, const ImageSlicesEditorWidget& other):
        Widget(refCounter, other)
    {
        InitializeImagePreview();
    }

    ImageSlicesEditorWidget& ImageSlicesEditorWidget::operator=(const ImageSlicesEditorWidget& other)
    {
        Widget::operator=(other);
        InitializeImagePreview();
        return *this;
    }

    void ImageSlicesEditorWidget::Setup(const AssetRef<ImageAsset>& image, const Ref<BorderIProperty>& borderProperty)
    {
        mBorderProperty = borderProperty;

        mPreviewImage->SetImageAsset(image);
        mPreviewImage->GetImage()->mode = SpriteMode::Default;

        FitImage();
    }

    void ImageSlicesEditorWidget::Setup(const AssetRef<VectorImageAsset>& image, const Ref<BorderIProperty>& borderProperty)
    {
        mBorderProperty = borderProperty;

        mPreviewImage->SetImageSource(AssetRef<Asset>(image ? image->GetUID() : UID::empty));
        if (auto sprite = DynamicCast<VectorSprite>(mPreviewImage->GetImageDrawable()))
            sprite->SetMode(SpriteMode::Default);

        FitImage();
    }

    Vec2F ImageSlicesEditorWidget::GetImageSize() const
    {
        auto drawable = mPreviewImage->GetImageDrawable();
        if (auto vector = DynamicCast<VectorSprite>(drawable))
            return vector->GetOriginalSize();

        if (auto sprite = DynamicCast<Sprite>(drawable))
            return sprite->GetOriginalSize();

        return Vec2F(1, 1);
    }

    Ref<IRectDrawable> ImageSlicesEditorWidget::GetPreviewDrawable() const
    {
        return mPreviewImage->GetImageDrawable();
    }

    String ImageSlicesEditorWidget::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ImageSlicesEditorWidget::InitializeImagePreview()
    {
        layout->minHeight = 200;

        auto separatorImg = o2UI.CreateImage("ui/UI4_Separator.svg");
        *separatorImg->layout = WidgetLayout::HorStretch(VerAlign::Bottom, -6, -15, 5, -4);
        AddChild(separatorImg);

        mPreviewImageBack = mmake<Image>();
        mPreviewImageBack->SetImage(CreateGridSprite());
        *mPreviewImageBack->layout = WidgetLayout::BothStretch();
        AddChild(mPreviewImageBack);

        mPreviewImage = mmake<PreviewImage>();
        *mPreviewImage->layout = WidgetLayout::BothStretch();
        AddChild(mPreviewImage);

        InitializeSliceHandles();
    }

    void ImageSlicesEditorWidget::InitializeSliceHandles()
    {
        // Left handle
        mBorderLeftHandle = mmake<WidgetDragHandle>(mmake<VectorSprite>("ui/ver_slice_line.svg"));

        mBorderLeftHandle->localToWidgetOffsetTransformFunc = [&](const Vec2F& point) {
            return point / GetImageSize() * mPreviewImage->layout->GetSize2D();
        };

        mBorderLeftHandle->widgetOffsetToLocalTransformFunc = [&](const Vec2F& point) {
            return point / mPreviewImage->layout->GetSize2D() * GetImageSize();
        };

        mBorderLeftHandle->checkPositionFunc = [&](const Vec2F& point) {
            return Vec2F(Math::Clamp(point.x, 0.0f, (float)GetImageSize().x), GetImageSize().y*0.5f);
        };

        mBorderLeftHandle->onChangedPos = [&](const Vec2F& point) {
            mBordersSmoothValue.left = point.x;
            UpdateBordersValue();
        };

        mBorderLeftHandle->cursorType = CursorType::SizeWE;

        // Right handle
        mBorderRightHandle = mmake<WidgetDragHandle>(*mBorderLeftHandle);

        mBorderRightHandle->onChangedPos = [&](const Vec2F& point) {
            mBordersSmoothValue.right = (float)GetImageSize().x - point.x;
            UpdateBordersValue();
        };

        mPreviewImage->AddChild(mBorderLeftHandle);
        mPreviewImage->AddChild(mBorderRightHandle);

        // Top handle
        mBorderTopHandle = mmake<WidgetDragHandle>(mmake<VectorSprite>("ui/hor_slice_line.svg"));

        mBorderTopHandle->localToWidgetOffsetTransformFunc = [&](const Vec2F& point) {
            return point / GetImageSize() * mPreviewImage->layout->GetSize2D();
        };

        mBorderTopHandle->widgetOffsetToLocalTransformFunc = [&](const Vec2F& point) {
            return point / mPreviewImage->layout->GetSize2D() * GetImageSize();
        };

        mBorderTopHandle->checkPositionFunc = [&](const Vec2F& point) {
            return Vec2F(GetImageSize().x*0.5f, Math::Clamp(point.y, 0.0f, (float)GetImageSize().y));
        };

        mBorderTopHandle->onChangedPos = [&](const Vec2F& point) {
            mBordersSmoothValue.top = (float)GetImageSize().y - point.y;
            UpdateBordersValue();
        };

        mBorderTopHandle->cursorType = CursorType::SizeNS;

        // Bottom handle
        mBorderBottomHandle = mmake<WidgetDragHandle>(*mBorderTopHandle);

        mBorderBottomHandle->onChangedPos = [&](const Vec2F& point) {
            mBordersSmoothValue.bottom = point.y;
            UpdateBordersValue();
        };

        mPreviewImage->AddChild(mBorderTopHandle);
        mPreviewImage->AddChild(mBorderBottomHandle);
    }

    void ImageSlicesEditorWidget::FitImage()
    {
        if (layout->IsDirty())
            UpdateTransform();

        Vec2F maxSize = layout->size2D;
        Vec2F imageSize = GetImageSize();
        if (imageSize.x <= 0.0f || imageSize.y <= 0.0f || maxSize.x <= 0.0f || maxSize.y <= 0.0f)
            return;

        float heightFitScale = maxSize.y/imageSize.y;
        float widthFitDifference = maxSize.x - imageSize.x*heightFitScale;

        float widthFitScale = maxSize.x/imageSize.x;
        float heightFitDifference = maxSize.y - imageSize.y*widthFitScale;

        bool fitByHeight = true;
        if (widthFitDifference > 0.0f && heightFitDifference > 0.0f)
            fitByHeight = heightFitDifference < widthFitDifference;
        else if (widthFitDifference > 0.0f)
            fitByHeight = true;
        else
            fitByHeight = false;

        float imageSizeAspect = imageSize.x/imageSize.y;
        float contentAspect = maxSize.x/maxSize.y;

        if (fitByHeight)
        {
            float d = imageSizeAspect*0.5f/contentAspect;
            *mPreviewImage->layout = WidgetLayout(Vec2F(0.5f - d, 0.0f), Vec2F(0.5f + d, 1.0f), Vec2F(), Vec2F());
        }
        else
        {
            float d = 1.0f/imageSizeAspect*0.5f*contentAspect;
            *mPreviewImage->layout = WidgetLayout(Vec2F(0.0f, 0.5f - d), Vec2F(1.0f, 0.5f + d), Vec2F(), Vec2F());
        }

        *mPreviewImageBack->layout = *mPreviewImage->layout;

        UpdateBordersAnchors();
        mBordersSmoothValue = mBorderProperty->GetCommonValue();
    }

    void ImageSlicesEditorWidget::UpdateBordersAnchors()
    {
        BorderI borders = mBorderProperty->GetCommonValue();
        Vec2F imageSize = GetImageSize();

        mPreviewImage->UpdateTransform();
        mBorderLeftHandle->GetRegularDrawable()->SetSize(Vec2F(10.0f, mPreviewImage->layout->GetHeight()));
        mBorderRightHandle->GetRegularDrawable()->SetSize(Vec2F(10.0f, mPreviewImage->layout->GetHeight()));
        mBorderTopHandle->GetRegularDrawable()->SetSize(Vec2F(mPreviewImage->layout->GetWidth(), 10.0f));
        mBorderBottomHandle->GetRegularDrawable()->SetSize(Vec2F(mPreviewImage->layout->GetWidth(), 10.0f));

        mBorderLeftHandle->SetPosition(Vec2F((float)borders.left, 0));
        mBorderRightHandle->SetPosition(Vec2F(imageSize.x - (float)borders.right, 0));
        mBorderTopHandle->SetPosition(Vec2F(0, imageSize.y - (float)borders.top));
        mBorderBottomHandle->SetPosition(Vec2F(0, (float)borders.bottom));
    }

    void ImageSlicesEditorWidget::UpdateBordersValue()
    {
        mBorderProperty->SetValue(mBordersSmoothValue);
        UpdateBordersAnchors();
    }

    Ref<Sprite> ImageSlicesEditorWidget::CreateGridSprite()
    {
        Color4 color1(1.0f, 1.0f, 1.0f, 1.0f), color2(0.7f, 0.7f, 0.7f, 1.0f);
        Bitmap backLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
        backLayerBitmap.Fill(color1);
        backLayerBitmap.FillRect(0, 10, 10, 0, color2);
        backLayerBitmap.FillRect(10, 20, 20, 10, color2);

        auto res = mmake<Sprite>(backLayerBitmap);
        res->SetMode(SpriteMode::Tiled);
        return res;
    }

    ImageSlicesEditorWidget::PreviewImage::PreviewImage(RefCounter* refCounter):
        Image(refCounter)
    {}

    void ImageSlicesEditorWidget::PreviewImage::Draw()
    {
        mIsClipped = false;

        // A vector image is drawn at the preview scale; a raster one shows its pixels
        auto sprite = GetImage();
        if (!sprite)
        {
            if (GetImageDrawable())
                Image::Draw();

            return;
        }

        if (auto texture = sprite->GetTexture())
        {
            auto prevFilter = texture->GetFilter();
            texture->SetFilter(Texture::Filter::Nearest);

            Image::Draw();

            texture->SetFilter(prevFilter);
        }
    }

    String ImageSlicesEditorWidget::PreviewImage::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ImageSlicesEditorWidget::PreviewImage>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ImageSlicesEditorWidget>);
// --- META ---

DECLARE_CLASS(Editor::ImageSlicesEditorWidget, Editor__ImageSlicesEditorWidget);

DECLARE_CLASS(Editor::ImageSlicesEditorWidget::PreviewImage, Editor__ImageSlicesEditorWidget__PreviewImage);
// --- END META ---
