#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"

// ------------------------------------------------------------------------------------------------
// What a layout takes from its UI kit, without drawing (the non-SVG part of AssetsLine
// shared/uiDoc/render.ts): the kit graphic a block draws (by kit=, else by its type: TYPE_SKIN), the text
// style a label is drawn with, how a label wraps and shrinks into its box, and how a layout uses its kit
// (kitReport / kitCoverage). The renderer and the export build on these
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // Design pixels one pixel of a kit graphic takes in this layout
    inline double KitScale(const UiDoc& doc, const UiKitElement& el) { return KitScale(doc.dpi, el.dpi); }

    // A graphic's size in the layout's design pixels
    inline UiSize DesignSize(const UiDoc& doc, const UiKitElement& el) { return DesignSize(doc.dpi, el.width, el.height, el.dpi); }

    // The element types a block of the type takes a graphic of, in order of preference (none for text, group, screen)
    const Vector<String>& TypeSkin(const String& nodeType);

    // ----------------------------------------------------------------------------------------------
    // What a block draws from the kit: its graphic, a progress bar's fill, a slider's handle; explicit when
    // kit= names it; missing when the block wants a graphic and gets none (drawn dashed as a placeholder)
    // ----------------------------------------------------------------------------------------------
    struct UiSkin
    {
        const UiKitElement* el = nullptr;
        const UiKitElement* fill = nullptr;
        const UiKitElement* handle = nullptr;
        bool                explicitKit = false;
        bool                missing = false;
    };

    // What a block draws from the kit (null kit = none). Icons and pictures are only taken by name (kit=) or by a word of
    // their id - any other icon would be a wrong one
    UiSkin SkinOf(const UiNode& n, const UiKit* kit);

    // The role a label plays when its block names no style (labelOf: the block it is the label of - a button...)
    String RoleOf(const UiNode& n, const UiNode* labelOf = nullptr);

    // The text style a label is drawn with: its style=, else the kit's style for its role, else the kit's body style,
    // else the default for the role
    TextStyle StyleOf(const UiNode& n, const UiKit* kit, const UiNode* labelOf = nullptr);

    // The style for a label in a given role: the block's own style= wins
    TextStyle StyleForRole(const UiNode& n, const UiKit* kit, const String& role);

    // ------------------------------------------------------------
    // A label laid into a box: its lines, the size and the line step
    // ------------------------------------------------------------
    struct UiTextFit
    {
        Vector<String> lines;
        double         size = 0;
        double         lineH = 0;
    };

    // The lines and the size a label is drawn at in a box w x h (padX on both sides): as given (size: the block's fontSize,
    // 0 = the style's), wrapped when it has the height for it, shrunk (to 40 % at most) when it still does not fit. The
    // text is upper-cased when the style says so. Widths are EstimateTextWidth's
    UiTextFit LayoutText(const String& text, double w, double h, const TextStyle& style, double size = 0, double padX = 0);

    // ---------------------------------------------------------------------------
    // How a layout uses its kit: placeholders, unused graphics, character icons
    // ---------------------------------------------------------------------------
    struct UiKitReport
    {
        Vector<Pair<String, String>> missing; // Blocks (id, type) that want a graphic and get none
        Vector<String>               unused;  // Kit graphics no block draws
        Vector<String>               glyphs;  // Labels that fake an icon with characters (heart star gear + ...)
        int                          covered = 0;
        int                          total = 0;
    };

    // How a layout uses its kit
    UiKitReport KitReport(const UiDoc& doc, const UiKit* kit);

    // ----------------------------------------------------------------------------
    // Which blocks the kit skins and which it leaves as line art (the card's coverage)
    // ----------------------------------------------------------------------------
    struct UiKitCoverage
    {
        int            skinned = 0;
        Vector<String> missing;
        int            total = 0;
    };

    // The kit coverage of a layout
    UiKitCoverage KitCoverage(const UiDoc& doc, const UiKit* kit);

    // True for a text that stands in for an icon: 1 to 3 characters, none a letter, digit or space
    bool IsGlyphText(const String& text);

    // The anchor's point(s) on the parent box: one, or two / four along stretched axes (the anchor gizmo)
    Vector<Vec2F> AnchorPoints(const UiNode& n, const UiRect& parent);
}
