#pragma once

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/PipelineValue.h"

using namespace o2;

namespace Editor
{
    // Node kind grouping in the add menu
    enum class PipelineNodeCategory { Source, Transform, AI, Output, Flow };

    // Returns lower-case name of the category: "source", "transform", "ai", "output" or "flow"
    String PipelineNodeCategoryToString(PipelineNodeCategory category);

    // ------------------------------------------------
    // Static description of a node type for the editor
    // ------------------------------------------------
    struct PipelineNodeSchema
    {
        String               type;                                       // Registry key of the node type
        String               label;                                      // Human readable name shown in the node header and add menu
        PipelineNodeCategory category = PipelineNodeCategory::Transform; // Group in the add menu
        String               description;                                // Short help text for the node

        Vector<PipelinePort> inputs;  // Fixed input ports, ids are generated per node
        Vector<PipelinePort> outputs; // Output ports, ids are generated per node

        bool hasPlay = false;    // Terminal node: has the play button that pulls the whole branch
        bool instant = false;    // Local and free: re-applied automatically on every parameter change
        bool perPortRun = false; // Computes its outputs one at a time: each is run, cached and previewed on its own
        bool dynamicOutputs = false; // Decides its outputs while it runs: one run returns all of them, cached together, ports follow
        bool partialInputs = false;  // Runs with the inputs that have a result: one not produced yet is left out, not waited for
        bool retired = false;        // No longer offered in any add menu; nodes already in pipelines still load and run

        Vector<PipelinePortType> addableInputs; // Input kinds the user may add with "+"

        Vec2F defaultSize; // Preferred initial size, zero for auto

    public:
        // Returns true when a fixed input or an addable input has the type
        bool AcceptsInputType(PipelinePortType type) const;

        // Returns true when an output has the type
        bool ProducesOutputType(PipelinePortType type) const;
    };

    // --------------------------
    // Result of running one node
    // --------------------------
    struct PipelineRunResult
    {
        bool                       ok = true; // False when the node failed
        String                     error;     // Failure message when not ok
        Map<String, PipelineValue> outputs;   // Produced values by output port NAME
        Vector<String>             order;     // Output names in the order the node made them, for a dynamicOutputs node

    public:
        // Returns a failed result with the error message
        static PipelineRunResult Fail(const String& error);

        // Returns a successful result with one value on the port
        static PipelineRunResult Single(const PipelineValue& value, const String& portName = "out");
    };

    // ------------------------------------------------------------------------------------
    // Result of a node's paid second step over a stored generation (IPipelineNodeImpl::Refine)
    // ------------------------------------------------------------------------------------
    struct PipelineRefineResult
    {
        PipelineValue value;       // What takes the generation's place
        bool          keep = true; // False leaves it out of the cache: a model call failed, the next run tries again
    };

    // ---------------------------------------------------------
    // What a node sees while it runs: settings, logging, inputs
    // ---------------------------------------------------------
    class PipelineExecContext : public RefCounterable
    {
    public:
        PipelineSettings settings; // Provider keys for this run

        Function<void(const String&)>                log;         // Log line sink
        Function<void(int, int, int, const String&)> signalRetry; // Provider retry report: attempt, max, status, reason
        Function<bool()>                             isCancelled; // Cancellation check

        int  seed = -1;          // Effective seed, -1 when none
        bool cachedOnly = false; // Never call a provider, cached results only

        Map<String, PipelineValue> inputsById; // Inputs keyed by input port id

        String pipelineId; // Owner pipeline uid, for caches
        String assetsPath; // Project assets folder, where finish nodes write

        String outputPortId; // Output being computed, per-port nodes only
        String outputPort;   // Name of that output

        bool assetsChanged = false; // Set by finish nodes when the assets folder got new files

        String nodeId;        // Node being run
        String workDir;       // A folder of the node's own (its debug trace), dynamicOutputs nodes only
        bool   fresh = false; // The node is regenerated (↻): memoised model answers are skipped

        Function<bool(const String& key, const String& ext, String& data)>       memoGet; // Memoised model answers of the pipeline
        Function<void(const String& key, const String& ext, const String& data)> memoPut; // Stores a model answer

        Function<void(const String&)> phase; // Tells the editor what the node (or the part being run) does beyond its generation

    public:
        // Passes the message to the log callback when it is set
        void Log(const String& message) const;

        // Passes what the node does now (check, redo) to the phase callback when it is set
        void Phase(const String& name) const;

        // Reads a memoised model answer; false when there is none or no store
        bool MemoGet(const String& key, const String& ext, String& data) const;

        // Stores a model answer when there is a store
        void MemoPut(const String& key, const String& ext, const String& data) const;

        // Returns true when the cancellation callback is set and reports cancel
        bool IsCancelled() const;

        // Passes a provider retry to the callback when it is set
        void SignalRetry(int attempt, int max, int status, const String& reason) const;
    };

    // --------------------------------------------------------------------
    // Node implementation: schema plus the coroutine that produces outputs
    // --------------------------------------------------------------------
    class IPipelineNodeImpl : public RefCounterable
    {
    public:
        // Virtual destructor
        virtual ~IPipelineNodeImpl() = default;

