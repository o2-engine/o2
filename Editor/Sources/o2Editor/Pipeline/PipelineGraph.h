#pragma once

#include "o2/Assets/Types/PipelineAsset.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // Data kind carried by a pipeline port
    enum class PipelinePortType { Text, Image, Video, Audio };

    // Returns lower-case name of the port type: "text", "image", "video" or "audio"
    String PipelinePortTypeToString(PipelinePortType type);

    // Parses port type from its lower-case name, unknown names give Text
    PipelinePortType PipelinePortTypeFromString(const String& str);

    // ------------------------------------------------
    // Input or output port of a node. Custom ports are
    // user-added inputs stored in node config
    // ------------------------------------------------
    struct PipelinePort : public ISerializable
    {
        String           id;                                // Unique port id, edges refer to it @SERIALIZABLE
        String           name;                              // Display name, run-time inputs are keyed by it @SERIALIZABLE
        PipelinePortType portType = PipelinePortType::Text; // Kind of data the port carries @SERIALIZABLE
        bool             custom = false;                    // True for user-added inputs stored in node config @SERIALIZABLE

    public:
        // Default constructor
        PipelinePort() = default;

        // Constructor with id, name, type and custom flag
        PipelinePort(const String& id, const String& name, PipelinePortType type, bool custom = false);

        // Returns true when id, name, type and custom flag are equal
        bool operator==(const PipelinePort& other) const;

        SERIALIZABLE(PipelinePort);
    };

    // ----------------------------------------------------------
    // Pipeline node: type, placement and a free-form JSON config
    // ----------------------------------------------------------
    class PipelineNode : public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        String id;       // Unique node id @SERIALIZABLE
        String nodeType; // Node type key in the registry @SERIALIZABLE
        Vec2F  position; // Position in graph space @SERIALIZABLE
        Vec2F  size;     // Widget size, zero means auto size @SERIALIZABLE

        Vector<PipelinePort> inputs;  // Input ports, fixed ones first then custom @SERIALIZABLE
        Vector<PipelinePort> outputs; // Output ports @SERIALIZABLE

        DataDocument config; // Node parameters; serialized by hand in OnSerialize and OnDeserialized

    public:
        // Default constructor
        PipelineNode();

        // Constructor with ref counter
        explicit PipelineNode(RefCounter* refCounter);

        // Copy-constructor
        PipelineNode(const PipelineNode& other);

        // Copy-constructor with ref counter
        PipelineNode(RefCounter* refCounter, const PipelineNode& other);

        // Assign operator, copies config and keeps it an object
        PipelineNode& operator=(const PipelineNode& other);

        // Returns a new random id for a node or port
        static String GenerateId();

        // Returns input port by id, null when missing
        const PipelinePort* FindInput(const String& portId) const;

        // Returns output port by id, null when missing
        const PipelinePort* FindOutput(const String& portId) const;

        // Returns first input port with the name, null when missing
        const PipelinePort* FindInputByName(const String& name) const;

        // Returns true when config has the key
        bool HasConfig(const String& key) const;

        // Returns config value as string, def when missing
        String GetConfigString(const String& key, const String& def = "") const;

        // Returns config value as number, def when missing
        float GetConfigNumber(const String& key, float def = 0.0f) const;

        // Returns config value as bool, accepts non-zero numbers and "true"/"1" strings, def when missing
        bool GetConfigBool(const String& key, bool def = false) const;

        // Returns raw config value, null when missing
        const DataValue* GetConfigValue(const String& key) const;

        // Sets string config value
        void SetConfigString(const String& key, const String& value);

        // Sets number config value
        void SetConfigNumber(const String& key, float value);

        // Sets bool config value
        void SetConfigBool(const String& key, bool value);

        // Removes config key when present
        void RemoveConfig(const String& key);

        // Returns custom inputs stored in config.customInputs: [{id, name, type}]
        Vector<PipelinePort> GetCustomInputs() const;

        // Stores custom inputs into config.customInputs
        void SetCustomInputs(const Vector<PipelinePort>& ports);

        // Rebuilds the input ports: fixed ports first (keeping ids of existing ones), custom inputs after them
        void RegenerateInputs(const Vector<PipelinePort>& fixedInputs);

        SERIALIZABLE(PipelineNode);
        CLONEABLE_REF(PipelineNode);

    protected:
        // Writes config into the node when it is not empty
        void OnSerialize(DataValue& node) const override;

        // Reads config from the node, makes it an empty object when missing
        void OnDeserialized(const DataValue& node) override;
    };

    // ---------------------------------------------
    // Link between an output port and an input port
    // ---------------------------------------------
    class PipelineEdge : public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        String id;         // Unique edge id @SERIALIZABLE
        String fromNodeId; // Source node id @SERIALIZABLE
        String fromPortId; // Source output port id @SERIALIZABLE
        String toNodeId;   // Target node id @SERIALIZABLE
        String toPortId;   // Target input port id @SERIALIZABLE

        Vector<Vec2F> points; // Bend points in world space, source to target @SERIALIZABLE

    public:
        // Default constructor
        PipelineEdge();

        // Constructor with ref counter
        explicit PipelineEdge(RefCounter* refCounter);

        // Copy-constructor
        PipelineEdge(const PipelineEdge& other);

        // Copy-constructor with ref counter
        PipelineEdge(RefCounter* refCounter, const PipelineEdge& other);

        SERIALIZABLE(PipelineEdge);
        CLONEABLE_REF(PipelineEdge);
    };

    // -----------------------------------------------
    // Whole pipeline: nodes, edges and the saved view
    // -----------------------------------------------
    class PipelineGraph : public ISerializable
    {
    public:
        String id; // Key of the results cache, kept across save-as and copies; empty until the editor adopts one @SERIALIZABLE

        Vector<Ref<PipelineNode>> nodes; // All nodes @SERIALIZABLE
        Vector<Ref<PipelineEdge>> edges; // All edges @SERIALIZABLE

        Vec2F cameraPosition;     // Saved editor view position @SERIALIZABLE
        float cameraScale = 1.0f; // Saved editor view scale @SERIALIZABLE

    public:
        // Default constructor
        PipelineGraph() = default;

        // Copy-constructor, deep copies nodes and edges
        PipelineGraph(const PipelineGraph& other);

        // Reads the graph from the "graph" member of the asset document; a missing member gives an empty graph
        void LoadFromAsset(const PipelineAsset& asset);

        // Writes the graph into the "graph" member of the asset document
        void SaveToAsset(PipelineAsset& asset) const;

        // Assign operator, deep copies nodes and edges
        PipelineGraph& operator=(const PipelineGraph& other);

        // Returns node by id, null when missing
        Ref<PipelineNode> FindNode(const String& id) const;

        // Returns edge by id, null when missing
        Ref<PipelineEdge> FindEdge(const String& id) const;

        // Returns edges ending at the node
        Vector<Ref<PipelineEdge>> GetIncomingEdges(const String& nodeId) const;

        // Returns edges starting at the node
        Vector<Ref<PipelineEdge>> GetOutgoingEdges(const String& nodeId) const;

        // Returns the nodes a whole-graph run targets: every node nothing else consumes, finish nodes
        // first. Their upstream branches together cover every node that produces something
        Vector<String> GetRunTargets() const;

        // Returns the edge ending at the input port of the node, null when unconnected
        Ref<PipelineEdge> FindEdgeToPort(const String& nodeId, const String& portId) const;

        // Removes node and every edge attached to it
        void RemoveNode(const String& id);

        // Removes edge by id
        void RemoveEdge(const String& id);

        // Drops edges whose endpoints no longer exist
        void RemoveDanglingEdges();

        // Returns validation errors: missing ports, type mismatches, duplicate targets, cycles
        Vector<String> Validate() const;

        // Returns true when adding an edge from -> to would make a cycle
        bool WouldMakeCycle(const String& fromNodeId, const String& toNodeId) const;

        // Returns effective seed of every seeded node, following "inherit seed" over the image input
        Map<String, int> ResolveSeeds() const;

        // Returns content signature of every node: type + config + upstream signatures
        Map<String, String> ComputeSignatures() const;

        // Returns the upstream signatures of a node keyed the way ComputeNodeSignature expects them
        Map<String, String> UpstreamSignatures(const PipelineNode& node, const Map<String, String>& nodeSignatures) const;

        // Returns content signature of one node with given upstream signatures
        static String ComputeNodeSignature(const PipelineNode& node, const Map<String, String>& upstreamByPortKey,
                                           const Vector<String>& extraExcludeKeys, int seed, const String& variant = "");

        // Returns key of an upstream signature for the port: name, or name#id when the name is duplicated
        static String UpstreamSigKey(const PipelineNode& node, const PipelinePort& port);

        // Returns config keys that never take part in signatures
        static const Vector<String>& GetUiOnlyConfigKeys();

        // Returns true for node types whose generation depends on a seed
        static bool IsSeededType(const String& type);

        SERIALIZABLE(PipelineGraph);
    };
}
// --- META ---

