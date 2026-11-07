#pragma once

#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutModel.h"

namespace Editor::UiLayout
{
    // JavaScript truthiness of a member: false for a missing one, null, false, 0, NaN and ""
    bool JsTruthy(const Json* value);

    // String(value ?? '') of a member: "" for a missing one and null
    String JsText(const Json* value);

    // Text without leading and trailing whitespace, as String#trim
    String JsTrim(const String& text);

    // The JSON in a model answer (fenced or not); false when there is none
    bool JsonOf(const String& text, Json& out);

    // A box_2d [ymin, xmin, ymax, xmax] on a 0..1000 grid in design pixels; false when it is not one
    bool BoxFrom2d(const Json* value, double w, double h, Box& out);

    // How the analysis of a design is asked and read (AssetsLine's analysisFrame): Gemini models answer Gemini's box_2d
    // grid; any other model answers pixels of the picture it is sent, the design scaled down to what the model sees without
    // scaling it itself (at most 1568 px on the long side and 1.15 megapixels)
    struct AnalysisFrame
    {
        String boxes = "box_2d"; // box_2d or pixels
        int    w = 0;            // The picture the model is sent
        int    h = 0;
        double scale = 1.0;      // Its size over the design's, 1 when it is the design
    };

    // True for a model that places boxes on Gemini's 0..1000 grid: the Gemini family, directly or through OpenRouter
    bool UsesBox2d(const String& model);

    // The frame of the analysis of a w x h design by the model
    AnalysisFrame AnalysisFrameOf(const String& model, int w, int h);

    // A pixel box [x0, y0, x1, y1] of the picture the model saw in design pixels; false when it is not one
    bool BoxFromPixels(const Json* value, const AnalysisFrame& frame, double w, double h, Box& out);

    // A #RRGGBB of a member, empty when it is not a colour
    String ColorOf(const Json* value);

    // One element of an analysis or review answer without its id and parent; null when it is not a valid element
    Ref<UiEl> ElementOf(const Json& raw, double w, double h, const AnalysisFrame* frame = nullptr);

    // The analysis answer as the working model: grids expanded into cells and their texts, ids unique and
    // file-safe, parents that do not contain their children moved to the nearest enclosing element
    bool ParseAnalysis(const String& text, int w, int h, UiDoc& out, String* error = nullptr, const AnalysisFrame* frame = nullptr);

    // Parents must exist, come earlier and contain at least 70 % of the child, else the smallest earlier non-text
    // element that does takes it; grid cells keep their grid
    void FixParents(UiDoc& doc);
}
