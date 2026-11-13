#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"

// ------------------------------------------------------------------------------------------------
// ui_structure.json (XuToWei/Image-To-UI's schema - what the skill authors when it rebuilds a screen from a
// picture) -> a layout document (port of AssetsLine shared/uiDoc/fromStructure.ts). The inverse of
// UiDocStructure.h, used by the AI UI layout node's build from a reference: the skill measures and checks in
// its own format, the node keeps the result as its own adaptive layout, skinned by kit names and text styles.
// Geometry comes from the boxes the skill's verifier resolved (all_elements_legend.json: path -> absolute box),
// not from re-deriving its align / layout rules: every block is placed at its box by its anchor (stretched
// along an axis it fills, or where `responsive` says so); a row / column / grid stays a layout only when the
// solver puts the children on those boxes - otherwise its children are anchored blocks
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // ----------------------------------------
    // What a conversion made and what it missed
    // ----------------------------------------
    struct FromStructureStats
    {
        int            blocks = 0, layouts = 0, layoutsFlattened = 0, buttons = 0, texts = 0;
        Vector<String> assetsMissing; // Asset paths the kit has no graphic for, in first-seen order
    };

    // -------------------------------------------------------------------------------------------
    // A conversion's input: the structure, the resolved boxes and the kit its graphics and styles come from
    // -------------------------------------------------------------------------------------------
    struct FromStructureInput
    {
        Json                  structure;     // ui_structure.json, parsed
        Map<String, UiRect>   boxes;         // Resolved absolute boxes by element path (root/popup/title), from the skill's legend
        const UiKit*          kit = nullptr; // The node's kit: its graphics are matched by asset file name, its styles by look
        Map<String, String>   assetNames;    // Asset file (lower case) -> the kit graphic's name, when the file is not <name>.png
        std::optional<double> dpi;
    };

    // ----------------------------------------------------------------
    // The layout, the styles made for it, what was not carried, counts
    // ----------------------------------------------------------------
    struct FromStructureResult
    {
        UiDoc              doc;
        Vector<TextStyle>  styles;   // Styles made for labels no kit style fits
        Vector<String>     warnings;
        FromStructureStats stats;
    };

    // ui_structure.json -> a layout document
    FromStructureResult FromUiStructure(const FromStructureInput& input);

    // The skill's legend (all_elements_legend.json) -> boxes by path
    Map<String, UiRect> LegendBoxes(const Json& legend);

    // The block type a kit element type is drawn as (ELEMENT_NODE): button -> button, frame -> panel, progressFill ->
    // image...; "" for an unknown type
    String ElementNodeType(const String& elementType);
}
