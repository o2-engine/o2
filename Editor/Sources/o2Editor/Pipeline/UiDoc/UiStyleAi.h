#pragma once

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/UiDoc/UiDocModel.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"

// ------------------------------------------------------------------------------------------------
// A text style read off a picture: the AI text style node's run (port of AssetsLine
// backend/src/uiKit/styleAi.ts extractTextStyle). One vision call of the node's frame of the picture,
// flattened onto grey where it is transparent; the answer's measurements are brought to the styles'
// scale (px on a 1080-px-wide screen) by ParseStyleExtractAnswer
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // The colour transparent areas of a picture are shown to the style model as
    extern const char* const STYLE_MATTE;

    // ------------------------------------------------
    // A picture as the style model gets it
    // ------------------------------------------------
    struct UiStylePicture
    {
        String png;
        int    width = 0, height = 0;
        bool   matte = false;     // Transparent areas were flattened onto STYLE_MATTE
        double fullWidth = 0;     // A frame of a larger picture: that one's width at the same scale (0 = the whole picture)
    };

    // A picture for the style model: at most max px on its longer side, transparency flattened onto STYLE_MATTE; false when
    // it does not decode
    bool StylePictureForModel(const String& png, UiStylePicture& out, int max = 1024);

    // The picture a style is read off: the frame (fractions; none = the whole picture) as the model gets it, and the
    // width of the whole picture at that scale
    bool StyleSourcePicture(const String& image, const PipelineImageOps::CropRect* frame, UiStylePicture& out);

    // -------------------------------------
    // What a style read off a picture gave
    // -------------------------------------
    struct UiStyleExtractResult
    {
        bool   ok = false;
        String error;
        Json   style;   // The style JSON with `extracted` ({name, role, text?})
        String model;
    };

    // The text style of lettering in a picture: one vision call (a test stand-in through SetUiAiTextStub). Fails when the
    // picture has no lettering or the model answers no style
    Coroutine<UiStyleExtractResult> ExtractTextStyle(Ref<PipelineExecContext> ctx, String image, String hint, String model,
                                                    std::optional<PipelineImageOps::CropRect> frame, String name, String role);

    // A style's sample (shared/uiDoc/sample.ts): one label on a flat screen of the background colour, on one line at the
    // style's own size - the screen widens with the text and leaves room for what the effects add around the letters
    UiDoc SampleDoc(const TextStyle& style, const String& text, const String& background);

    // The node's name / role set over a stored extracted style (the AI text style node's local step); text that is not a
    // JSON object comes back as it is
    String WithStyleOverrides(const String& styleJson, const String& name, const String& role);
}
