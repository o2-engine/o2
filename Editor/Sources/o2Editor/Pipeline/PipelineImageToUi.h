#pragma once

#include <optional>

#include "o2Editor/Pipeline/PipelineGraph.h"

using namespace o2;

namespace Editor
{
    // The node's settings as a run reads them
    struct PipelineImageToUiConfig
    {
        String model;               // Vision model of the analysis and the agent
        String imageModel;          // Image model of the mattes and the plate
        int    maxTurns = 60;       // Agent turns at most, 4..120 (the card offers 20 / 40 / 60 / 90 / 120)
        String sprites = "auto";    // auto or ai
        String background = "auto"; // auto, image, color or none
        String screenName;          // Canvas name, [A-Za-z0-9_-] only
        String hint;                // Notes for the model
        String assetPath;           // Assets folder the prototype is written to
    };

    // What the card shows of a layout: its counts
    struct PipelineImageToUiSummary
    {
        int    elements = 0;
        int    sprites = 0;
        int    texts = 0;
        String name;
        int    width = 0;
        int    height = 0;
    };

    // How the skill ended on the node's last run: the summary of its trace
    struct PipelineImageToUiRun
    {
        bool complete = false;
        int  checks = 0;
        int  blockers = 0;
        int  turns = 0;
    };

    // --------------------------------------------------------------------------------------------------
    // AI image to UI: what the o2 editor, the AssetsLine server and web editor and the Unity plugin agree on
    // (AssetsLine's shared/imageToUi.ts). The outputs are preview (the skill's reconstruction.png), layout
    // (its ui_structure.json) and one image output per sprite the layout names, named by the file stem, id sp-<name>
    // --------------------------------------------------------------------------------------------------
    namespace PipelineImageToUi
    {
        extern const String type;              // imageToUi
        extern const String defaultModel;      // gemini-3.1-pro-preview
        extern const String defaultImageModel; // gemini-3.1-flash-image
        extern const String defaultScreenName; // Screen
        extern const String defaultAssetPath;  // Generated/UI
        constexpr int       defaultMaxTurns = 60;

        // Returns the agent turn limits the card offers: 20, 40, 60, 90, 120
        const Vector<int>& TurnOptions();

        // Returns the output name of a sprite key: preview and layout become sprite_<key>
        String SpriteName(const String& key);

        // Returns the sprite outputs of a node in order: every image output but preview
        Vector<PipelinePort> SpritePorts(const PipelineNode& node);

        // Returns the run settings of a node: defaults for missing keys, the turns clamped to 4..120, the screen name cleaned
        PipelineImageToUiConfig ReadConfig(const PipelineNode& node);

        // Returns the counts of a layout (sprites: the distinct assets its nodes name), nothing when it is not one
        std::optional<PipelineImageToUiSummary> SummarizeLayout(const String& json);

        // Returns how the skill ended from a trace.json (its summary), nothing without one
        std::optional<PipelineImageToUiRun> ReadRunSummary(const String& traceJson);

        // Returns "completed · 3 checks" or "not completed · 2 blockers"
        String RunLine(const PipelineImageToUiRun& run);

        // Returns the card's summary line: "completed · 3 checks · 9 elements · 3 sprites", without the run's part when there is
        // no run summary; sprites >= 0 counts the sprite outputs instead of the sprites the layout names
        String SummaryLine(const PipelineImageToUiSummary& summary, const PipelineImageToUiRun* run, int sprites = -1);
    }
}
