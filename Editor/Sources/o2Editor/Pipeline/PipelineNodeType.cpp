#include "o2Editor/stdafx.h"
#include "PipelineNodeType.h"

#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"

namespace Editor
{
    String PipelineNodeCategoryToString(PipelineNodeCategory category)
    {
        switch (category)
        {
            case PipelineNodeCategory::Source: return "source";
            case PipelineNodeCategory::Transform: return "transform";
            case PipelineNodeCategory::AI: return "ai";
            case PipelineNodeCategory::Output: return "output";
            default: return "flow";
        }
    }

    bool PipelineNodeSchema::AcceptsInputType(PipelinePortType type) const
    {
        return inputs.Any([&](const PipelinePort& p) { return p.portType == type; }) || addableInputs.Contains(type);
    }

    bool PipelineNodeSchema::ProducesOutputType(PipelinePortType type) const
    {
        return outputs.Any([&](const PipelinePort& p) { return p.portType == type; });
    }

    PipelineRunResult PipelineRunResult::Fail(const String& error)
    {
        PipelineRunResult res;
        res.ok = false;
        res.error = error;
        return res;
    }

    PipelineRunResult PipelineRunResult::Single(const PipelineValue& value, const String& portName /*= "out"*/)
    {
        PipelineRunResult res;
        res.outputs[portName] = value;
        return res;
    }

    void PipelineExecContext::Log(const String& message) const
    {
        if (log)
            log(message);
    }

    bool PipelineExecContext::IsCancelled() const
    {
        return isCancelled && isCancelled();
    }

    void PipelineExecContext::SignalRetry(int attempt, int max, int status, const String& reason) const
    {
        if (signalRetry)
            signalRetry(attempt, max, status, reason);
    }

    void PipelineExecContext::Phase(const String& name) const
    {
        if (phase)
            phase(name);
    }

    bool PipelineExecContext::MemoGet(const String& key, const String& ext, String& data) const
    {
        return memoGet && memoGet(key, ext, data);
    }

    void PipelineExecContext::MemoPut(const String& key, const String& ext, const String& data) const
    {
        if (memoPut)
            memoPut(key, ext, data);
    }

    Coroutine<PipelineRefineResult> IPipelineNodeImpl::Refine(Ref<PipelineExecContext> ctx, Map<String, PipelineValue> inputs,
                                                              Ref<PipelineNode> node, PipelineValue stored, String key)
    {
        PipelineRefineResult res;
        res.value = stored;
        co_return res;
    }

    Vector<Ref<IPipelineNodeImpl>>& PipelineNodeRegistry::Impls()
    {
        static Vector<Ref<IPipelineNodeImpl>> impls;
        return impls;
    }

    void PipelineNodeRegistry::EnsureBuiltins()
    {
        static bool registered = false;
        if (registered)
            return;

        registered = true;
        RegisterBuiltinPipelineNodes();
    }

    void PipelineNodeRegistry::Register(const Ref<IPipelineNodeImpl>& impl)
    {
        auto& impls = Impls();
        impls.RemoveAll([&](const Ref<IPipelineNodeImpl>& x) { return x->GetSchema().type == impl->GetSchema().type; });
        impls.Add(impl);
    }

    Ref<IPipelineNodeImpl> PipelineNodeRegistry::Get(const String& type)
    {
        EnsureBuiltins();
        return Impls().FindOrDefault([&](const Ref<IPipelineNodeImpl>& x) { return x->GetSchema().type == type; });
    }

    const PipelineNodeSchema* PipelineNodeRegistry::GetSchema(const String& type)
    {
        auto impl = Get(type);
        return impl ? &impl->GetSchema() : nullptr;
    }

    const Vector<Ref<IPipelineNodeImpl>>& PipelineNodeRegistry::All()
    {
        EnsureBuiltins();
        return Impls();
    }

    Vector<const PipelineNodeSchema*> PipelineNodeRegistry::AllSchemas()
    {
        Vector<const PipelineNodeSchema*> res;
        for (auto& impl : All())
            res.Add(&impl->GetSchema());
        return res;
    }

