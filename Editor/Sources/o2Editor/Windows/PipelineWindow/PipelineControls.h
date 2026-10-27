#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/IRectDrawable.h"
#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Windows/PipelineWindow/PipelineRichText.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
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

    // ------------------------------------------------------------------------
    // Rounded rectangle outline drawn inside the rectangle, like a CSS border
    // ------------------------------------------------------------------------
    class PipelineRoundedFrame : public IRectDrawable
    {
    public:
        float radius = 8.0f; // Corner radius @SERIALIZABLE
        float width = 1.0f;  // Line width, inside the rectangle @SERIALIZABLE

    public:
        // Default constructor
        PipelineRoundedFrame() = default;

        // Copy-constructor
        PipelineRoundedFrame(const PipelineRoundedFrame& other);

        // Draws the antialiased outline
        void Draw() override;

        SERIALIZABLE(PipelineRoundedFrame);
        CLONEABLE_REF(PipelineRoundedFrame);
    };

    // -----------------------------------------------------------------------
    // Plate of a floating menu: the popover shadow, the surface and its border
    // -----------------------------------------------------------------------
    class PipelineMenuBack : public IRectDrawable
    {
    public:
        float radius = 12.0f; // Corner radius @SERIALIZABLE

    public:
        // Default constructor
        PipelineMenuBack() = default;

        // Copy-constructor
        PipelineMenuBack(const PipelineMenuBack& other);

        // Draws the shadow, the fill and the border in the current theme
        void Draw() override;

        SERIALIZABLE(PipelineMenuBack);
        CLONEABLE_REF(PipelineMenuBack);
    };

    // ----------------------------------------------------------------------------------
    // Port marker: the shape says the data type - circle for text, square for image,
    // diamond for audio, triangle for video - so a link still reads without its colour
    // ----------------------------------------------------------------------------------
    class PipelinePortMarker : public IRectDrawable
    {
    public:
        PipelinePortType portType = PipelinePortType::Text; // Type the shape stands for @SERIALIZABLE

    public:
        // Default constructor
        PipelinePortMarker() = default;

        // Copy-constructor
        PipelinePortMarker(const PipelinePortMarker& other);

        // Draws the shape of the port type filling the rectangle
        void Draw() override;

        SERIALIZABLE(PipelinePortMarker);
        CLONEABLE_REF(PipelinePortMarker);
    };

    // ---------------------------------------------------------------------------------
    // Fold arrow of a collapsible row: a triangle pointing right when closed, turning to
    // point down as it opens
    // ---------------------------------------------------------------------------------
    class PipelineFoldArrow : public IRectDrawable
    {
    public:
        float open = 0.0f; // 0 closed (pointing right), 1 open (pointing down) @SERIALIZABLE

    public:
        // Default constructor
        PipelineFoldArrow() = default;

        // Copy-constructor
        PipelineFoldArrow(const PipelineFoldArrow& other);

        // Draws the triangle centred in the rectangle, turned by the open amount
        void Draw() override;

        SERIALIZABLE(PipelineFoldArrow);
        CLONEABLE_REF(PipelineFoldArrow);
    };

    // ----------------------------------------
    // Factory helpers for the pipeline widgets
    // ----------------------------------------
    namespace PipelineControls
    {
        // Returns true while a card zoomed too far out draws its content only; controls skip drawing then
        bool IsFarView();

        // Sets the far view flag for the drawing that follows
        void SetFarView(bool enabled);

        // Draws a rounded rectangle outline; the width is in screen pixels
        void DrawRoundedFrame(const RectF& rect, float radius, const Color4& color, float widthPixels);

        // AssetsLine control metrics (styles.css, node body); text sizes in pixels
        constexpr float bodyFont = 11.5f;     // Body text, fields, row captions
        constexpr float smallFont = 11.0f;    // Switch captions, segments, buttons, hints
        constexpr float monoFont = 10.5f;     // Slider values and metadata in the mono font
        constexpr float fieldRadius = 9.0f;   // Fields and text results
        constexpr float controlRadius = 8.0f; // Buttons, selects, wells (--r-ctl)
        constexpr float rowLabelWidth = 92.0f;// Caption column of a parameter row (.prow)
        constexpr float rowGap = 8.0f;        // Gap between a row caption and its control
        constexpr float switchWidth = 32.0f;  // Switch track (.tg)
        constexpr float switchHeight = 18.0f;

        // Returns a text drawable in the theme font
        Ref<Text> MakeThemeText(const String& text, float size, int weight, const Color4& color,
                           HorAlign horAlign = HorAlign::Left, VerAlign verAlign = VerAlign::Middle);

        // Sets the theme font, size and colour of a label
        void StyleLabel(const Ref<Label>& label, float size, int weight, const Color4& color);

        // Adds a rounded fill layer whose strength comes from the colour alpha, as layer transparency
        Ref<WidgetLayer> AddFillLayer(Widget& widget, const String& name, const Color4& color, float radius,
                                      const Layout& layout = Layout::BothStretch(), float depth = 0.0f);

        // Adds a rounded border layer drawn inside the rectangle; its strength comes from the colour alpha
        Ref<WidgetLayer> AddBorderLayer(Widget& widget, const String& name, const Color4& color, float radius, float width = 1.0f,
                                        const Layout& layout = Layout::BothStretch(), float depth = 0.0f);

        // Creates a caption label: the dim one is a muted, wrapping hint
        Ref<Label> MakeLabel(const String& text, bool dim = false);

        // Creates a field: a single-line input or a word-wrapped multi-line one, with the placeholder shown while empty
        Ref<EditBox> MakeEditBox(const String& text, bool multiline, const String& placeholder = "");

        // Restyles an edit box as a field: fill, border, accent border while focused, theme font
        void SkinField(const Ref<EditBox>& edit, bool multiline);

        // Creates a select with the items, inserting the value when it is not listed, and selects the value
        Ref<DropDown> MakeDropDown(const Vector<String>& items, const String& value);

        // Creates a switch (32x18 track and knob) with the caption at its right and the value
        Ref<Toggle> MakeCheckbox(const String& caption, bool value);

        // Creates the switch of a head row: an 11 px dim caption 6 px after the track, as wide as its text
        Ref<Toggle> MakeHeadSwitch(const String& caption, bool value);

        // Creates the Crop toggle of a head row (.crop-toggle): glyph and caption in a line border, the accent border while on
        Ref<Toggle> MakeCropToggle(bool value);

        // Returns the width of a text in the theme font
        float TextWidth(const String& text, float size, int weight = 400);

        // Creates a button with the caption: field fill, line border, the regular text
        Ref<Button> MakeButton(const String& caption);

        // Creates a quiet button: no fill, line border, the dim text
        Ref<Button> MakeGhostButton(const String& caption);

        // Creates the accent button of a node, at most one per card: accent fill, ink text in semibold
        Ref<Button> MakePrimaryButton(const String& caption);

        // Creates an icon button: the tinted icon, a wash on hover, an optional plate behind
        Ref<Button> MakeIconButton(const String& icon, const Color4& iconColor, const Color4& backColor);

        // Creates one segment of a segmented control: caption, the active pill while on
        Ref<Toggle> MakeSegment(const String& caption, bool value);

        // Adds the segmented control's frame (field fill, line border) to its row container
        void SkinSegmentedRow(Widget& row);

        // Returns the hover row of a menu: a rounded wash in the theme
        Ref<IRectDrawable> MakeMenuHover();

        // Returns the check mark of a menu row in the accent
        Ref<IRectDrawable> MakeMenuCheck();

        // Returns the search glyph of a menu in the quietest text
        Ref<IRectDrawable> MakeMenuSearchIcon();

        // Returns a separator line of a menu
        Ref<IRectDrawable> MakeMenuLine();

        // Restyles a context menu in the theme: plate, hover row, item text, sub menu arrow, separators; its sub menus too
        void SkinMenu(const Ref<class ContextMenu>& menu);

        // Returns the regular text colour of the current theme
        Color4 TextColor();

        // Returns the dimmed text colour of the current theme
        Color4 DimTextColor();

        // Returns the accent colour of the current theme
        Color4 AccentColor();

        // Creates a horizontal row: a fixed-width caption label followed by the control
        Ref<HorizontalLayout> MakeRow(const String& label, const Ref<Widget>& control, float labelWidth = rowLabelWidth);

        // Formats a slider value: an integer, one or two decimals depending on the step
        String FormatNumber(float value, float step);
    }

    // -------------------------------------------------------------------
    // Edit box of the pipeline cards: shows a placeholder while it is empty
    // -------------------------------------------------------------------
    class PipelineField : public EditBox
    {
    public:
        // Default constructor
        explicit PipelineField(RefCounter* refCounter);

        // Sets the text shown while the field is empty and not focused
        void SetPlaceholder(const String& text);

        // Draws the field and the placeholder over it while empty
        void Draw() override;

        SERIALIZABLE(PipelineField);

    protected:
        Ref<Text> mPlaceholder; // Placeholder text, null without one
    };

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

        // Returns true for a child without a maximum width: it takes a share of the spare width of its line
        static bool IsFlexible(const Ref<Widget>& child);
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

        // Narrows the caption to its text and drops the value text, for a toolbar that shares its line with other controls
        void SetCompact(bool compact);

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

        bool  mPendingComplete = false; // True after a user change until the cursor is released
        bool  mCompact = false;         // Caption only as wide as its text, no value text
        bool  mActive = false;          // True while the track is hovered or dragged: the fill is the accent
        float mFillStrength = 0.28f;    // Strength of the fill at rest

    protected:
        // Places the caption, the track and the value
        void LayoutParts();

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

        // Shows "loading.." in place of the hint while the picture decodes in the background
        void SetLoading(bool loading);

        // Returns true when an image is shown
        bool HasImage() const { return mHasImage; }

        // Returns the rectangle where the image is drawn (world space)
        RectF GetImageRect() const;

        // Returns the size of the source bitmap
        Vec2I GetImageSize() const { return mImageSize; }

        // Draws the view; zoomed far out a flat fill stands in for the checker tiles
        void Draw() override;

        // Sets whether the view draws the result card frame: border and shadow; the checker plate is always drawn
        void SetFramed(bool framed);

        SERIALIZABLE(PipelineImageView);

    protected:
        Ref<WidgetLayer> mImageLayer;       // Image sprite layer
        Ref<WidgetLayer> mHintLayer;        // Hint text layer, shown when there is no image
        Vec2I            mImageSize;        // Size of the source bitmap
        bool             mHasImage = false; // True when an image is shown
        bool             mFramed = true;    // Draws the card frame: border and shadow
        WeakRef<Bitmap>  mShownBitmap;      // Bitmap on screen, setting it again changes nothing
        String           mHint;             // Text shown when there is no image
        bool             mLoading = false;  // The picture is on its way

    protected:
        // Draws the plate: shadow, checker, border
        void DrawFrame();


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
        Ref<class ScrollArea> mScroll; // Scroll area holding the text
        Ref<PipelineRichText> mLabel;  // Markdown text, wrapped
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

