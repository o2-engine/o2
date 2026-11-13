#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSkin.h"

// ------------------------------------------------------------------------------------------------
// The layout agent (port of AssetsLine shared/uiDoc/agent.ts). It reads the format, the UI kit (names,
// types, sizes, nine-slices, purposes, text styles), the references wired into the node, the current layout
// as UIML with every block's solved box, a picture of the screen as it renders now, and the conversation
// so far; it answers with a short message and either edits (@update / @add / ...) or the whole screen in
// UIML, plus @style / @asset lines that add to the node's own kit, and @done / @markup. One request is
// worked in turns (UiLayoutAgentRun); all of it is one step of the node's history. The prompts are the
// web's byte for byte for the same inputs
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // The agent's default model
    extern const char* const UI_AGENT_DEFAULT_MODEL;

    // Turns the layout pass of a screen rebuilt from its reference may take (the node's agentTurns)
    constexpr int UI_AGENT_TURNS = 60;

    // Turns of the plain compare-and-fix loop (when the layout pass is off or did not end with a layout)
    constexpr int UI_AGENT_FIX_TURNS = 6;

    // How many graphics one request may ask to be drawn
    constexpr int AGENT_ASSETS_MAX = 12;

    // The kit additions (@style, and @asset when the node may draw) as the agent reads them
    String UiKitAdditionsSpec(bool draw);

    // The UIML format as the agent reads it
    const String& UimlSpec();

    // The edit commands as the agent reads them
    const String& UiEditsSpec();

    // ---------------------------------------------------------------------------
    // A reference wired into the node: a picture (sent along) or a text (quoted)
    // ---------------------------------------------------------------------------
    struct UiAgentReference
    {
        String name;
        String kind = "image"; // image | text
        String text;           // A text reference's text
        double w = 0, h = 0;   // A picture's full size (0 = not said)
    };

    // -------------------------------------------------------------------------------
    // One of the places where the screen differs most from the reference, with its blocks
    // -------------------------------------------------------------------------------
    struct UiAgentCompareRegion
    {
        UiRect         box;   // Design pixels
        double         share = 0;
        Vector<String> blocks;
    };

    // ---------------------------------------------------------------------------------
    // The screen compared with the first reference picture: both carry the measuring grid
    // ---------------------------------------------------------------------------------
    struct UiAgentCompare
    {
        double                       score = 0;   // 0..1, how alike
        double                       gridStep = 100;
        Vector<UiAgentCompareRegion> regions;
        bool                         zoom = false; // A picture with those places enlarged goes along (the last image)
    };

    // ------------------------------------------------------------------
    // Where the work on one request stands (a request is worked in turns)
    // ------------------------------------------------------------------
    struct UiAgentRunState
    {
        int                           turn = 1;
        int                           maxTurns = 1;
        Vector<String>                log;       // What happened in this run before this turn, one line each
        bool                          built = false; // The layout was drafted from the reference picture in this run
        std::optional<UiAgentCompare> compare;
        Vector<String>                findings;  // What a check of the screen found
        bool                          canMarkup = false; // The agent may ask for the screen to be rebuilt (@markup)
    };

    // ----------------------------------------------------------------------------------------------
    // Everything one request reads. Pictures go along numbered in this order: the screenshot (when
    // withScreenshot), the kit sheet (when withKitSheet), then the reference pictures
    // ----------------------------------------------------------------------------------------------
    struct UiAgentContext
    {
        UiDoc                    doc;
        const UiKit*             kit = nullptr;   // Null = no kit (an empty one is summarised)
        Vector<UiStep>           steps;           // The node's history (the last 10 are quoted)
        String                   prompt;
        Vector<UiAgentReference> references;
        String                   selected;        // The block selected in the workspace, "" = none
        bool                     withScreenshot = false;
        bool                     withKitSheet = false;
        Vector<UiStepPoint>      points;          // Places the user pointed at (design pixels)
        bool                     pinned = false;  // The screenshot and the first reference carry the points as pins
        bool                     canDraw = false; // The node may draw graphics the kit lacks (@asset)
        String                   lang;            // The editor's language: the agent's words to the user are in it
        std::optional<UiAgentRunState> run;
    };

    // The blocks under a point, innermost first (up to three)
    Vector<String> BlocksAt(const UiDoc& doc, double x, double y);

    // The full prompt of one request
    String BuildAgentPrompt(const UiAgentContext& ctx);

    // The check after an answer: the screen it made rendered with the kit (image #1) next to the kit sheet (image #2), and
    // what of the kit it left unused; the agent fixes the kit usage only. missing: (id, type) of blocks without a graphic
    String BuildReviewPrompt(const UiDoc& doc, const UiKit& kit, const String& request, const Vector<Pair<String, String>>& missing,
                             const Vector<String>& unused, const Vector<String>& glyphs);

    // The check prompt from a kit report
    String BuildReviewPrompt(const UiDoc& doc, const UiKit& kit, const String& request, const UiKitReport& report);

    // ------------------------------------------------
    // A graphic the agent asked to be made (@asset)
    // ------------------------------------------------
    struct UiAssetRequest
    {
        String                name;
        String                type;
        String                assetType;
        double                w = 256, h = 256; // Design pixels on the screen
        std::optional<UiRect> box;              // Where it is in the first reference picture, that picture's pixels
        String                purpose;
        String                describe;
    };

    // The request as JSON {name, type, assetType, w, h, box?, purpose?, describe}
    Json AssetRequestToJson(const UiAssetRequest& request);

    // -------------------------------------------------------------
    // The @style / @asset lines of an answer, and the answer without them
    // -------------------------------------------------------------
    struct UiKitAdditions
    {
        String                 rest;
        Vector<TextStyle>      styles;
        Vector<UiAssetRequest> assets;
        Vector<String>         errors;
    };
    UiKitAdditions ParseKitAdditions(const String& answer);

    // ---------------------------------------------------------------------
    // The model's answer: its message, the ops and what will not apply
    // ---------------------------------------------------------------------
    struct UiAgentAnswer
    {
        String                 reply;
        Vector<UiOp>           ops;
        Vector<String>         errors;
        Vector<TextStyle>      styles;
        Vector<UiAssetRequest> assets;
        bool                   done = false;   // The agent said it is finished (@done)
        bool                   markup = false; // The agent asked for the screen to be rebuilt from the reference (@markup)
    };

    // The model's text -> its message and the operations (a fenced ```uiml-edit / ```uiml block, or bare edit lines, or a
    // bare screen), with the edits that will not apply to doc reported
    UiAgentAnswer ParseAgentAnswer(const String& answer, const UiDoc& doc);
}
