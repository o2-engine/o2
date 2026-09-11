#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

using namespace o2;

namespace o2
{
    class Bitmap;
    class Button;
    class HorizontalLayout;
    class Sprite;
    class Text;
    class Toggle;
}

namespace Editor
{
    // ------------------------------------------------------------------------
    // Brush editor over an optional background image. The committed strokes
    // are stored in the node config as a PNG data URL ("drawing", "dw", "dh"),
    // brush settings and the active tool live in UI-only keys. An optional
    // region box ("roi") is edited with the region tool
    // ------------------------------------------------------------------------
    class PipelinePaintEditor : public Widget, public CursorAreaEventsListener
    {
    public:
        Function<void(const String&, bool)> onConfigChanged; // Called with the config key and the completed flag after a write

    public:
        // Default constructor
        explicit PipelinePaintEditor(RefCounter* refCounter);

        // Sets the node, enables the region tool when asked and loads the drawing
        void Init(const Ref<PipelineNode>& node, bool withRegion);

        // Sets the background image; the drawing resolution follows its size
        void SetBackground(const Ref<Bitmap>& bitmap);

        // Returns the background image, null for a blank canvas
        const Ref<Bitmap>& GetBackground() const { return mBackground; }

        // Returns the stage rectangle the drawing is shown in, world space
        RectF GetStageRectangle() const { return GetStageRect(); }

        // Reloads the strokes and the tool settings from the node config
        void RefreshFromConfig();

        // Returns the smallest height fitting the toolbar and the paint area
        float GetMinHeight() const;

        // Returns the smallest height fitting the toolbar wrapped into the width and the paint area
        float GetMinHeightForWidth(float width) const;

        // Returns the drawing resolution in pixels
        const Vec2I& GetResolution() const { return mResolution; }

        // Updates the transform; places the toolbar lines and the palette row for the current width
        void UpdateSelfTransform() override;

        // Updates the widget; without a background, a blank canvas takes the stage size after a short delay
        void Update(float dt) override;

        // Draws the background, the drawing, the region frame and the brush outline
        void Draw() override;

        // Returns true when the point is inside the stage and the region tool is not active
        bool IsUnderPoint(const Vec2F& point) override;

        SERIALIZABLE(PipelinePaintEditor);

    protected:
        static const float toolbarHeight;   // Height of the tool row
        static const int   maxHistory = 15; // Undo steps kept

        Ref<PipelineNode> mNode;               // Edited node
        bool              mWithRegion = false; // True when the region tool is available

        Ref<Toggle>           mBrushToggle;         // Brush tool toggle
        Ref<Toggle>           mEraserToggle;        // Eraser tool toggle
        Ref<Toggle>           mRegionToggle;        // Region tool toggle
        Ref<Button>           mColorButton;         // Brush color swatch, opens the palette
        Ref<PipelineSlider>   mSizeSlider;          // Brush size slider
        Ref<PipelineSlider>   mOpacitySlider;       // Brush opacity slider
        Ref<Button>           mUndoButton;          // Undo button
        Ref<Button>           mRedoButton;          // Redo button
        Ref<Button>           mClearButton;         // Clears the drawing
        Ref<PipelineWrapRow>  mToolbar;             // Tools, color, sliders and actions, wrapping when the width runs out
        Ref<PipelineWrapRow>  mPaletteRow;          // Color swatches below the toolbar, wrapping when the width runs out
        bool                  mPaletteOpen = false; // True when the palette row is shown

        Ref<Bitmap> mBackground;        // Background image, null for a blank canvas
        Ref<Bitmap> mBase;              // Committed strokes
        Ref<Bitmap> mStroke;            // Stroke being painted
        Ref<Bitmap> mComposed;          // Base with the current stroke blended over, uploaded to the texture
        TextureRef  mBackgroundTexture; // Background texture
        TextureRef  mTexture;           // Texture of the composed drawing
        Ref<Sprite> mBackgroundSprite;  // Background sprite
        Ref<Sprite> mSprite;            // Composed drawing sprite
        Ref<Text>   mHintText;          // Hint text drawable

        Vec2I mResolution;           // Drawing resolution in pixels: the stored drawing size, the background size or twice the stage of a blank canvas
        Vec2F mCommittedArea;        // Stage size the resolution was last taken from, blank canvas only
        float mResizeTimer = -1.0f;  // Delay before the drawing follows a resized stage, negative when idle
        bool  mTextureDirty = false; // True when the composed bitmap must be uploaded to the texture

