#pragma once

#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

using namespace o2;

namespace o2
{
    class Button;
    class EditBox;
    class Toggle;
    class Widget;
    class Text;
    class VectorSprite;
}

namespace Editor
{
    // -------------------------------------------------------------------------------------
    // Pictures of the image-tool cards in the AssetsLine look: the drawing stage, part cells,
    // region boxes with their number tags and remove tabs; colours from the current theme
    // -------------------------------------------------------------------------------------
    namespace PipelineImageTheme
    {
        // Returns the screen-constant scale of handles and tags: one canvas unit per screen pixel, shrinking past 1.5
        // (AssetsLine --fk = min(1/zoom, 1.5))
        float HandleScale();

        // Draws the checker of cells of the size from the rectangle's top-left; the first cell gets the second colour when
        // firstSecond is set; cells on the rounded corners are pulled in by the radius
        void DrawChecker(const RectF& rect, float cell, const Color4& first, const Color4& second, bool firstSecond, float radius);

        // Draws the drawing stage plate (.draw-area): the card shadow, the ck2/ck1 checker of 8 px squares, a line border, r 9
        void DrawStagePlate(const RectF& area, bool shadow);

        // Draws a part cell plate (.result-cell): shadow, ck1 with ck2 squares of 6 px, the line border or, selected, the
        // accent border with its inner ring, r 7
        void DrawCellPlate(const RectF& cell, bool selected, bool shadow);

        // Draws the number tag of a region box at its top-left corner (.roi-idx): 20 px, rounded at the bottom right,
        // dark or, for the selected box, the accent
        void DrawRegionTag(const RectF& box, int index, bool selected, Text& text);

        // Returns the remove tab rectangle of a region box (.roi-del): 22 px at its top-right corner, over the border
        RectF RemoveTabRect(const RectF& box);

        // Draws the remove tab of a region box: dark, rounded at the bottom left, the trash glyph in white
        void DrawRemoveTab(const RectF& box, VectorSprite& trash, bool hovered);

        // Draws a region box outline one screen pixel wide: solid for the selected box, dashed for the others
        void DrawBoxOutline(const RectF& box, bool selected);

        // Restyles a field as the name field of a selected part cell (.cell-name): 18 px, r 5, the accent border, 10.5 px centred
        void SkinCellNameField(const Ref<EditBox>& edit);

        // Turns a quiet button into the accent one of the parts row (.grid-actions .auto): accent text, a faint accent border
        void MakeAccentGhost(const Ref<Button>& button);

        // Lets the captions of a widget and its children, sized to their text, keep every letter: o2's dots ending reserves
        // room for "..." even when the text fits
        void KeepWholeCaption(Widget& widget);

        // Creates a head-row toggle (.crop-toggle without a glyph): the caption in a line border, the accent border while on
        Ref<Toggle> MakeSmallToggle(const String& caption, bool value);

        // Draws the crop / region handles round a box (.crop-handle): L brackets outside the corners, bars outside the
        // edge middles, white, screen sized
        void DrawBoxHandles(const RectF& box);
    }

    // ---------------------------------------------------------------------------------
    // Picture of a part or sprite cell (.result-cell-img): its own plate, the accent border
    // while selected, red after a failed run, the dark tool strip of the selected cell
    // ---------------------------------------------------------------------------------
    class PipelineCellImage : public PipelineImageView
    {
    public:
        bool selected = false; // Accent border
        bool failed = false;   // Red border
        bool tools = false;    // The tool strip is shown; its back is drawn here, under the buttons

    public:
        // Default constructor
        explicit PipelineCellImage(RefCounter* refCounter);

        // Draws the plate, the picture and the back of the tool strip
        void Draw() override;
    };

    // ---------------------------------------------------------------------------------
    // Frame handles of a region or crop box, drawn as the AssetsLine brackets and bars:
    // the handles keep their drag logic, their own art turns invisible
    // ---------------------------------------------------------------------------------
    class PipelineRegionHandles : public FrameHandles
    {
    public:
        // Default constructor; swaps the handle art for invisible grab areas
        PipelineRegionHandles();

        // Registers the handles for input at a screen-constant grab size and draws the brackets, the bars and nothing else
        void DrawStyled();

        // Returns the box the handles frame, world space
        RectF GetBox() const;
    };
}
