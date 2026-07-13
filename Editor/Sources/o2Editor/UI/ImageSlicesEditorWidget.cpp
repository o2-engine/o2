#include "o2Editor/stdafx.h"
#include "ImageSlicesEditorWidget.h"

#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Properties/Basic/BorderIntProperty.h"

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

    String ImageSlicesEditorWidget::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ImageSlicesEditorWidget::InitializeImagePreview()
    {
        layout->minHeight = 200;

        auto separatorImg = o2UI.CreateImage("ui/UI4_Separator.png");
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
        mBorderLeftHandle = mmake<WidgetDragHandle>(mmake<Sprite>("ui/ver_slice_line.png"));

        mBorderLeftHandle->localToWidgetOffsetTransformFunc = [&](const Vec2F& point) {
            return point / mPreviewImage->GetImage()->GetOriginalSize() * mPreviewImage->layout->GetSize2D();
        };

        mBorderLeftHandle->widgetOffsetToLocalTransformFunc = [&](const Vec2F& point) {
            return point / mPreviewImage->layout->GetSize2D() * mPreviewImage->GetImage()->GetOriginalSize();
        };

        mBorderLeftHandle->checkPositionFunc = [&](const Vec2F& point) {
            return Vec2F(Math::Clamp(point.x, 0.0f, (float)mPreviewImage->GetImage()->GetOriginalSize().x), mPreviewImage->GetImage()->GetOriginalSize().y*0.5f);
        };

        mBorderLeftHandle->onChangedPos = [&](const Vec2F& point) {
            mBordersSmoothValue.left = point.x;
            UpdateBordersValue();
        };

        mBorderLeftHandle->cursorType = CursorType::SizeWE;

        // Right handle
        mBorderRightHandle = mmake<WidgetDragHandle>(*mBorderLeftHandle);

        mBorderRightHandle->onChangedPos = [&](const Vec2F& point) {
            mBordersSmoothValue.right = (float)mPreviewImage->GetImage()->GetOriginalSize().x - point.x;
            UpdateBordersValue();
        };

        mPreviewImage->AddChild(mBorderLeftHandle);
        mPreviewImage->AddChild(mBorderRightHandle);

        // Top handle
        mBorderTopHandle = mmake<WidgetDragHandle>(mmake<Sprite>("ui/hor_slice_line.png"));

        mBorderTopHandle->localToWidgetOffsetTransformFunc = [&](const Vec2F& point) {
            return point / mPreviewImage->GetImage()->GetOriginalSize() * mPreviewImage->layout->GetSize2D();
        };

        mBorderTopHandle->widgetOffsetToLocalTransformFunc = [&](const Vec2F& point) {
            return point / mPreviewImage->layout->GetSize2D() * mPreviewImage->GetImage()->GetOriginalSize();
        };

        mBorderTopHandle->checkPositionFunc = [&](const Vec2F& point) {
            return Vec2F(mPreviewImage->GetImage()->GetOriginalSize().x*0.5f, Math::Clamp(point.y, 0.0f, (float)mPreviewImage->GetImage()->GetOriginalSize().y));
        };

        mBorderTopHandle->onChangedPos = [&](const Vec2F& point) {
            mBordersSmoothValue.top = (float)mPreviewImage->GetImage()->GetOriginalSize().y - point.y;
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
        Vec2F imageSize = mPreviewImage->GetImage()->GetOriginalSize();

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
        Vec2F imageSize = mPreviewImage->GetImage()->GetOriginalSize();

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
        if (auto texture = mImage.Lock()->GetTexture())
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
