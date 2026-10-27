#pragma once

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

namespace Editor::UiLayout
{
    // Store of memoised model answers, scoped to the pipeline's content cache
    struct AiMemo
    {
        Function<bool(const String& key, const String& ext, String& data)>       get; // Returns false when nothing is stored
        Function<void(const String& key, const String& ext, const String& data)> put;
    };

    // ------------------------------------------------------------------------------------------------
    // The node's model calls. Every call is memoised by its request (model, prompt, images): a re-run after
    // a local change replays the answers it already paid for; a forced regeneration (↻) asks again
    // ------------------------------------------------------------------------------------------------
    class AiCtx: public RefCounterable
    {
    public:
        Ref<PipelineExecContext> exec; // Settings, log, retries and cancellation of the run

        AiMemo memo;          // Memoised answers, none when empty
        bool   fresh = false; // Skip the memoised answers and store fresh ones

        String textModel;  // Vision model of the analysis and the review
        String imageModel; // Image model of the sprite mattes and the background plate
        int    seed = -1;  // Effective seed, -1 when none

        Function<void(const String& tag, const String& ext, const String& data)> record; // Gets every answer, for fixtures

        Atomic<int> textCalls{ 0 }; // Text requests sent
        Atomic<int> imageCalls{ 0 }; // Image requests sent
        Atomic<int> replayed{ 0 };   // Answers taken from the memo or the replay hook

    public:
        // Passes the message to the run log; the parallel cut jobs log one at a time
        void Log(const String& message) const;

        // Passes an answer to record, one at a time
        void Record(const String& tag, const String& ext, const String& data) const;

    private:
        mutable Mutex logMutex;
    };

    // A copy of the context for one of several jobs running at once: the run's log, cancel and retry callbacks lock weak
    // references to the executor and the run, whose counts are not atomic, so the copies call them one at a time under the
    // mutex. Make and drop the copies where the run itself goes on
    Ref<AiCtx> ParallelAiCtx(const Ref<AiCtx>& ai, Mutex* mutex);

    // Answer of a model call
    struct AiAnswer
    {
        bool   ok = false;
        String error;
        String data; // Text, or PNG bytes
    };

    // Memo key of a text call: "uil-t-" and the first 40 hex digits of sha256 over "text", the model, the prompt and the
    // images, each followed by a NUL byte
    String TextMemoKey(const String& model, const String& prompt, const Vector<String>& images);

    // Memo key of an image call: "uil-i-" and sha256 over "image", the model, the prompt, the frame, the seed and the
    // references, each followed by a NUL byte
    String ImageMemoKey(const String& model, const String& prompt, const String& aspectRatio, int seed, const Vector<String>& refs);

    // Returns the mime type of image bytes: image/jpeg for FF D8, else image/png
    String ImageMimeOf(const String& bytes);

    // Asks the vision model; images are PNG or JPEG bytes
    Coroutine<AiAnswer> AiText(const Ref<AiCtx>& ai, String prompt, Vector<String> images, String tag);

    // Asks the image model for one render; refs are PNG bytes, seed -2 takes the context's
    Coroutine<AiAnswer> AiImage(const Ref<AiCtx>& ai, String prompt, Vector<String> refs, String tag, String aspectRatio, int seed = -2);

    // A model call as the test stand-in sees it
    struct AiReplayCall
    {
        String         tag;    // analysis, cut <look>, background, background-strict, agent turn-<n>
        String         ext;    // .txt for the vision model, .png for the image model
        String         prompt;
        Vector<String> images; // Image bytes sent with it
    };

    // Test stand-in: while set, every call is answered by it; a call it has no answer for fails
    using AiReplay = Function<bool(const AiReplayCall& call, String& data)>;
    void SetAiReplay(const AiReplay& replay);
}
