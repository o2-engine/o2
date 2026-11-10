#pragma once

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/UiDoc/UiDocModel.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // The Text style, UI kit and AI UI layout nodes (AssetsLine's textStyle / uiKit / uiLayout): what they produce,
    // shared with the cards that show it
    // ---------------------------------------------------------------------------------------------------------------
    namespace PipelineUiNodes
    {
        // Returns the picture the AI UI layout node outputs: the layout drawn with the kit (line art without one) at its
        // design size, a PNG tagged with the layout's DPI
        PipelineValue RenderLayoutPicture(const UiDoc::UiDoc& doc, const UiDoc::UiKit* kit);

        // Returns the Text style node's output: its config cleaned into a style, as JSON
        String TextStyleOutput(const PipelineNode& node);

        // Builds the UI kit a UI kit node makes from its inputs (keyed by input port id) and its per-element settings
        UiDoc::UiKit BuildKit(const PipelineNode& node, const Map<String, PipelineValue>& inputsById,
                              const Function<void(const String&)>& log);
    }
}
