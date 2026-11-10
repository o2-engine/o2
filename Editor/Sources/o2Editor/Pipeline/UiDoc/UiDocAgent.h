#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSkin.h"

// ------------------------------------------------------------------------------------------------
// The layout agent (port of AssetsLine shared/uiDoc/agent.ts): one model call per request. It reads the
// format, the UI kit (names, types, sizes, nine-slices, text styles), the references wired into the node,
// the current layout as UIML with every block's solved box, a picture of the screen as it renders now, and
// the conversation so far; it answers with a short message and either edits (@update / @add / ...) or the
// whole screen in UIML. Its answer becomes one step of the node's history. The prompts are the web's byte
// for byte for the same inputs
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // The agent's default model
    extern const char* const UI_AGENT_DEFAULT_MODEL;

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
    };

    // The full prompt of one request
    String BuildAgentPrompt(const UiAgentContext& ctx);

    // The check after an answer: the screen it made rendered with the kit (image #1) next to the kit sheet (image #2), and
    // what of the kit it left unused; the agent fixes the kit usage only. missing: (id, type) of blocks without a graphic
    String BuildReviewPrompt(const UiDoc& doc, const UiKit& kit, const String& request, const Vector<Pair<String, String>>& missing,
                             const Vector<String>& unused, const Vector<String>& glyphs);

    // The check prompt from a kit report
    String BuildReviewPrompt(const UiDoc& doc, const UiKit& kit, const String& request, const UiKitReport& report);

    // ---------------------------------------------------------------------
    // The model's answer: its message, the ops and what will not apply
    // ---------------------------------------------------------------------
    struct UiAgentAnswer
    {
        String         reply;
        Vector<UiOp>   ops;
        Vector<String> errors;
    };

    // The model's text -> its message and the operations (a fenced ```uiml-edit / ```uiml block, or bare edit lines, or a
    // bare screen), with the edits that will not apply to doc reported
    UiAgentAnswer ParseAgentAnswer(const String& answer, const UiDoc& doc);
}
