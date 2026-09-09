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

        bool hasPlay = false; // Terminal node: has the play button that pulls the whole branch
        bool instant = false; // Local and free: re-applied automatically on every parameter change

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

    public:
        // Returns a failed result with the error message
        static PipelineRunResult Fail(const String& error);

        // Returns a successful result with one value on the port
        static PipelineRunResult Single(const PipelineValue& value, const String& portName = "out");
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

        bool assetsChanged = false; // Set by finish nodes when the assets folder got new files

    public:
        // Passes the message to the log callback when it is set
        void Log(const String& message) const;

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
