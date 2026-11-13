#pragma once

#include <cmath>
#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocModel.h"

// ------------------------------------------------------------------------------------------------
// Edits of a layout and the node's queue of them (port of AssetsLine shared/uiDoc/ops.ts). The AI UI
// layout node keeps no document of its own: config `steps` is the list of edits, each made by the agent
// (with the user's prompt and its reply) or by the user. The layout is those edits applied in order to an
// empty screen - so undo drops the last step, and the history can show the screen after any step.
//
// Ops and steps stay JSON objects (UiOp = Json, UiStep wraps one): what a node's config stores reads and
// writes back unchanged, member order included, and new ones are built in the web's key order
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // --- ops --------------------------------------------------------------------------------------------

    // The op's kind: doc, screen, update, add, remove, move, replace ("" when it has none)
    String OpKind(const UiOp& op);

    // {op: "doc", doc}: the whole screen
    UiOp MakeDocOp(const UiDoc& doc);

    // {op: "screen", name?, width?, height?, dpi?}
    UiOp MakeScreenOp(const String& name, std::optional<double> width, std::optional<double> height, std::optional<double> dpi);

    // {op: "screen", width, height, dpi, scale, fromDpi}: the whole layout brought to other units (every pixel value times
    // scale) - a layout made in a reference picture's pixels brought to its kit's device
    UiOp MakeRescaleOp(double width, double height, double dpi, double scale, double fromDpi);

    // {op: "update", id, set?, unset?}: set is an object of raw values (cleaned when applied); key=- is unset
    UiOp MakeUpdateOp(const String& id, const Json& set, const Vector<String>& unset = {});

    // {op: "add", parent, nodes, index?, after?, before?}
    UiOp MakeAddOp(const String& parent, const Vector<Json>& nodes, std::optional<double> index = std::nullopt,
                   const String& after = "", const String& before = "");

    // {op: "remove", ids}
    UiOp MakeRemoveOp(const Vector<String>& ids);

    // {op: "move", id, parent, index?, after?, before?} ("" parent = the same one)
    UiOp MakeMoveOp(const String& id, const String& parent, std::optional<double> index = std::nullopt, const String& after = "",
                    const String& before = "");

    // {op: "replace", id, node}
    UiOp MakeReplaceOp(const String& id, const Json& node);

    // --- applying ------------------------------------------------------------------------------------------

    // ------------------------------------------
    // A document after edits, with the failed ones
    // ------------------------------------------
    struct UiApplyResult
    {
        UiDoc          doc;
        Vector<String> errors; // One message per edit that failed (the web's texts: @update: no block "x"...)
    };

    // Applies one op to the document in place; false with the message when it fails (what it did before failing stays,
    // as in the web)
    bool ApplyOp(UiDoc& doc, const UiOp& op, String& error);

    // The document after the ops (a copy); failed ops are skipped and reported
    UiApplyResult ApplyOps(const UiDoc& doc, const Vector<UiOp>& ops);

    // --- steps ----------------------------------------------------------------------------------------------

    // ------------------------------------------------------------------
    // One action of a hand-edit step: its label, how many ops, when
    // ------------------------------------------------------------------
    struct UiStepPart
    {
        String label;
        double n = 0;
        String at;
    };

    // ------------------------------------------------------------------------------------------
    // One step of the node's history, as stored: {id, by: agent|user, prompt?, reply?, ops, at, model?,
    // review?: {reply, edits}, parts?: [{label, n, at}], partOf?}
    // ------------------------------------------------------------------------------------------
    struct UiStep
    {
        Json data; // The step object; the accessors read and write it

    public:
        UiStep() : data(Json::Object()) {}
        explicit UiStep(const Json& json) : data(json) {}

        String Id() const;
        String By() const;                     // agent | user
        bool IsAgent() const { return By() == "agent"; }
        std::optional<String> Prompt() const;  // The user's request (agent steps) or what the user did
        std::optional<String> Reply() const;   // The agent's answer to the user
        String At() const;                     // ISO time
        std::optional<String> Model() const;
        std::optional<String> PartOf() const;  // Redo only: the step an undone action came out of

        // The ops (an empty list when the step has none)
        const Vector<UiOp>& Ops() const;

        // The check pass after the answer: what it said and how many edits it added; false when there was none
        bool Review(String& reply, double& edits) const;

        // Sets the check pass result
        void SetReview(const String& reply, double edits);
    };

    // ---------------------------------------------------------------------------------
    // A place the user pointed at (design pixels of the screen as it was), with an older note
    // ---------------------------------------------------------------------------------
    struct UiStepPoint
    {
        double x = 0, y = 0;
        String text;

        bool operator==(const UiStepPoint& other) const { return x == other.x && y == other.y && text == other.text; }
    };

    // Points as stored: inside the screen (w x h), at most 20, each note trimmed to 2000 characters
    Vector<UiStepPoint> CleanPoints(const Json* raw, double w = INFINITY, double h = INFINITY);

    // Points as the JSON a step stores: [{x, y, text}]
    Json PointsToJson(const Vector<UiStepPoint>& points);

    // The numbers of the points a text names as [1], [2]... (of count points), in the order it first names them
    Vector<int> PinsNamed(const String& text, int count);

    // A draft with the notes of points it does not name written into it: "[n] note" on a line of its own
    String DraftWithNotes(const String& draft, const Vector<UiStepPoint>& points);

    // A request as it is sent: the points its text names, in the order it names them, and the text renumbered to match
    struct UiPinnedRequest
    {
        String              text;
        Vector<UiStepPoint> points;
    };
    UiPinnedRequest RequestWithPins(const String& draft, const Vector<UiStepPoint>& points);

    // A new agent step in the web's key order: {id, by: "agent", prompt, reply, ops, at, model}
    UiStep MakeAgentStep(const String& prompt, const String& reply, const Vector<UiOp>& ops, Int64 nowMs, const String& model);

    // Config keys of the layout node that never change what it renders
    const Vector<String>& UiLayoutUiOnly();

    // The steps of a node's config (entries without an ops list are skipped)
    Vector<UiStep> StepsOf(const Json& config);

    // The redo list of a node's config (config.uiRedo)
    Vector<UiStep> RedoOf(const Json& config);

    // Steps as the JSON array a config stores
    Json StepsToJson(const Vector<UiStep>& steps);

    // The screen a node starts from, before any step (config.screenName names it)
    UiDoc BaseDoc(const Json& config);

    // The layout after the first `count` steps (JavaScript slice: negative counts from the end; none = all). Failed edits
    // are skipped and reported
    UiApplyResult DocAt(const Json& config, std::optional<int> count = std::nullopt);

    // The steps, the redo list and the layout of a node's o2 config (the same as with its JSON)
    Vector<UiStep> StepsOf(const DataValue& config);
    Vector<UiStep> RedoOf(const DataValue& config);
    UiApplyResult DocAt(const DataValue& config, std::optional<int> count = std::nullopt);

    // Writes the steps (config.steps) and the redo list (config.uiRedo) into a node's o2 config
    void WriteSteps(DataValue& config, const Vector<UiStep>& steps, const Vector<UiStep>& redo);

    // The layout after the first `count` of the given steps on the config's base screen
    UiApplyResult DocAtSteps(const Json& config, const Vector<UiStep>& steps, std::optional<int> count = std::nullopt);

    // --- what a step changed --------------------------------------------------------------------------------

    // ---------------------------------------------------------
    // Blocks a step added, removed, changed, moved to another parent
    // ---------------------------------------------------------
    struct DocDiff
    {
        Vector<String> added, removed, changed, moved;
        bool           screen = false; // Name, size or dpi changed
    };

    // What changed from a to b
    DocDiff DiffDocs(const UiDoc& a, const UiDoc& b);

    // "+2 ~1 up-down1 -1 screen", or an em dash for nothing
    String DiffSummary(const DocDiff& d);

    // A new step id: s + base-36 time + 4 random base-36 characters
    String NewStepId(Int64 nowMs = 0);

    // --- hand edits: one step, many actions -------------------------------------------------------------------

    // Hand edits more than this apart start a new step (10 min)
    constexpr double HAND_STEP_IDLE_MS = 10*60000.0;

    // Updates of one field this close together are one action (typing a label, holding an arrow key)
    constexpr double HAND_FOLD_MS = 4000.0;

    // The actions of a step (an agent step, or an old user step, is one)
    Vector<UiStepPart> StepParts(const UiStep& step);

    // What a user step did, in words: its actions' labels joined with "; "
    String UserStepText(const UiStep& step);

    // The steps after one more hand action. fold: a field typed into (the same label on the same block a moment later)
    // stays one action; a gesture on the stage (fold = false) is always its own
    Vector<UiStep> PushUserAction(const Vector<UiStep>& steps, const Vector<UiOp>& ops, const String& label, Int64 nowMs,
                                  bool fold = true);

    // ------------------------------------
    // The history after an undo or a redo
    // ------------------------------------
    struct UiUndoResult
    {
        Vector<UiStep> steps;
        Vector<UiStep> redo;
    };

    // Undo: the last action of a run of hand edits, else the whole last step; false when there is nothing to undo
    bool UndoAction(const Vector<UiStep>& steps, const Vector<UiStep>& redo, UiUndoResult& out);

    // Redo: an undone action goes back into its step, anything else comes back as a step; false when there is nothing
    bool RedoAction(const Vector<UiStep>& steps, const Vector<UiStep>& redo, UiUndoResult& out);
}
