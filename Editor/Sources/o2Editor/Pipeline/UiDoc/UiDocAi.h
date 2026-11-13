#pragma once

#include <optional>

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAgent.h"

// ------------------------------------------------------------------------------------------------
// The AI of the UI kit, Text style and AI UI layout nodes (port of AssetsLine backend/src/routes/uiAi.ts).
// None of the nodes calls a model while it runs; their cards call these and store what comes back in the
// node's config:
//   AskUiAgent          - one request to the layout agent -> one step (with the check pass);
//   GenerateUiTextStyle - the Text style node's AI;
//   AnalyzeUiKit        - the meta of kit pictures (name, type, asset type, variant, description,
//                         background, nine-slice where the pixels show no stretch), the background
//                         removed by the image model on request (stored as an upload).
// Models are reached through AiRouter (local agents included); the coroutines take what the card has
// gathered (the node's config, its inputs' values) and can be started from the UI without blocking it
// (Start(JobThread::Main), as the extract's auto split does)
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // A stand-in for the text model: while set, every UI AI text call goes to it (prompt and pictures in, the answer out);
    // for tests. An empty function restores the router
    using UiAiTextStub = Function<AiTextResult(const String& model, const String& prompt, const Vector<AiImageRef>& images)>;
    void SetUiAiTextStub(const UiAiTextStub& stub);

    // Returns true while a text stand-in is set
    bool HasUiAiTextStub();

    // A text model call: the test stand-in when one is set, else AiRouter::GenerateText
    Coroutine<AiTextResult> UiAiText(Ref<PipelineExecContext> ctx, String model, String prompt, Vector<AiImageRef> images);

    // A picture for a vision call: at most max px on its long side (PNG bytes; smaller ones go as they are)
    String PictureForModel(const String& png, int max = 1024);

    // The density a PNG carries in its AssetsLine:dpi text chunk (pngMeta.ts); none when it has no such tag
    std::optional<double> PngDpiTag(const String& png);

    // --- the layout agent --------------------------------------------------------------------------------------

    // ------------------------------------------------------------------------------------------
    // An input of the layout node the agent reads: a picture (PNG bytes, with its DPI when the caller
    // knows it - else the PNG's tag) or a text
    // ------------------------------------------------------------------------------------------
    struct UiAgentInput
    {
        String                name;
        bool                  image = true;
        String                png;
        std::optional<double> dpi;
        String                text;
    };

    // --------------------------------------------------------------------------------------------
    // One request to the layout agent. The kit is the kit node's value as it is now (re-run an instant kit
    // node from its cache first, as the web does, so the agent never works from a stale kit)
    // --------------------------------------------------------------------------------------------
    struct UiAgentRequest
    {
        Json                  config = Json::Object(); // The AI UI layout node's config (steps, agentModel, agentReview...)
        std::optional<UiKit>  kit;                     // None: no kit connected (or not computed)
        Vector<UiAgentInput>  references;              // The node's image / text inputs, in input order
        String                prompt;
        String                selected;                // The block selected in the workspace, "" = none
        String                model;                   // "" = the node's agentModel, else gpt-6.1-sol
        Int64                 nowMs = 0;               // The step's time; 0 = now
    };

    // ----------------------------------------------------------------------------------
    // The agent's step (with the check pass in it), the edits that did not apply, the calls
    // ----------------------------------------------------------------------------------
    struct UiAgentResponse
    {
        bool           ok = false;
        String         error;
        UiStep         step;
        Vector<String> errors;
        int            calls = 0; // Model calls made: 1, or 2 with the check pass
    };

    // Asks the layout agent: the prompt with the current render, the kit sheet and the reference pictures; an empty layout
    // starts at the first reference picture's size and DPI (a screen op first in the step); when the answer leaves kit
    // graphics unused while blocks stay placeholders (or draw icons with characters) and agentReview is not false, one
    // check pass fixes the kit usage in the same step (step.review)
    Coroutine<UiAgentResponse> AskUiAgent(Ref<PipelineExecContext> ctx, UiAgentRequest request);

    // Stores the agent's step in the layout node's config as the web card does: steps + the step, the redo list and the
    // draft cleared
    void StoreAgentStep(DataValue& config, const UiStep& step);

    // A hand action into the layout node's config: it joins the run of hand edits (PushUserAction); the redo list cleared
    void CommitUserAction(DataValue& config, const Vector<UiOp>& ops, const String& label, Int64 nowMs, bool fold = true);

    // Undo / redo by action in the layout node's config (steps and uiRedo); false when there is nothing to do
    bool UndoInConfig(DataValue& config);
    bool RedoInConfig(DataValue& config);

    // --- the Text style node ---------------------------------------------------------------------------------------

    // ---------------------------------------------
    // A request to the Text style node's AI
    // ---------------------------------------------
    struct UiStyleRequest
    {
        Json           config = Json::Object(); // The Text style node's config (it IS the style; styleModel)
        Vector<String> references;               // PNG bytes of the pictures wired into it (the lettering to match)
        String         prompt;
        String         model;                    // "" = the node's styleModel, else gpt-6.1-sol
    };

    // ---------------------------
    // The style the model answered
    // ---------------------------
    struct UiStyleResponse
    {
        bool      ok = false;
        String    error;
        TextStyle style;
        String    model;
    };

    // Asks the text model for a style: the font catalogue, the current style and the request, the pictures as references;
    // the name is kept and what the answer leaves out stays
    Coroutine<UiStyleResponse> GenerateUiTextStyle(Ref<PipelineExecContext> ctx, UiStyleRequest request);

    // Writes the style into the Text style node's config as the web card does: every field but the name, a missing
    // description as null
    void StoreTextStyle(DataValue& config, const TextStyle& style);

    // --- the UI kit node ------------------------------------------------------------------------------------------------

    // -------------------------------------
    // One picture of the kit to analyse
    // -------------------------------------
    struct UiKitPictureIn
    {
        String portId;
        String name; // The input's name
        String png;
    };

    // ----------------------------------------
    // A request to the UI kit node's AI
    // ----------------------------------------
    struct UiKitAnalyzeRequest
    {
        Json                   config = Json::Object(); // The kit node's config (kitModel, kitImageModel)
        Vector<UiKitPictureIn> pictures;                // The image inputs asked about that have values, in input order
        bool                   removeBg = false;        // Remove complex backgrounds with the image model
        String                 model;                   // "" = kitModel, else gpt-6.1-sol
        String                 imageModel;              // "" = kitImageModel, else openai/gpt-5-image-mini
    };

    // ----------------------------------------------------------------------------------------
    // The answer: per input port the item fields as the web returns them ({name, type, assetType, variant,
    // description, bg, ai, background, prepared?, nineSlice?, nineFrom?}), and notes for the user
    // ----------------------------------------------------------------------------------------
    struct UiKitAnalyzeResponse
    {
        bool           ok = false;
        String         error;
        Json           items = Json::Object();
        Vector<String> notes;
        String         model;
    };

    // The prompt of the kit analysis for n pictures with their names and the sizes the model sees ("WxH")
    String KitAnalyzePrompt(int n, const Vector<String>& names, const Vector<String>& sizes);

    // The JSON array in a model's answer (fences dropped); empty when there is none
    Json ParseJsonArrayAnswer(const String& text);

    // Asks the vision model about the pictures (one call for all), keys flat backdrops locally, removes complex ones with the
    // image model when asked (the cut stored as an upload, prepared = {uploadId, from: InputHash}), and takes the model's
    // nine-slice only on the axes where the pixels show no stretchable run (scaled from the size it saw; nineFrom: "ai")
    Coroutine<UiKitAnalyzeResponse> AnalyzeUiKit(Ref<PipelineExecContext> ctx, UiKitAnalyzeRequest request);

    // Merges an analysis into the kit node's config items: borders set by hand stay, a background removed earlier stays
    // removed when this pass did not redo it (see the report for where this differs from the web card)
    void MergeKitAnalysis(DataValue& config, const Json& items);
    Json MergeKitAnalysis(const Json& config, const Json& items);
}
