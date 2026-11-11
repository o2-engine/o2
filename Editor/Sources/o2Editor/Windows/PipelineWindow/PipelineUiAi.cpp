#include "o2Editor/stdafx.h"
#include "PipelineUiAi.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2/Assets/Assets.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "o2Editor/Pipeline/UiDoc/UiDocExport.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspace.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsContext.h"

namespace Editor
{
    namespace PipelineUiAi
    {
        // Running calls by "<kind>:<node id>"; a finished one is dropped the next time its slot is used
        static Map<String, Coroutine<void>>& Jobs()
        {
            static Map<String, Coroutine<void>> jobs;
            return jobs;
        }

        static Map<String, String>& Busy()
        {
            static Map<String, String> busy;
            return busy;
        }

        static Map<String, String>& Errors()
        {
            static Map<String, String> errors;
            return errors;
        }

        static String Key(const String& kind, const String& nodeId)
        {
            return kind + ":" + nodeId;
        }

        static void SetBusy(const String& kind, const String& nodeId, const String& what)
        {
            if (what.IsEmpty())
                Busy().Remove(Key(kind, nodeId));
            else
                Busy()[Key(kind, nodeId)] = what;
        }

        bool IsBusy(const String& kind, const String& nodeId)
        {
            return Busy().ContainsKey(Key(kind, nodeId));
        }

        String KitBusyPort(const String& nodeId)
        {
            String port;
            Busy().TryGetValue(Key("kit", nodeId), port);
            return port;
        }

        String LastError(const String& kind, const String& nodeId)
        {
            String error;
            Errors().TryGetValue(Key(kind, nodeId), error);
            return error;
        }

        static void SetError(const String& kind, const String& nodeId, const String& error)
        {
            if (error.IsEmpty())
                Errors().Remove(Key(kind, nodeId));
            else
                Errors()[Key(kind, nodeId)] = error;
        }

        static Ref<PipelineExecContext> MakeContext(const Ref<PipelineEditor>& editor, const String& nodeId)
        {
            auto ctx = mmake<PipelineExecContext>();
            ctx->settings = PipelineSettings::Load();
            ctx->pipelineId = editor->GetPipelineId();
            ctx->nodeId = nodeId;
            WeakRef<PipelineEditor> weakEditor(editor);
            ctx->log = [weakEditor](const String& message) { if (auto e = weakEditor.Lock()) { if (e->onLog) e->onLog(message); } };
            return ctx;
        }

        static Ref<PipelineNode> FindNode(const Ref<PipelineEditor>& editor, const String& nodeId)
        {
            auto graph = editor ? editor->GetGraph() : nullptr;
            return graph ? graph->FindNode(nodeId) : nullptr;
        }

        // Writes the node's config as one undo step and tells the card; false when the node is gone or nothing changed
        static bool WriteConfig(const Ref<PipelineEditor>& editor, const String& nodeId, const String& key,
                                const Function<bool(DataValue&)>& change)
        {
            auto context = mmake<UiWsContext>();
            context->editor = editor;
            context->nodeId = nodeId;
            return context->Node() && context->Write(key, true, change);
        }

        static void Run(const String& kind, const String& nodeId, Coroutine<void> job)
        {
            Jobs()[Key(kind, nodeId)] = job;
            Jobs()[Key(kind, nodeId)].Start(JobThread::Main);
        }

