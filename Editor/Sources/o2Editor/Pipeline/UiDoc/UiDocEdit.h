#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"

// ------------------------------------------------------------------------------------------------
// Hand edits on the layout's stage (port of AssetsLine shared/uiDoc/edit.ts - Unity's rect tool in spirit):
// a block dragged, resized or re-anchored ends up as the props that put it there - the same anchor,
// offsets, size and margins the agent writes - so the edit holds on other screens the way the block is
// anchored. Inside a row / column / grid the parent places its children: a drag reorders them and a
// resize sets the size the layout gives them. Pure functions; the workspace draws and drives them
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // ---------------------------------------------------------------
    // Fields to set (an object of numbers / arrays / texts) and to remove
    // ---------------------------------------------------------------
    struct UiPatch
    {
        Json           set = Json::Object();
        Vector<String> unset;

        // True when it changes nothing
        bool IsEmpty() const { return set.Count() == 0 && unset.IsEmpty(); }
    };

    // The props that place n at rect r inside its parent's rect p - keeping its anchor, or with anchor (when given). With
    // size the block's width and height are written too (a resize, a new parent); a move keeps the size it has. Offsets
    // of 0 are unset rather than written
    UiPatch PlaceAt(const UiNode& n, const UiRect& p, const UiRect& r, const Anchor* anchor = nullptr, bool size = false);

    // A child of a row / column / grid resized from `from` to `to`: the size it asks the layout for; a new size along the
    // main axis drops its grow share
    UiPatch SizeInLayout(const UiNode& n, const UiNode& parent, const UiRect& from, const UiRect& to);

    // The update op of a patch ({op: "update", id, set?, unset?}); false when the patch changes nothing
    bool PatchOp(const String& id, const UiPatch& patch, UiOp& out);

    // The anchor point of a parent nearest to a point: its corners, edge middles and centre
    Anchor AnchorAt(const UiRect& p, double x, double y);

    // -----------------------------------------------------------------------------
    // The anchor's corners in the parent: one point, or the two ends of a stretched axis
    // -----------------------------------------------------------------------------
    struct UiAnchorBox
    {
        double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    };

    // Where the block's anchor sits in its parent's box (min / max, like Unity's anchors)
    UiAnchorBox AnchorBox(const UiNode& n, const UiRect& p);

    // ------------------------------------------------------------
    // Where a dragged child lands: before a sibling or after the last
    // ------------------------------------------------------------
    struct UiSlot
    {
        String before; // "" = none
        String after;  // "" = none
    };

    // Where a child dragged inside a row / column / grid lands among its shown siblings, from the point under the pointer
    // and the siblings' boxes before the drag; false when it has no siblings to go by
    bool LayoutSlot(const UiNode& parent, const UiRects& rects, const String& dragId, double x, double y, UiSlot& out);

    // Whether a move op leaves the block where it is (same parent, same neighbours)
    bool MoveIsNoop(const UiDoc& doc, const UiOp& moveOp);

    // --- snapping -----------------------------------------------------------------------------------------------

    // ------------------------------------------------
    // A guide to draw: along x (vertical) or y, at, from-to
    // ------------------------------------------------
    struct UiSnapLine
    {
        char   axis = 'x';
        double at = 0, from = 0, to = 0;
    };

    // ------------------------------------------------------------------------
    // What a block snaps to: x and y lines (edges and centres) and the span of guides
    // ------------------------------------------------------------------------
    struct UiSnapTargets
    {
        Vector<double> xs, ys;
        UiRect         span;
    };

    // ------------------------------------------
    // A snapped box and the guides it snapped to
    // ------------------------------------------
    struct UiSnap
    {
        UiRect             rect;
        Vector<UiSnapLine> lines;
    };

    // ------------------------------------
    // The edges a resize moves (n, s, e, w)
    // ------------------------------------
    struct UiEdges
    {
        bool n = false, s = false, e = false, w = false;
    };

    // What a block snaps to while it is dragged: its parent's edges and centre, its shown siblings' edges and centres
    UiSnapTargets SnapTargets(const UiDoc& doc, const UiRects& rects, const String& id);

    // A moved block's box snapped (its edges or centre within tol of a target), with the guides to draw
    UiSnap SnapMove(const UiRect& r, const UiSnapTargets& t, double tol);

    // A resized block's moving edges snapped
    UiSnap SnapResize(const UiRect& r, const UiEdges& edges, const UiSnapTargets& t, double tol);

    // --- keyboard -----------------------------------------------------------------------------------------------

    // Arrow keys: a block nudged by (dx, dy) design pixels - its offsets (or margins) when anchored, the layout's nudge
    // (x / y) inside a row / column / grid; false when there is nothing to do
    bool NudgeOp(const UiDoc& doc, const UiRects& rects, const String& id, double dx, double dy, UiOp& out);

    // A block moved to another parent keeps its place on the screen (Unity's worldPositionStays): the move ({op: "move",
    // id, parent, before?, after?}), then the props that put it where it was when the new parent anchors its children
    // (into a row / column / grid its x / y are dropped instead)
    Vector<UiOp> ReparentOps(const UiDoc& doc, const UiRects& rects, const String& id, const String& parent,
                             const String& before = "", const String& after = "");
}
