#include "o2Editor/stdafx.h"
#include "PipelineNodeType.h"

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

        return node;
    }

    void PipelineNodeRegistry::SyncNodeWithSchema(const Ref<PipelineNode>& node)
    {
        auto schema = GetSchema(node->nodeType);
        if (!schema)
            return;

        for (auto& port : schema->outputs)
        {
            if (!node->outputs.Any([&](const PipelinePort& p) { return p.name == port.name; }))
                node->outputs.Add(PipelinePort(PipelineNode::GenerateId(), port.name, port.portType, false));
        }

        node->RegenerateInputs(schema->inputs);
    }

    bool PipelineNodeRegistry::IsFinishType(const String& type)
    {
        return type == "finishText" || type == "finishImage" || type == "finishVideo" || type == "finishAudio";
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