PRE_ENUM_META(Editor::PipelinePortType);

CLASS_BASES_META(Editor::PipelinePort)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::PipelinePort)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(id);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(name);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(PipelinePortType::Text).NAME(portType);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(custom);
}
END_META;
CLASS_METHODS_META(Editor::PipelinePort)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&, const String&, PipelinePortType, bool);
}
END_META;

CLASS_BASES_META(Editor::PipelineNode)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineNode)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(id);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(nodeType);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(position);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(size);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(inputs);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(outputs);
    FIELD().PUBLIC().NAME(config);
}
END_META;
CLASS_METHODS_META(Editor::PipelineNode)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineNode&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const PipelineNode&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GenerateId);
    FUNCTION().PUBLIC().SIGNATURE(const PipelinePort*, FindInput, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const PipelinePort*, FindOutput, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const PipelinePort*, FindInputByName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, HasConfig, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetConfigString, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetConfigNumber, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetConfigBool, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(const DataValue*, GetConfigValue, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetConfigString, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetConfigNumber, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetConfigBool, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveConfig, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<PipelinePort>, GetCustomInputs);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCustomInputs, const Vector<PipelinePort>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RegenerateInputs, const Vector<PipelinePort>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(Editor::PipelineEdge)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineEdge)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(id);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(fromNodeId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(fromPortId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(toNodeId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(toPortId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(points);
}
END_META;
CLASS_METHODS_META(Editor::PipelineEdge)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineEdge&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const PipelineEdge&);
}
END_META;

CLASS_BASES_META(Editor::PipelineGraph)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineGraph)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(id);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(nodes);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(edges);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(cameraPosition);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(cameraScale);
}
END_META;
CLASS_METHODS_META(Editor::PipelineGraph)
{

    typedef Map<String, int> _tmp1;
    typedef Map<String, String> _tmp2;
    typedef Map<String, String> _tmp3;
    typedef const Map<String, String>& _tmp4;
    typedef const Map<String, String>& _tmp5;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const PipelineGraph&);
    FUNCTION().PUBLIC().SIGNATURE(void, LoadFromAsset, const PipelineAsset&);
    FUNCTION().PUBLIC().SIGNATURE(void, SaveToAsset, PipelineAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineNode>, FindNode, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineEdge>, FindEdge, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<PipelineEdge>>, GetIncomingEdges, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<PipelineEdge>>, GetOutgoingEdges, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<String>, GetRunTargets);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineEdge>, FindEdgeToPort, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveNode, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveEdge, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveDanglingEdges);
    FUNCTION().PUBLIC().SIGNATURE(Vector<String>, Validate);
    FUNCTION().PUBLIC().SIGNATURE(bool, WouldMakeCycle, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, ResolveSeeds);
    FUNCTION().PUBLIC().SIGNATURE(_tmp2, ComputeSignatures);
    FUNCTION().PUBLIC().SIGNATURE(_tmp3, UpstreamSignatures, const PipelineNode&, _tmp4);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, ComputeNodeSignature, const PipelineNode&, _tmp5, const Vector<String>&, int, const String&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, UpstreamSigKey, const PipelineNode&, const PipelinePort&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Vector<String>&, GetUiOnlyConfigKeys);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsSeededType, const String&);
}
END_META;
// --- END META ---
