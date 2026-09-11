#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/IRectDrawable.h"
#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineValue.h"

using namespace o2;

namespace o2
{
    class Button;
    class DropDown;
    class EditBox;
    class HorizontalLayout;
    class HorizontalProgress;
    class Label;
    class Sprite;
    class Toggle;
}

namespace Editor
{
    // -------------------------------------------------------------------------------
    // Filled rectangle with rounded corners on the chosen sides; the card header band
    // -------------------------------------------------------------------------------
    class PipelineRoundedRect : public IRectDrawable
    {
    public:
        float radius = 8.0f;       // Corner radius in pixels @SERIALIZABLE
        bool  roundTop = true;     // Rounds the top corners @SERIALIZABLE
        bool  roundBottom = false; // Rounds the bottom corners @SERIALIZABLE

    public:
        // Default constructor
        PipelineRoundedRect() = default;

        // Copy-constructor
        PipelineRoundedRect(const PipelineRoundedRect& other);

        // Draws the rectangle as a filled polygon with arcs on the rounded corners
        void Draw() override;

        SERIALIZABLE(PipelineRoundedRect);
        CLONEABLE_REF(PipelineRoundedRect);
    };

    // ----------------------------------------
    // Factory helpers for the pipeline widgets
    // ----------------------------------------
    namespace PipelineControls
    {
        // Returns true while a card zoomed too far out draws its content only; controls skip drawing then
        bool IsFarView();

        // Sets the far view flag for the drawing that follows
        void SetFarView(bool far);

        // Draws a rounded rectangle outline; the width is in screen pixels
        void DrawRoundedFrame(const RectF& rect, float radius, const Color4& color, float widthPixels);

        // Creates a left-aligned label; a dim label uses the dim text color and wraps
        Ref<Label> MakeLabel(const String& text, bool dim = false);

        // Creates a single-line edit box or a word-wrapped multi-line one with the text
        Ref<EditBox> MakeEditBox(const String& text, bool multiline, const String& placeholder = "");

        // Creates a drop down with the items, inserting the value when it is not listed, and selects the value
        Ref<DropDown> MakeDropDown(const Vector<String>& items, const String& value);

        // Creates a check box toggle with the caption and the value
        Ref<Toggle> MakeCheckbox(const String& caption, bool value);

        // Creates a standard button with the caption
        Ref<Button> MakeButton(const String& caption);

        // Creates a "pipeline icon" button with the tinted icon and an optional back plate
        Ref<Button> MakeIconButton(const String& icon, const Color4& iconColor, const Color4& backColor);

        // Creates a "pipeline segment" toggle with the caption and the value
        Ref<Toggle> MakeSegment(const String& caption, bool value);

        extern const Color4 textColor;    // Regular text color of the pipeline controls
        extern const Color4 dimTextColor; // Dimmed text color for hints and secondary text
        extern const Color4 accentColor;  // Accent color of icons and highlights

        // Creates a horizontal row: a fixed-width caption label followed by the control
        Ref<HorizontalLayout> MakeRow(const String& label, const Ref<Widget>& control, float labelWidth = 64.0f);

        // Formats a slider value: an integer, one or two decimals depending on the step
        String FormatNumber(float value, float step);
    }

    // -----------------------------------------------------------------------------------
    // Row of controls that wraps onto new lines when the width runs out: each child takes
    // its minimum width, children without a maximum width share the spare width of their line
    // -----------------------------------------------------------------------------------
    class PipelineWrapRow : public Widget
    {
    public:
        float spacing = 4.0f;     // Gap between the controls of a line
        float lineHeight = 22.0f; // Height of one line
        float lineSpacing = 3.0f; // Gap between lines

    public:
        // Default constructor
        explicit PipelineWrapRow(RefCounter* refCounter);

        // Returns the height of the lines the enabled children take in the width
        float GetHeightForWidth(float width) const;

        // Updates the transform and lays the enabled children out into lines for the current width
        void UpdateSelfTransform() override;

        // Updates the widget and lays the children out again when one of them was enabled or disabled
        void Update(float dt) override;

        SERIALIZABLE(PipelineWrapRow);

    protected:
        Vector<int> mLaidOutEnabled; // Enabled flags of the children at the last layout

