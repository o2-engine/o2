#include "o2Editor/stdafx.h"
#include "AssetsLineSyncInternal.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    using namespace AssetsLineSyncUtils;

    namespace
    {
        // One AssetsLine result key and the local file it corresponds to
        struct LocalResult
        {
            String nodeId;
            String portId;
            String kind;  // main, src, part or raw
            String path;  // The local file; for a node's own result without one yet, empty
        };

        const Vector<String>& MainExtensions()
        {
            static Vector<String> exts = { "png", "mp4", "mp3", "wav", "ogg", "m4a", "flac", "txt" };
            return exts;
        }

        String ExistingMainPreview(const String& pipelineId, const String& nodeId)
        {
            for (auto& ext : MainExtensions())
            {
                String path = PipelineExecutor::GetPreviewPath(pipelineId, nodeId, ext);
                if (o2FileSystem.IsFileExist(path))
                    return path;
            }

            return "";
        }

        String Extension(const String& path)
        {
            return o2FileSystem.GetFileExtension(path).ToLowerCase();
        }

        String ContentExtension(const PipelineNode& node, const String& portId, const String& mainPreview)
        {
            auto port = portId.IsEmpty() ? (node.outputs.IsEmpty() ? nullptr : &node.outputs[0]) : node.FindOutput(portId);
            if (!port)
                return "";

            switch (port->portType)
            {
                case PipelinePortType::Image: return "png";
                case PipelinePortType::Video: return "mp4";
                case PipelinePortType::Text: return "txt";
                default: return mainPreview.IsEmpty() ? String() : Extension(mainPreview);
            }
        }

        // Result keys of the graph and their local files; chroma nodes add their raw renders in the content cache
        Map<String, LocalResult> CollectLocalResults(const String& id, const PipelineGraph& graph,
                                                     const Map<String, String>& sigs, const Map<String, int>& seeds)
        {
            Map<String, LocalResult> results;
            for (auto& node : graph.nodes)
            {
                auto schema = PipelineNodeRegistry::GetSchema(node->nodeType);
                bool perPort = schema && schema->perPortRun && !node->outputs.IsEmpty();
                bool multi = perPort && node->outputs.Count() > 1;

                results[node->id] = { node->id, "", "main", ExistingMainPreview(id, node->id) };
                results[node->id + ".src"] = { node->id, "", "src", PipelineExecutor::GetSourcePreviewPath(id, node->id) };
                if (multi)
                {
                    for (auto& port : node->outputs)
                        results[node->id + "#" + port.id] = { node->id, port.id, "part", PipelineExecutor::GetPortPreviewPath(id, node->id, port.id, "png") };
                }

                // Each part decides by its own effective settings whether it has a raw render
                Vector<String> rawPorts;
                if (multi)
                {
                    for (auto& port : node->outputs)
                    {
                        if (PipelineTransparency::UsesChromaPostStep(*node, port.id))
                            rawPorts.Add(port.id);
                    }
                }
                else if (PipelineTransparency::UsesChromaPostStep(*node, perPort ? node->outputs[0].id : String()))
                    rawPorts.Add(perPort ? node->outputs[0].id : String());

                if (rawPorts.IsEmpty())
                    continue;

                int seed = -1;
                seeds.TryGetValue(node->id, seed);
                auto upstream = graph.UpstreamSignatures(*node, sigs);
                for (auto& portId : rawPorts)
                {
                    String path = PipelineExecutor::GetContentPath(id, PipelineExecutor::ContentSignature(*node, upstream, seed, portId), "png");
                    if (multi)
                        results[node->id + "#" + portId + ".raw"] = { node->id, portId, "raw", path };
                    else
                        results[node->id + ".raw"] = { node->id, "", "raw", path };
                }
            }

            return results;
        }

        // A local run then reuses the downloaded generation instead of asking the model again. A node generated
        // again keeps its signature, so with replace a different stored generation gives way to the downloaded one
        void SeedContent(const String& id, const PipelineGraph& graph, const PipelineNode& node,
                         const Map<String, String>& sigs, const Map<String, int>& seeds, bool replace)
        {
            if (PipelineNodeRegistry::IsFinishType(node.nodeType))
                return;

            // A chroma render is cached raw, before the cut the preview shows: it comes as its own ".raw" result
            auto schema = PipelineNodeRegistry::GetSchema(node.nodeType);
            bool perPort = schema && schema->perPortRun && !node.outputs.IsEmpty();
            if (!perPort && (node.outputs.Count() != 1 || PipelineTransparency::UsesChromaPostStep(node)))
                return;

            int seed = -1;
            seeds.TryGetValue(node.id, seed);
            auto upstream = graph.UpstreamSignatures(node, sigs);
            String main = ExistingMainPreview(id, node.id);

            auto copy = [&](const String& sig, const String& ext, const String& source)
            {
                String dest = PipelineExecutor::GetContentPath(id, sig, ext);
                if (ext.IsEmpty() || source.IsEmpty() || !o2FileSystem.IsFileExist(source))
                    return;

                if (o2FileSystem.IsFileExist(dest))
                {
                    if (!replace)
                        return;

                    String body = PipelineUtils::ReadFileBytes(source);
                    if (PipelineUtils::ReadFileBytes(dest) != body)
                        PipelineUtils::WriteFileBytes(dest, body);
                    return;
                }

                o2FileSystem.FileCopy(source, dest);
            };

            if (perPort)
            {
                for (auto& port : node.outputs)
                {
                    if (PipelineTransparency::UsesChromaPostStep(node, port.id))
                        continue;

                    String source = node.outputs.Count() > 1 ? PipelineExecutor::GetPortPreviewPath(id, node.id, port.id, "png") : main;
                    copy(PipelineExecutor::ContentSignature(node, upstream, seed, port.id), "png", source);
                }
                return;
            }

            String src = PipelineExecutor::GetSourcePreviewPath(id, node.id);
            String source = node.GetConfigBool("cropEnabled", false) && o2FileSystem.IsFileExist(src) ? src : main;
            copy(PipelineExecutor::ContentSignature(node, upstream, seed), ContentExtension(node, "", main), source);
        }
    }

    bool AssetsLineSync::AssignAssetUploadIds(PipelineGraph& graph)
    {
        static const Vector<String> uploadable = { "png", "jpg", "jpeg", "webp", "gif", "mp3", "wav", "ogg", "m4a", "flac", "mp4" };

        bool changed = false;
        String assetsPath = GetAssetsPath();
        for (auto& node : graph.nodes)
        {
            if (node->nodeType != "sourceImage" && node->nodeType != "sourceAudio")
                continue;

            String assetPath = node->GetConfigString("assetPath", "").Trimed(" \n\r\t");
            String full = assetsPath + assetPath;
            String ext = Extension(assetPath);
            if (assetPath.IsEmpty() || !uploadable.Contains(ext) || !o2FileSystem.IsFileExist(full))
                continue;

            // The asset's content names its upload: an edited asset is a new upload and a changed source
            String key = assetPath + "|" + Stamp(full);
            String uploadId;
            if (!mAssetUploadIds.TryGetValue(key, uploadId))
            {
                uploadId = "o2-" + PipelineUtils::Fnv1a64Hex(PipelineUtils::ReadFileBytes(full)).SubStr(0, 24) + "." + ext;
                mAssetUploadIds[key] = uploadId;
            }

            if (node->GetConfigString("uploadId", "") != uploadId)
            {
                node->SetConfigString("uploadId", uploadId);
                changed = true;
            }
        }

        return changed;
    }

    Coroutine<bool> AssetsLineSync::SyncResults(Pass& pass, const String& id)
    {
        Ref<AssetsLineSync> self(this);
        auto stateIt = mPipelines.find(id);
        if (stateIt == mPipelines.end())
            co_return true;

        PipelineGraph graph;
        if (!ParseDocument(ReadBase(id), graph))
            co_return true;

        auto sigs = graph.ComputeSignatures();
        auto seeds = graph.ResolveSeeds();
        auto locals = CollectLocalResults(id, graph, sigs, seeds);

        // Nothing moved on either side since the last pass: no need to ask
        {
            auto& state = mPipelines[id];
            bool localMoved = false;
            for (auto& kv : locals)
            {
                String stamp = kv.second.path.IsEmpty() ? String() : Stamp(kv.second.path);
                AssetsLineResultState known;
                bool has = state.results.TryGetValue(kv.first, known);
                if (!stamp.IsEmpty() && (!has || !SameStamp(known.stamp, stamp)))
                {
                    localMoved = true;
                    break;
                }
            }

            if (!localMoved && pass.resultsSeq == mResultsSeq && !pass.changedIds.Contains(id))
                co_return true;
        }

        String base = "/api/sync/pipelines/" + AssetsLineClient::Encode(id);
        auto manifest = co_await pass.client.Request(HttpMethod::Get, base + "/results");
        if (!manifest.ok || !manifest.json.IsObject())
        {
            if (manifest.offline)
                pass.offline = true;

            if (pass.error.IsEmpty())
                pass.error = "reading results of " + graph.name + ": " + manifest.error;

            co_return false;
        }

        auto& state = mPipelines[id];
        auto rev = manifest.json.FindMember("rev");
        if (!rev || !rev->IsNumber() || (int)*rev != state.rev)
            co_return true;

        Map<String, String> serverV;
        Map<String, String> serverExt;
        if (auto results = manifest.json.FindMember("results"); results && results->IsObject())
        {
            for (auto it = results->BeginMember(); it != results->EndMember(); ++it)
            {
                String key = it->name.GetString();
                auto v = it->value.FindMember("v");
                auto ext = it->value.FindMember("ext");
                serverV[key] = v ? PipelineUtils::ValueToString(*v) : String();
                serverExt[key] = ext ? PipelineUtils::ValueToString(*ext) : String();
            }
        }

        Vector<String> serverFresh;
        if (auto fresh = manifest.json.FindMember("fresh"); fresh && fresh->IsArray())
        {
            for (auto& item : *fresh)
                serverFresh.Add(PipelineUtils::ValueToString(item));
        }

        auto localFresh = PipelineExecutor::ComputeFreshNodes(id, graph);
        Vector<String> downloaded, uploaded, failed;
        bool ok = true;

        Vector<String> keys;
        for (auto& kv : locals) keys.Add(kv.first);
        for (auto& kv : serverV) if (!keys.Contains(kv.first)) keys.Add(kv.first);

        for (auto& key : keys)
        {
            auto localIt = locals.find(key);
            if (localIt == locals.end())
                continue;

            auto& local = localIt->second;
            String stamp = local.path.IsEmpty() ? String() : Stamp(local.path);
            String v;
            serverV.TryGetValue(key, v);

            AssetsLineResultState known;
            bool has = state.results.TryGetValue(key, known);
            bool serverChanged = !v.IsEmpty() && (!has || v != known.v);
            bool localChanged = !stamp.IsEmpty() && (!has || !SameStamp(known.stamp, stamp));
            if (!serverChanged && !localChanged)
                continue;

            bool download = serverChanged;
            if (serverChanged && localChanged)
            {
                // A take on each side: the one that matches the current graph wins, the shared one otherwise
                download = serverFresh.Contains(local.nodeId) || !localFresh.Contains(local.nodeId);
                Log(graph.name + ": both sides have a new result for " + key + ", keeping the " + (download ? "stored" : "local") + " one");
            }

            if (download)
            {
                auto answer = co_await pass.client.Request(HttpMethod::Get, base + "/result?key=" + AssetsLineClient::Encode(key), "", "", 300.0f);
                if (!answer.ok)
                {
                    failed.Add(local.nodeId);
                    if (answer.offline) { pass.offline = true; ok = false; break; }
                    continue;
                }

                String path = local.path;
                if (local.kind == "main")
                {
                    String ext;
                    serverExt.TryGetValue(key, ext);
                    ext = ext.StartsWith(".") ? ext.SubStr(1) : ext;
                    for (auto& other : MainExtensions())
                        o2FileSystem.FileDelete(PipelineExecutor::GetPreviewPath(id, local.nodeId, other));

                    path = PipelineExecutor::GetPreviewPath(id, local.nodeId, ext.IsEmpty() ? String("png") : ext);

                    // A single-part extract shows its part in the grid too
                    auto node = graph.FindNode(local.nodeId);
                    auto schema = node ? PipelineNodeRegistry::GetSchema(node->nodeType) : nullptr;
                    if (schema && schema->perPortRun && node->outputs.Count() == 1)
                        PipelineUtils::WriteFileBytes(PipelineExecutor::GetPortPreviewPath(id, local.nodeId, node->outputs[0].id, "png"), answer.body);
                }

                PipelineUtils::WriteFileBytes(path, answer.body);
                state.results[key] = { v, Stamp(path) };
                downloaded.Add(local.nodeId);
            }
            else
            {
                String ext = Extension(local.path);
                String query = "/result?key=" + AssetsLineClient::Encode(key) + "&ext=." + ext + "&rev=" + (String)state.rev;
                auto answer = co_await pass.client.Request(HttpMethod::Put, base + query, PipelineUtils::ReadFileBytes(local.path),
                                                           "application/octet-stream", 300.0f);
                if (!answer.ok)
                {
                    failed.Add(local.nodeId);
                    if (answer.offline) { pass.offline = true; ok = false; break; }
                    if (answer.code != "stale-rev")
                        Log(graph.name + ": could not upload " + key + ": " + answer.error);
                    continue;
                }

                auto newV = answer.json.IsObject() ? answer.json.FindMember("v") : nullptr;
                state.results[key] = { newV ? PipelineUtils::ValueToString(*newV) : String(), stamp };
                uploaded.Add(local.nodeId);
            }
        }

        // Source files: whichever side has one hands it to the other
        auto uploads = manifest.json.FindMember("uploads");
        for (auto& node : graph.nodes)
        {
            if (!ok || (node->nodeType != "sourceImage" && node->nodeType != "sourceAudio"))
                continue;

            String uploadId = node->GetConfigString("uploadId", "").Trimed(" \n\r\t");
            if (uploadId.IsEmpty())
                continue;

            String assetPath = node->GetConfigString("assetPath", "").Trimed(" \n\r\t");
            String localPath = assetPath.IsEmpty() ? PipelineUtils::GetUploadPath(uploadId) : pass.assetsPath + assetPath;
            auto stored = uploads && uploads->IsObject() ? uploads->FindMember(uploadId.Data()) : nullptr;
            bool remoteHas = stored && stored->IsBoolean() && (bool)*stored;
            if (!remoteHas && o2FileSystem.IsFileExist(localPath))
            {
                auto answer = co_await pass.client.Request(HttpMethod::Put, "/api/sync/uploads/" + AssetsLineClient::Encode(uploadId),
                                                           PipelineUtils::ReadFileBytes(localPath), "application/octet-stream", 300.0f);
                if (!answer.ok)
                    Log(graph.name + ": could not upload source " + uploadId + ": " + answer.error);
            }
            else if (remoteHas && assetPath.IsEmpty() && !o2FileSystem.IsFileExist(localPath))
            {
                auto answer = co_await pass.client.Request(HttpMethod::Get, "/api/uploads/" + AssetsLineClient::Encode(uploadId), "", "", 300.0f);
                if (answer.ok)
                    PipelineUtils::WriteFileBytes(localPath, answer.body);
            }
        }

        if (!ok)
            co_return false;

        // A key is settled when its last known state matches both sides now
        auto settled = [&](const String& nodeId)
        {
            if (failed.Contains(nodeId))
                return false;

            for (auto& kv : locals)
            {
                if (kv.second.nodeId != nodeId)
                    continue;

                String v;
                serverV.TryGetValue(kv.first, v);
                AssetsLineResultState known;
                bool has = state.results.TryGetValue(kv.first, known);
                String stamp = Stamp(kv.second.kind == "main" ? ExistingMainPreview(id, nodeId) : kv.second.path);
                if ((!stamp.IsEmpty() || !v.IsEmpty()) && !(has && SameStamp(known.stamp, stamp)))
                    return false;
            }

            return true;
        };

        // Up to date here: up to date there. A node generated again here is marked again, so the stored
        // generation there gives way to the new one
        Vector<String> markThere;
        for (auto& nodeId : localFresh)
        {
            if ((!serverFresh.Contains(nodeId) || uploaded.Contains(nodeId)) && settled(nodeId))
                markThere.Add(nodeId);
        }

        if (!markThere.IsEmpty())
        {
            DataDocument body;
            body.SetObject();
            body["rev"] = state.rev;
            auto& list = body["nodeIds"];
            list.SetArray();
            for (auto& nodeId : markThere)
                list.AddElement() = nodeId;

            auto answer = co_await pass.client.SendJson(HttpMethod::Post, base + "/fresh", body);
            if (!answer.ok && answer.offline)
            {
                pass.offline = true;
                co_return false;
            }
        }

        // Up to date there: up to date here, with the generation where a local run looks for it. A result that
        // came now is the node generated again there: it replaces the generation kept here
        bool changedHere = !downloaded.IsEmpty();
        for (auto& nodeId : serverFresh)
        {
            auto node = graph.FindNode(nodeId);
            String sig;
            bool arrived = downloaded.Contains(nodeId);
            if (!node || (localFresh.Contains(nodeId) && !arrived) || !sigs.TryGetValue(nodeId, sig) || !settled(nodeId))
                continue;

            SeedContent(id, graph, *node, sigs, seeds, arrived);
            PipelineExecutor::MarkRan(id, sig);
            changedHere = true;
        }

        // What reaches a finish node becomes a project asset, as a local run of it would make it
        if (pass.config.writeFinishAssets)
        {
            for (auto& node : graph.nodes)
            {
                if (!PipelineNodeRegistry::IsFinishType(node->nodeType))
                    continue;

                bool upstreamMoved = false;
                Vector<String> stack = { node->id };
                Vector<String> seen;
                while (!stack.IsEmpty() && !upstreamMoved)
                {
                    String current = stack.PopBack();
                    for (auto& edge : graph.GetIncomingEdges(current))
                    {
                        upstreamMoved = upstreamMoved || downloaded.Contains(edge->fromNodeId);
                        if (!seen.Contains(edge->fromNodeId))
                        {
                            seen.Add(edge->fromNodeId);
                            stack.Add(edge->fromNodeId);
                        }
                    }
                }

                if (downloaded.Contains(node->id))
                {
                    String preview = ExistingMainPreview(id, node->id);
                    String target = pass.assetsPath + PipelineNodeRegistry::GetFinishAssetPath(*node) + "." + Extension(preview);
                    if (!preview.IsEmpty() && o2FileSystem.FileCopy(preview, target))
                    {
                        pass.assetsChanged = true;
                        Log(graph.name + ": wrote " + target.SubStr(pass.assetsPath.Length()));
                    }
                }
                else if (upstreamMoved)
                {
                    auto executor = mmake<PipelineExecutor>();
                    executor->assetsPathOverride = pass.assetsPath;
                    executor->Execute(id, graph, node->id, {}, true);
                    while (executor->IsRunning())
                    {
                        co_await WaitTime(0.02f);
                        co_await SwitchToMain();
                    }

                    pass.assetsChanged = true;
                    changedHere = true;
                }
            }
        }

        if (changedHere && onResultsChanged)
            onResultsChanged(id);

        co_return true;
    }
}
