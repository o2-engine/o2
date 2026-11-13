#include "o2Editor/stdafx.h"
#include "PipelineExecutor.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    bool PipelineExecutor::RanExists(const String& pipelineId, const String& sig)
    {
        return o2FileSystem.IsFileExist(GetRanMarkerPath(pipelineId, sig));
    }

    void PipelineExecutor::MarkRan(const String& pipelineId, const String& sig)
    {
        PipelineUtils::WriteFileBytes(GetRanMarkerPath(pipelineId, sig), "1");
    }

    void PipelineExecutor::ClearNodeCache(const String& pipelineId, const PipelineNode& node, const String& sig)
    {
        for (auto& ext : Vector<String>{ "png", "txt", "mp4", "mp3", "wav", "ogg", "m4a", "flac" })
            o2FileSystem.FileDelete(GetPreviewPath(pipelineId, node.id, ext));

        o2FileSystem.FileDelete(GetSourcePreviewPath(pipelineId, node.id));
        if (!sig.IsEmpty())
            o2FileSystem.FileDelete(GetRanMarkerPath(pipelineId, sig));

        auto schema = PipelineNodeRegistry::GetSchema(node.nodeType);
        if (schema && schema->dynamicOutputs)
            RemoveStalePortPreviews(pipelineId, node.id, {});
    }

    Vector<String> PipelineExecutor::ComputeFreshNodes(const String& pipelineId, const PipelineGraph& graph)
    {
        Vector<String> res;
        auto sigs = graph.ComputeSignatures();
        for (auto& kv : sigs)
        {
            if (!kv.first.Contains("#") && RanExists(pipelineId, kv.second))
                res.Add(kv.first);
        }
        return res;
    }

    PipelineValue PipelineExecutor::LoadContent(const String& pipelineId, const String& sig, PipelinePortType type)
    {
        return LoadValueFile(GetCachePath(pipelineId) + "content/" + sig, type);
    }

    void PipelineExecutor::SaveContent(const String& pipelineId, const String& sig, const PipelineValue& value)
    {
        PipelineUtils::WriteFileBytes(GetContentPath(pipelineId, sig, value.GetExtension()), value.data);
    }

    void PipelineExecutor::DeleteContent(const String& pipelineId, const String& sig)
    {
        for (auto& ext : Vector<String>{ "png", "txt", "mp4", "mp3", "wav", "ogg", "m4a", "flac" })
            o2FileSystem.FileDelete(GetContentPath(pipelineId, sig, ext));
    }

    void PipelineExecutor::AttachWork(PipelineExecContext& ctx, const String& pipelineId, const String& nodeId, bool fresh)
    {
        ctx.workDir = GetWorkPath(pipelineId, nodeId);
        ctx.fresh = fresh;
        ctx.memoGet = [pipelineId](const String& key, const String& ext, String& data)
        {
            String path = GetContentPath(pipelineId, key, ext.StartsWith(".") ? ext.SubStr(1) : ext);
            if (!o2FileSystem.IsFileExist(path))
                return false;
            data = PipelineUtils::ReadFileBytes(path);
            return !data.IsEmpty();
        };
        ctx.memoPut = [pipelineId](const String& key, const String& ext, const String& data)
        {
            PipelineUtils::WriteFileBytes(GetContentPath(pipelineId, key, ext.StartsWith(".") ? ext.SubStr(1) : ext), data);
        };
    }

    String PipelineExecutor::RefineSignature(const String& cacheSig, const String& key, const PipelineValue& stored)
    {
        static const char* hex = "0123456789abcdef";
        String digest = PipelineUtils::Sha256(cacheSig + "|" + key + "|" + stored.data), out;
        for (int i = 0; i < 16; i++)
        {
            unsigned char c = (unsigned char)digest[i];
            out += hex[c >> 4];
            out += hex[c & 15];
        }
        return out;
    }

    Coroutine<PipelineExecutor::Refined> PipelineExecutor::RefineStored(Ref<Run> run, Ref<PipelineNode> node,
                                                                        Map<String, PipelineValue> inputs, String cacheSig,
                                                                        String portId, String portName, PipelineValue stored,
                                                                        bool restart)
    {
        Refined res;
        res.value = stored;

        // A picture's generation is bytes; a text style's is its JSON
        auto impl = PipelineNodeRegistry::Get(node->nodeType);
        String key = impl && stored.IsValid() ? impl->RefineKey(*node, portId) : String();
        if (key.IsEmpty() || (stored.IsTextLike() && stored.type != PipelinePortType::TextStyle))
            co_return res;

        String nodeId = node->id;
        String label = node->nodeType + (portName.IsEmpty() ? String() : " . " + portName);
        String refineSig = RefineSignature(cacheSig, key, stored);
        if (restart)
            DeleteContent(run->pipelineId, refineSig);

        auto ctx = MakeContext(run, nodeId);
        ctx->outputPortId = portId;
        ctx->outputPort = portName;
        AttachWork(*ctx, run->pipelineId, nodeId, restart);

        PipelineValue cached = LoadContent(run->pipelineId, refineSig, stored.type);
        if (cached.IsValid())
        {
            EmitLog(label + ": check cache hit (" + refineSig.SubStr(0, 8) + ")");
            impl->RefineReplayed(*ctx, *node, refineSig);
            res.value = cached;
            co_return res;
        }

        // A cached-only run (the editor's own re-apply: a slider moved, a part re-tagged) never pays for the step: the stored
        // generation stands as it is, and the node is not marked as run - the step is still owed to a real run
        if (run->cachedOnly)
        {
            EmitLog(label + ": check not run yet - the stored generation as it is");
            res.owed = true;
            co_return res;
        }

        WeakRef<PipelineExecutor> self(this);
        ctx->phase = [self, nodeId, portId](const String& name)
        {
            if (auto s = self.Lock())
                s->EmitState(nodeId, "running", "", portId, name);
        };

        PipelineRefineResult refined = co_await impl->Refine(ctx, inputs, node, stored, refineSig);
        if (run->cancelled)
            co_return res;

        if (refined.value.IsValid())
            res.value = refined.value;
        if (refined.keep && refined.value.IsValid())
            SaveContent(run->pipelineId, refineSig, refined.value);

        res.computed = true;
        co_return res;
    }
}
