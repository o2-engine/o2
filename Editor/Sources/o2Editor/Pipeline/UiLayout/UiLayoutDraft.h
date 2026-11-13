#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutModel.h"

// ------------------------------------------------------------------------------------------------
// The elements a reference picture was read into (the AI UI layout node's markup) -> the layout document
// (port of AssetsLine backend/src/uiLayout/draft.ts): every element is a block at its measured box, anchored
// so that the screen adapts (what spans its parent stretches with it, what sits by an edge or in the middle
// stays there), drawn by the kit graphic the markup chose or by the graphic cut for its look, its label in
// a text style. Inside this namespace UiDoc is the analysis model; the layout document is
// Editor::UiDoc::UiDoc
// ------------------------------------------------------------------------------------------------
namespace Editor::UiLayout
{
    // --------------------------------------------------------------------------------------------
    // What a draft is drawn with: the kit, the graphics cut for looks, the styles told for labels
    // --------------------------------------------------------------------------------------------
    struct DraftOptions
    {
        const ::Editor::UiDoc::UiKit*         kit = nullptr;
        Map<String, String>                   assetOf;         // Look key -> the name of the graphic cut for it
        Map<String, String>                   styleOf;         // Text element id -> its text style
        Vector<::Editor::UiDoc::TextStyle>    styles;          // Every style there is (the kit's, then the ones read off the picture)
        std::optional<double>                 dpi;
        String                                name;
        String                                backgroundKit;   // The backdrop's kit graphic ("" = none)
        String                                backgroundColor; // The screen's colour when there is no backdrop graphic ("" = none)
    };

    // ------------------------------------------------------------------------
    // The layout, the styles made for it, the blocks' reference boxes, a count
    // ------------------------------------------------------------------------
    struct DraftResult
    {
        ::Editor::UiDoc::UiDoc                      doc;
        Vector<::Editor::UiDoc::TextStyle>          styles; // Styles made here for labels that were told none
        Map<String, ::Editor::UiDoc::UiRect>        boxes;  // Block id -> the box its element has on the reference (design pixels)
        int                                         blocks = 0;
    };

    // Where a block holds on to its parent: stretched along an axis it spans, else by the nearer edge or the middle
    ::Editor::UiDoc::Anchor AnchorFor(const ::Editor::UiDoc::UiRect& r, const ::Editor::UiDoc::UiRect& p);

    // The analysis -> the layout document
    DraftResult DraftFromAnalysis(const UiDoc& analysis, const DraftOptions& o);
}