        Vector<String> mUndo;        // Previous drawings as data URLs
        Vector<String> mRedo;        // Undone drawings as data URLs
        String         mSelfDrawing; // Last drawing written by this editor, so own writes are not reloaded

        bool  mPainting = false; // True while a stroke is painted
        Vec2F mLastPoint;        // Last stroke point in image pixels
        int   mDirtyMinX = 0;    // Left edge of the area touched by the current stroke
        int   mDirtyMinY = 0;    // Top edge of the touched area
        int   mDirtyMaxX = 0;    // Right edge of the touched area, exclusive
        int   mDirtyMaxY = 0;    // Bottom edge of the touched area, exclusive
        bool  mHovered = false;  // True while the cursor is over the stage
        Vec2F mHoverPoint;       // Cursor position of the brush outline

        Ref<FrameHandles> mRegionFrame;            // Region box handles
        bool              mRegionSyncing = false;  // True while the frame is set from the config
        bool              mRegionDragging = false; // True while the region handles are dragged

    protected:
        // Returns the active tool: "brush", "eraser" or "roi"
        String GetTool() const;

        // Stores the tool in the config, notifies and updates the toolbar
        void SetTool(const String& tool);

        // Returns the brush diameter in stage pixels from the config, 1..80
        float GetBrushSize() const;

        // Returns the brush opacity from the config, 0..1
        float GetBrushOpacity() const;

        // Returns the opaque brush color from the config
        Color4 GetBrushColor() const;

        // Creates the tool toggles, the color button, the sliders, the action buttons and the palette row
        void BuildToolbar();

        // Updates the toggles, the color swatch, the sliders and the history buttons from the config
        void UpdateToolbar();

        // Shows or hides the palette row
        void TogglePalette();

        // Returns the widget rectangle below the toolbar and the palette
        RectF GetAreaRect() const;

        // Returns the height of the toolbar and the open palette wrapped into the width
        float BarsHeight(float width) const;

        // Returns the resolution a blank canvas takes for the stage size: twice the stage, capped on the longer side
        static Vec2I CanvasResolution(const Vec2F& stage);

        // Returns the drawing rectangle: the area fitted to the background aspect, inset for the region handles
        RectF GetStageRect() const;

        // Converts a screen point to image pixels (y down)
        Vec2F ToImage(const Vec2F& canvasPoint) const;

        // Creates the base, stroke and composed bitmaps of the resolution when they are missing
        void EnsureBuffers();

        // Sets the drawing resolution, rescaling the base or reloading it from the config
        void SetResolution(const Vec2I& resolution, bool rescale);

        // Decodes the PNG data URL into the base bitmap, fitted to the resolution
        void LoadDrawing(const String& dataUrl);

        // Fills the bitmap with transparent black
        void ClearBitmap(Bitmap& bitmap);

        // Draws an anti-aliased disc into the stroke bitmap and grows the touched area
        void StampDisc(const Vec2F& center, float radius);

        // Stamps discs along the segment given in image pixels
        void PaintSegment(const Vec2F& from, const Vec2F& to);

        // Blends the stroke over the base into the composed bitmap and marks the texture dirty
        void ComposePreview();

        // Merges the stroke into the base, pushes the history and writes the drawing to the config
        void CommitStroke();

        // Stores the drawing data URL and its size in the config and notifies
        void WriteDrawing(const String& dataUrl);

        // Returns the base bitmap as a PNG data URL, empty when there is no base
        String EncodeBase() const;

        // Pushes the previous drawing to the undo list and clears the redo list
        void PushHistory(const String& previous);

        // Restores the previous drawing from the undo list
        void OnUndo();

        // Restores the next drawing from the redo list
        void OnRedo();

        // Clears the drawing, keeping the current one in the history
        void OnClear();

        // Sets the region frame from the config "roi", defaulting to the central 80%
        void SyncRegionFromConfig();

        // Called when the region frame is transformed; writes the normalized "roi" box
        void OnRegionTransformed(const Basis& basis);

        // Called when the region transform is completed; notifies as completed
        void OnRegionCompleted();

