#include "o2Editor/stdafx.h"
#include "PipelineExecutor.h"

#include "o2/Utils/Coroutines/Awaiters.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"

namespace Editor
{
    using UiLayout::Json;

    String PipelineExecutor::GetWorkPath(const String& pipelineId, const String& nodeId)
    {
        String safe;
        for (int i = 0; i < nodeId.Length(); i++)
        {
            char c = nodeId[i];
            bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
            safe += keep ? c : '_';
        }
        return GetCachePath(pipelineId) + "work/" + safe + "/";
    }

    String PipelineExecutor::DynamicPortId(const String& name)
    {
        String id = "sp-";
        for (int i = 0; i < name.Length(); i++)
        {
            char c = name[i];
            bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
            id += keep ? c : '_';
        }
        return id;
    }

    Vector<PipelinePort> PipelineExecutor::DynamicPorts(const PipelineNode& node, const Vector<PipelinePort>& schemaOutputs,
                                                        const Vector<Pair<String, PipelineValue>>& values)
    {
        auto has = [&](const String& name) { return values.Any([&](const Pair<String, PipelineValue>& v) { return v.first == name; }); };

        Vector<PipelinePort> ports;
        for (auto& schemaPort : schemaOutputs)
        {
            if (!has(schemaPort.name))
                continue;

            auto own = node.outputs.FindOrDefault([&](const PipelinePort& p) { return p.name == schemaPort.name; });
            ports.Add(PipelinePort(own.id.IsEmpty() ? schemaPort.id : own.id, schemaPort.name, schemaPort.portType, false));
        }

        for (auto& value : values)
        {
            if (schemaOutputs.Any([&](const PipelinePort& p) { return p.name == value.first; }))
                continue;

            auto own = node.outputs.FindOrDefault([&](const PipelinePort& p) { return p.name == value.first; });
            ports.Add(PipelinePort(own.id.IsEmpty() ? DynamicPortId(value.first) : own.id, value.first, value.second.type, false));
        }
        return ports;
    }

    String PipelineExecutor::BundleKey(const String& sig, const String& name)
    {
        static const char* hex = "0123456789abcdef";
        String digest = PipelineUtils::Sha256(sig + "#" + name), key;
        for (int i = 0; i < 16; i++)
        {
            unsigned char c = (unsigned char)digest[i];
            key += hex[c >> 4];
            key += hex[c & 15];
        }
        return key;
    }

    static String BundleManifestPath(const String& pipelineId, const String& sig)
    {
        return PipelineExecutor::GetContentPath(pipelineId, sig + "-outputs", "json");
    }

    static String ExtensionOf(const PipelineValue& value)
    {
        return value.type == PipelinePortType::Text ? String("txt") : value.GetExtension();
    }

    bool PipelineExecutor::LoadBundle(const String& pipelineId, const String& sig, Vector<Pair<String, PipelineValue>>& values)
    {
        String path = BundleManifestPath(pipelineId, sig);
        if (!o2FileSystem.IsFileExist(path))
            return false;

        Json manifest;
        if (!Json::Parse(PipelineUtils::ReadFileBytes(path), manifest) || !manifest.Get("outputs") || !manifest.Get("outputs")->IsArray())
            return false;

        Vector<Pair<String, PipelineValue>> loaded;
        for (auto& output : manifest.Get("outputs")->Items())
        {
            String name = output.Get("name") ? output.Get("name")->Str() : String();
            String key = output.Get("key") ? output.Get("key")->Str() : String();
            String ext = output.Get("ext") ? output.Get("ext")->Str() : String();
            if (ext.StartsWith("."))
                ext = ext.SubStr(1);

            String file = GetContentPath(pipelineId, key, ext);
            if (name.IsEmpty() || key.IsEmpty() || !o2FileSystem.IsFileExist(file))
                return false;

            auto type = PipelinePortTypeFromString(output.Get("type") ? output.Get("type")->Str() : String("text"));
            String bytes = PipelineUtils::ReadFileBytes(file);
            PipelineValue value = type == PipelinePortType::Text ? PipelineValue::Text(bytes) :
                PipelineValue::Bytes(type, bytes, PipelineUtils::MimeForExtension(ext));
            loaded.Add(Pair<String, PipelineValue>(name, value));
        }

        values = loaded;
        return true;
    }

    void PipelineExecutor::SaveBundle(const String& pipelineId, const String& sig, const Vector<Pair<String, PipelineValue>>& values)
    {
        Json outputs = Json::Array();
        for (auto& v : values)
        {
            String key = BundleKey(sig, v.first), ext = ExtensionOf(v.second);
            PipelineUtils::WriteFileBytes(GetContentPath(pipelineId, key, ext), v.second.IsImage() ? v.second.GetPngBytes() : v.second.data);

            Json entry = Json::Object();
            entry["name"] = v.first;
            entry["type"] = PipelinePortTypeToString(v.second.type);
            entry["key"] = key;
            entry["ext"] = "." + ext;
            outputs.Add(entry);
        }

        Json manifest = Json::Object();
        manifest["outputs"] = outputs;
        PipelineUtils::WriteFileBytes(BundleManifestPath(pipelineId, sig), manifest.Dump());
    }

    void PipelineExecutor::DeleteBundle(const String& pipelineId, const String& sig)
    {
        o2FileSystem.FileDelete(BundleManifestPath(pipelineId, sig));
    }

