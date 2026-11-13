#include "o2Editor/stdafx.h"
#include "AssetsLineSyncInternal.h"

#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/PipelineZip.h"

#include <filesystem>

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
            String kind;  // main, src, part, raw or trace
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

        bool IsPngPath(const String& path)
        {
            return Extension(path) == "png";
        }

        String ReadHead(const String& path, int count)
        {
            InFile file(path);
            if (!file.IsOpened())
                return String();

            String head;
            head.resize(Math::Min<int>(count, (int)file.GetDataSize()));
            if (!head.IsEmpty())
                file.ReadData(&head[0], (UInt)head.Length());
            return head;
        }

        // A picture kept under a .png name in another format (AssetsLine used to store WebP answers so) becomes a PNG;
        // true when the file was rewritten
        bool RepairPng(const String& path)
        {
            if (path.IsEmpty() || !IsPngPath(path) || !o2FileSystem.IsFileExist(path) || IsPngBytes(ReadHead(path, 8)))
                return false;

            String png = ToPngBytes(PipelineUtils::ReadFileBytes(path));
            if (png.IsEmpty())
                return false;

            PipelineUtils::WriteFileBytes(path, png);
            return true;
        }

        // Result keys of the graph and their local files; chroma nodes add their raw renders in the content cache
        // The folder of a node's debug steps
        String TraceFolder(const String& id, const String& nodeId)
        {
            return PipelineExecutor::GetWorkPath(id, nodeId) + "trace/";
        }

        // The trace folder as one zip: its files, stored (the pictures are PNGs already)
        String ZipTrace(const String& folder)
        {
            Vector<PipelineZip::Entry> entries;
            std::error_code ec;
            for (auto& item : std::filesystem::directory_iterator(folder.Data(), ec))
            {
                if (!item.is_regular_file(ec))
                    continue;

                String name = item.path().filename().string().c_str();
                entries.Add({ name, PipelineUtils::ReadFileBytes(folder + name) });
            }
            return PipelineZip::Write(entries, false);
        }

        // Replaces the trace folder with the files of the zip, plain names only; false when the zip is not a trace
        bool UnzipTrace(const String& folder, const String& zip)
        {
            Vector<PipelineZip::Entry> entries;
            String error;
            if (!PipelineZip::Read(zip, entries, error))
                return false;

            bool hasTrace = false;
            for (auto& entry : entries)
            {
                if (entry.name.IsEmpty() || entry.name.Contains("/") || entry.name.Contains("\\") || entry.name.StartsWith("."))
                    return false;
                hasTrace = hasTrace || entry.name == "trace.json";
            }
            if (!hasTrace)
                return false;

            String dir = folder;
            while (dir.EndsWith("/"))
                dir = dir.SubStr(0, dir.Length() - 1);
            String fresh = dir + ".in", old = dir + ".old";
            std::error_code ec;
            std::filesystem::remove_all(fresh.Data(), ec);
            std::filesystem::create_directories(fresh.Data(), ec);
            for (auto& entry : entries)
                PipelineUtils::WriteFileBytes(fresh + "/" + entry.name, entry.data);
            std::filesystem::remove_all(old.Data(), ec);
            std::filesystem::rename(dir.Data(), old.Data(), ec);
            std::filesystem::rename(fresh.Data(), dir.Data(), ec);
            std::filesystem::remove_all(old.Data(), ec);
            return o2FileSystem.IsFileExist(dir + "/trace.json");
        }

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

                // A dynamic outputs node keeps every output but the first as a part, the text one as .txt, and its
                // debug steps (AI image to UI's trace folder) as one more key
                if (schema && schema->dynamicOutputs)
                {
                    results[node->id + ".trace"] = { node->id, "", "trace", TraceFolder(id, node->id) + "trace.json" };
                    for (int i = 1; i < node->outputs.Count(); i++)
                    {
                        auto& port = node->outputs[i];
                        String ext = port.portType == PipelinePortType::Text ? "txt" : "png";
                        results[node->id + "#" + port.id] = { node->id, port.id, "part", PipelineExecutor::GetPortPreviewPath(id, node->id, port.id, ext) };
                    }
                    continue;
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

        // The outputs of a dynamic outputs node as they are here (main preview and one file per other output) become the bundle
        // a local run of it would have stored, so the editor shows the run and can export it without asking the models again
        void SeedBundle(const String& id, const PipelineNode& node, const String& sig, bool replace)
        {
            Vector<Pair<String, PipelineValue>> existing;
            if (!replace && PipelineExecutor::LoadBundle(id, sig, existing))
                return;

            if (node.outputs.IsEmpty())
                return;

            String main = ExistingMainPreview(id, node.id);
            if (main.IsEmpty())
                return;

            Vector<Pair<String, PipelineValue>> values;
            auto& first = node.outputs[0];
            values.Add({ first.name, PipelineExecutor::LoadPreview(id, node) });
            for (int i = 1; i < node.outputs.Count(); i++)
            {
                auto& port = node.outputs[i];
                PipelineValue value = PipelineExecutor::LoadPortPreview(id, node.id, port.id, port.portType);
                if (!value.IsValid())
                    return;
                values.Add({ port.name, value });
            }

            if (!values[0].second.IsValid())
                return;

            PipelineExecutor::SaveBundle(id, sig, values);
        }

        // A local run then reuses the downloaded generation instead of asking the model again. A node generated
        // again keeps its signature, so with replace a different stored generation gives way to the downloaded one
        void SeedContent(const String& id, const PipelineGraph& graph, const PipelineNode& node,
                         const Map<String, String>& sigs, const Map<String, int>& seeds, bool replace)
        {
            if (PipelineNodeRegistry::IsFinishType(node.nodeType))
                return;

            auto schema = PipelineNodeRegistry::GetSchema(node.nodeType);
            String nodeSig;
            if (schema && schema->dynamicOutputs && sigs.TryGetValue(node.id, nodeSig))
            {
                SeedBundle(id, node, nodeSig, replace);
                return;
            }

            // A chroma render is cached raw, before the cut the preview shows: it comes as its own ".raw" result
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

        // A repaired file stays in sync when it was in sync before: only its format changed, not the result
        bool repaired = false;
        {
            auto& state = mPipelines[id];
            for (auto& kv : locals)
            {
                String before = kv.second.path.IsEmpty() ? String() : Stamp(kv.second.path);
                if (!RepairPng(kv.second.path))
                    continue;

                repaired = true;
                Log(graph.name + ": converted the result " + kv.first + " to PNG");
                AssetsLineResultState known;
                if (state.results.TryGetValue(kv.first, known) && SameStamp(known.stamp, before))
                    state.results[kv.first].stamp = Stamp(kv.second.path);
            }

            if (repaired && onResultsChanged)
                onResultsChanged(id);
        }

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

        // Debug steps first: AssetsLine reads a run's summary from them when the result arrives
        Vector<String> keys;
        for (auto& kv : locals) if (kv.second.kind == "trace") keys.Add(kv.first);
        for (auto& kv : locals) if (kv.second.kind != "trace") keys.Add(kv.first);
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

            // Debug steps go up once the node's run here is over and up to date, never half a run
            if (local.kind == "trace" && !download && !localFresh.Contains(local.nodeId))
                continue;

            if (download)
            {
                auto answer = co_await pass.client.Request(HttpMethod::Get, base + "/result?key=" + AssetsLineClient::Encode(key), "", "", 300.0f);
                if (!answer.ok)
                {
                    if (local.kind != "trace")
                        failed.Add(local.nodeId);
                    if (answer.offline) { pass.offline = true; ok = false; break; }
                    continue;
                }

                if (local.kind == "trace")
                {
                    if (UnzipTrace(TraceFolder(id, local.nodeId), answer.body))
                        state.results[key] = { v, Stamp(local.path) };
                    else
                        Log(graph.name + ": the debug steps of " + local.nodeId + " could not be read");
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
                }

                String body = answer.body;
                if (IsPngPath(path) && !IsPngBytes(body))
                {
                    String png = ToPngBytes(body);
                    if (png.IsEmpty())
                        Log(graph.name + ": the result " + key + " is not an image the editor can read");
                    else
                        body = png;
                }

                // A single-part extract shows its part in the grid too
                if (local.kind == "main")
                {
                    auto node = graph.FindNode(local.nodeId);
                    auto schema = node ? PipelineNodeRegistry::GetSchema(node->nodeType) : nullptr;
                    if (schema && schema->perPortRun && node->outputs.Count() == 1)
                        PipelineUtils::WriteFileBytes(PipelineExecutor::GetPortPreviewPath(id, local.nodeId, node->outputs[0].id, "png"), body);
                }

                PipelineUtils::WriteFileBytes(path, body);
                state.results[key] = { v, Stamp(path) };
                downloaded.Add(local.nodeId);
            }
            else
            {
                bool trace = local.kind == "trace";
                String ext = trace ? String("zip") : Extension(local.path);
                String query = "/result?key=" + AssetsLineClient::Encode(key) + "&ext=." + ext + "&rev=" + (String)state.rev;
                String bytes = trace ? ZipTrace(TraceFolder(id, local.nodeId)) : PipelineUtils::ReadFileBytes(local.path);
                auto answer = co_await pass.client.Request(HttpMethod::Put, base + query, bytes, "application/octet-stream", 300.0f);
                if (!answer.ok)
                {
                    if (!trace)
                        failed.Add(local.nodeId);
                    if (answer.offline) { pass.offline = true; ok = false; break; }
                    if (answer.code != "stale-rev")
                    {
                        Log(graph.name + ": could not upload " + key + ": " + answer.error);

                        // A server without debug steps refuses them: not again until the next run here
                        if (trace)
                            state.results[key] = { known.v, stamp };
                    }
                    continue;
                }

                auto newV = answer.json.IsObject() ? answer.json.FindMember("v") : nullptr;
                state.results[key] = { newV ? PipelineUtils::ValueToString(*newV) : String(), stamp };
                if (!trace)
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
                // Debug steps never hold a node back
                if (kv.second.nodeId != nodeId || kv.second.kind == "trace")
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