        bool CurrentKit(const Ref<PipelineEditor>& editor, const PipelineNode& layoutNode, UiDoc::UiKit& kit)
        {
            auto graph = editor ? editor->GetGraph() : nullptr;
            auto port = layoutNode.inputs.FindOrDefault([](const PipelinePort& p) { return p.portType == PipelinePortType::UiKit; });
            auto edge = graph && !port.id.IsEmpty() ? graph->FindEdgeToPort(layoutNode.id, port.id) : nullptr;
            if (!edge)
                return false;

            // The kit node made again from what its inputs show now: an instant node may not have caught up yet
            auto kitNode = graph->FindNode(edge->fromNodeId);
            if (kitNode && kitNode->nodeType == "uiKit")
            {
                Map<String, PipelineValue> byId;
                for (auto& input : kitNode->inputs)
                {
                    auto value = editor->GetInputValueById(kitNode, input.id);
                    if (value.IsValid())
                        byId[input.id] = value;
                }
                kit = PipelineUiNodes::BuildKit(*kitNode, byId, {});
                return true;
            }

            auto layout = graph->FindNode(layoutNode.id);
            auto value = layout ? editor->GetInputValueById(layout, port.id) : PipelineValue();
            if (!value.IsValid() || value.type != PipelinePortType::UiKit)
                return false;

            kit = UiDoc::ParseKit(value.data);
            return true;
        }

        void SendAgent(const Ref<PipelineEditor>& editor, const String& nodeId, const String& prompt, const String& selected)
        {
            auto node = FindNode(editor, nodeId);
            if (!node || prompt.Trimed(" \n\r\t").IsEmpty() || IsBusy("agent", nodeId))
                return;

            UiDoc::UiAgentRequest request;
            request.config = UiDoc::FromDataValue(node->config);
            request.prompt = prompt;
            request.selected = selected;
            UiDoc::UiKit kit;
            if (CurrentKit(editor, *node, kit))
                request.kit = kit;

            // The references: every input but the kit, pictures and texts, in input order
            for (auto& port : node->inputs)
            {
                if (port.portType == PipelinePortType::UiKit)
                    continue;

                auto value = editor->GetInputValueById(node, port.id);
                if (!value.IsValid())
                    continue;

                UiDoc::UiAgentInput input;
                input.name = port.name.IsEmpty() ? String("input") : port.name;
                if (value.IsImage())
                    input.png = value.GetPngBytes();
                else if (value.IsText())
                {
                    input.image = false;
                    input.text = value.data;
                }
                else
                    continue;

                request.references.Add(input);
            }

            SetBusy("agent", nodeId, "*");
            SetError("agent", nodeId, "");
            UiAgentState::SetBusy(nodeId, true);
            UiAgentState::SetError(nodeId, "");
            UiWorkspace::SetAgentBusy(nodeId, true);
            UiWorkspace::SetAgentError(nodeId, "");

            WeakRef<PipelineEditor> weakEditor(editor);
            Run("agent", nodeId, [](WeakRef<PipelineEditor> weakEditor, String nodeId, Ref<PipelineExecContext> ctx,
                                   UiDoc::UiAgentRequest request) -> Coroutine<void>
            {
                auto response = co_await UiDoc::AskUiAgent(ctx, request);
                SetBusy("agent", nodeId, "");
                UiAgentState::SetBusy(nodeId, false);
                UiWorkspace::SetAgentBusy(nodeId, false);

                auto editor = weakEditor.Lock();
                if (!editor)
                    co_return;

                if (!response.ok)
                {
                    SetError("agent", nodeId, response.error);
                    UiAgentState::SetError(nodeId, response.error);
                    UiWorkspace::SetAgentError(nodeId, response.error);
                    ctx->Log("AI UI layout: " + response.error);
                }
                else
                {
                    if (!response.errors.IsEmpty())
                        ctx->Log("AI UI layout: " + (String)response.errors.Count() + " edit(s) did not apply");
                    UiWorkspace::AppendAgentStep(editor, nodeId, response.step.data);
                }

                if (auto widget = editor->GetNodeWidget(nodeId))
                {
                    if (auto body = widget->GetBody())
                        body->OnConfigChanged();
                }
            }(weakEditor, nodeId, MakeContext(editor, nodeId), request));
        }