    protected:
        // Returns the width a child takes before sharing: its minimum width or a default
        static float ItemWidth(const Ref<Widget>& child);
    };

    // -------------------------------------------------------------
    // Slider row: caption, standard progress bar and the value text
    // -------------------------------------------------------------
    class PipelineSlider : public Widget
    {
    public:
        Function<void(float, bool)> onChanged; // Called with the value and the completed flag when the value changes

    public:
        // Default constructor
        explicit PipelineSlider(RefCounter* refCounter);

        // Sets the caption, the range, the step, the value and the suffix shown after the value
        void Setup(const String& label, float minValue, float maxValue, float step, float value, const String& suffix = "");

        // Sets the value snapped to the step and clamped to the range; notifies as completed when asked
        void SetValue(float value, bool notify = false);

        // Returns the current value
        float GetValue() const { return mValue; }

        // Returns the width of the track between the caption and the value
        float GetTrackWidth() const;

        // Updates the widget; fires the completed callback once the cursor is released after a change
        void Update(float dt) override;

        SERIALIZABLE(PipelineSlider);

    protected:
        float  mMin = 0.0f;   // Range minimum
        float  mMax = 1.0f;   // Range maximum
        float  mStep = 0.01f; // Value snapping step
        float  mValue = 0.0f; // Current value
        String mSuffix;       // Text appended to the value

        Ref<WidgetLayer>        mLabelLayer; // Caption text layer
        Ref<WidgetLayer>        mValueLayer; // Value text layer
        Ref<HorizontalProgress> mProgress;   // Progress bar used as the slider track

        bool mPendingComplete = false; // True after a user change until the cursor is released

    protected:
        // Called when the progress bar is changed by the user; snaps the value and notifies
        void OnProgressChanged(float value);

        // Updates the progress bar position and the value text
        void UpdateVisuals();
    };

    // ------------------------------------------------------------------------------
    // Checkerboard-backed image preview keeping the aspect ratio, with an empty hint
    // ------------------------------------------------------------------------------
    class PipelineImageView : public Widget
    {
    public:
        // Default constructor
        explicit PipelineImageView(RefCounter* refCounter);

        // Shows the bitmap, downscaled to 1024 px for display; null shows the hint instead
        void SetBitmap(const Ref<Bitmap>& bitmap);

        // Sets the text shown when there is no image
        void SetHint(const String& hint);

        // Returns true when an image is shown
        bool HasImage() const { return mHasImage; }

        // Returns the rectangle where the image is drawn (world space)
        RectF GetImageRect() const;

        // Returns the size of the source bitmap
        Vec2I GetImageSize() const { return mImageSize; }

        // Draws the view; zoomed far out a flat fill stands in for the checker tiles
        void Draw() override;

        SERIALIZABLE(PipelineImageView);

    protected:
        Ref<WidgetLayer> mCheckerLayer;     // Tiled checkerboard layer
        Ref<WidgetLayer> mImageLayer;       // Image sprite layer
        Ref<WidgetLayer> mHintLayer;        // Hint text layer, shown when there is no image
        Ref<WidgetLayer> mFrameLayer;       // Frame sprite layer
        Vec2I            mImageSize;        // Size of the source bitmap
        bool             mHasImage = false; // True when an image is shown

    protected:
        // Updates layers layouts
        void UpdateLayersLayouts() override;
    };

    // ---------------------------------
    // Scrollable multi-line text result
    // ---------------------------------
    class PipelineTextView : public Widget
    {
    public:
        // Default constructor
        explicit PipelineTextView(RefCounter* refCounter);

        // Sets the text; an empty text shows the hint instead
        void SetText(const String& text);

        // Sets the text shown when there is no result
        void SetHint(const String& hint);

        SERIALIZABLE(PipelineTextView);

    protected:
        Ref<class ScrollArea> mScroll;    // Scroll area holding the text label
        Ref<Label>            mLabel;     // Wrapped text label
        Ref<Label>            mHintLabel; // Hint label, shown when the text is empty
    };

    // -------------------------------------------------------
    // Colour swatch with a hex field; opens the colour picker
    // -------------------------------------------------------
    class PipelineColorField : public Widget
    {
    public:
        Function<void(const Color4&, bool)> onChanged; // Called with the color and the completed flag when the color changes