    void PipelineExecutor::RemoveStalePortPreviews(const String& pipelineId, const String& nodeId, const Vector<String>& keepPortIds)
    {
        String folder = GetCachePath(pipelineId) + "previews/";
        if (!o2FileSystem.IsFolderExist(folder))
            return;

        String prefix = nodeId + ".";
        for (auto& file : o2FileSystem.GetFolderInfo(folder).files)
        {
            String name = o2FileSystem.GetPathWithoutDirectories(file.path);
            if (!name.StartsWith(prefix))
                continue;

            String rest = name.SubStr(prefix.Length());
            int dot = rest.FindLast(".");
            if (dot <= 0)
                continue;

            String portId = rest.SubStr(0, dot);
            if (portId != "src" && !keepPortIds.Contains(portId))
                o2FileSystem.FileDelete(folder + name);
        }
    }

    namespace
    {
        Mutex               inflightMutex;
        Map<String, bool>   inflightBundles; // "<pipeline>:<sig>" being computed
    }

    Coroutine<bool> PipelineExecutor::EvaluateDynamic(Ref<Run> run, Ref<PipelineNode> node, Ref<PipelineExecContext> ctx,
                                                      Map<String, PipelineValue> inputs, String sig, Map<String, PipelineValue>* byPortId,
                                                      PipelineValue* preview, bool* produced, String* error)
    {
        auto impl = PipelineNodeRegistry::Get(node->nodeType);
        String nodeId = node->id;
        String pipelineId = run->pipelineId;
        bool fresh = run->bypass.Contains(nodeId);
        if (fresh)
            DeleteBundle(pipelineId, sig);

        // One computation per signature at a time: another run of the same node waits for it and takes its result
        String inflightKey = pipelineId + ":" + sig;
        for (;;)
        {
            bool busy = false;
            {
                ScopeLock<Mutex> lock(inflightMutex);
                busy = inflightBundles.ContainsKey(inflightKey);
                if (!busy)
                    inflightBundles[inflightKey] = true;
            }

            if (!busy)
                break;

            if (run->cancelled)
                co_return false;

            co_await WaitNextFrame();
        }

        Vector<Pair<String, PipelineValue>> values;
        String source = "computed";
        if (LoadBundle(pipelineId, sig, values))
            source = "cache";
        else if (run->cachedOnly)
        {
            {
                ScopeLock<Mutex> lock(inflightMutex);
                inflightBundles.Remove(inflightKey);
            }
            run->notCached = true;
            co_return false;
        }
        else
        {
            ctx->workDir = GetWorkPath(pipelineId, nodeId);
            ctx->fresh = fresh;
            ctx->memoGet = [pipelineId](const String& key, const String& ext, String& data)
            {
                String path = GetContentPath(pipelineId, key, ext.StartsWith(".") ? ext.SubStr(1) : ext);
                if (!o2FileSystem.IsFileExist(path))
                    return false;
                data = PipelineUtils::ReadFileBytes(path);
                return !data.IsEmpty();
            };
            ctx->memoPut = [pipelineId](const String& key, const String& ext, const String& data)
            {
                PipelineUtils::WriteFileBytes(GetContentPath(pipelineId, key, ext.StartsWith(".") ? ext.SubStr(1) : ext), data);
            };

            PipelineRunResult result = co_await impl->Run(ctx, inputs, node);
            if (run->cancelled)
            {
                ScopeLock<Mutex> lock(inflightMutex);
                inflightBundles.Remove(inflightKey);
                co_return false;
            }

            if (!result.ok)
                *error = result.error.IsEmpty() ? String("Node " + node->nodeType + " failed") : result.error;
            else
            {
                Vector<String> order = result.order;
                for (auto& kv : result.outputs)
                {
                    if (!order.Contains(kv.first))
                        order.Add(kv.first);
                }
                for (auto& name : order)
                {
                    auto it = result.outputs.find(name);
                    if (it != result.outputs.end() && it->second.IsValid())
                        values.Add(Pair<String, PipelineValue>(name, it->second));
                }

                if (values.IsEmpty())
                    *error = "Node " + node->nodeType + " produced nothing";
                else
                    SaveBundle(pipelineId, sig, values);
            }
        }

        {
            ScopeLock<Mutex> lock(inflightMutex);
            inflightBundles.Remove(inflightKey);
        }

        if (!error->IsEmpty())
            co_return false;

        *produced = source == "computed";
        if (!*produced)
            EmitLog(node->nodeType + ": cache hit (" + sig.SubStr(0, 8) + ")");

        auto ports = DynamicPorts(*node, impl->GetSchema().outputs, values);
        for (auto& port : ports)
        {
            for (auto& v : values)
            {
                if (v.first == port.name)
                    (*byPortId)[port.id] = v.second;
            }
        }

        PipelineExecEvent ev;
        ev.type = PipelineExecEvent::Type::NodePorts;
        ev.nodeId = nodeId;
        ev.ports = ports;
        Emit(ev);

        if (!run->cachedOnly || *produced || nodeId == run->targetNodeId)
        {
            Vector<String> keep;
            for (int i = 1; i < ports.Count(); i++)
            {
                WritePortPreview(run, nodeId, ports[i].id, (*byPortId)[ports[i].id]);
                keep.Add(ports[i].id);
            }
            RemoveStalePortPreviews(pipelineId, nodeId, keep);
        }

        if (!ports.IsEmpty())
            *preview = (*byPortId)[ports[0].id];

        node->outputs = ports;
        co_return true;
    }
}