CLASS_BASES_META(Editor::PipelineRoundedFrame)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineRoundedFrame)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(8.0f).NAME(radius);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(width);
}
END_META;
CLASS_METHODS_META(Editor::PipelineRoundedFrame)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineRoundedFrame&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;

CLASS_BASES_META(Editor::PipelineMenuBack)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineMenuBack)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(12.0f).NAME(radius);
}
END_META;
CLASS_METHODS_META(Editor::PipelineMenuBack)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineMenuBack&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;

CLASS_BASES_META(Editor::PipelinePortMarker)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(Editor::PipelinePortMarker)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(PipelinePortType::Text).NAME(portType);
}
END_META;
CLASS_METHODS_META(Editor::PipelinePortMarker)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelinePortMarker&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;

CLASS_BASES_META(Editor::PipelineFoldArrow)
{
    BASE_CLASS(o2::IRectDrawable);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineFoldArrow)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(open);
}
END_META;
CLASS_METHODS_META(Editor::PipelineFoldArrow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineFoldArrow&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;

CLASS_BASES_META(Editor::PipelineField)
{
    BASE_CLASS(o2::EditBox);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineField)
{
    FIELD().PROTECTED().NAME(mPlaceholder);
}
END_META;
CLASS_METHODS_META(Editor::PipelineField)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlaceholder, const String&);
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
    FUNCTION().PROTECTED().SIGNATURE_STATIC(bool, IsFlexible, const Ref<Widget>&);
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
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCompact);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mActive);
    FIELD().PROTECTED().DEFAULT_VALUE(0.28f).NAME(mFillStrength);
}
END_META;
CLASS_METHODS_META(Editor::PipelineSlider)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const String&, float, float, float, float, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, float, bool);
    FUNCTION().PUBLIC().SIGNATURE(float, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTrackWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCompact, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, LayoutParts);
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
    FIELD().PROTECTED().NAME(mImageLayer);
    FIELD().PROTECTED().NAME(mHintLayer);
    FIELD().PROTECTED().NAME(mImageSize);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasImage);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mFramed);
    FIELD().PROTECTED().NAME(mShownBitmap);
    FIELD().PROTECTED().NAME(mHint);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mLoading);
}
END_META;
CLASS_METHODS_META(Editor::PipelineImageView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBitmap, const Ref<Bitmap>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHint, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLoading, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, HasImage);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetImageRect);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetImageSize);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFramed, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawFrame);
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
