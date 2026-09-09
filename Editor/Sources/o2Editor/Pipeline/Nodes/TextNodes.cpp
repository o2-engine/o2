#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"

#include "o2/Utils/FileSystem/FileSystem.h"

namespace Editor
{
    // Writes bytes into the project assets folder and returns the relative path actually used
    String WriteFinishAsset(const Ref<PipelineExecContext>& ctx, const PipelineNode& node, const String& defaultName,
                            const String& ext, const String& bytes)
    {
        String path = node.GetConfigString("assetPath", "").Trimed(" \n\r\t");
        if (path.IsEmpty())
            path = defaultName;

        path.ReplaceAll("\\", "/");
        while (path.StartsWith("/"))
            path = path.SubStr(1);

        String currentExt = o2FileSystem.GetFileExtension(path).ToLowerCase();
        if (currentExt.IsEmpty())
            path += "." + ext;
        else if (currentExt != ext)
            path = path.SubStr(0, path.Length() - currentExt.Length() - 1) + "." + ext;

        String full = ctx->assetsPath + path;
        if (!PipelineUtils::WriteFileBytes(full, bytes))
            return "";

        ctx->assetsChanged = true;
        ctx->Log("saved asset " + path + " (" + (String)bytes.Length() + " bytes)");
        return path;
    }

    // Inline text typed into the node
    class SourceTextNode : public PipelineNodeBase
    {
    public:
        SourceTextNode()
        {
            mSchema.type = "sourceText";
            mSchema.label = "Text source";
            mSchema.category = PipelineNodeCategory::Source;
            mSchema.description = "Inline text. Type into the field; the text feeds the connected nodes.";
            mSchema.outputs = { Out("out", PipelinePortType::Text) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            co_return PipelineRunResult::Single(PipelineValue::Text(node->GetConfigString("text", "")));
        }
    };

    // Template with {name} placeholders filled from the custom text inputs
    class TextComposeNode : public PipelineNodeBase
    {
    public:
        TextComposeNode()
        {
            mSchema.type = "textCompose";
            mSchema.label = "Text compose";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.description = "Substitute {name} placeholders in the template with text from per-variable inputs.";
            mSchema.inputs = { In("template", PipelinePortType::Text) };
            mSchema.outputs = { Out("out", PipelinePortType::Text) };
            mSchema.addableInputs = { PipelinePortType::Text };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto templateValue = Input(inputs, "template");
            if (!templateValue || !templateValue->IsText())
                co_return PipelineRunResult::Fail("textCompose: template input is not connected");

            String result = templateValue->data;
            for (auto& custom : node->GetCustomInputs())
            {
                if (custom.portType != PipelinePortType::Text) continue;
                String name = custom.name.Trimed();
                if (name.IsEmpty()) continue;
                auto v = Input(inputs, custom.name);
                String text = v && v->IsText() ? v->data : String();
                result.ReplaceAll("{" + name + "}", text);
            }

            co_return PipelineRunResult::Single(PipelineValue::Text(result));
        }
    };

    // Joins the custom text inputs in order with a separator
    class TextConcatNode : public PipelineNodeBase
    {
    public:
        TextConcatNode()
        {
            mSchema.type = "textConcat";
            mSchema.label = "Text concat";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.description = "Concatenate multiple text inputs in the configured order, joined by a separator.";
            mSchema.outputs = { Out("out", PipelinePortType::Text) };
            mSchema.addableInputs = { PipelinePortType::Text };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            String separator = node->GetConfigBool("newlineSeparator", false) ? String("\n\n") : node->GetConfigString("separator", "");
            String result;
            bool first = true;
            for (auto& custom : node->GetCustomInputs())
            {
                if (custom.portType != PipelinePortType::Text) continue;
                auto v = Input(inputs, custom.name);
                String piece = v && v->IsText() ? v->data : String();
                if (!first) result += separator;
                first = false;
                result += piece;
            }

            co_return PipelineRunResult::Single(PipelineValue::Text(result));
        }
    };

    // Terminal node saving the text as an asset
    class FinishTextNode : public PipelineNodeBase
    {
    public:
        FinishTextNode()
        {
            mSchema.type = "finishText";
            mSchema.label = "Finish - text";
            mSchema.category = PipelineNodeCategory::Output;
            mSchema.description = "Terminal node. Press Play to evaluate the pipeline; the text is saved as an asset in the Assets folder.";
            mSchema.inputs = { In("in", PipelinePortType::Text) };
            mSchema.hasPlay = true;
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto value = Input(inputs, "in");
            if (!value || !value->IsText())
                co_return PipelineRunResult::Fail("finishText: input \"in\" is not connected");

            if (WriteFinishAsset(ctx, *node, "Generated/output", "txt", value->data).IsEmpty())
                co_return PipelineRunResult::Fail("finishText: failed to write the asset file");

            co_return PipelineRunResult::Single(*value, "result");
        }
    };

    void RegisterTextNodes()
    {
        PipelineNodeRegistry::Register(mmake<SourceTextNode>());
        PipelineNodeRegistry::Register(mmake<TextComposeNode>());
        PipelineNodeRegistry::Register(mmake<TextConcatNode>());
        PipelineNodeRegistry::Register(mmake<FinishTextNode>());
    }
}
