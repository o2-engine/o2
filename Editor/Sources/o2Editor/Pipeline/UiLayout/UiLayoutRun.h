#pragma once

#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutSlice.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutTrace.h"

namespace Editor::UiLayout
{
    // What the node asks of a run
    struct ImageToUiConfig
    {
        int    maxTurns = 60;        // Agent turns at most
        String sprites = "auto";     // auto: a flat backdrop is keyed out locally; ai: every look is cut by the model
        String background = "auto";  // auto, image, color, none
        String name = "Screen";      // Canvas name
        String hint;                 // Notes for the model
        String python;               // Python of the agent, empty to search
    };

    // A sprite the layout uses
    struct ImageToUiSprite
    {
        String                   key;  // File stem in the sliced-asset folder: the layout's asset is <key>.png
        String                   png;
        std::optional<NineSlice> nine; // Its nine-slice borders, written as .png.meta for the skill
    };

    // What a run produced
    struct ImageToUiResult
    {
        bool                    ok = false;
        String                  error;
        String                  preview; // task/reconstruction.png
        String                  layout;  // task/ui_structure.json, byte for byte
        Vector<ImageToUiSprite> sprites; // Every sprite the layout names, in the order of the asset folder
        Json                    summary; // The agent's summary with the node's counters
        bool                    complete = false;
        Vector<String>          blockers;
    };

    // ------------------------------------------------------------------------------------------------
    // One run of the image-to-UI node, AssetsLine's runImageToUi: the analysis names the artwork, the
    // texts are measured by their ink (for what the cuts erase), every look is cut into a sprite and the
    // backdrop gets a colour or a plate; then XuToWei/Image-To-UI's skill runs unchanged on the design and
    // that folder of sliced sprites, with the vision model as its agent (o2/Tools/ImageToUi/agent/agent.py).
    // Its ui_structure.json, reconstruction.png and the sprites the structure names are the outputs.
    // workDir: the node's own folder (input/, sprites/, fonts/, task/, agent-view/, trace/)
    // ------------------------------------------------------------------------------------------------
    Coroutine<ImageToUiResult> RunImageToUi(Ref<AiCtx> ai, String designPng, ImageToUiConfig cfg, Ref<Trace> trace, String workDir);
}
