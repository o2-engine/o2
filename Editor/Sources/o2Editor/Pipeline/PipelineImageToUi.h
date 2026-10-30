#pragma once

#include <optional>

#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"

using namespace o2;

namespace Editor
{
    // Where a note of an edit points: canvas pixels on the source (design) or on the result, the two have the same size
    struct PipelineImageToUiEditPoint
    {
        String on = "result"; // design or result
        int    x = 0;
        int    y = 0;

        bool operator==(const PipelineImageToUiEditPoint& other) const { return on == other.on && x == other.x && y == other.y; }
    };

    // A note of an edit round: what to change, at a point or (none) on the whole screen
    struct PipelineImageToUiEditItem
    {
        String                                    text;
        std::optional<PipelineImageToUiEditPoint> at;

        bool operator==(const PipelineImageToUiEditItem& other) const { return text == other.text && at == other.at; }
    };

    // The notes sent together: one round of edits after a run
    struct PipelineImageToUiEditRound
    {
        String                            id;
        Vector<PipelineImageToUiEditItem> items;

        bool operator==(const PipelineImageToUiEditRound& other) const { return id == other.id && items == other.items; }
    };

    // A numbered point the edits dialog draws over a picture: a note of the draft, or (past) of an applied round
    struct PipelineImageToUiPin
    {
        int    n = 0;
        String on;
        int    x = 0;
        int    y = 0;
        bool   past = false;
    };

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

        // --- edits after a run -------------------------------------------------------------------------------------------------
        // Once the node has a result the user asks for changes: notes, each with a point on the source or the result, or none.
        // The notes sent together are one round; config edits holds the rounds in order and is part of the signature (each
        // round's result is cached on its own, so removing the last one brings the previous result back). editsDraft (the notes
        // not sent yet) and editsView (the dialog is open) are UI-only. A run continues the skill from the layout as the
        // previous round left it

        constexpr int editTextMax = 2000; // A note's characters at most (UTF-16 units, as JavaScript counts them)
        constexpr int editItemsMax = 20;  // Notes of a round at most

        // Returns the notes of a draft or a stored round cleaned: the text trimmed and cut, empty notes dropped, a point's side
        // design or result (anything else), its coordinates rounded and not below 0, at most editItemsMax
        Vector<PipelineImageToUiEditItem> EditItems(const UiLayout::Json& raw);

        // Returns the rounds of a config edits value in order; a round with no note is dropped, a missing id is e<n>
        Vector<PipelineImageToUiEditRound> Edits(const UiLayout::Json& raw);

        // Returns the rounds in the node's config
        Vector<PipelineImageToUiEditRound> Edits(const PipelineNode& node);

        // Returns the node's draft as stored: empty notes kept (the dialog's fields), points as they are
        Vector<PipelineImageToUiEditItem> ReadDraft(const PipelineNode& node);

        // Returns the agent's turns for one round: half the node's budget, at least 12
        int EditTurns(int maxTurns);

        // Returns the notes as JSON, [{text, at?: {on, x, y}}]
        UiLayout::Json EditItemsJson(const Vector<PipelineImageToUiEditItem>& items);

        // Returns the rounds as JSON, [{id, items}]
        UiLayout::Json EditsJson(const Vector<PipelineImageToUiEditRound>& rounds);

        // Writes the rounds into the node's config edits
        void WriteEdits(PipelineNode& node, const Vector<PipelineImageToUiEditRound>& rounds);

        // Writes the notes not sent yet into the node's config editsDraft
        void WriteDraft(PipelineNode& node, const Vector<PipelineImageToUiEditItem>& items);

        // Returns the id of a new round: "e" and the milliseconds since the epoch in base 36
        String NewEditId(Int64 epochMs);

        // Appends a round of the notes (cleaned) with the id, empties the draft and closes the dialog; false when no note is left
        bool ApplyEditRound(PipelineNode& node, const Vector<PipelineImageToUiEditItem>& items, const String& id);

        // Drops the last round; false when there is none
        bool UndoEditRound(PipelineNode& node);

        // Returns the numbered points of the draft (numbered by their place in the list) and, when viewRound is one of the
        // rounds, the grey ones of that round
        Vector<PipelineImageToUiPin> EditPins(const Vector<PipelineImageToUiEditItem>& draft, const Vector<PipelineImageToUiEditRound>& rounds,
                                              int viewRound);

        // Returns where a note points, "Source · 120, 245" / "Result · 120, 245", or "whole screen"
        String EditPlace(const PipelineImageToUiEditItem& item);

        // Returns true and the pixel of a picture (size in its own pixels) under a point of the rectangle it is drawn in (y up),
        // false outside it
        bool PictureClick(const RectF& drawn, const Vec2I& size, const Vec2F& point, int& x, int& y);

        // Returns true when the dialog may apply: not running, a result to change and at least one non-empty note
        bool CanApplyEdits(bool running, bool hasResult, const Vector<PipelineImageToUiEditItem>& draft);

        // Returns the trace of the node's current state, as the AssetsLine server answers trace.json?edits=<ids>: the kept
        // <work>/rounds/<k>/trace.json when rounds.json's rounds start with exactly the ids of the edits (k of them, 0 for the
        // run without edits), else the live <work>/trace/trace.json. Its pictures are in <work>/trace/ either way
        String StateTracePath(const String& workDir, const Vector<PipelineImageToUiEditRound>& edits);

        // Returns the node config value under the key as JSON, null when missing
        UiLayout::Json ConfigJson(const PipelineNode& node, const String& key);

        // Writes JSON into the node config under the key
        void SetConfigJson(PipelineNode& node, const String& key, const UiLayout::Json& value);
    }
}
