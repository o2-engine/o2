#pragma once

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

using namespace o2;

namespace Editor
{
    // --------------------------------
    // Progress event of a pipeline run
    // --------------------------------
    struct PipelineExecEvent
    {
        // Event kinds; the field comments below say which kind fills them
        enum class Type { NodeState, NodeOutput, Retry, Log, Done, Fatal };

        Type   type = Type::Log; // Kind of event
        String nodeId;           // Node the event is about, empty for Log, Done and Fatal
        String state;            // NodeState: running / done / error
        String error;            // NodeState error, Fatal reason
        String message;          // Log line, Retry reason

        PipelineValue value;          // NodeOutput: the produced value
        String        portId;         // NodeOutput: output port of a per-port node, empty for the node result
        String        previewPath;    // NodeOutput: where the preview file was written
        String        srcPreviewPath; // NodeOutput: uncropped source preview when a crop was applied

        int attempt = 0, maxAttempts = 0, status = 0; // Retry: attempt number, attempts limit and HTTP status of the failure
    };

    // -------------------------------------------------------------------------
    // Pull-based pipeline executor. Evaluates the branch feeding a target node,
    // reusing the per-pipeline content cache keyed by node signatures. Runs as
    // a main-thread coroutine so provider calls never block the editor
    // -------------------------------------------------------------------------
    class PipelineExecutor : public RefCounterable
    {
    public:
        Function<void(const PipelineExecEvent&)> onEvent; // Called with every progress event of a run

        String assetsPathOverride; // Finish nodes write here instead of the project assets folder when set

    public:
        // Default constructor
        PipelineExecutor();

        // Constructor with reference counter
        explicit PipelineExecutor(RefCounter* refCounter);

        // Starts a run of the branch ending at targetNodeId. bypassNodeIds are recomputed even when cached
        void Execute(const String& pipelineId, const PipelineGraph& graph, const String& targetNodeId,
                     const Vector<String>& bypassNodeIds = {}, bool cachedOnly = false);

        // Runs one node from the stored previews of its inputs
        void ExecuteSingle(const String& pipelineId, const PipelineGraph& graph, const String& nodeId);

        // Requests the current run to stop; it finishes with "Cancelled" once the active node returns
        void Cancel();

        // Returns true while a run coroutine is alive
        bool IsRunning() const;

        // Returns the cache folder of a pipeline: <work path>/cache/<pipeline id>/
        static String GetCachePath(const String& pipelineId);

        // Returns the path of the preview file of a node with the given extension
        static String GetPreviewPath(const String& pipelineId, const String& nodeId, const String& ext);

        // Returns path of the preview file of one output of a per-port node
        static String GetPortPreviewPath(const String& pipelineId, const String& nodeId, const String& portId, const String& ext);

        // Returns the path of the uncropped source preview of a node
        static String GetSourcePreviewPath(const String& pipelineId, const String& nodeId);

        // Returns the cache signature of one output of a per-port node: the shared config without the keys
        // the implementation excludes, plus what makes this port its own
        static String PortSignature(const PipelineNode& node, const Map<String, String>& upstreamSigs, int seed, const String& portId);

        // Returns the path of the content cached under a node signature
        static String GetContentPath(const String& pipelineId, const String& sig, const String& ext);

        // Returns the path of the marker recording a successful run of a node signature
        static String GetRanMarkerPath(const String& pipelineId, const String& sig);

        // Returns the stored preview of a node (any known extension), invalid when none
        static PipelineValue LoadPreview(const String& pipelineId, const PipelineNode& node, String* pathOut = nullptr);

        // Loads the cached preview of one output of a per-port node, invalid when it was never produced
        static PipelineValue LoadPortPreview(const String& pipelineId, const String& nodeId, const String& portId,
                                             PipelinePortType type = PipelinePortType::Image, String* pathOut = nullptr);

        // Returns true when the node signature was produced by a previous run
        static bool RanExists(const String& pipelineId, const String& sig);

        // Records that the node signature has been produced
        static void MarkRan(const String& pipelineId, const String& sig);

        // Drops the preview and freshness of a node
        static void ClearNodeCache(const String& pipelineId, const PipelineNode& node, const String& sig);