        // Called when cursor pressed on this; starts a stroke
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame; extends the stroke
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released; commits the stroke
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken; commits the stroke
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor moved; tracks the brush outline position
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when cursor enters this object; shows the brush outline
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object; hides the brush outline
        void OnCursorExit(const Input::Cursor& cursor) override;

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelinePaintEditor)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::PipelinePaintEditor)
{
    FIELD().PUBLIC().NAME(onConfigChanged);
    FIELD().PROTECTED().NAME(mNode);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mWithRegion);
    FIELD().PROTECTED().NAME(mBrushToggle);
    FIELD().PROTECTED().NAME(mEraserToggle);
    FIELD().PROTECTED().NAME(mRegionToggle);
    FIELD().PROTECTED().NAME(mColorButton);
    FIELD().PROTECTED().NAME(mSizeSlider);
    FIELD().PROTECTED().NAME(mOpacitySlider);
    FIELD().PROTECTED().NAME(mUndoButton);
    FIELD().PROTECTED().NAME(mRedoButton);
    FIELD().PROTECTED().NAME(mClearButton);
    FIELD().PROTECTED().NAME(mToolbar);
    FIELD().PROTECTED().NAME(mPaletteRow);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPaletteOpen);
    FIELD().PROTECTED().NAME(mBackground);
    FIELD().PROTECTED().NAME(mBase);
    FIELD().PROTECTED().NAME(mStroke);
    FIELD().PROTECTED().NAME(mComposed);
    FIELD().PROTECTED().NAME(mBackgroundTexture);
    FIELD().PROTECTED().NAME(mTexture);
    FIELD().PROTECTED().NAME(mBackgroundSprite);
    FIELD().PROTECTED().NAME(mSprite);
    FIELD().PROTECTED().NAME(mHintText);
    FIELD().PROTECTED().NAME(mResolution);
    FIELD().PROTECTED().NAME(mCommittedArea);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mResizeTimer);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mTextureDirty);
    FIELD().PROTECTED().NAME(mUndo);
    FIELD().PROTECTED().NAME(mRedo);
    FIELD().PROTECTED().NAME(mSelfDrawing);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPainting);
    FIELD().PROTECTED().NAME(mLastPoint);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDirtyMinX);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDirtyMinY);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDirtyMaxX);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDirtyMaxY);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHovered);
    FIELD().PROTECTED().NAME(mHoverPoint);
    FIELD().PROTECTED().NAME(mRegionFrame);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mRegionSyncing);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mRegionDragging);
}
END_META;
CLASS_METHODS_META(Editor::PipelinePaintEditor)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Init, const Ref<PipelineNode>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBackground, const Ref<Bitmap>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Bitmap>&, GetBackground);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetStageRectangle);
    FUNCTION().PUBLIC().SIGNATURE(void, RefreshFromConfig);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMinHeight);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMinHeightForWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(const Vec2I&, GetResolution);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(String, GetTool);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTool, const String&);
    FUNCTION().PROTECTED().SIGNATURE(float, GetBrushSize);
    FUNCTION().PROTECTED().SIGNATURE(float, GetBrushOpacity);
    FUNCTION().PROTECTED().SIGNATURE(Color4, GetBrushColor);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildToolbar);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateToolbar);
    FUNCTION().PROTECTED().SIGNATURE(void, TogglePalette);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetAreaRect);
    FUNCTION().PROTECTED().SIGNATURE(float, BarsHeight, float);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(Vec2I, CanvasResolution, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetStageRect);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, ToImage, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, EnsureBuffers);
    FUNCTION().PROTECTED().SIGNATURE(void, SetResolution, const Vec2I&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadDrawing, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, ClearBitmap, Bitmap&);
    FUNCTION().PROTECTED().SIGNATURE(void, StampDisc, const Vec2F&, float);
    FUNCTION().PROTECTED().SIGNATURE(void, PaintSegment, const Vec2F&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, ComposePreview);
    FUNCTION().PROTECTED().SIGNATURE(void, CommitStroke);
    FUNCTION().PROTECTED().SIGNATURE(void, WriteDrawing, const String&);
    FUNCTION().PROTECTED().SIGNATURE(String, EncodeBase);
    FUNCTION().PROTECTED().SIGNATURE(void, PushHistory, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUndo);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRedo);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClear);
    FUNCTION().PROTECTED().SIGNATURE(void, SyncRegionFromConfig);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRegionTransformed, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRegionCompleted);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
}
END_META;
// --- END META ---
