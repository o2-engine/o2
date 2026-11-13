#pragma once

#include <unordered_map>

#include "o2/Utils/Math/Rect.h"
#include "o2Editor/Pipeline/UiDoc/UiDocModel.h"

// ------------------------------------------------------------------------------------------------
// The layout solver (port of AssetsLine shared/uiDoc/solve.ts): document + screen size -> a rectangle for
// every block. The same function draws the cards, renders the node's picture and positions the exported
// prefab, so a layout looks the same everywhere.
// - a block whose parent has no layout is placed by its anchor: on a corner, edge or the centre it keeps
//   its size and sits x / y away from that point (inwards); along a stretched axis it fills the parent
//   minus its margins;
// - row / column: children along the main axis with gap inside pad; grow shares the free space; justify
//   spreads them when nothing grows; align places them across (stretch, the default, fills the cross axis
//   when the child gives no size there); x / y nudge a child;
// - grid: cols columns of cellW x cellH cells (default the width split evenly, square cells), row by row;
// - hidden blocks take no room (they still get a box, anchored, for the inspector).
// Boxes are doubles, top-left origin, y down - the web's numbers bit for bit
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // -------------------------------------------------------
    // A block's box in design pixels: top-left origin, y down
    // -------------------------------------------------------
    struct UiRect
    {
        double x = 0, y = 0, w = 0, h = 0;

        UiRect() = default;
        UiRect(double x, double y, double w, double h): x(x), y(y), w(w), h(h) {}

        bool operator==(const UiRect& other) const { return x == other.x && y == other.y && w == other.w && h == other.h; }
        bool operator!=(const UiRect& other) const { return !(*this == other); }

        // The box as an o2 rect with the same numbers, still y down: left = x, top = y, right = x + w, bottom = y + h
        RectF ToRectF() const { return RectF((float)x, (float)y, (float)(x + w), (float)(y + h)); }
    };

    // -------------------------------------------------------------------------------------------
    // Every block's box by id, in the order the solver placed them (the web's Map order: the root, then
    // each parent's shown children in layout order, then its hidden ones, depth first)
    // -------------------------------------------------------------------------------------------
    class UiRects
    {
    public:
        // Sets a block's box (a known id keeps its place)
        void Set(const String& id, const UiRect& rect);

        // The box of a block, null when it has none
        const UiRect* Find(const String& id) const;

        // True when the block has a box
        bool Has(const String& id) const { return Find(id) != nullptr; }

        // The box of a block, an empty one when it has none
        UiRect Get(const String& id) const;

        // The ids in order
        const Vector<String>& Ids() const { return mIds; }

        // How many boxes
        int Count() const { return mIds.Count(); }

    private:
        Vector<String>                       mIds;
        Vector<UiRect>                       mRects;
        std::unordered_map<std::string, int> mIndex;
    };

    // A block placed by its anchor inside the parent's box
    UiRect AnchoredRect(const UiNode& n, const UiRect& parent);

    // ----------------------------------------------------
    // Columns, cell size and the grid's own extent of a grid
    // ----------------------------------------------------
    struct UiGridGeometry
    {
        double cols = 3, cw = 0, ch = 0, gx = 0, gy = 0, gridW = 0, gridH = 0;
    };

    // The grid geometry of a grid block inside box (its padding already taken off) for count children
    UiGridGeometry GridGeometry(const UiNode& n, const UiRect& box, int count);

    // Every block's box at the size (the design size when w or h is 0 or less), absolute
    UiRects SolveLayout(const UiDoc& doc, double w = 0, double h = 0);

    // The deepest shown block under a point (the root never), "" when none; turned blocks take the point in their own frame
    // (their turns and their parents' undone, UiDocRotate.h)
    String HitTest(const UiDoc& doc, const UiRects& rects, double x, double y);
}
