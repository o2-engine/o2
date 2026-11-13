#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutDraft.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunLog.h"

// ------------------------------------------------------------------------------------------------
// The markup of a reference picture - how the AI UI layout agent starts a screen it is asked to rebuild
// (port of AssetsLine backend/src/uiLayout/markup.ts). The AI image to UI node's analysis, adapted to the
// layout node:
//  1. one vision call reads the screen into elements (boxes, hierarchy, texts) IN TERMS OF THE KIT: the kit
//     sheet is in front of the model, an element a kit graphic can draw names that graphic, and only
//     artwork the kit has nothing for is described as a "look" to cut out of the picture;
//  2. the texts are measured on the pixels (ink box, size, colour), grids fitted;
//  3. the lettering is read into text styles (kit styles reused), each label told its style;
//  4. the looks the kit lacks are cut out of the picture and become the node's own graphics;
//  5. the elements become the layout's blocks at their boxes (UiLayoutDraft)
// ------------------------------------------------------------------------------------------------
namespace Editor::UiLayout
{
    // What the layout node adds to the analysis rules whatever the kit: flat shapes need no artwork, and the order of the
    // list is the order of drawing
    const String& MarkupFlatRule();

    // What the analysis prompt adds when the screen is rebuilt with a kit (sheetImage: the kit sheet's number)
    String KitMarkupRules(const ::Editor::UiDoc::UiKit& kit, double layoutDpi, int sheetImage);

    // A label found on the mockup
    struct MarkupLabel
    {
        String text;
        Box    box;
    };

    // The prompt that reads the lettering of a mockup into text styles and says which style each label is drawn in
    String MarkupStylesPrompt(const Vector<MarkupLabel>& labels, const ::Editor::UiDoc::UiKit* kit, int w, int h);

    // The styles answer: the new styles (renamed around the kit's) and the style of each label (empty: none)
    struct MarkupStyles
    {
        Vector<::Editor::UiDoc::TextStyle> styles;
        Vector<String>                     byLabel;
    };
    MarkupStyles ParseMarkupStyles(const String& answer, const ::Editor::UiDoc::UiKit* kit, int labels);

    // A kit graphic for a name the model wrote: exact, else the one name that contains it or is contained; null for none
    const ::Editor::UiDoc::UiKitElement* KitByName(const ::Editor::UiDoc::UiKit* kit, const String& name);

    // The font size at which a text in the default font shows glyphs inkH pixels high (the web renders it at 100 px)
    double InkFontSize(const String& text, const String& weight, double inkH);

    // A label's measured ink is kept only where it agrees with the box the model gave; otherwise the model's box - tight
    // around the glyphs with their outline - gives the size
    void TrustText(UiEl& e);

    // What a markup is given
    struct MarkupInput
    {
        Ref<AiCtx>                       ai;
        String                           design;      // The reference, PNG
        double                           designDpi = 480;
        const ::Editor::UiDoc::UiKit*    kit = nullptr; // Every connected kit and what the node made before
        String                           hint;
        String                           name;
        bool                             canDraw = true; // Cut what the kit lacks out of the picture
        String                           workDir;
        Ref<RunLog>                      log;
    };

    // What a markup made: the draft and the node's own graphics and styles
    struct MarkupResult
    {
        bool                                  ok = false;
        String                                error;
        DraftResult                           draft;
        Vector<::Editor::UiDoc::UiOwnAsset>   assets;
        Vector<::Editor::UiDoc::TextStyle>    styles;     // Only the styles the draft uses
        std::optional<::Editor::UiDoc::UiKit> kit;        // The kit with the graphics and styles made here
        int                                   fromKit = 0; // Elements drawn by graphics of the kit / cut here / left bare
        int                                   cut = 0;
        int                                   bare = 0;
        String                                designFlatPng; // The reference as the analysis saw it: flattened on white
    };

    // Reads a reference picture into a first layout in terms of the kit (see above); the acts go to the log, the trace to
    // <work>/trace
    Coroutine<MarkupResult> MarkupReference(MarkupInput input);
}