    public:
        // Default constructor
        explicit PipelineColorField(RefCounter* refCounter);

        // Sets the caption and the color
        void Setup(const String& label, const Color4& color);

        // Sets the color and updates the swatch and the hex text
        void SetColor(const Color4& color);

        // Returns the current color
        Color4 GetColor() const { return mColor; }

        SERIALIZABLE(PipelineColorField);

    protected:
        Color4           mColor;      // Current color
        Ref<WidgetLayer> mLabelLayer; // Caption text layer
        Ref<Button>      mSwatch;     // Color swatch button
        Ref<EditBox>     mHexEdit;    // Hex color edit box

    protected:
        // Called when the swatch is pressed; opens the color picker dialog
        void OnSwatchPressed();

        // Called when the hex edit is completed; applies the parsed color or restores the text
        void OnHexChanged(const WString& text);

        // Updates the swatch color and the hex text
        void UpdateVisuals();
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineRoundedRect)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineRoundedRect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(8.0f).NAME(radius);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(roundTop);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(roundBottom);
}
END_META;
CLASS_METHODS_META(Editor::PipelineRoundedRect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineRoundedRect&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;

CLASS_BASES_META(Editor::PipelineWrapRow)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineWrapRow)
{
    FIELD().PUBLIC().DEFAULT_VALUE(4.0f).NAME(spacing);
    FIELD().PUBLIC().DEFAULT_VALUE(22.0f).NAME(lineHeight);
    FIELD().PUBLIC().DEFAULT_VALUE(3.0f).NAME(lineSpacing);
    FIELD().PROTECTED().NAME(mLaidOutEnabled);
}
END_META;
CLASS_METHODS_META(Editor::PipelineWrapRow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHeightForWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(float, ItemWidth, const Ref<Widget>&);
}
END_META;

CLASS_BASES_META(Editor::PipelineSlider)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineSlider)
{
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mMin);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mMax);
    FIELD().PROTECTED().DEFAULT_VALUE(0.01f).NAME(mStep);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mValue);
    FIELD().PROTECTED().NAME(mSuffix);
    FIELD().PROTECTED().NAME(mLabelLayer);
    FIELD().PROTECTED().NAME(mValueLayer);
    FIELD().PROTECTED().NAME(mProgress);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPendingComplete);
}
END_META;
CLASS_METHODS_META(Editor::PipelineSlider)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const String&, float, float, float, float, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, float, bool);
    FUNCTION().PUBLIC().SIGNATURE(float, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTrackWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnProgressChanged, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisuals);
}
END_META;

CLASS_BASES_META(Editor::PipelineImageView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineImageView)
{
    FIELD().PROTECTED().NAME(mCheckerLayer);
    FIELD().PROTECTED().NAME(mImageLayer);
    FIELD().PROTECTED().NAME(mHintLayer);
    FIELD().PROTECTED().NAME(mFrameLayer);
    FIELD().PROTECTED().NAME(mImageSize);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasImage);
}
END_META;
CLASS_METHODS_META(Editor::PipelineImageView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBitmap, const Ref<Bitmap>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHint, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, HasImage);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetImageRect);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetImageSize);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
}
END_META;

CLASS_BASES_META(Editor::PipelineTextView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineTextView)
{
    FIELD().PROTECTED().NAME(mScroll);
    FIELD().PROTECTED().NAME(mLabel);
    FIELD().PROTECTED().NAME(mHintLabel);
}
END_META;
CLASS_METHODS_META(Editor::PipelineTextView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetText, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHint, const String&);
}
END_META;

CLASS_BASES_META(Editor::PipelineColorField)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineColorField)
{
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PROTECTED().NAME(mColor);
    FIELD().PROTECTED().NAME(mLabelLayer);
    FIELD().PROTECTED().NAME(mSwatch);
    FIELD().PROTECTED().NAME(mHexEdit);
}
END_META;
CLASS_METHODS_META(Editor::PipelineColorField)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const String&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetColor);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSwatchPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHexChanged, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisuals);
}
END_META;
// --- END META ---
