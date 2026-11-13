#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

// ------------------------------------------------------------------------------------------------
// The layout agent's eyes (port of AssetsLine backend/src/uiLayout/compare.ts): the screen rendered with
// the kit, a measuring grid laid over a picture (the same grid over the screen and over the reference, so
// positions can be read off and compared), where the screen differs from the reference, those places side
// by side, enlarged, and the pixel difference as a picture. The pictures are for the model and the agent's
// panel; they are drawn with the editor's own pixel tools, not byte for byte the web's
// ------------------------------------------------------------------------------------------------
namespace Editor::UiLayout
{
    // The screen as a picture, at most max px on its longer side (never larger than the design): with the kit when it has
    // anything, else as line art
    UiImage RenderDocShot(const ::Editor::UiDoc::UiDoc& doc, const ::Editor::UiDoc::UiKit* kit, int max = 1024);

    // The picture scaled to fit max x max (a smaller one is returned as it is)
    UiImage FitInside(const UiImage& img, int max);

    // The grid's step for a screen: about ten lines along its longer side, a round number of design pixels
    double GridStep(double w, double h);

    // A picture showing a w x h design whole, with grid lines every step design pixels and their coordinates along the top
    // and the left edge (step 0: GridStep)
    UiImage WithGrid(const UiImage& img, double w, double h, double step = 0);

    // A place where two pictures of one design differ
    struct DiffRegion
    {
        Box    box;       // Design pixels
        double share = 0; // How much of the difference of the whole screen is here, 0..1
    };

    // How alike two pictures of one design are
    struct ShotCompare
    {
        double             score = 0; // 1 = the same picture, 0 = nothing alike
        Vector<DiffRegion> regions;   // The places that differ most, largest first
    };

    // Where two pictures of the same w x h design differ. Both are compared small and blurred, so what counts is placement,
    // size, presence and colour of things - not the pixels of a font or a gradient
    ShotCompare CompareShots(const UiImage& reference, const UiImage& shot, double w, double h, int maxRegions = 5);

    // The blocks a place is about: those that lie mostly inside it, or cover a good part of it (innermost first)
    Vector<String> BlocksIn(const ::Editor::UiDoc::UiDoc& doc, const Box& box, int max = 6);

    // The places that differ, enlarged, the reference on the left and the screen as it is now on the right; false when there
    // is no place
    bool SideBySide(const UiImage& reference, const UiImage& shot, double w, double h, const Vector<DiffRegion>& regions,
                    UiImage& out);

    // Numbered pins drawn on a picture; the points are in a w x h space the picture shows whole
    UiImage WithPins(const UiImage& img, const Vector<::Editor::UiDoc::UiStepPoint>& points, double w, double h);

    // The pixel difference of two pictures of one design: the reference dimmed to grey, and over it every pixel that differs -
    // yellow where a little, red where much
    UiImage DiffPicture(const UiImage& reference, const UiImage& shot, int max = 900);

    // Small pictures side by side on a dark card (what was cut, for the activity list)
    UiImage ContactStrip(const Vector<UiImage>& pictures, int cell = 96, int cols = 8);

    // Draws a caption of the 5x7 font at about px pixels high, outlined, with its top-left at (x, y)
    void DrawLabel(UiImage& img, const String& text, double x, double y, const Rgb& color, int px);
}
