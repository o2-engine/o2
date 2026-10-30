#pragma once

#include "o2Editor/Pipeline/PipelineImageToUi.h"
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

        Vector<PipelineImageToUiEditRound> edits; // Rounds of the user's edits, applied in order after the run
        int                                editTurns = 30; // The agent's turns for one round of edits
    };

    // How a run may start
    struct ImageToUiRunOptions
    {
        bool   fresh = false; // Regenerate: nothing kept from earlier runs, every model asked again
        String baseKey;       // What the run without edits depends on (the design, every setting but the edits, the seed):
                              // rounds continue a run only when it matches
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
        Vector<ImageToUiSprite> sprites; // Every sprite the layout names, in the order the layout first uses them
        Json                    summary; // The agent's summary with the node's counters and the rounds of edits
        bool                    complete = false;
        Vector<String>          blockers;
    };

    // ------------------------------------------------------------------------------------------------
    // One run of the image-to-UI node, AssetsLine's runImageToUi: the analysis names the artwork, the
    // texts are measured by their ink (for what the cuts erase), every look is cut into a sprite and the
    // backdrop gets a colour or a plate; then XuToWei/Image-To-UI's skill runs unchanged on the design and
    // that folder of sliced sprites, with the vision model as its agent (o2/Tools/ImageToUi/agent/agent.py).
    // Then each round of edits continues the skill from the layout as the previous round left it
    // (agent.py --revise). Its ui_structure.json, reconstruction.png and the sprites the structure names
    // are the outputs. The kept states (rounds.json, rounds/<k>/) let a run continue from the newest
    // state its edits share with them; one run at a time in a work folder.
    // workDir: the node's own folder (input/, sprites/, fonts/, task/, agent-view/, trace/, rounds/, edits/)
    // ------------------------------------------------------------------------------------------------
    Coroutine<ImageToUiResult> RunImageToUi(Ref<AiCtx> ai, String designPng, ImageToUiConfig cfg, Ref<Trace> trace, String workDir,
                                            ImageToUiRunOptions opts);

    // Returns the key of a round in rounds.json: the first 16 hex digits of sha256 over the JSON of its notes
    String ImageToUiRoundHash(const PipelineImageToUiEditRound& round);

    // Returns the base key of a run: sha256 in hex over the design bytes and the JSON of the settings the run without edits
    // depends on
    String ImageToUiBaseKey(const String& designPng, const Json& settings);
}
