#include "o2Editor/stdafx.h"
#include "AssetsLineSyncInternal.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    namespace AssetsLineSyncUtils
    {
        bool ParseDocument(const String& text, PipelineGraph& graph)
        {
            DataDocument doc;
            if (!doc.LoadFromData(text) || !doc.IsObject())
                return false;

            if (PipelineGraph::IsLegacyDocument(doc))
            {
                graph = PipelineGraph();
                graph.Deserialize(*doc.FindMember("graph"));
                return true;
            }

            return ParseDocument(static_cast<const DataValue&>(doc), graph);
        }

        bool ParseDocument(const DataValue& json, PipelineGraph& graph)
        {
            if (!graph.LoadFromJson(json))
                return false;

            for (auto key : { "rev", "updatedAt" })
            {
                if (graph.extra.IsObject() && graph.extra.FindMember(key))
                    graph.extra.RemoveMember(key);
            }

            return true;
        }

        void RemapAllIds(PipelineGraph& graph)
        {
            Map<String, String> nodeIds;
            Map<String, String> portIds;
            for (auto& node : graph.nodes)
            {
                String id = PipelineNode::GenerateId();
                nodeIds[node->id] = id;
                node->id = id;
                for (auto& port : node->inputs) { String pid = PipelineNode::GenerateId(); portIds[port.id] = pid; port.id = pid; }
                for (auto& port : node->outputs) { String pid = PipelineNode::GenerateId(); portIds[port.id] = pid; port.id = pid; }
                node->RemapConfigPortIds(portIds);
            }

            auto mapped = [](const Map<String, String>& map, const String& id) { String v; return map.TryGetValue(id, v) ? v : id; };
            for (auto& edge : graph.edges)
            {
                edge->id = PipelineNode::GenerateId();
                edge->fromNodeId = mapped(nodeIds, edge->fromNodeId);
                edge->toNodeId = mapped(nodeIds, edge->toNodeId);
                edge->fromPortId = mapped(portIds, edge->fromPortId);
                edge->toPortId = mapped(portIds, edge->toPortId);
            }
        }

        String SafeFileName(const String& name)
        {
            String result;
            for (int i = 0; i < name.Length(); i++)
            {
                char c = name[i];
                result += (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') ? '_' : c;
            }

            result = result.Trimed(" .");
            return result.IsEmpty() ? String("pipeline") : result;
        }

        String Stamp(const String& path)
        {
            if (!o2FileSystem.IsFileExist(path))
                return String();

            // Edit times have whole seconds: a small file rewritten at once with the same size differs only in content
            const int hashedSize = 256*1024;
            String stamp = PipelineUtils::FileSignature(path);
            if (o2FileSystem.GetFileInfo(path).size <= hashedSize)
                stamp += "#" + PipelineUtils::Fnv1a64Hex(PipelineUtils::ReadFileBytes(path));

            return stamp;
        }

        bool SameStamp(const String& known, const String& now)
        {
            if (known.Find("#") < 0)
            {
                int hash = now.Find("#");
                return known == (hash < 0 ? now : now.SubStr(0, hash));
            }

            return known == now;
        }

        bool MoveWithMeta(const String& from, const String& to)
        {
            o2FileSystem.FolderCreate(o2FileSystem.GetParentPath(to), true);
            if (!o2FileSystem.FileMove(from, to))
                return false;

            if (o2FileSystem.IsFileExist(from + ".meta"))
                o2FileSystem.FileMove(from + ".meta", to + ".meta");

            return true;
        }

        void DeleteWithMeta(const String& path)
        {
            o2FileSystem.FileDelete(path);
            if (o2FileSystem.IsFileExist(path + ".meta"))
                o2FileSystem.FileDelete(path + ".meta");
        }

        String MetaUid(const String& path)
        {
            DataDocument meta;
            if (!meta.LoadFromFile(path + ".meta") || !meta.IsObject())
                return "";

            auto value = meta.FindMember("Value");
            auto id = value && value->IsObject() ? value->FindMember("mId") : nullptr;
            return id ? PipelineUtils::ValueToString(*id) : String();
        }
    }

    using namespace AssetsLineSyncUtils;

    static void CollectPipelineFiles(const FolderInfo& folder, Vector<String>& files)
    {
        for (auto& file : folder.files)
        {
            if (file.path.EndsWith(".pipeline"))
                files.Add(file.path);
        }

        for (auto& sub : folder.folders)
            CollectPipelineFiles(sub, files);
    }

    static String Relative(const String& assetsPath, String path)
    {
        path.ReplaceAll("\\", "/");
        while (path.Contains("//"))
            path.ReplaceAll("//", "/");

        String root = assetsPath;
        while (root.Contains("//"))
            root.ReplaceAll("//", "/");

        return path.StartsWith(root) ? path.SubStr(root.Length()) : path;
    }

    String AssetsLineSync::ContentText(const PipelineGraph& graph)
    {
        PipelineGraph content = graph;
        content.cameraPosition = Vec2F();
        content.cameraScale = 1.0f;
        return content.ToJsonString();
    }

    Map<String, AssetsLineSync::LocalPipeline> AssetsLineSync::ScanLocal(Pass& pass)
    {
        Map<String, LocalPipeline> result;
        String root = pass.assetsPath + pass.folder;
        if (!o2FileSystem.IsFolderExist(root))
            return result;

        Vector<String> files;
        CollectPipelineFiles(o2FileSystem.GetFolderInfo(root), files);
        for (auto& file : files)
            file = Relative(pass.assetsPath, file);

        files.Sort([](const String& a, const String& b) { return strcmp(a.Data(), b.Data()) < 0; });

        struct Read
        {
            String        file;
            String        stamp;
            PipelineGraph graph;
            bool          write = false;
        };

        Vector<Read> reads;
        for (auto& file : files)
        {
            Read read;
            read.file = file;
            String full = pass.assetsPath + file;
            read.stamp = Stamp(full);
            if (!ParseDocument(PipelineUtils::ReadFileBytes(full), read.graph))
            {
                Log("skipped " + file + ": not a pipeline document");
                continue;
            }

            // A pipeline without an id is keyed like the editor keys its results: by the asset
            if (read.graph.id.IsEmpty())
            {
                read.graph.id = MetaUid(full);
                if (read.graph.id.IsEmpty())
                {
                    read.graph.id = PipelineNode::GenerateId();
                    read.write = true;
                }
            }

            read.graph.name = o2FileSystem.GetFileNameWithoutExtension(o2FileSystem.GetPathWithoutDirectories(file));
            reads.Add(read);
        }

        // A copied file carries its original's id; the one the sync knows keeps it, a copy gets new ids
        Map<String, int> keeper;
        for (int i = 0; i < reads.Count(); i++)
        {
            auto& id = reads[i].graph.id;
            auto known = mPipelines.find(id);
            bool isKnownFile = known != mPipelines.end() && known->second.file == reads[i].file;
            if (!keeper.ContainsKey(id) || isKnownFile)
                keeper[id] = i;
        }

        for (int i = 0; i < reads.Count(); i++)
        {
            auto& read = reads[i];
            if (keeper[read.graph.id] != i)
            {
                RemapAllIds(read.graph);
                read.graph.id = PipelineNode::GenerateId();
                read.write = true;
                Log(read.file + " is a copy: it got ids of its own");
            }

            if (AssignAssetUploadIds(read.graph))
                read.write = true;

            if (read.write)
            {
                if (isPipelineFileBusy && isPipelineFileBusy(read.file))
                {
                    pass.busyIds.Add(read.graph.id);
                    continue;
                }

                PipelineUtils::WriteFileBytes(pass.assetsPath + read.file, read.graph.ToJsonString());
                read.stamp = Stamp(pass.assetsPath + read.file);
                pass.assetsChanged = true;
                if (onPipelineFileChanged)
                    onPipelineFileChanged(read.file);
            }

            LocalPipeline local;
            local.file = read.file;
            local.stamp = read.stamp;
            local.graph = read.graph;
            local.content = ContentText(read.graph);
            result[read.graph.id] = local;
        }

        return result;
    }

    bool AssetsLineSync::WriteLocal(Pass& pass, const String& id, const PipelineGraph& graph, const String& currentFile,
                                    String& writtenFile)
    {
        String name = SafeFileName(graph.name);
        String dir = currentFile.IsEmpty() ? pass.folder : o2FileSystem.GetParentPath(currentFile);
        if (!dir.IsEmpty() && !dir.EndsWith("/"))
            dir += "/";

        String desired = dir + name + ".pipeline";
        if (!currentFile.IsEmpty() &&
            o2FileSystem.GetFileNameWithoutExtension(o2FileSystem.GetPathWithoutDirectories(currentFile)) == name)
            desired = currentFile;

        for (int i = 2; desired != currentFile && o2FileSystem.IsFileExist(pass.assetsPath + desired); i++)
            desired = dir + name + " " + (String)i + ".pipeline";

        if (!currentFile.IsEmpty() && desired != currentFile && o2FileSystem.IsFileExist(pass.assetsPath + currentFile))
        {
            if (!MoveWithMeta(pass.assetsPath + currentFile, pass.assetsPath + desired))
            {
                pass.error = "could not rename " + currentFile + " to " + desired;
                return false;
            }

            Log("renamed " + currentFile + " to " + desired);
            if (onPipelineFileChanged)
                onPipelineFileChanged(currentFile);
        }

        if (!PipelineUtils::WriteFileBytes(pass.assetsPath + desired, graph.ToJsonString()))
        {
            pass.error = "could not write " + desired;
            return false;
        }

        writtenFile = desired;
        pass.assetsChanged = true;
        if (onPipelineFileChanged)
            onPipelineFileChanged(desired);

        return true;
    }

    static void Fail(AssetsLineSync::Pass& pass, const AssetsLineResponse& response, const String& what)
    {
        if (response.offline)
            pass.offline = true;

        if (pass.error.IsEmpty())
            pass.error = what + ": " + response.error;
    }

    Coroutine<bool> AssetsLineSync::PushDocument(Pass& pass, LocalPipeline& local, AssetsLinePipelineState* state)
    {
        Ref<AssetsLineSync> self(this);
        String id = local.graph.id;
        String path = "/api/sync/pipelines/" + AssetsLineClient::Encode(id);

        auto setBase = [&](DataDocument& body)
        {
            PipelineGraph base;
            String text = state ? ReadBase(id) : String();
            if (!text.IsEmpty() && ParseDocument(text, base))
                base.SaveToJson(body["base"]);
            else
                body["base"].SetNull();
        };

        DataDocument body;
        body.SetObject();
        local.graph.SaveToJson(body["pipeline"]);
        if (state)
        {
            body["baseRev"] = state->rev;
            if (!state->baseIsServer)
                setBase(body);
        }

        auto response = co_await pass.client.SendJson(HttpMethod::Put, path, body);
        if (response.code == "rev-mismatch")
        {
            setBase(body);
            response = co_await pass.client.SendJson(HttpMethod::Put, path, body);
        }

        // Another stored pipeline has this name: this one takes a free name, here and there
        Vector<String> tried;
        for (auto& kv : pass.remoteName)
            tried.Add(kv.second.ToLowerCase());

        String takenName = local.graph.name;
        for (int attempt = 2; response.code == "name-taken" && attempt < 20; attempt++)
        {
            String candidate = takenName + " (o2)";
            for (int i = 2; tried.Contains(candidate.ToLowerCase()); i++)
                candidate = takenName + " (o2 " + (String)i + ")";

            tried.Add(candidate.ToLowerCase());
            PipelineGraph renamed = local.graph;
            renamed.name = candidate;
            String written;
            if (!WriteLocal(pass, id, renamed, local.file, written))
                co_return false;

            local.graph.name = candidate;
            local.file = written;
            local.stamp = Stamp(pass.assetsPath + written);
            local.graph.SaveToJson(body["pipeline"]);
            response = co_await pass.client.SendJson(HttpMethod::Put, path, body);
        }

        if (!response.ok || !response.json.IsObject() || !response.json.FindMember("pipeline"))
        {
            Fail(pass, response, "saving " + local.graph.name);
            co_return false;
        }

        auto& storedJson = response.json["pipeline"];
        auto rev = storedJson.FindMember("rev");
        auto merged = response.json.FindMember("merged");
        bool wasMerged = merged && merged->IsBoolean() && (bool)*merged;

        PipelineGraph stored;
        ParseDocument(storedJson, stored);
        stored.cameraPosition = local.graph.cameraPosition;
        stored.cameraScale = local.graph.cameraScale;

        AssetsLinePipelineState next = state ? *state : AssetsLinePipelineState();
        next.rev = rev && rev->IsNumber() ? (int)*rev : 0;
        next.name = stored.name;
        next.file = local.file;

        bool untouched = Stamp(pass.assetsPath + local.file) == local.stamp &&
            !(isPipelineFileBusy && isPipelineFileBusy(local.file));
        if (!wasMerged)
        {
            next.baseIsServer = true;
            WriteBase(id, local.graph.ToJsonString());
        }
        else if (untouched)
        {
            String written;
            if (!WriteLocal(pass, id, stored, local.file, written))
                co_return false;

            next.file = written;
            next.baseIsServer = true;
            WriteBase(id, stored.ToJsonString());
            local.graph = stored;
            local.file = written;
            local.stamp = Stamp(pass.assetsPath + written);
            local.content = ContentText(stored);
            Log("merged " + stored.name + " with changes made elsewhere");
        }
        else
        {
            // Edited again while saving: the next push is relative to what was sent
            next.baseIsServer = false;
            WriteBase(id, local.graph.ToJsonString());
        }

        mPipelines[id] = next;
        pass.remoteRev[id] = next.rev;
        pass.remoteName[id] = next.name;
        pass.changedIds.Add(id);
        Log("pushed " + next.name + " (rev " + (String)next.rev + ")");
        co_return true;
    }

    Coroutine<bool> AssetsLineSync::PullDocument(Pass& pass, const String& id, LocalPipeline* local)
    {
        Ref<AssetsLineSync> self(this);
        auto response = co_await pass.client.Request(HttpMethod::Get, "/api/sync/pipelines/" + AssetsLineClient::Encode(id));
        if (!response.ok || !response.json.IsObject() || !response.json.FindMember("pipeline"))
        {
            Fail(pass, response, "reading " + pass.remoteName[id]);
            co_return false;
        }

        auto& storedJson = response.json["pipeline"];
        auto rev = storedJson.FindMember("rev");
        PipelineGraph graph;
        if (!ParseDocument(storedJson, graph))
        {
            pass.error = "the stored " + pass.remoteName[id] + " is not a pipeline";
            co_return false;
        }

        String currentFile;
        if (local)
        {
            // Edited here since the scan: the next pass pushes and the server merges
            if (Stamp(pass.assetsPath + local->file) != local->stamp || (isPipelineFileBusy && isPipelineFileBusy(local->file)))
            {
                pass.busyIds.Add(id);
                co_return true;
            }

            graph.cameraPosition = local->graph.cameraPosition;
            graph.cameraScale = local->graph.cameraScale;
            currentFile = local->file;
        }

        String written;
        if (!WriteLocal(pass, id, graph, currentFile, written))
            co_return false;

        AssetsLinePipelineState next;
        if (auto known = mPipelines.find(id); known != mPipelines.end())
            next = known->second;

        next.rev = rev && rev->IsNumber() ? (int)*rev : 0;
        next.name = graph.name;
        next.file = written;
        next.baseIsServer = true;
        mPipelines[id] = next;
        WriteBase(id, graph.ToJsonString());

        LocalPipeline pulled;
        pulled.file = written;
        pulled.stamp = Stamp(pass.assetsPath + written);
        pulled.graph = graph;
        pulled.content = ContentText(graph);
        pass.local[id] = pulled;
        pass.changedIds.Add(id);
        Log("pulled " + graph.name + " (rev " + (String)next.rev + ")");
        co_return true;
    }

    Coroutine<bool> AssetsLineSync::SyncDocuments(Pass& pass)
    {
        Ref<AssetsLineSync> self(this);
        pass.local = ScanLocal(pass);

        Vector<String> ids;
        for (auto& kv : pass.local) ids.Add(kv.first);
        for (auto& kv : pass.remoteRev) if (!ids.Contains(kv.first)) ids.Add(kv.first);
        for (auto& kv : mPipelines) if (!ids.Contains(kv.first)) ids.Add(kv.first);

        bool ok = true;
        for (auto& id : ids)
        {
            if (pass.busyIds.Contains(id))
                continue;

            auto localIt = pass.local.find(id);
            LocalPipeline* local = localIt != pass.local.end() ? &localIt->second : nullptr;
            bool remote = pass.remoteRev.ContainsKey(id);
            auto stateIt = mPipelines.find(id);
            AssetsLinePipelineState* state = stateIt != mPipelines.end() ? &stateIt->second : nullptr;

            if (local && isPipelineFileBusy && isPipelineFileBusy(local->file))
            {
                pass.busyIds.Add(id);
                continue;
            }

            String baseContent;
            if (state)
            {
                PipelineGraph base;
                if (ParseDocument(ReadBase(id), base))
                    baseContent = ContentText(base);
            }

            bool stepOk = true;
            if (local && remote)
            {
                bool localChanged = !state || local->content != baseContent;
                bool remoteChanged = state && pass.remoteRev[id] != state->rev;
                if (localChanged)
                    stepOk = co_await PushDocument(pass, *local, state);
                else if (remoteChanged)
                    stepOk = co_await PullDocument(pass, id, local);
                else if (state->file != local->file)
                    state->file = local->file;
            }
            else if (local && !remote)
            {
                if (state && local->content == baseContent)
                {
                    // Deleted in AssetsLine, untouched here
                    DeleteWithMeta(pass.assetsPath + local->file);
                    pass.assetsChanged = true;
                    if (onPipelineFileChanged)
                        onPipelineFileChanged(local->file);

                    Log("removed " + local->file + ": the pipeline was deleted in AssetsLine");
                    mPipelines.Remove(id);
                    WriteBase(id, "");
                    pass.local.Remove(id);
                }
                else
                {
                    // New here, or edited here after it was deleted there: it is stored again
                    if (state)
                        mPipelines.Remove(id);

                    stepOk = co_await PushDocument(pass, *local, nullptr);
                }
            }
            else if (!local && remote)
            {
                if (state && pass.remoteRev[id] == state->rev)
                {
                    // Deleted here, untouched there
                    auto response = co_await pass.client.Request(
                        HttpMethod::Delete, "/api/sync/pipelines/" + AssetsLineClient::Encode(id) + "?baseRev=" + (String)state->rev);
                    if (response.ok)
                    {
                        Log("deleted " + state->name + " in AssetsLine");
                        mPipelines.Remove(id);
                        WriteBase(id, "");
                    }
                    else if (response.code == "changed")
                        stepOk = co_await PullDocument(pass, id, nullptr);
                    else
                    {
                        Fail(pass, response, "deleting " + state->name);
                        stepOk = false;
                    }
                }
                else
                    stepOk = co_await PullDocument(pass, id, nullptr);
            }
            else if (state)
            {
                mPipelines.Remove(id);
                WriteBase(id, "");
            }

            if (!stepOk)
            {
                ok = false;
                if (pass.offline)
                    break;
            }
        }

        co_return ok;
    }
}
