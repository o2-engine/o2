#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/Widget.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"

using namespace o2;

namespace o2
{
    class Bitmap;
    class Sprite;
    class Text;
}

namespace Editor
{
    class PipelineImageView;

    // ----------------------------------------------------------------------------
    // Drawing shared by the input | result views: images fitted into a box and cut
    // at a divider, and the divider with its grip
    // ----------------------------------------------------------------------------
    namespace PipelinePairDraw
    {
        // Returns the rectangle an image of the size takes when it is fitted into the box
        RectF FitRect(const Vec2I& imageSize, const RectF& box);

        // Draws the sprite over the image rectangle, only the part between the two x coordinates
        void DrawClipped(Sprite& sprite, const Vec2I& textureSize, const RectF& imageRect, float fromX, float toX);

        // Creates the tiled checkerboard sprite drawn under the images
        Ref<Sprite> MakeChecker();

        // Draws the checkerboard over the part of the rectangle between the two x coordinates
        void DrawChecker(Sprite& checker, const RectF& rect, float fromX, float toX);

        constexpr float maxBadgeScale = 1.5f; // Largest factor of the screen-sized grip, reached when zoomed far out
        constexpr float gripWidth = 5.0f;     // Divider grip width, screen pixels
        constexpr float gripHeight = 18.0f;   // Divider grip height, screen pixels

        // Returns the size of a screen pixel in the units being drawn
        float PixelSize();

        // Returns the factor that keeps the grip screen-sized, capped when zoomed far out
        float BadgeScale(float pixelSize);

        // Returns the divider grip rectangle at x
        RectF GripRect(const RectF& box, float x, float pixelSize);

        // Draws the dark divider line, one screen pixel wide, across the box at x and its grip, fainter unless active
        void DrawDivider(const RectF& box, float x, bool active);

        // Returns a display copy of the bitmap, downscaled to 1024 px on its longer side
        Ref<Bitmap> DisplayCopy(const Ref<Bitmap>& bitmap);
    }

    // -------------------------------------------------------------------------------------------------
    // Input and result in one box: the input left of a vertical divider, the result right of it, each cut
    // at the divider. Pressing anywhere moves the divider there and drags it
    // -------------------------------------------------------------------------------------------------
    class PipelineCompareView : public Widget, public CursorAreaEventsListener
    {
    public:
        // Default constructor
        explicit PipelineCompareView(RefCounter* refCounter);

        // Sets the node whose divider the view moves
        void SetNode(const String& nodeId);

        // Sets the two images
        void SetImages(const Ref<Bitmap>& input, const Ref<Bitmap>& result);

        // Returns the divider x in world space
        float GetDividerX() const;

        // Draws both images cut at the divider and the divider
        void Draw() override;

        // Returns true inside the view
        bool IsUnderPoint(const Vec2F& point) override;

        SERIALIZABLE(PipelineCompareView);

    protected:
        String      mNodeId;         // Node the divider belongs to
        Ref<Sprite> mInputSprite;    // Input image
        Ref<Sprite> mResultSprite;   // Result image
        Vec2I       mInputSize;      // Input display texture size
        Vec2I       mResultSize;     // Result display texture size
        Ref<Sprite> mChecker;        // Checkerboard behind the images
        bool        mHovered = false; // The cursor is over the view: the grip comes forward

    protected:
        // Moves the divider to the pressed point
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Drags the divider
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Brings the grip forward
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Fades the grip
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Moves the divider to the x coordinate
        void MoveDivider(float x);

        REF_COUNTERABLE_IMPL(Widget);
    };

    // ---------------------------------------------------------------------------------------------------
    // The row of an image-to-image node: the image feeding its input on the left, its result on the right,
    // or the compare view in the same place when the setting asks for it and both images exist
    // ---------------------------------------------------------------------------------------------------
    class PipelineIoPair : public Widget
    {
    public:
        // Default constructor
        explicit PipelineIoPair(RefCounter* refCounter);

        // Sets the node and the widget the right pane shows: the result preview, or the crop editor
        void Setup(const String& nodeId, const String& nodeType, const Ref<Widget>& result);

        // Shows the images; the result widget itself is filled by the body
        void SetImages(const Ref<Bitmap>& input, const Ref<Bitmap>& result);

        // Keeps the pair side by side while a crop frame is edited
        void SetCropOn(bool cropOn);

        // Returns the row height before a hand-sized card adds to it
        float GetMinHeight() const;

        // Returns true while the compare view is shown
        bool IsComparing() const;

        // Returns the input pane
        const Ref<PipelineImageView>& GetInputView() const { return mInputView; }

        // Returns the compare view
        const Ref<PipelineCompareView>& GetCompareView() const { return mCompare; }

        // Returns the result pane widget
        const Ref<Widget>& GetResultWidget() const { return mResult; }

        // Places the panes
        void UpdateSelfTransform() override;

        SERIALIZABLE(PipelineIoPair);

    protected:
        String                   mNodeId;           // Node shown
        String                   mNodeType;         // Its type
        Ref<PipelineImageView>   mInputView;        // Left pane
        Ref<Widget>              mResult;           // Right pane
        Ref<PipelineCompareView> mCompare;          // Both in one box
        bool                     mHasInput = false;  // An image feeds the input
        bool                     mHasResult = false; // The node has a result
        bool                     mCropOn = false;    // A crop frame is edited in the result pane

    protected:
        // Shows the compare view or the two panes
        void UpdateMode();
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineCompareView)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineCompareView)
{
    FIELD().PROTECTED().NAME(mNodeId);
    FIELD().PROTECTED().NAME(mInputSprite);
    FIELD().PROTECTED().NAME(mResultSprite);
    FIELD().PROTECTED().NAME(mInputSize);
    FIELD().PROTECTED().NAME(mResultSize);
    FIELD().PROTECTED().NAME(mChecker);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHovered);
}
END_META;
CLASS_METHODS_META(Editor::PipelineCompareView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNode, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetImages, const Ref<Bitmap>&, const Ref<Bitmap>&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDividerX);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveDivider, float);
}
END_META;

CLASS_BASES_META(Editor::PipelineIoPair)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineIoPair)
{
    FIELD().PROTECTED().NAME(mNodeId);
    FIELD().PROTECTED().NAME(mNodeType);
    FIELD().PROTECTED().NAME(mInputView);
    FIELD().PROTECTED().NAME(mResult);
    FIELD().PROTECTED().NAME(mCompare);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasInput);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasResult);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCropOn);
}
END_META;
CLASS_METHODS_META(Editor::PipelineIoPair)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const String&, const String&, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetImages, const Ref<Bitmap>&, const Ref<Bitmap>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCropOn, bool);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMinHeight);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsComparing);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<PipelineImageView>&, GetInputView);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<PipelineCompareView>&, GetCompareView);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Widget>&, GetResultWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateMode);
}
END_META;
// --- END META ---
