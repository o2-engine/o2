#include "o2Editor/stdafx.h"
#include "PipelineExecutor.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    PipelineExecutor::PipelineExecutor()
    {}

    PipelineExecutor::PipelineExecutor(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    const Vector<String>& PipelineExecutor::GetAudioExtensions()
    {
        static Vector<String> exts = { "mp3", "wav", "ogg", "m4a", "flac" };
        return exts;
    }

    String PipelineExecutor::GetCachePath(const String& pipelineId)
    {
        return PipelineUtils::GetWorkPath() + "cache/" + pipelineId + "/";
    }

    String PipelineExecutor::GetPreviewPath(const String& pipelineId, const String& nodeId, const String& ext)
    {
        return GetCachePath(pipelineId) + "previews/" + nodeId + "." + ext;
    }

    String PipelineExecutor::GetSourcePreviewPath(const String& pipelineId, const String& nodeId)
    {
        return GetCachePath(pipelineId) + "previews/" + nodeId + ".src.png";
    }

    String PipelineExecutor::GetContentPath(const String& pipelineId, const String& sig, const String& ext)
    {
        return GetCachePath(pipelineId) + "content/" + sig + "." + ext;
    }

    String PipelineExecutor::GetRanMarkerPath(const String& pipelineId, const String& sig)
    {
        return GetCachePath(pipelineId) + "ran/" + sig;
    }

    static Vector<String> ExtensionsFor(PipelinePortType type)
    {
        switch (type)
        {
            case PipelinePortType::Image: return { "png" };
            case PipelinePortType::Video: return { "mp4" };
            case PipelinePortType::Audio: return PipelineExecutor::GetAudioExtensions();
            default: return { "txt" };
        }
    }

    PipelineValue PipelineExecutor::LoadValueFile(const String& pathWithoutExt, PipelinePortType type, String* pathOut /*= nullptr*/)
    {
        for (auto& ext : ExtensionsFor(type))
        {
            String path = pathWithoutExt + "." + ext;
            if (!o2FileSystem.IsFileExist(path))
                continue;

            String bytes = PipelineUtils::ReadFileBytes(path);
            if (pathOut)
                *pathOut = path;

            if (type == PipelinePortType::Text)
                return PipelineValue::Text(bytes);

            return PipelineValue::Bytes(type, bytes, PipelineUtils::MimeForExtension(ext));
        }

        return PipelineValue();
    }

    static PipelinePortType PreviewTypeOf(const PipelineNode& node)
    {
        if (PipelineNodeRegistry::IsFinishType(node.nodeType))
            return node.inputs.IsEmpty() ? PipelinePortType::Text : node.inputs[0].portType;

        return node.outputs.IsEmpty() ? PipelinePortType::Text : node.outputs[0].portType;
    }

    PipelineValue PipelineExecutor::LoadPreview(const String& pipelineId, const PipelineNode& node, String* pathOut /*= nullptr*/)
    {
        return LoadValueFile(GetCachePath(pipelineId) + "previews/" + node.id, PreviewTypeOf(node), pathOut);
    }

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
    }

    Vector<String> PipelineExecutor::ComputeFreshNodes(const String& pipelineId, const PipelineGraph& graph)
    {
        Vector<String> res;
        auto sigs = graph.ComputeSignatures();
        for (auto& kv : sigs)
        {
            if (RanExists(pipelineId, kv.second))
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

    void PipelineExecutor::Execute(const String& pipelineId, const PipelineGraph& graph, const String& targetNodeId,
                                   const Vector<String>& bypassNodeIds /*= {}*/, bool cachedOnly /*= false*/)
    {
        if (IsRunning())
        {
            PipelineExecEvent ev;
            ev.type = PipelineExecEvent::Type::Fatal;
            ev.error = "Pipeline is already running";
            Emit(ev);
            return;
        }

        auto run = mmake<Run>();
        run->pipelineId = pipelineId;
        run->graph = graph;
        run->targetNodeId = targetNodeId;
        run->bypass = bypassNodeIds;
        run->cachedOnly = cachedOnly;
        mCurrent = run;

        mCoroutine = ExecuteCoroutine(run);
        mCoroutine.Start(JobThread::Main);
    }

    void PipelineExecutor::ExecuteSingle(const String& pipelineId, const PipelineGraph& graph, const String& nodeId)
    {
        if (IsRunning())
            return;

        auto run = mmake<Run>();
        run->pipelineId = pipelineId;
        run->graph = graph;
        run->targetNodeId = nodeId;
        mCurrent = run;

        mCoroutine = ExecuteSingleCoroutine(run);
        mCoroutine.Start(JobThread::Main);
    }

    void PipelineExecutor::Cancel()
    {
        if (mCurrent)
            mCurrent->cancelled = true;
    }

    bool PipelineExecutor::IsRunning() const
    {
        return mCurrent && mCoroutine.IsValid() && !mCoroutine.IsDone();
    }

    void PipelineExecutor::Emit(const PipelineExecEvent& event)
    {
        if (onEvent)
            onEvent(event);
    }

    void PipelineExecutor::EmitState(const String& nodeId, const String& state, const String& error /*= ""*/)
    {
        PipelineExecEvent ev;
        ev.type = PipelineExecEvent::Type::NodeState;
        ev.nodeId = nodeId;
        ev.state = state;
        ev.error = error;
        Emit(ev);
    }

    void PipelineExecutor::EmitLog(const String& message)
    {
        PipelineExecEvent ev;
        ev.type = PipelineExecEvent::Type::Log;
        ev.message = message;
        Emit(ev);
    }

    Ref<PipelineExecContext> PipelineExecutor::MakeContext(const Ref<Run>& run, const String& nodeId)
    {
        auto ctx = mmake<PipelineExecContext>();
        ctx->settings = PipelineSettings::Load();
        ctx->pipelineId = run->pipelineId;
        ctx->cachedOnly = run->cachedOnly;
        ctx->assetsPath = assetsPathOverride.IsEmpty() ? o2Assets.GetAssetsPath() : assetsPathOverride;

        WeakRef<PipelineExecutor> self(this);
        WeakRef<Run> weakRun(run);
        ctx->log = [self](const String& message) { if (auto s = self.Lock()) s->EmitLog(message); };
        ctx->isCancelled = [weakRun]() { auto r = weakRun.Lock(); return !r || r->cancelled; };
        ctx->signalRetry = [self, nodeId](int attempt, int max, int status, const String& reason)
        {
            if (auto s = self.Lock())
            {
                PipelineExecEvent ev;
                ev.type = PipelineExecEvent::Type::Retry;
                ev.nodeId = nodeId;
                ev.attempt = attempt;
                ev.maxAttempts = max;
                ev.status = status;
                ev.message = reason;
                s->Emit(ev);
            }
        };

        int seed = -1;
        run->seeds.TryGetValue(nodeId, seed);
        ctx->seed = seed;
        return ctx;
    }

    void PipelineExecutor::WritePreview(const Ref<Run>& run, const String& nodeId, const PipelineValue& value, const PipelineValue* srcValue)
    {
        // Other containers of a previous run must not shadow the fresh one
        for (auto& ext : Vector<String>{ "png", "txt", "mp4", "mp3", "wav", "ogg", "m4a", "flac" })
        {
            if (ext != value.GetExtension())
                o2FileSystem.FileDelete(GetPreviewPath(run->pipelineId, nodeId, ext));
        }

        String path = GetPreviewPath(run->pipelineId, nodeId, value.GetExtension());
        PipelineUtils::WriteFileBytes(path, value.IsImage() ? value.GetPngBytes() : value.data);

        PipelineExecEvent ev;
        ev.type = PipelineExecEvent::Type::NodeOutput;
        ev.nodeId = nodeId;
        ev.value = value;
        ev.previewPath = path;

        String srcPath = GetSourcePreviewPath(run->pipelineId, nodeId);
        if (srcValue && srcValue->IsImage())
        {
            PipelineUtils::WriteFileBytes(srcPath, srcValue->GetPngBytes());
            ev.srcPreviewPath = srcPath;
        }
        else
            o2FileSystem.FileDelete(srcPath);

        Emit(ev);
    }

    void PipelineExecutor::FinishRun(const Ref<Run>& run, const String& fatal)
    {
#if IS_EDITOR
        if (run->assetsChanged)
        {
            EmitLog("Rebuilding assets");
            o2Assets.RebuildAssets();
        }
#endif

        PipelineExecEvent ev;
        if (fatal.IsEmpty())
            ev.type = PipelineExecEvent::Type::Done;
        else
        {
            ev.type = PipelineExecEvent::Type::Fatal;
            ev.error = fatal;
        }

        if (mCurrent == run)
            mCurrent = nullptr;

        Emit(ev);
    }

    Coroutine<void> PipelineExecutor::ExecuteCoroutine(Ref<Run> run)
    {
        Ref<PipelineExecutor> self(this);

        auto errors = run->graph.Validate();
        if (!errors.IsEmpty())
        {
            String joined;
            for (auto& e : errors) joined += (joined.IsEmpty() ? "" : "; ") + e;
            FinishRun(run, joined);
            co_return;
        }

        if (!run->graph.FindNode(run->targetNodeId))
        {
            FinishRun(run, "Target node not found");
            co_return;
        }

        run->seeds = run->graph.ResolveSeeds();

        bool ok = co_await Evaluate(run, run->targetNodeId);
        if (run->cancelled)
            FinishRun(run, "Cancelled");
        else if (run->notCached)
            FinishRun(run, "Not cached");
        else if (!ok)
            FinishRun(run, "Pipeline failed");
        else
            FinishRun(run, "");
    }

    Coroutine<bool> PipelineExecutor::Evaluate(Ref<Run> run, String nodeId)
    {
        if (run->cancelled)
            co_return false;

        if (run->outputs.ContainsKey(nodeId))
            co_return true;

        if (run->visiting.ContainsKey(nodeId))
        {
            EmitState(nodeId, "error", "Cycle detected at node");
            co_return false;
        }

        auto node = run->graph.FindNode(nodeId);
        if (!node)
            co_return false;

        run->visiting[nodeId] = true;

        auto incoming = run->graph.GetIncomingEdges(nodeId);
        for (auto& edge : incoming)
        {
            bool ok = co_await Evaluate(run, edge->fromNodeId);
            if (!ok)
            {
                run->visiting.Remove(nodeId);
                co_return false;
            }
        }

        Map<String, PipelineValue> inputs;
        Map<String, PipelineValue> inputsById;
        Map<String, String> upstreamSigs;
        for (auto& edge : incoming)
        {
            auto port = node->FindInput(edge->toPortId);
            if (!port)
            {
                EmitState(nodeId, "error", "Edge target port not found on " + node->nodeType);
                run->visiting.Remove(nodeId);
                co_return false;
            }

            Map<String, PipelineValue>* fromOutputs = nullptr;
            auto it = run->outputs.find(edge->fromNodeId);
            if (it != run->outputs.end())
                fromOutputs = &it->second;

            if (fromOutputs)
            {
                auto vit = fromOutputs->find(edge->fromPortId);
                if (vit != fromOutputs->end() && vit->second.IsValid())
                {
                    inputs[port->name] = vit->second;
                    inputsById[port->id] = vit->second;
                }
            }

            String upstreamSig;
            if (run->sigByNode.TryGetValue(edge->fromNodeId, upstreamSig))
                upstreamSigs[PipelineGraph::UpstreamSigKey(*node, *port)] = upstreamSig;
        }

        int nodeSeed = -1;
        run->seeds.TryGetValue(nodeId, nodeSeed);
        String sig = PipelineGraph::ComputeNodeSignature(*node, upstreamSigs, {}, nodeSeed);
        run->sigByNode[nodeId] = sig;

        bool isFinish = PipelineNodeRegistry::IsFinishType(node->nodeType);
        bool cacheable = !isFinish && node->outputs.Count() == 1;

        auto impl = PipelineNodeRegistry::Get(node->nodeType);
        if (!impl)
        {
            EmitState(nodeId, "error", "Unknown node type: " + node->nodeType);
            run->visiting.Remove(nodeId);
            co_return false;
        }

        auto ctx = MakeContext(run, nodeId);
        ctx->inputsById = inputsById;

        EmitState(nodeId, "running");

        Map<String, PipelineValue> byPortId;
        PipelineValue finishValue;
        PipelineValue srcValue;
        bool produced = true;
        String error;

        if (cacheable)
        {
            auto& outPort = node->outputs[0];
            bool chroma = PipelineTransparency::UsesChromaPostStep(*node);
            Vector<String> exclude = { "crop", "cropEnabled" };
            if (chroma)
                exclude.Add(PipelineTransparency::ChromaConfigKeys());

            String cacheSig = PipelineGraph::ComputeNodeSignature(*node, upstreamSigs, exclude, nodeSeed, chroma ? "chroma-raw" : "");
            if (run->bypass.Contains(nodeId))
                DeleteContent(run->pipelineId, cacheSig);

            PipelineValue stored = LoadContent(run->pipelineId, cacheSig, outPort.portType);
            if (stored.IsValid())
            {
                produced = false;
                EmitLog(node->nodeType + ": cache hit (" + cacheSig.SubStr(0, 8) + ")");
            }
            else
            {
                if (run->cachedOnly && !impl->GetSchema().instant)
                {
                    run->notCached = true;
                    run->visiting.Remove(nodeId);
                    co_return false;
                }

                PipelineRunResult result = co_await impl->Run(ctx, inputs, node);
                if (run->cancelled)
                {
                    run->visiting.Remove(nodeId);
                    co_return false;
                }

                if (!result.ok)
                    error = result.error;
                else
                {
                    auto it = result.outputs.find(outPort.name);
                    if (it == result.outputs.end() || !it->second.IsValid())
                        error = "Node " + node->nodeType + " produced no output \"" + outPort.name + "\"";
                    else
                    {
                        stored = it->second;
                        SaveContent(run->pipelineId, cacheSig, stored);
                    }
                }
            }

            if (error.IsEmpty())
            {
                PipelineValue value = stored;
                if (chroma && value.IsImage())
                {
                    if (auto raw = value.GetBitmap())
                        value = PipelineValue::Image(PipelineTransparency::ApplyChromaPostStep(*node, *raw));
                }

                PipelineValue downstream = value;
                PipelineImageOps::CropRect crop;
                if (value.IsImage() && node->GetConfigBool("cropEnabled", false) && ReadNodeCrop(*node, "crop", crop))
                {
                    if (auto bmp = value.GetBitmap())
                    {
                        downstream = PipelineValue::Image(PipelineImageOps::Crop(*bmp, crop));
                        srcValue = value;
                    }
                }

                byPortId[outPort.id] = downstream;
            }
        }
        else
        {
            if (run->cachedOnly && !impl->GetSchema().instant && !isFinish)
            {
                run->notCached = true;
                run->visiting.Remove(nodeId);
                co_return false;
            }

            PipelineRunResult result = co_await impl->Run(ctx, inputs, node);
            if (run->cancelled)
            {
                run->visiting.Remove(nodeId);
                co_return false;
            }

            if (!result.ok)
                error = result.error;
            else if (isFinish)
            {
                if (!result.outputs.empty())
                    finishValue = result.outputs.begin()->second;
            }
            else
            {
                for (auto& kv : result.outputs)
                {
                    auto port = node->outputs.Find([&](const PipelinePort& p) { return p.name == kv.first; });
                    if (!port)
                    {
                        error = "Node " + node->nodeType + " produced unknown output \"" + kv.first + "\"";
                        break;
                    }
                    byPortId[port->id] = kv.second;
                }
            }
        }

        if (ctx->assetsChanged)
            run->assetsChanged = true;

        if (!error.IsEmpty())
        {
            EmitState(nodeId, "error", error);
            EmitLog(node->nodeType + ": " + error);
            run->visiting.Remove(nodeId);
            co_return false;
        }

        run->outputs[nodeId] = byPortId;

        PipelineValue preview;
        if (isFinish)
            preview = finishValue.IsValid() ? finishValue : (inputs.empty() ? PipelineValue() : inputs.begin()->second);
        else if (!byPortId.empty())
            preview = byPortId.begin()->second;

        bool writePreview = !run->cachedOnly || produced || nodeId == run->targetNodeId;
        if (preview.IsValid() && writePreview)
            WritePreview(run, nodeId, preview, srcValue.IsValid() ? &srcValue : nullptr);

        MarkRan(run->pipelineId, sig);
        EmitState(nodeId, "done");
        run->visiting.Remove(nodeId);
        co_return true;
    }

    Coroutine<void> PipelineExecutor::ExecuteSingleCoroutine(Ref<Run> run)
    {
        Ref<PipelineExecutor> self(this);

        auto node = run->graph.FindNode(run->targetNodeId);
        if (!node)
        {
            FinishRun(run, "Node not found");
            co_return;
        }

        run->seeds = run->graph.ResolveSeeds();

        Map<String, PipelineValue> inputs;
        Map<String, PipelineValue> inputsById;
        for (auto& edge : run->graph.GetIncomingEdges(node->id))
        {
            auto source = run->graph.FindNode(edge->fromNodeId);
            if (!source) continue;
            auto sourcePort = source->FindOutput(edge->fromPortId);
            auto targetPort = node->FindInput(edge->toPortId);
            if (!sourcePort || !targetPort) continue;

            PipelineValue value = LoadValueFile(GetCachePath(run->pipelineId) + "previews/" + source->id, sourcePort->portType);
            if (!value.IsValid())
            {
                String err = "Cached output of upstream node " + source->nodeType + " not found - run the full pipeline first";
                EmitState(node->id, "error", err);
                FinishRun(run, err);
                co_return;
            }
            inputs[targetPort->name] = value;
            inputsById[targetPort->id] = value;
        }

        auto impl = PipelineNodeRegistry::Get(node->nodeType);
        if (!impl)
        {
            FinishRun(run, "Unknown node type: " + node->nodeType);
            co_return;
        }

        auto ctx = MakeContext(run, node->id);
        ctx->inputsById = inputsById;
        EmitState(node->id, "running");

        PipelineRunResult result = co_await impl->Run(ctx, inputs, node);
        if (run->cancelled)
        {
            FinishRun(run, "Cancelled");
            co_return;
        }

        if (!result.ok)
        {
            EmitState(node->id, "error", result.error);
            FinishRun(run, result.error);
            co_return;
        }

        if (ctx->assetsChanged)
            run->assetsChanged = true;

        bool isFinish = PipelineNodeRegistry::IsFinishType(node->nodeType);
        PipelineValue preview;
        if (isFinish)
            preview = !result.outputs.empty() ? result.outputs.begin()->second : (inputs.empty() ? PipelineValue() : inputs.begin()->second);
        else if (!result.outputs.empty())
            preview = result.outputs.begin()->second;

        if (preview.IsValid())
            WritePreview(run, node->id, preview, nullptr);

        // Freshness bookkeeping: store the result under the current signature when the inputs are up to date
        if (!isFinish && node->outputs.Count() == 1 && preview.IsValid())
        {
            auto sigs = run->graph.ComputeSignatures();
            bool upstreamFresh = true;
            Map<String, String> upstream;
            for (auto& edge : run->graph.GetIncomingEdges(node->id))
            {
                String s;
                if (!sigs.TryGetValue(edge->fromNodeId, s) || !RanExists(run->pipelineId, s))
                    upstreamFresh = false;
                if (auto port = node->FindInput(edge->toPortId))
                    upstream[PipelineGraph::UpstreamSigKey(*node, *port)] = s;
            }

            if (upstreamFresh)
            {
                int seed = -1;
                run->seeds.TryGetValue(node->id, seed);
                String cacheSig = PipelineGraph::ComputeNodeSignature(*node, upstream, { "crop", "cropEnabled" }, seed);
                SaveContent(run->pipelineId, cacheSig, preview);
                String sig;
                if (sigs.TryGetValue(node->id, sig))
                    MarkRan(run->pipelineId, sig);
            }
        }

        EmitState(node->id, "done");
        FinishRun(run, "");
    }
}
// --- META ---

ENUM_META(Editor::PipelineExecEvent::Type, Editor__PipelineExecEvent__Type)
{
    ENUM_ENTRY(Done);
    ENUM_ENTRY(Fatal);
    ENUM_ENTRY(Log);
    ENUM_ENTRY(NodeOutput);
    ENUM_ENTRY(NodeState);
    ENUM_ENTRY(Retry);
}
END_ENUM_META;
// --- END META ---