        // Returns static description of the node type
        virtual const PipelineNodeSchema& GetSchema() const = 0;

        // Runs the node as a coroutine. Inputs are keyed by input port NAME
        virtual Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx,
                                                 const Map<String, PipelineValue>& inputs,
                                                 const Ref<PipelineNode>& node) = 0;

        // Fills the config a fresh node of this type starts with
        virtual void InitNode(const Ref<PipelineNode>& node) const {}

        // Config keys the cache of a per-port node ignores, because its port variant carries them
        virtual Vector<String> PortCacheExcludedKeys() const { return {}; }

        // Config keys the cache of one output of a per-port node ignores: PortCacheExcludedKeys and what that output's
        // variant carries for itself (an extract part's own model)
        virtual Vector<String> PortCacheExcludedKeysOf(const PipelineNode& node, const String& portId) const { return PortCacheExcludedKeys(); }

        // Returns what makes one output of a per-port node its own, folded into its cache signature
        virtual String PortCacheVariant(const PipelineNode& node, const String& portId) const { return portId; }

        // Rebuilds the outputs a per-port node takes from its own config; the schema outputs are left alone when it does
        virtual bool SyncPorts(const Ref<PipelineNode>& node) const { return false; }

        // Config keys that do not change what the node produces, besides the UI-only ones every node has
        virtual Vector<String> SignatureExcludedKeys() const { return {}; }

        // Config keys the stored generation of a single-output node leaves out: a local step reads them (PostStep), or a
        // second step keyed on its own does
        virtual Vector<String> ContentExcludedKeys(const PipelineNode& node) const { return {}; }

        // A cheap local step over the stored generation, applied after every run - a cache hit included - like the crop
        virtual PipelineValue PostStep(const PipelineNode& node, const PipelineValue& value) const { return value; }

        // The key of a paid second step over a port's stored generation (AI extract and AI text style: the check against the
        // reference), cached under a key of its own beside the generation's; empty while the step is off. portId is empty for a
        // single-output node
        virtual String RefineKey(const PipelineNode& node, const String& portId) const { return ""; }

        // Runs the second step over the stored generation (ctx carries the part, the work folder, the memo and the phase
        // callback); key is the step's cache key. Returns what takes the generation's place
        virtual Coroutine<PipelineRefineResult> Refine(Ref<PipelineExecContext> ctx, Map<String, PipelineValue> inputs,
                                                       Ref<PipelineNode> node, PipelineValue stored, String key);

        // The stored result of the second step's key was read back instead of computed (the step's debug data follows it)
        virtual void RefineReplayed(const PipelineExecContext& ctx, const PipelineNode& node, const String& key) const {}
    };

    // -------------------------------------------
    // Registry of all node types, built on demand
    // -------------------------------------------
    class PipelineNodeRegistry
    {
    public:
        // Adds implementation, replacing a registered one with the same type
        static void Register(const Ref<IPipelineNodeImpl>& impl);

        // Returns implementation by type, null when unknown
        static Ref<IPipelineNodeImpl> Get(const String& type);

        // Returns schema by type, null when unknown
        static const PipelineNodeSchema* GetSchema(const String& type);

        // Returns all registered implementations
        static const Vector<Ref<IPipelineNodeImpl>>& All();

        // Returns schemas of all registered implementations
        static Vector<const PipelineNodeSchema*> AllSchemas();

        // Creates a node instance of the type with fresh port ids and default config, null when unknown
        static Ref<PipelineNode> CreateNode(const String& type, const Vec2F& position);

        // Adds the outputs a schema declares but the node lacks, refreshes fixed inputs
        static void SyncNodeWithSchema(const Ref<PipelineNode>& node);

        // Returns true for finish node types, which write results into the assets folder
        static bool IsFinishType(const String& type);

        // Returns true for an input only the node's AI reads from its card (every input of a Text style, every input of an
        // AI UI layout but its kit): a run neither computes it nor folds it into the node's signature
        static bool IsReferenceInput(const PipelineNode& node, const String& portId);

        // Returns the upstream signature a consumer folds in for one incoming edge: a UI kit below an AI extract part keys on
        // the part-density rule's version too (how a part's density is worked out is a post-step); one of an AI UI layout's
        // own graphics is its stored picture (the upload and its density), whatever happens to the layout around it
        static String UpstreamSigFor(const String& sig, const PipelineNode* from, const String& fromPortId, const PipelineNode& to);

        // Bumped when the way an AI extract part's density is worked out changes
        static constexpr int PART_DENSITY_VERSION = 3;

        // Returns where a finish node writes inside the assets folder, without the extension: its
        // assetPath, or Generated/<the file name the web editor downloads it as>, or Generated/output
        static String GetFinishAssetPath(const PipelineNode& node);

    private:
        // Returns the implementations storage
        static Vector<Ref<IPipelineNodeImpl>>& Impls();

        // Registers built-in node types on first call
        static void EnsureBuiltins();
    };

    // Registers every built-in node type (called once by the registry)
    void RegisterBuiltinPipelineNodes();
}
// --- META ---

PRE_ENUM_META(Editor::PipelineNodeCategory);
// --- END META ---
