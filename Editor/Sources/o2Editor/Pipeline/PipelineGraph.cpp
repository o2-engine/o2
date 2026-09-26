#include "o2Editor/stdafx.h"
#include "PipelineGraph.h"

#include "o2/Utils/Types/UID.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
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

        extra = static_cast<const DataValue&>(other.extra);
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

        extra = static_cast<const DataValue&>(other.extra);
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
    {
        extra = static_cast<const DataValue&>(other.extra);
    }

    PipelineGraph::PipelineGraph(const PipelineGraph& other)
    {
        *this = other;
    }

    void PipelineGraph::LoadFromAsset(const PipelineAsset& asset)
    {
        if (IsLegacyDocument(asset.document))
        {
            *this = PipelineGraph();
            Deserialize(*asset.document.FindMember("graph"));
            return;
        }

        if (!LoadFromJson(asset.document))
            *this = PipelineGraph();
    }

    void PipelineGraph::SaveToAsset(PipelineAsset& asset) const
    {
        asset.document.Clear();
        SaveToJson(asset.document);
    }

    bool PipelineGraph::IsLegacyDocument(const DataValue& document)
    {
        return document.IsObject() && document.FindMember("graph") != nullptr && document.FindMember("nodes") == nullptr;
    }

    Vec2F PipelineGraph::GetNominalViewSize()
    {
        return Vec2F(1600.0f, 1000.0f);
    }

    // Whole numbers stay integers and the rest is rounded, so a document written back unchanged stays byte-stable

    static void SetJsonNumber(DataValue& value, float number)
    {
        double rounded = std::round((double)number);
        if (std::fabs((double)number - rounded) < 0.0005 && std::fabs(rounded) < 2.0e9)
            value = (int)rounded;
        else
            value = std::round((double)number * 1000.0) / 1000.0;
    }

    static float JsonNumber(const DataValue* value, float def = 0.0f)
    {
        return value ? PipelineUtils::ValueToNumber(*value, def) : def;
    }

    static String JsonString(const DataValue* value)
    {
        return value ? PipelineUtils::ValueToString(*value, "") : String();
    }

    static Vec2F ReadPoint(const DataValue* value, const char* xName, const char* yName)
    {
        if (!value || !value->IsObject())
            return Vec2F();

        return Vec2F(JsonNumber(value->FindMember(xName)), JsonNumber(value->FindMember(yName)));
    }

    static void WritePoint(DataValue& value, const Vec2F& point, const char* xName, const char* yName)
    {
        value.SetObject();
        SetJsonNumber(value.AddMember(xName), point.x);
        SetJsonNumber(value.AddMember(yName), point.y);
    }

    static void CopyUnknownMembers(const DataValue& from, DataValue& to, const Vector<const char*>& known)
    {
        to.SetObject();
        if (!from.IsObject())
            return;

        for (auto it = from.BeginMember(); it != from.EndMember(); ++it)
        {
            String name = it->name.GetString();
            if (known.Any([&](const char* k) { return name == k; }))
                continue;

            to.AddMember(name.Data()) = it->value;
        }
    }

    static void AppendMembers(const DataValue& from, DataValue& to)
    {
        if (!from.IsObject())
            return;

        for (auto it = from.BeginMember(); it != from.EndMember(); ++it)
        {
            if (!to.FindMember(it->name.GetString()))
                to.AddMember(it->name.GetString()) = it->value;
        }
    }

    static bool ReadPort(const DataValue& json, PipelinePort& port)
    {
        if (!json.IsObject())
            return false;

        port.id = JsonString(json.FindMember("id"));
        port.name = JsonString(json.FindMember("name"));
        port.portType = PipelinePortTypeFromString(JsonString(json.FindMember("type")));
        port.color = JsonString(json.FindMember("color"));
        auto custom = json.FindMember("custom");
        port.custom = custom && custom->IsBoolean() && (bool)*custom;
        return !port.id.IsEmpty();
    }

    static void WritePort(DataValue& json, const PipelinePort& port)
    {
        json.SetObject();
        json.AddMember("id") = port.id;
        json.AddMember("name") = port.name;
        json.AddMember("type") = PipelinePortTypeToString(port.portType);
        if (!port.color.IsEmpty())
            json.AddMember("color") = port.color;

        if (port.custom)
            json.AddMember("custom") = true;
    }

    void PipelineNode::RemapConfigPortIds(const Map<String, String>& portIdMap)
    {
        if (!config.IsObject())
            return;

        Map<String, String> idMap = portIdMap;
        auto mapId = [&](const String& id) { String v; return idMap.TryGetValue(id, v) ? v : id; };
        auto remapStringMember = [&](const char* key)
        {
            if (auto value = config.FindMember(key); value && value->IsString())
                *value = mapId(value->GetString());
        };

        if (auto customs = config.FindMember("customInputs"); customs && customs->IsArray())
        {
            for (auto& item : *customs)
                if (auto id = item.IsObject() ? item.FindMember("id") : nullptr; id && id->IsString())
                    *id = mapId(id->GetString());
        }

        // Composer: duplicated layers own ids of their own, minted anew and mapped alongside
        if (auto dups = config.FindMember("dupLayers"); dups && dups->IsArray())
        {
            for (auto& item : *dups)
                if (auto id = item.IsObject() ? item.FindMember("id") : nullptr; id && id->IsString())
                    idMap[id->GetString()] = GenerateId();

            for (auto& item : *dups)
            {
                if (!item.IsObject())
                    continue;

                if (auto id = item.FindMember("id"); id && id->IsString()) *id = mapId(id->GetString());
                if (auto src = item.FindMember("srcPortId"); src && src->IsString()) *src = mapId(src->GetString());
            }
        }

        if (auto layers = config.FindMember("layers"); layers && layers->IsObject())
        {
            DataDocument remapped;
            remapped.SetObject();
            for (auto it = layers->BeginMember(); it != layers->EndMember(); ++it)
                remapped.AddMember(mapId(it->name.GetString()).Data()) = it->value;

            *layers = static_cast<const DataValue&>(remapped);
        }

        if (auto order = config.FindMember("layerOrder"); order && order->IsArray())
        {
            for (auto& item : *order)
                if (item.IsString()) item = mapId(item.GetString());
        }

        remapStringMember("selectedLayer");
        remapStringMember("openLayerSettings");

        // AI extract: one region per output port, keyed by the port id
        if (auto regions = config.FindMember("regions"); regions && regions->IsArray())
        {
            for (auto& item : *regions)
                if (auto id = item.IsObject() ? item.FindMember("id") : nullptr; id && id->IsString())
                    *id = mapId(id->GetString());
        }

        remapStringMember("selectedRegion");
    }

    bool PipelineNode::LoadFromJson(const DataValue& json)
    {
        if (!json.IsObject())
            return false;

        id = JsonString(json.FindMember("id"));
        nodeType = JsonString(json.FindMember("type"));
        position = ReadPoint(json.FindMember("position"), "x", "y");
        size = ReadPoint(json.FindMember("size"), "width", "height");

        config.Clear();
        if (auto cfg = json.FindMember("config"))
            config = *cfg;

        if (!config.IsObject())
            config.SetObject();

        inputs.Clear();
        outputs.Clear();
        PipelinePort port;
        if (auto list = json.FindMember("inputs"); list && list->IsArray())
        {
            for (auto& item : *list)
                if (ReadPort(item, port)) inputs.Add(port);
        }

        if (auto list = json.FindMember("outputs"); list && list->IsArray())
        {
            for (auto& item : *list)
                if (ReadPort(item, port)) outputs.Add(port);
        }

        extra.Clear();
        CopyUnknownMembers(json, extra, { "id", "type", "position", "size", "config", "inputs", "outputs" });
        return !id.IsEmpty() && !nodeType.IsEmpty();
    }

    void PipelineNode::SaveToJson(DataValue& json) const
    {
        json.SetObject();
        json.AddMember("id") = id;
        json.AddMember("type") = nodeType;
        WritePoint(json.AddMember("position"), position, "x", "y");
        if (size != Vec2F())
            WritePoint(json.AddMember("size"), size, "width", "height");

        auto& cfg = json.AddMember("config");
        cfg = static_cast<const DataValue&>(config);
        if (!cfg.IsObject())
            cfg.SetObject();

        auto& inList = json.AddMember("inputs");
        inList.SetArray();
        for (auto& port : inputs)
            WritePort(inList.AddElement(), port);

        auto& outList = json.AddMember("outputs");
        outList.SetArray();
        for (auto& port : outputs)
            WritePort(outList.AddElement(), port);

        AppendMembers(extra, json);
    }

    bool PipelineEdge::LoadFromJson(const DataValue& json)
    {
        if (!json.IsObject())
            return false;

        id = JsonString(json.FindMember("id"));
        fromNodeId = JsonString(json.FindMember("fromNodeId"));
        fromPortId = JsonString(json.FindMember("fromPortId"));
        toNodeId = JsonString(json.FindMember("toNodeId"));
        toPortId = JsonString(json.FindMember("toPortId"));

        points.Clear();
        if (auto list = json.FindMember("points"); list && list->IsArray())
        {
            for (auto& item : *list)
                points.Add(ReadPoint(&item, "x", "y"));
        }

        extra.Clear();
        CopyUnknownMembers(json, extra, { "id", "fromNodeId", "fromPortId", "toNodeId", "toPortId", "points" });
        if (id.IsEmpty())
            id = PipelineNode::GenerateId();

        return !fromNodeId.IsEmpty() && !fromPortId.IsEmpty() && !toNodeId.IsEmpty() && !toPortId.IsEmpty();
    }

    void PipelineEdge::SaveToJson(DataValue& json) const
    {
        json.SetObject();
        json.AddMember("id") = id;
        json.AddMember("fromNodeId") = fromNodeId;
        json.AddMember("fromPortId") = fromPortId;
        json.AddMember("toNodeId") = toNodeId;
        json.AddMember("toPortId") = toPortId;
        if (!points.IsEmpty())
        {
            auto& list = json.AddMember("points");
            list.SetArray();
            for (auto& point : points)
                WritePoint(list.AddElement(), point, "x", "y");
        }

        AppendMembers(extra, json);
    }

    bool PipelineGraph::LoadFromJson(const DataValue& json)
    {
        *this = PipelineGraph();
        if (!json.IsObject() || IsLegacyDocument(json))
            return false;

        auto nodeList = json.FindMember("nodes");
        auto edgeList = json.FindMember("edges");
        if (!nodeList || !nodeList->IsArray() || (edgeList && !edgeList->IsArray()))
            return false;

        id = JsonString(json.FindMember("id"));
        name = JsonString(json.FindMember("name"));

        for (auto& item : *nodeList)
        {
            auto node = mmake<PipelineNode>();
            if (node->LoadFromJson(item))
                nodes.Add(node);
        }

        if (edgeList)
        {
            for (auto& item : *edgeList)
            {
                auto edge = mmake<PipelineEdge>();
                if (edge->LoadFromJson(item) && FindNode(edge->fromNodeId) && FindNode(edge->toNodeId))
                    edges.Add(edge);
            }
        }

        // AssetsLine keeps screen = world * scale + (x, y), y down; the editor a canvas centre (y up) and units per pixel
        cameraPosition = Vec2F();
        cameraScale = 1.0f;
        if (auto camera = json.FindMember("camera"); camera && camera->IsObject())
        {
            float scale = JsonNumber(camera->FindMember("scale"), 0.0f);
            if (scale > 0.0f)
            {
                Vec2F view = GetNominalViewSize();
                Vec2F offset = ReadPoint(camera, "x", "y");
                Vec2F center = (view * 0.5f - offset) / scale;
                cameraPosition = Vec2F(center.x, -center.y);
                cameraScale = 1.0f / scale;
            }
        }

        extra.Clear();
        CopyUnknownMembers(json, extra, { "schemaVersion", "id", "name", "nodes", "edges", "camera" });
        return true;
    }

    void PipelineGraph::SaveToJson(DataValue& json) const
    {
        json.SetObject();
        json.AddMember("schemaVersion") = 1;
        json.AddMember("id") = id;
        json.AddMember("name") = name;

        auto& nodeList = json.AddMember("nodes");
        nodeList.SetArray();
        for (auto& node : nodes)
            node->SaveToJson(nodeList.AddElement());

        auto& edgeList = json.AddMember("edges");
        edgeList.SetArray();
        for (auto& edge : edges)
            edge->SaveToJson(edgeList.AddElement());

        if (cameraScale > 0.0f && (cameraPosition != Vec2F() || cameraScale != 1.0f))
        {
            Vec2F view = GetNominalViewSize();
            float scale = 1.0f / cameraScale;
            Vec2F center(cameraPosition.x, -cameraPosition.y);
            Vec2F offset = view * 0.5f - center * scale;
            auto& camera = json.AddMember("camera");
            camera.SetObject();
            camera.AddMember("x") = std::round((double)offset.x * 100.0) / 100.0;
            camera.AddMember("y") = std::round((double)offset.y * 100.0) / 100.0;
            camera.AddMember("scale") = std::round((double)scale * 1.0e6) / 1.0e6;
        }

        AppendMembers(extra, json);
    }

    bool PipelineGraph::LoadFromJsonString(const String& text)
    {
        DataDocument doc;
        if (!doc.LoadFromData(text))
        {
            *this = PipelineGraph();
            return false;
        }

        return LoadFromJson(doc);
    }

    String PipelineGraph::ToJsonString() const
    {
        DataDocument doc;
        SaveToJson(doc);
        return doc.SaveAsString();
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
        name = other.name;
        cameraPosition = other.cameraPosition;
        cameraScale = other.cameraScale;
        extra = static_cast<const DataValue&>(other.extra);

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
            "drawOver", "drawTool", "brushSize", "brushColor", "brushOpacity", "paramsOpen", "selectedRegion",
            "splitRatio", "fieldH", "selectedLayer", "layersPanelW", "openLayerSettings",
            "viewZoom", "viewPanX", "viewPanY", "cmpBg", "cmpBgEnabled", "checker", "layersFolder", "layersName"
        };
        return keys;
    }

    bool PipelineGraph::IsSeededType(const String& type)
    {
        return type == "nanoBananaGen" || type == "imageEdit" || type == "imageExtract";
    }

    // AssetsLine's implicit seed (32-bit FNV-1a over UTF-16 code units), so both editors render alike
    static int HashSeed(const String& id)
    {
        UInt32 h = 2166136261u;
        WString wide = id;
        for (auto c : wide)
        {
            h ^= (UInt32)c;
            h *= 16777619u;
        }

        return (int)(h % 2147483647u);
    }

    // JavaScript's Number(value) for a config value: false for NaN and infinities
    static bool JsNumber(const DataValue* value, double& out)
    {
        if (!value)
            return false;

        if (value->IsNull()) { out = 0.0; return true; }
        if (value->IsBoolean()) { out = (bool)*value ? 1.0 : 0.0; return true; }
        if (value->IsNumber()) { out = (double)*value; return std::isfinite(out); }
        if (!value->IsString())
            return false;

        String text = PipelineUtils::Trim(value->GetString());
        if (text.IsEmpty()) { out = 0.0; return true; }

        const char* begin = text.Data();
        char* end = nullptr;
        if (text.Length() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
            out = (double)std::strtoull(begin + 2, &end, 16);
        else
        {
            if (text.Contains("inf") || text.Contains("INF") || text.Contains("nan") || text.Contains("NAN"))
                return false;

            out = std::strtod(begin, &end);
        }

        return end == begin + text.Length() && std::isfinite(out);
    }

    static int OwnSeed(const PipelineNode& node)
    {
        double seed = 0.0;
        if (JsNumber(node.GetConfigValue("seed"), seed))
            return (int)Math::Clamp(std::floor(seed), -2147483648.0, 2147483647.0);

        return HashSeed(node.id);
    }

    // Seed inheritance is on unless the config says a literal false
    static bool InheritsSeed(const PipelineNode& node)
    {
        auto value = node.GetConfigValue("inheritSeed");
        return !(value && value->IsBoolean() && !(bool)*value);
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
            if (InheritsSeed(*node))
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

    String PipelineGraph::OutputSigKey(const PipelineNode& fromNode, const String& fromPortId)
    {
        auto schema = PipelineNodeRegistry::GetSchema(fromNode.nodeType);
        return schema && schema->perPortRun ? fromNode.id + "#" + fromPortId : fromNode.id;
    }

    String PipelineGraph::ComputePortSignature(const PipelineNode& node, const Map<String, String>& upstreamByPortKey,
                                               int seed, const String& portId, bool rawRender /*= true*/)
    {
        auto impl = PipelineNodeRegistry::Get(node.nodeType);
        bool chroma = rawRender && PipelineTransparency::UsesChromaPostStep(node);

        Vector<String> exclude = { "crop", "cropEnabled" };
        if (impl)
            exclude.Add(impl->PortCacheExcludedKeys());
        if (chroma)
            exclude.Add(PipelineTransparency::ChromaConfigKeys());

        String variant = (impl ? impl->PortCacheVariant(node, portId) : portId) + (chroma ? "|chroma-raw" : "");
        return ComputeNodeSignature(node, upstreamByPortKey, exclude, seed, variant);
    }

    Map<String, String> PipelineGraph::UpstreamSignatures(const PipelineNode& node, const Map<String, String>& nodeSignatures) const
    {
        Map<String, String> upstream;
        for (auto& edge : GetIncomingEdges(node.id))
        {
            auto port = node.FindInput(edge->toPortId);
            if (!port)
                continue;

            String sig;
            auto from = FindNode(edge->fromNodeId);
            if (!from || !nodeSignatures.TryGetValue(OutputSigKey(*from, edge->fromPortId), sig))
                nodeSignatures.TryGetValue(edge->fromNodeId, sig);

            if (!sig.IsEmpty())
                upstream[UpstreamSigKey(node, *port)] = sig;
        }
        return upstream;
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
                if (us.IsEmpty())
                    continue;

                // A per-port upstream caches every part apart: hash the part this edge reads
                if (auto from = FindNode(edge->fromNodeId))
                    result.TryGetValue(OutputSigKey(*from, edge->fromPortId), us);

                upstream[UpstreamSigKey(*node, *port)] = us;
            }

            int seed = -1;
            seeds.TryGetValue(id, seed);
            String sig = ComputeNodeSignature(*node, upstream, {}, seed);
            result[id] = sig;

            auto schema = PipelineNodeRegistry::GetSchema(node->nodeType);
            if (schema && schema->perPortRun)
            {
                for (auto& output : node->outputs)
                    result[id + "#" + output.id] = ComputePortSignature(*node, upstream, seed, output.id, false);
            }

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
