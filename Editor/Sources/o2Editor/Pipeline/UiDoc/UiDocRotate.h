#pragma once

#include <array>
#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"

// ------------------------------------------------------------------------------------------------
// Rotation of layout blocks (port of AssetsLine shared/uiDoc/rotate.ts). A block's `rotation` (degrees,
// clockwise on the screen) turns it about the centre of its box - an o2 widget with its pivot in
// the middle. The layout itself never sees it: boxes are solved as if nothing were turned (a row lays its
// children out the same), and the turn is drawn on top; the blocks inside a turned block turn with it. So a
// point on the screen is in a block when it is in the block's box after the turns of the block and of its
// parents are undone - WorldMatrix is that chain, InBlock the test. Design pixels, y down
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // A point (or a direction) in design pixels, y down
    struct UiPoint
    {
        double x = 0, y = 0;
    };

    // An affine map as SVG writes it: x' = a*x + c*y + e, y' = b*x + d*y + f -> { a, b, c, d, e, f }
    using UiMat = std::array<double, 6>;

    // The map that changes nothing
    constexpr UiMat UI_IDENTITY = { 1, 0, 0, 1, 0, 0 };

    // m after n: n is applied first
    UiMat Mul(const UiMat& m, const UiMat& n);

    // A turn by deg degrees (clockwise on the screen) about (cx, cy); the identity for 0
    UiMat RotationAbout(double deg, double cx, double cy);

    // A point through a map
    UiPoint Apply(const UiMat& m, const UiPoint& p);

    // A direction through a map (no translation)
    UiPoint ApplyLinear(const UiMat& m, const UiPoint& v);

    // The inverse map
    UiMat Invert(const UiMat& m);

    // The centre of a box
    UiPoint Centre(const UiRect& r);

    // A block's own turn, degrees (0 for none)
    double TurnOf(const UiNode& n);

    // The blocks from the root down to the id (the root first); empty when there is no such block
    Vector<const UiNode*> PathTo(const UiDoc& doc, const String& id);

    // From the solved (unturned) boxes to where things are on the screen, for the block: the turns of its parents, and with
    // self its own
    UiMat WorldMatrix(const UiDoc& doc, const UiRects& rects, const String& id, bool self = true);

    // How far a block is turned on the screen: its parents' turns and, with self, its own
    double WorldTurn(const UiDoc& doc, const String& id, bool self = true);

    // The four corners of a box through a map: top-left, top-right, bottom-right, bottom-left
    std::array<UiPoint, 4> Corners(const UiRect& r, const UiMat& m);

    // Whether a point of the screen is on the block, its turns and its parents' undone
    bool InBlock(const UiDoc& doc, const UiRects& rects, const String& id, const UiPoint& p);

    // A turn as it is stored: degrees in (-180, 180], a tenth of a degree; none for 0 or not a number
    std::optional<double> CleanTurn(double v);

    // CleanTurn of any value, read as JavaScript's Number() reads it ("12.5" is 12.5, null 0)
    std::optional<double> CleanTurn(const Json* v);

    // The axis-aligned box a w x h rectangle takes when turned
    UiSize TurnedBounds(double w, double h, double deg);

    // The size of a rectangle whose turned box is W x H - what a picture of a tilted element measures. aspect (w / h), when
    // it is known (a kit graphic), makes it exact at any angle; without it the sizes are solved where that is well posed
    // (under 35 degrees) and taken square beyond
    UiSize UnturnedSize(double W, double H, double deg, std::optional<double> aspect = std::nullopt);
}