        void GenerateStyle(const Ref<PipelineEditor>& editor, const String& nodeId)
        {
            auto node = FindNode(editor, nodeId);
            String prompt = node ? node->GetConfigString("stylePrompt", "").Trimed(" \n\r\t") : String();
            if (!node || prompt.IsEmpty() || IsBusy("style", nodeId))
                return;

            UiDoc::UiStyleRequest request;
            request.config = UiDoc::FromDataValue(node->config);
            request.prompt = prompt;
            for (auto& port : node->inputs)
            {
                auto value = editor->GetInputValueById(node, port.id);
                if (value.IsImage())
                    request.references.Add(value.GetPngBytes());
            }

            SetBusy("style", nodeId, "*");
            SetError("style", nodeId, "");
            WeakRef<PipelineEditor> weakEditor(editor);
            Run("style", nodeId, [](WeakRef<PipelineEditor> weakEditor, String nodeId, Ref<PipelineExecContext> ctx,
                                   UiDoc::UiStyleRequest request) -> Coroutine<void>
            {
                auto response = co_await UiDoc::GenerateUiTextStyle(ctx, request);
                SetBusy("style", nodeId, "");
                auto editor = weakEditor.Lock();
                if (!editor)
                    co_return;

                if (!response.ok)
                {
                    SetError("style", nodeId, response.error);
                    ctx->Log("Text style: " + response.error);
                    if (auto widget = editor->GetNodeWidget(nodeId))
                        if (auto body = widget->GetBody()) body->OnConfigChanged();
                    co_return;
                }

                // The config IS the style: one undo step
                WriteConfig(editor, nodeId, "font", [&](DataValue& config)
                {
                    UiDoc::StoreTextStyle(config, response.style);
                    return true;
                });
            }(weakEditor, nodeId, MakeContext(editor, nodeId), request));
        }

        void AnalyzeKit(const Ref<PipelineEditor>& editor, const String& nodeId, const String& portId)
        {
            auto node = FindNode(editor, nodeId);
            if (!node || IsBusy("kit", nodeId))
                return;

            UiDoc::UiKitAnalyzeRequest request;
            request.config = UiDoc::FromDataValue(node->config);
            request.removeBg = node->GetConfigBool("kitRemoveBg", false);
            for (auto& port : node->inputs)
            {
                if (port.portType != PipelinePortType::Image || (!portId.IsEmpty() && port.id != portId))
                    continue;

                auto value = editor->GetInputValueById(node, port.id);
                if (value.IsImage())
                    request.pictures.Add({ port.id, port.name, value.GetPngBytes() });
            }

            if (request.pictures.IsEmpty())
            {
                SetError("kit", nodeId, "No picture to describe: connect the inputs and run what feeds them");
                return;
            }

            SetBusy("kit", nodeId, portId.IsEmpty() ? String("*") : portId);
            SetError("kit", nodeId, "");
            WeakRef<PipelineEditor> weakEditor(editor);
            Run("kit", nodeId, [](WeakRef<PipelineEditor> weakEditor, String nodeId, Ref<PipelineExecContext> ctx,
                                 UiDoc::UiKitAnalyzeRequest request) -> Coroutine<void>
            {
                auto response = co_await UiDoc::AnalyzeUiKit(ctx, request);
                SetBusy("kit", nodeId, "");
                auto editor = weakEditor.Lock();
                if (!editor)
                    co_return;

                for (auto& note : response.notes)
                    ctx->Log("UI kit: " + note);

                if (!response.ok)
                {
                    SetError("kit", nodeId, response.error);
                    ctx->Log("UI kit: " + response.error);
                    if (auto widget = editor->GetNodeWidget(nodeId))
                        if (auto body = widget->GetBody()) body->OnConfigChanged();
                    co_return;
                }

                WriteConfig(editor, nodeId, "items", [&](DataValue& config)
                {
                    UiDoc::MergeKitAnalysis(config, response.items);
                    return true;
                });
            }(weakEditor, nodeId, MakeContext(editor, nodeId), request));
        }

