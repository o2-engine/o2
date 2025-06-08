#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Image.h"

using namespace o2;

namespace o2
{
    FORWARD_CLASS_REF(WidgetDragHandle);
}

namespace Editor
{
    FORWARD_CLASS_REF(BorderIProperty);

    // -----------------------------
    // Editor of image slices widget
    // -----------------------------
    class ImageSlicesEditorWidget : public Widget
    {
    public:
        // Default constructor
        ImageSlicesEditorWidget(RefCounter* refCounter);

        // Default copy-constructor
        ImageSlicesEditorWidget(RefCounter* refCounter, const ImageSlicesEditorWidget& other);

        // Copy operator
        ImageSlicesEditorWidget& operator=(const ImageSlicesEditorWidget& other);

        // Sets image and property
        void Setup(const AssetRef<ImageAsset>& image, const Ref<BorderIProperty>& borderProperty);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(ImageSlicesEditorWidget);
        CLONEABLE_REF(ImageSlicesEditorWidget);

    public:
        // --------------------
        // Image preview widget
        // --------------------
        class PreviewImage : public Image
        {
        public:
            // Default constructor
            PreviewImage(RefCounter* refCounter);

            // Sets texture filter to nearest and draws it
            void Draw() override;

            // Returns create menu category in editor
            static String GetCreateMenuCategory();

            SERIALIZABLE(PreviewImage);
            CLONEABLE_REF(PreviewImage);
        };

    private:
        Ref<PreviewImage> mPreviewImage;     // Image preview widget
        Ref<Image>        mPreviewImageBack; // Image preview background with chessmate pattern

        Ref<WidgetDragHandle> mBorderLeftHandle;   // Left border handle
        Ref<WidgetDragHandle> mBorderRightHandle;  // Right border handle
        Ref<WidgetDragHandle> mBorderTopHandle;    // Top border handle
        Ref<WidgetDragHandle> mBorderBottomHandle; // Bottom border handle

        BorderF mBordersSmoothValue; // Smoothed borders values

        Ref<BorderIProperty> mBorderProperty; // Image borders property

    protected:
        // Initializes image preview widgets and border handles
        void InitializeImagePreview();

        // Initializes image slices handles
        void InitializeSliceHandles();

        // Fits image by size
        void FitImage();

        // Updates borders anchors layouts
        void UpdateBordersAnchors();

        // Updates targets assets borders values by floored mBordersSmoothValue
        void UpdateBordersValue();

        // Creates grid sprite
        Ref<Sprite> CreateGridSprite();
    };
}
// --- META ---

CLASS_BASES_META(Editor::ImageSlicesEditorWidget)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::ImageSlicesEditorWidget)
{
    FIELD().PRIVATE().NAME(mPreviewImage);
    FIELD().PRIVATE().NAME(mPreviewImageBack);
    FIELD().PRIVATE().NAME(mBorderLeftHandle);
    FIELD().PRIVATE().NAME(mBorderRightHandle);
    FIELD().PRIVATE().NAME(mBorderTopHandle);
    FIELD().PRIVATE().NAME(mBorderBottomHandle);
    FIELD().PRIVATE().NAME(mBordersSmoothValue);
    FIELD().PRIVATE().NAME(mBorderProperty);
}
END_META;
CLASS_METHODS_META(Editor::ImageSlicesEditorWidget)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ImageSlicesEditorWidget&);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const AssetRef<ImageAsset>&, const Ref<BorderIProperty>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeImagePreview);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeSliceHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, FitImage);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateBordersAnchors);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateBordersValue);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Sprite>, CreateGridSprite);
}
END_META;

CLASS_BASES_META(Editor::ImageSlicesEditorWidget::PreviewImage)
{
    BASE_CLASS(o2::Image);
}
END_META;
CLASS_FIELDS_META(Editor::ImageSlicesEditorWidget::PreviewImage)
{
}
END_META;
CLASS_METHODS_META(Editor::ImageSlicesEditorWidget::PreviewImage)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
}
END_META;
// --- END META ---
