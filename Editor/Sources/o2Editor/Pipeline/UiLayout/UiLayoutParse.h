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

    // A #RRGGBB of a member, empty when it is not a colour
    String ColorOf(const Json* value);

    // One element of an analysis or review answer without its id and parent; null when it is not a valid element
    Ref<UiEl> ElementOf(const Json& raw, double w, double h);

    // The analysis answer as the working model: grids expanded into cells and their texts, ids unique and
    // file-safe, parents that do not contain their children moved to the nearest enclosing element
    bool ParseAnalysis(const String& text, int w, int h, UiDoc& out, String* error = nullptr);

    // Parents must exist, come earlier and contain at least 70 % of the child, else the smallest earlier non-text
    // element that does takes it; grid cells keep their grid
    void FixParents(UiDoc& doc);
}
