#include "o2Editor/stdafx.h"
#include "PipelineGraph.h"

#include "o2/Utils/Types/UID.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    String PipelinePortTypeToString(PipelinePortType type)
    {
        switch (type)
        {
            case PipelinePortType::Image: return "image";
            case PipelinePortType::Video: return "video";
            case PipelinePortType::Audio: return "audio";
            default: return "text";
        }
    }

    PipelinePortType PipelinePortTypeFromString(const String& str)
    {
        if (str == "image") return PipelinePortType::Image;
        if (str == "video") return PipelinePortType::Video;
        if (str == "audio") return PipelinePortType::Audio;
        return PipelinePortType::Text;
    }

    PipelinePort::PipelinePort(const String& id, const String& name, PipelinePortType type, bool custom /*= false*/):
        id(id), name(name), portType(type), custom(custom)
    {}

    bool PipelinePort::operator==(const PipelinePort& other) const
    {
        return id == other.id && name == other.name && portType == other.portType && custom == other.custom;
    }

    PipelineNode::PipelineNode()
    {
        config.SetObject();
    }

    PipelineNode::PipelineNode(RefCounter* refCounter):
        RefCounterable(refCounter)
    {
        config.SetObject();
    }

    PipelineNode::PipelineNode(const PipelineNode& other):
        PipelineNode(nullptr, other)
    {}

    PipelineNode::PipelineNode(RefCounter* refCounter, const PipelineNode& other):
        RefCounterable(refCounter), id(other.id), nodeType(other.nodeType), position(other.position), size(other.size),
        inputs(other.inputs), outputs(other.outputs)
    {
        config = static_cast<const DataValue&>(other.config);
        if (!config.IsObject())
            config.SetObject();
    }

    PipelineNode& PipelineNode::operator=(const PipelineNode& other)
    {
        id = other.id;
        nodeType = other.nodeType;
        position = other.position;
        size = other.size;
        inputs = other.inputs;
        outputs = other.outputs;
        config = static_cast<const DataValue&>(other.config);
        if (!config.IsObject())
            config.SetObject();

        return *this;
    }

    String PipelineNode::GenerateId()
    {
        UID uid;
        uid.Randomize();
        return (String)uid;
    }

    const PipelinePort* PipelineNode::FindInput(const String& portId) const
    {
        return inputs.Find([&](const PipelinePort& p) { return p.id == portId; });
    }

    const PipelinePort* PipelineNode::FindOutput(const String& portId) const
    {
        return outputs.Find([&](const PipelinePort& p) { return p.id == portId; });
    }

    const PipelinePort* PipelineNode::FindInputByName(const String& name) const
    {
        return inputs.Find([&](const PipelinePort& p) { return p.name == name; });
    }

    bool PipelineNode::HasConfig(const String& key) const
    {
        return config.IsObject() && config.FindMember(key.Data()) != nullptr;
    }

    const DataValue* PipelineNode::GetConfigValue(const String& key) const
    {
        if (!config.IsObject())
            return nullptr;

        return config.FindMember(key.Data());
    }

    String PipelineNode::GetConfigString(const String& key, const String& def /*= ""*/) const
    {
        auto value = GetConfigValue(key);
        if (!value)
            return def;

        return PipelineUtils::ValueToString(*value, def);
    }

    float PipelineNode::GetConfigNumber(const String& key, float def /*= 0.0f*/) const
    {
        auto value = GetConfigValue(key);
        if (!value)
            return def;

        return PipelineUtils::ValueToNumber(*value, def);
    }

    bool PipelineNode::GetConfigBool(const String& key, bool def /*= false*/) const
    {
        auto value = GetConfigValue(key);
        if (!value)
            return def;

        if (value->IsBoolean())
            return (bool)*value;

        if (value->IsNumber())
            return PipelineUtils::ValueToNumber(*value, 0.0f) != 0.0f;

        if (value->IsString())
        {
            String s = value->GetString();
            return s == "true" || s == "1";
        }

        return def;
    }

    void PipelineNode::SetConfigString(const String& key, const String& value)
    {
        if (!config.IsObject())
            config.SetObject();

        config[key.Data()] = value;
    }

    void PipelineNode::SetConfigNumber(const String& key, float value)
    {
        if (!config.IsObject())
            config.SetObject();

        config[key.Data()] = value;
    }

    void PipelineNode::SetConfigBool(const String& key, bool value)
    {
        if (!config.IsObject())
            config.SetObject();

        config[key.Data()] = value;
    }

    void PipelineNode::RemoveConfig(const String& key)
    {
        if (config.IsObject() && config.FindMember(key.Data()))
            config.RemoveMember(key.Data());
    }

    Vector<PipelinePort> PipelineNode::GetCustomInputs() const
    {
        Vector<PipelinePort> res;
        auto arr = GetConfigValue("customInputs");
        if (!arr || !arr->IsArray())
            return res;

        for (auto& item : *arr)
        {
            if (!item.IsObject())
                continue;

            PipelinePort port;
            port.custom = true;
            if (auto id = item.FindMember("id")) port.id = PipelineUtils::ValueToString(*id);
            if (auto name = item.FindMember("name")) port.name = PipelineUtils::ValueToString(*name);
            if (auto type = item.FindMember("type")) port.portType = PipelinePortTypeFromString(PipelineUtils::ValueToString(*type));
            if (port.id.IsEmpty())
                continue;

            res.Add(port);
        }

        return res;
    }

    void PipelineNode::SetCustomInputs(const Vector<PipelinePort>& ports)
    {
        if (!config.IsObject())
            config.SetObject();

        RemoveConfig("customInputs");
        auto& arr = config["customInputs"];
        arr.SetArray();
        for (auto& port : ports)
        {
            auto& item = arr.AddElement();
            item.SetObject();
            item["id"] = port.id;
            item["name"] = port.name;
            item["type"] = PipelinePortTypeToString(port.portType);
        }
    }

    void PipelineNode::RegenerateInputs(const Vector<PipelinePort>& fixedInputs)
    {
        Vector<PipelinePort> result;

        for (auto& fixed : fixedInputs)
        {
            auto existing = inputs.Find([&](const PipelinePort& p) { return !p.custom && p.name == fixed.name; });
            PipelinePort port = fixed;
            if (existing)
                port.id = existing->id;
            else if (port.id.IsEmpty())
                port.id = GenerateId();

            port.custom = false;
            result.Add(port);
        }

        for (auto& custom : GetCustomInputs())
            result.Add(custom);

        inputs = result;
    }

    void PipelineNode::OnSerialize(DataValue& node) const
    {
        if (config.IsObject() && config.GetMembersCount() > 0)
            node.AddMember("config") = static_cast<const DataValue&>(config);
    }

    void PipelineNode::OnDeserialized(const DataValue& node)
    {
        config.Clear();
        if (auto cfg = node.FindMember("config"))
            config = *cfg;

        if (!config.IsObject())
            config.SetObject();
    }

    PipelineEdge::PipelineEdge()
    {}

    PipelineEdge::PipelineEdge(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    PipelineEdge::PipelineEdge(const PipelineEdge& other):
        PipelineEdge(nullptr, other)
    {}

    PipelineEdge::PipelineEdge(RefCounter* refCounter, const PipelineEdge& other):
        RefCounterable(refCounter), id(other.id), fromNodeId(other.fromNodeId), fromPortId(other.fromPortId),
        toNodeId(other.toNodeId), toPortId(other.toPortId), points(other.points)
    {}

    PipelineGraph::PipelineGraph(const PipelineGraph& other)
    {
        *this = other;
    }

    void PipelineGraph::LoadFromAsset(const PipelineAsset& asset)
    {
        id = "";
        nodes.Clear();
        edges.Clear();
        cameraPosition = Vec2F();
        cameraScale = 1.0f;

        if (auto graph = asset.document.FindMember("graph"))
            Deserialize(*graph);
    }

    void PipelineGraph::SaveToAsset(PipelineAsset& asset) const
    {
        // Serializing into an existing member keeps its old arrays, so the member is rebuilt from scratch
        asset.document.RemoveMember("graph");
        asset.document["graph"] = *this;
    }

    PipelineGraph& PipelineGraph::operator=(const PipelineGraph& other)
    {
        nodes.Clear();
        edges.Clear();

        for (auto& node : other.nodes)
            nodes.Add(mmake<PipelineNode>(*node));

        for (auto& edge : other.edges)
            edges.Add(mmake<PipelineEdge>(*edge));

        id = other.id;
        cameraPosition = other.cameraPosition;
        cameraScale = other.cameraScale;

        return *this;
    }

    Ref<PipelineNode> PipelineGraph::FindNode(const String& id) const
    {
        return nodes.FindOrDefault([&](const Ref<PipelineNode>& n) { return n->id == id; });
    }

    Ref<PipelineEdge> PipelineGraph::FindEdge(const String& id) const
    {
        return edges.FindOrDefault([&](const Ref<PipelineEdge>& e) { return e->id == id; });
    }

    Vector<Ref<PipelineEdge>> PipelineGraph::GetIncomingEdges(const String& nodeId) const
    {
        return edges.FindAll([&](const Ref<PipelineEdge>& e) { return e->toNodeId == nodeId; });
    }

    Vector<Ref<PipelineEdge>> PipelineGraph::GetOutgoingEdges(const String& nodeId) const
    {
        return edges.FindAll([&](const Ref<PipelineEdge>& e) { return e->fromNodeId == nodeId; });
    }

    Vector<String> PipelineGraph::GetRunTargets() const
    {
        Vector<String> finishes, others;
        for (auto& node : nodes)
        {
            if (!GetOutgoingEdges(node->id).IsEmpty())
                continue;

            if (PipelineNodeRegistry::IsFinishType(node->nodeType))
            {
                finishes.Add(node->id);
                continue;
            }

            // A source nobody consumes has nothing to compute
            auto schema = PipelineNodeRegistry::GetSchema(node->nodeType);
            if (schema && schema->category == PipelineNodeCategory::Source)
                continue;

            others.Add(node->id);
        }

        return finishes + others;
    }

    Ref<PipelineEdge> PipelineGraph::FindEdgeToPort(const String& nodeId, const String& portId) const
    {
        return edges.FindOrDefault([&](const Ref<PipelineEdge>& e) { return e->toNodeId == nodeId && e->toPortId == portId; });
    }

    void PipelineGraph::RemoveNode(const String& id)
    {
        nodes.RemoveAll([&](const Ref<PipelineNode>& n) { return n->id == id; });
        edges.RemoveAll([&](const Ref<PipelineEdge>& e) { return e->fromNodeId == id || e->toNodeId == id; });
    }

    void PipelineGraph::RemoveEdge(const String& id)
    {
        edges.RemoveAll([&](const Ref<PipelineEdge>& e) { return e->id == id; });
    }

    void PipelineGraph::RemoveDanglingEdges()
    {
        edges.RemoveAll([&](const Ref<PipelineEdge>& e)
        {
            auto from = FindNode(e->fromNodeId);
            auto to = FindNode(e->toNodeId);
            return !from || !to || !from->FindOutput(e->fromPortId) || !to->FindInput(e->toPortId);
        });
    }

    Vector<String> PipelineGraph::Validate() const
    {
        Vector<String> errors;

        Map<String, bool> targets;
        for (auto& edge : edges)
        {
            auto from = FindNode(edge->fromNodeId);
            auto to = FindNode(edge->toNodeId);
            if (!from) { errors.Add("Edge " + edge->id + ": source node not found"); continue; }
            if (!to) { errors.Add("Edge " + edge->id + ": target node not found"); continue; }

            auto fromPort = from->FindOutput(edge->fromPortId);
            auto toPort = to->FindInput(edge->toPortId);
            if (!fromPort) { errors.Add("Edge " + edge->id + ": source port missing on " + from->nodeType); continue; }
            if (!toPort) { errors.Add("Edge " + edge->id + ": target port missing on " + to->nodeType); continue; }

            if (fromPort->portType != toPort->portType)
            {
                errors.Add("Edge " + edge->id + ": type mismatch " + PipelinePortTypeToString(fromPort->portType) +
                           " -> " + PipelinePortTypeToString(toPort->portType));
            }

            String key = edge->toNodeId + ":" + edge->toPortId;
            if (targets.ContainsKey(key))
                errors.Add("Target port " + key + " has more than one incoming edge");
            else
                targets.Add(key, true);
        }

        // Cycle detection by DFS colouring: 0 unvisited, 1 on the current path, 2 finished
        Map<String, int> color;
        for (auto& node : nodes)
            color[node->id] = 0;

        Function<bool(const String&)> visit;
        String cycleNode;
        visit = [&](const String& id) -> bool
        {
            color[id] = 1;
            for (auto& edge : GetOutgoingEdges(id))
            {
                int c = 0;
                if (!color.TryGetValue(edge->toNodeId, c))
                    continue;

                if (c == 1) { cycleNode = edge->toNodeId; return true; }
                if (c == 0 && visit(edge->toNodeId)) return true;
            }
            color[id] = 2;
            return false;
        };

        for (auto& node : nodes)
        {
            if (color[node->id] == 0 && visit(node->id))
            {
                errors.Add("Pipeline contains a cycle (involves node " + cycleNode + ")");
                break;
            }
        }

        return errors;
    }

    bool PipelineGraph::WouldMakeCycle(const String& fromNodeId, const String& toNodeId) const
    {
        if (fromNodeId == toNodeId)
            return true;

        // Any path from "to" back to "from" closes a loop
        Vector<String> stack = { toNodeId };
        Map<String, bool> seen;
        while (!stack.IsEmpty())
        {
            String id = stack.Last();
            stack.RemoveAt(stack.Count() - 1);
            if (seen.ContainsKey(id))
                continue;

            seen[id] = true;
            if (id == fromNodeId)
                return true;

            for (auto& edge : GetOutgoingEdges(id))
                stack.Add(edge->toNodeId);
        }

        return false;
    }

    const Vector<String>& PipelineGraph::GetUiOnlyConfigKeys()
    {
        static Vector<String> keys = {
            "drawOver", "drawTool", "brushSize", "brushColor", "brushOpacity",
            "selectedLayer", "layersPanelW", "openLayerSettings",
            "viewZoom", "viewPanX", "viewPanY", "cmpBg", "cmpBgEnabled", "checker", "layersFolder", "layersName"
        };
        return keys;
    }

    bool PipelineGraph::IsSeededType(const String& type)
    {
        return type == "nanoBananaGen" || type == "imageEdit" || type == "imageExtract";
    }

    static int OwnSeed(const PipelineNode& node)
    {
        if (node.HasConfig("seed"))
        {
            String s = node.GetConfigString("seed", "");
            if (!s.Trimed().IsEmpty())
                return Math::Max(0, (int)Math::Floor(node.GetConfigNumber("seed", 0.0f)));
        }

        return (int)(PipelineUtils::Fnv1a64(node.id) % 2147483647ull);
    }

    Map<String, int> PipelineGraph::ResolveSeeds() const
    {
        Map<String, int> seeds;
        Map<String, bool> visiting;

        Function<int(const String&, bool&)> resolve;
        resolve = [&](const String& id, bool& found) -> int
        {
            found = false;
            auto node = FindNode(id);
            if (!node || !IsSeededType(node->nodeType))
                return 0;

            int cached = 0;
            if (seeds.TryGetValue(id, cached)) { found = true; return cached; }
            if (visiting.ContainsKey(id)) { found = true; return OwnSeed(*node); }

            visiting[id] = true;
            int seed = OwnSeed(*node);
            if (node->GetConfigBool("inheritSeed", true))
            {
                for (auto& edge : GetIncomingEdges(id))
                {
                    auto port = node->FindInput(edge->toPortId);
                    if (!port || port->portType != PipelinePortType::Image)
                        continue;

                    bool upFound = false;
                    int up = resolve(edge->fromNodeId, upFound);
                    if (upFound)
                        seed = up;

                    break;
                }
            }
            visiting.Remove(id);
            seeds[id] = seed;
            found = true;
            return seed;
        };

        for (auto& node : nodes)
        {
            bool found = false;
            resolve(node->id, found);
        }

        return seeds;
    }

    String PipelineGraph::UpstreamSigKey(const PipelineNode& node, const PipelinePort& port)
    {
        int sameName = node.inputs.Sum<int>([&](const PipelinePort& p) { return p.name == port.name ? 1 : 0; });
        return sameName > 1 ? port.name + "#" + port.id : port.name;
    }

    String PipelineGraph::ComputeNodeSignature(const PipelineNode& node, const Map<String, String>& upstreamByPortKey,
                                               const Vector<String>& extraExcludeKeys, int seed, const String& variant /*= ""*/)
    {
        Vector<String> exclude = GetUiOnlyConfigKeys();
        exclude.Add("seed");
        exclude.Add("inheritSeed");
        exclude.Add(extraExcludeKeys);

        String payload = "type=" + node.nodeType + "|config=" + PipelineUtils::CanonicalJson(node.config, exclude) + "|inputs=";
        for (auto& kv : upstreamByPortKey)
            payload += kv.first + ":" + kv.second + ";";

        if (node.nodeType == "sourceImage" || node.nodeType == "sourceAudio")
        {
            String upload = node.GetConfigString("uploadId", "");
            payload += "|upload:" + upload + ":" + PipelineUtils::FileSignature(PipelineUtils::GetUploadPath(upload));
        }

        if (seed >= 0)
            payload += "|seed:" + (String)seed;

        if (!variant.IsEmpty())
            payload += "|" + variant;

        return PipelineUtils::Fnv1a64Hex(payload);
    }

    Map<String, String> PipelineGraph::ComputeSignatures() const
    {
        Map<String, String> result;
        Map<String, bool> visiting;
        auto seeds = ResolveSeeds();

        Function<String(const String&)> sigOf;
        sigOf = [&](const String& id) -> String
        {
            String cached;
            if (result.TryGetValue(id, cached))
                return cached;

            if (visiting.ContainsKey(id))
                return "";

            auto node = FindNode(id);
            if (!node)
                return "";

            visiting[id] = true;
            Map<String, String> upstream;
            for (auto& edge : GetIncomingEdges(id))
            {
                auto port = node->FindInput(edge->toPortId);
                if (!port)
                    continue;

                String us = sigOf(edge->fromNodeId);
                if (!us.IsEmpty())
                    upstream[UpstreamSigKey(*node, *port)] = us;
            }

            int seed = -1;
            seeds.TryGetValue(id, seed);
            String sig = ComputeNodeSignature(*node, upstream, {}, seed);
            result[id] = sig;
            visiting.Remove(id);
            return sig;
        };

        for (auto& node : nodes)
            sigOf(node->id);

        return result;
    }
}
// --- META ---

ENUM_META(Editor::PipelinePortType, Editor__PipelinePortType)
{
    ENUM_ENTRY(Audio);
    ENUM_ENTRY(Image);
    ENUM_ENTRY(Text);
    ENUM_ENTRY(Video);
}
END_ENUM_META;

DECLARE_CLASS(Editor::PipelinePort, Editor__PipelinePort);

DECLARE_CLASS(Editor::PipelineNode, Editor__PipelineNode);

DECLARE_CLASS(Editor::PipelineEdge, Editor__PipelineEdge);

DECLARE_CLASS(Editor::PipelineGraph, Editor__PipelineGraph);
// --- END META ---
