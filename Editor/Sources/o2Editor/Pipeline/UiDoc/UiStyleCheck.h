#pragma once

#include <optional>

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

// ------------------------------------------------------------------------------------------------
// The AI text style's check against its source picture (port of AssetsLine backend/src/uiKit/
// styleCheck.ts and shared/styleCheck.ts). After the style is read, the words of the lettering are
// drawn in it by the layout renderer on a flat screen of the picture's own border colour, and a vision
// model compares the drawing with the picture: a verdict, a likeness, the differences and the fields of
// the style to change. A significant difference applies those fields (never the size, the name or the
// role) and goes round again, up to the limit; the attempt with the best likeness is kept. It runs as the
// node's second step (IPipelineNodeImpl::Refine) under the key style-check:v2:<model>:<limit>; the
// switch, the limit and the judge are the AI extract's keys (refCheck, refCheckMax, refCheckModel). The
// judge's answers are memoised by what was asked; the details land in the node's work folder
// (check/style.json and the pictures it names), which the card's popup reads
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // File name stem of the check inside the node's check folder
    extern const char* const STYLE_CHECK_FILE;

    // The refine key of a node's check: "style-check:v2:<model>:<limit>", empty while the check is off. Bump the version when
    // the judge's prompt changes, else the cached check answers
    String StyleCheckKey(const PipelineNode& node);

    // The colour around the lettering: the mean of the picture's border, where a label's backdrop usually shows (#RRGGBB)
    String StyleBackdropOf(const String& png);

    // The sample of a style as a picture for the judge: the words on the backdrop colour, at most 1024 px wide
    UiLayout::UiImage RenderStyleSample(const TextStyle& style, const String& text, const String& background);

    // ----------------------------------------------------
    // An earlier attempt as the next judge prompt lists it
    // ----------------------------------------------------
    struct StyleAttemptBrief
    {
        int                   n = 1;
        TextStyle             style;
        String                verdict; // empty = not judged
        std::optional<double> likeness;
        Vector<String>        differences;
    };

    // The judge's prompt: the style that drew the sample (what it may change, nothing else), the earlier attempts (it must
    // not walk back and forth between them), the font catalogue and how a look is written
    String StyleJudgePrompt(const TextStyle& style, const String& text, const String& hint, bool matte,
                            const Vector<StyleAttemptBrief>& earlier = {});

    // What changed between two styles, field by field ("font: a -> b", "- stroke 2px #000000", "+ ...")
    Vector<String> StyleChanges(const TextStyle& a, const TextStyle& b);

    // The fields a judge may change laid over the style; its size, name and role stay
    TextStyle ApplyJudgeFix(const TextStyle& style, const Json* fix);

    // ---------------------
    // What a check ended in
    // ---------------------
    struct StyleCheckOutcome
    {
        String data;         // The kept attempt's style JSON (the name, role and what the model called it as read)
        bool   final = true; // False when a model call failed: nothing is stored, the next run tries again
    };

    // Checks the style a node read (first, the stored JSON) against its source picture
    Coroutine<StyleCheckOutcome> RunStyleCheck(Ref<PipelineExecContext> ctx, Ref<PipelineNode> node, String source, String first,
                                               String key);

    // The stored result of a check was read back: the debug file becomes that check's, when its details are kept
    void ReplayStyleCheck(const PipelineExecContext& ctx, const String& key);
}