    Ref<PipelineNode> PipelineNodeRegistry::CreateNode(const String& type, const Vec2F& position)
    {
        auto schema = GetSchema(type);
        if (!schema)
            return nullptr;

        auto node = mmake<PipelineNode>();
        node->id = PipelineNode::GenerateId();
        node->nodeType = type;
        node->position = position;
        node->size = schema->defaultSize;

        for (auto& port : schema->inputs)
            node->inputs.Add(PipelinePort(PipelineNode::GenerateId(), port.name, port.portType, false));

        for (auto& port : schema->outputs)
            node->outputs.Add(PipelinePort(PipelineNode::GenerateId(), port.name, port.portType, false));

        if (auto impl = Get(type))
            impl->InitNode(node);

        return node;
    }

    void PipelineNodeRegistry::SyncNodeWithSchema(const Ref<PipelineNode>& node)
    {
        auto schema = GetSchema(node->nodeType);
        if (!schema)
            return;

        bool ownPorts = false;
        if (auto impl = Get(node->nodeType))
            ownPorts = impl->SyncPorts(node);

        if (!ownPorts)
        {
            for (auto& port : schema->outputs)
            {
                if (!node->outputs.Any([&](const PipelinePort& p) { return p.name == port.name; }))
                    node->outputs.Add(PipelinePort(PipelineNode::GenerateId(), port.name, port.portType, false));
            }
        }

        node->RegenerateInputs(schema->inputs);
    }

    bool PipelineNodeRegistry::IsReferenceInput(const PipelineNode& node, const String& portId)
    {
        if (node.nodeType == "textStyle")
            return true;

        if (node.nodeType == "uiLayout")
        {
            auto port = node.FindInput(portId);
            return port && port->portType != PipelinePortType::UiKit;
        }

        return false;
    }

    String PipelineNodeRegistry::UpstreamSigFor(const String& sig, const PipelineNode* from, const String& fromPortId,
                                                const PipelineNode& to)
    {
        if (sig.IsEmpty() || !from)
            return sig;

        if (to.nodeType == "uiKit" && from->nodeType == "imageExtract")
            return sig + "|part-density:" + (String)PART_DENSITY_VERSION;

        if (from->nodeType == "uiLayout")
        {
            auto port = from->FindOutput(fromPortId);
            if (port && !UiDoc::IsLayoutFixedOutput(port->name))
            {
                if (auto own = UiDoc::LayoutOwnPortByName(UiDoc::FromDataValue(from->config), port->name))
                    return UiDoc::OwnGraphicSig(*own);
            }
        }
        return sig;
    }

    bool PipelineNodeRegistry::IsFinishType(const String& type)
    {
        return type == "finishText" || type == "finishImage" || type == "finishVideo" || type == "finishAudio";
    }

    String PipelineNodeRegistry::GetFinishAssetPath(const PipelineNode& node)
    {
        String path = node.GetConfigString("assetPath", "").Trimed(" \n\r\t");
        if (!path.IsEmpty())
            return path;

        // A pipeline made in the AssetsLine web editor names the download, not an asset
        String name = node.GetConfigString("filename", "").Trimed(" \n\r\t");
        name.ReplaceAll("\\", "/");
        int slash = name.FindLast("/");
        if (slash >= 0)
            name = name.SubStr(slash + 1);

        int dot = name.FindLast(".");
        if (dot > 0)
            name = name.SubStr(0, dot);

        return "Generated/" + (name.IsEmpty() ? String("output") : name);
    }
}
// --- META ---

ENUM_META(Editor::PipelineNodeCategory, Editor__PipelineNodeCategory)
{
    ENUM_ENTRY(AI);
    ENUM_ENTRY(Flow);
    ENUM_ENTRY(Output);
    ENUM_ENTRY(Source);
    ENUM_ENTRY(Transform);
}
END_ENUM_META;
// --- END META ---
