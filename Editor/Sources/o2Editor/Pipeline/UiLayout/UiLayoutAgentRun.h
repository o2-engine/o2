#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocAgent.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunLog.h"

// ------------------------------------------------------------------------------------------------
// One request to the AI UI layout node's agent, worked in turns - the node's only way of asking a model
// (port of AssetsLine backend/src/uiLayout/agentRun.ts):
//  - A reference picture and an empty layout: the reference is first read into a draft (UiLayoutMarkup:
//    elements in terms of the kit, text styles, the graphics the kit lacks cut from the picture), then laid
//    out by the Image-To-UI skill with the kit as its sprite folder and the draft as its first
//    ui_structure.json (UiLayoutSkillBuild). The agent can ask for the same on a layout that exists (@markup).
//  - Without a skill pass (off, or it ended without a checked layout): turns. Each turn the agent gets the
//    layout, the kit, the screen rendered now and - while a reference is being rebuilt - the reference under
//    the same measuring grid, the places that differ most and those places enlarged side by side. It answers
//    with edits, text styles (@style) and graphics to make (@asset); they are applied, drawn, and the next turn
//    sees the result. It ends with @done, with a turn that changes nothing, or when the turns run out.
//  - An edit, a described screen, points: one turn does the request; a second one follows only when a check
//    of the result finds kit graphics unused beside placeholders, character icons or blocks off the screen.
// Everything the run did is one step of the node's history, with the acts (phases and turns) to unfold.
// Model answers of the turns are memoised under the run's id
// ------------------------------------------------------------------------------------------------
namespace Editor::UiLayout
{
    // A reference the node is wired to: a picture (in the layout's units: the device's pixels) or a text
    struct LayoutAgentRef
    {
        String name;
        bool   image = true;
        String png;
        double w = 0, h = 0, dpi = 480;
        String text;
    };

    // One request to the agent
    struct LayoutAgentInput
    {
        Ref<AiCtx>                              ai;          // The agent's model (textModel), the image model, the memo, the run
        String                                  runId;
        ::Editor::UiDoc::UiDoc                  doc;         // The layout now
        Vector<::Editor::UiDoc::UiStep>         steps;       // And the steps that made it
        std::optional<::Editor::UiDoc::UiKit>   kit;         // Every connected kit and the node's own graphics and styles so far
        Vector<LayoutAgentRef>                  refs;
        String                                  prompt;
        Vector<::Editor::UiDoc::UiStepPoint>    points;
        String                                  selected;
        String                                  lang;
        bool                                    canDraw = true; // Graphics the kit lacks may be made (cut, drawn)
        bool                                    review = true;  // Check an edit and let the agent fix what the check finds
        int                                     maxTurns = ::Editor::UiDoc::UI_AGENT_TURNS; // The skill pass's budget
        bool                                    skill = true;   // Lay a rebuilt screen out with the skill
        String                                  workDir;     // The node's own folder (runs/, the skill's folders, trace/)
        String                                  python;      // The skill's Python, empty to search
        Ref<RunLog>                             log;
        Int64                                   nowMs = 0;   // The step's time; 0 = now
    };

    // The step the run came to, and what did not apply
    struct LayoutAgentResult
    {
        bool                    ok = false;
        String                  error;
        ::Editor::UiDoc::UiStep step;
        Vector<String>          errors;
    };

    // What a check of an edited screen finds for the agent to fix (empty: nothing)
    Vector<String> CheckFindings(const ::Editor::UiDoc::UiDoc& doc, const ::Editor::UiDoc::UiKit* kit);

    // The turns of the draft's blocks put back on the blocks of the same id in a layout the skill rebuilt (it keeps the draft's
    // ids and measures every box upright). A block it turned itself keeps its own turn; the root never turns
    ::Editor::UiDoc::UiDoc KeepTurns(const ::Editor::UiDoc::UiDoc& doc, const ::Editor::UiDoc::UiDoc& draft);

    // Works one request to the layout agent (see above)
    Coroutine<LayoutAgentResult> RunLayoutAgent(LayoutAgentInput input);
}