        // Returns the set of node ids whose current data matches a previous successful run
        static Vector<String> ComputeFreshNodes(const String& pipelineId, const PipelineGraph& graph);

        // Returns the audio file extensions the cache recognizes
        static const Vector<String>& GetAudioExtensions();

        // Stores the value in the content cache under the signature, so later runs are served from it
        static void SaveContent(const String& pipelineId, const String& sig, const PipelineValue& value);

    private:
        // ----------------------------------------------------------------------
        // State of one run: graph snapshot, flags and the values produced so far
        // ----------------------------------------------------------------------
        struct Run : public RefCounterable
        {
            String         pipelineId;            // Pipeline the run belongs to, selects the cache folder
            PipelineGraph  graph;                 // Snapshot of the graph being executed
            String         targetNodeId;          // Node whose branch is evaluated
            Vector<String> bypass;                // Nodes recomputed even when their content is cached
            bool           cachedOnly = false;    // Only instant nodes may run, a missing cache ends the run with "Not cached"
            bool           cancelled = false;     // Set by Cancel(), checked after every node
            bool           notCached = false;     // A node needed a provider call in cachedOnly mode
            bool           assetsChanged = false; // A finish node wrote into the assets folder, rebuilt when the run ends

            Map<String, Map<String, PipelineValue>> outputs;     // Node id -> port id -> value
            Map<String, Vector<String>>             neededPorts; // Outputs of a per-port node this run consumes; the target computes all of its own
            Map<String, String>                     sigByNode; // Node signatures computed during this run
            Map<String, String>                     sigByPort; // Part signatures of per-port nodes, keyed "<node id>#<port id>"
            Map<String, int>                        seeds;     // Resolved seeds by node id
            Map<String, bool>                       visiting;  // Nodes on the evaluation stack, for cycle detection
        };

        Ref<Run>        mCurrent;   // Run in progress, null when idle
        Coroutine<void> mCoroutine; // Coroutine driving mCurrent on the main thread

    private:
        // Fills run->neededPorts: the outputs each node must produce for the target branch
        void CollectNeededPorts(const Ref<Run>& run);

        // Coroutine body of Execute: validates the graph, evaluates the target branch and finishes the run
        Coroutine<void> ExecuteCoroutine(Ref<Run> run);

        // Coroutine body of ExecuteSingle: runs one node on the stored previews of its inputs
        Coroutine<void> ExecuteSingleCoroutine(Ref<Run> run);

        // Evaluates the inputs of a node and then the node, serving cached content when its signature matches
        Coroutine<bool> Evaluate(Ref<Run> run, String nodeId);

        // Builds the context handed to a node: settings, paths, seed and the callbacks into this executor
        Ref<PipelineExecContext> MakeContext(const Ref<Run>& run, const String& nodeId);

        // Passes the event to onEvent when it is set
        void Emit(const PipelineExecEvent& event);

        // Emits a NodeState event: running / done / error
        void EmitState(const String& nodeId, const String& state, const String& error = "");

        // Emits a Log event
        void EmitLog(const String& message);

        // Stores the preview of one output of a per-port node and emits NodeOutput for it
        void WritePortPreview(const Ref<Run>& run, const String& nodeId, const String& portId, const PipelineValue& value);

        // Stores the preview of a node and emits NodeOutput
        void WritePreview(const Ref<Run>& run, const String& nodeId, const PipelineValue& value, const PipelineValue* srcValue);

        // Returns the content cached under a signature, invalid when none
        static PipelineValue LoadContent(const String& pipelineId, const String& sig, PipelinePortType type);

        // Writes the value into the content cache under a signature

        // Deletes every content file stored under a signature
        static void DeleteContent(const String& pipelineId, const String& sig);

        // Loads a value from pathWithoutExt trying the extensions of the port type, invalid when no file exists
        static PipelineValue LoadValueFile(const String& pathWithoutExt, PipelinePortType type, String* pathOut = nullptr);

        // Rebuilds assets when they changed, clears the current run and emits Done or Fatal
        void FinishRun(const Ref<Run>& run, const String& fatal);
    };
}
// --- META ---

PRE_ENUM_META(Editor::PipelineExecEvent::Type);
// --- END META ---