        void Download(const Ref<PipelineEditor>& editor, const String& nodeId)
        {
            auto node = FindNode(editor, nodeId);
            if (!node || IsBusy("export", nodeId))
                return;

            String target = UiDoc::ExportTargetOf(UiDoc::FromDataValue(node->config));
            if (target == UiDoc::UI_EXPORT_O2)
            {
                DownloadTo(editor, nodeId, "");
                return;
            }

            // The file goes where the user says, next to the project by default
            String assets = UiLayout::AbsolutePath(o2Assets.GetAssetsPath());
            while (UiLayout::HasSuffix(assets, "/"))
                assets = assets.SubStr(0, assets.Length() - 1);
            String folder = assets.IsEmpty() ? String() : o2FileSystem.GetParentPath(assets);
            String name = UiDoc::ExportFileName(UiDoc::DocAt(node->config).doc, target);
            Map<String, String> extensions;
            if (target == UiDoc::UI_EXPORT_UNITY)
                extensions["Unity package"] = "*.unitypackage";
            else
                extensions["Zip archive"] = "*.zip";
            String path = GetSaveFileNameDialog("Export the UI layout", extensions, folder.IsEmpty() ? name : folder + "/" + name);
            if (!path.IsEmpty())
                DownloadTo(editor, nodeId, path);
        }

        void DownloadTo(const Ref<PipelineEditor>& editor, const String& nodeId, const String& outputPath)
        {
            auto node = FindNode(editor, nodeId);
            if (!node || IsBusy("export", nodeId))
                return;

            UiDoc::UiLayoutExportRequest request;
            request.target = UiDoc::ExportTargetOf(UiDoc::FromDataValue(node->config));
            request.doc = UiDoc::DocAt(node->config).doc;
            UiDoc::UiKit kit;
            if (CurrentKit(editor, *node, kit))
                request.kit = kit;
            request.nodeId = nodeId;
            request.assetPath = node->GetConfigString("assetPath", "");
            request.assetsPath = o2Assets.GetAssetsPath();
            request.outputPath = outputPath;
            if (request.target == UiDoc::UI_EXPORT_JSON)
                request.picture = PipelineUiNodes::RenderLayoutPicture(request.doc, request.kit ? &*request.kit : nullptr).GetPngBytes();

            SetBusy("export", nodeId, "*");
            SetError("export", nodeId, "");
            if (auto widget = editor->GetNodeWidget(nodeId))
            {
                if (auto body = widget->GetBody())
                    body->OnConfigChanged();
            }

            WeakRef<PipelineEditor> weakEditor(editor);
            Run("export", nodeId, [](WeakRef<PipelineEditor> weakEditor, String nodeId, Ref<PipelineExecContext> ctx,
                                    UiDoc::UiLayoutExportRequest request) -> Coroutine<void>
            {
                auto res = co_await UiDoc::ExportLayout(request);
                SetBusy("export", nodeId, "");
                auto editor = weakEditor.Lock();
                if (!editor)
                    co_return;

                if (!res.ok)
                {
                    SetError("export", nodeId, res.error);
                    ctx->Log("AI UI layout: export failed: " + res.error);
                }
                else
                {
                    String where = request.target == UiDoc::UI_EXPORT_O2 ?
                        (res.prototype.IsEmpty() ? String("the project's Assets") : "Assets/" + res.prototype) :
                        request.outputPath;
                    ctx->Log("AI UI layout: exported for " + request.target + " to " + where);
                    for (auto& warning : res.warnings)
                        ctx->Log("AI UI layout: " + warning);
                }

                if (auto widget = editor->GetNodeWidget(nodeId))
                {
                    if (auto body = widget->GetBody())
                        body->OnConfigChanged();
                }
            }(weakEditor, nodeId, MakeContext(editor, nodeId), request));
        }

        void InstallWorkspaceHandlers()
        {
            static bool installed = false;
            if (installed)
                return;

            installed = true;
            UiWorkspace::SetAgentSendHandler([](const Ref<PipelineEditor>& editor, const String& nodeId, const String& prompt,
                                                const String& selected)
            {
                SendAgent(editor, nodeId, prompt, selected);
            });
            UiWorkspace::SetDownloadHandler([](const Ref<PipelineEditor>& editor, const String& nodeId) { Download(editor, nodeId); });
        }
    }
}
