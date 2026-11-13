#include "o2Editor/stdafx.h"
#include "UiWorkspace.h"

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWindow.h"

namespace Editor
{
    namespace UiWorkspace
    {
        // Never destroyed: the window must not outlive the UI at exit
        struct State
        {
            Ref<UiWsWindow>                                                       window;
            Function<void()>                                                      onClosed;
            AgentSendHandler                                                      send;
            Function<void(const Ref<PipelineEditor>&, const String&)>             save;
        };

        static State& St()
        {
            static State* state = new State();
            return *state;
        }

        static void Create()
        {
            PushEditorScopeOnStack scope;
            auto& st = St();
            st.window = mmake<UiWsWindow>();
            st.window->name = "ui layout workspace";
            *st.window->layout = WidgetLayout::BothStretch();
            st.window->onClosed = []()
            {
                if (St().onClosed)
                    St().onClosed();
            };
            st.window->onSend = [](const String& prompt, const String& selected)
            {
                auto& s = St();
                if (!s.window || !s.send)
                    return;
                auto editor = s.window->GetContext()->editor.Lock();
                if (editor)
                    s.send(editor, s.window->GetNodeId(), prompt, selected);
            };
            EditorUIRoot.AddWidget(st.window);
            st.window->SetEnabledForcible(false);
        }

        // The save button follows the handler
        static void WireSave()
        {
            auto& st = St();
            if (!st.window)
                return;

            if (!st.save)
            {
                st.window->onSave = Function<void()>();
                return;
            }
            st.window->onSave = []()
            {
                auto& s = St();
                auto editor = s.window ? s.window->GetContext()->editor.Lock() : nullptr;
                if (editor && s.save)
                    s.save(editor, s.window->GetNodeId());
            };
        }

        void Open(const Ref<PipelineEditor>& editor, const String& nodeId)
        {
            auto& st = St();
            if (!st.window)
                Create();

            PushEditorScopeOnStack scope;
            if (!st.window->GetParent())
                EditorUIRoot.AddWidget(st.window);
            WireSave();
            st.window->Bind(editor, nodeId);
        }

        bool IsOpen()
        {
            return St().window && St().window->IsShown();
        }

        bool IsOpenFor(const String& nodeId)
        {
            return IsOpen() && St().window->GetNodeId() == nodeId;
        }

        void Close()
        {
            if (IsOpen())
                St().window->Close();
        }

        void Refresh(const String& nodeId /*= ""*/)
        {
            if (!IsOpen() || (!nodeId.IsEmpty() && St().window->GetNodeId() != nodeId))
                return;

            PushEditorScopeOnStack scope;
            St().window->GetContext()->Refresh();
        }

        void SetOnClosed(const Function<void()>& onClosed)
        {
            St().onClosed = onClosed;
        }

        void SetAgentSendHandler(const AgentSendHandler& handler)
        {
            St().send = handler;
        }

        void SetAgentBusy(const String& nodeId, bool busy)
        {
            UiAgentState::SetBusy(nodeId, busy);
            if (busy)
                UiAgentState::SetError(nodeId, "");
        }

        bool IsAgentBusy(const String& nodeId)
        {
            return UiAgentState::IsBusy(nodeId);
        }

        void SetAgentError(const String& nodeId, const String& error)
        {
            UiAgentState::SetError(nodeId, error);
        }

        bool AppendAgentStep(const Ref<PipelineEditor>& editor, const String& nodeId, const UiDoc::Json& step)
        {
            auto context = mmake<UiWsContext>();
            context->editor = editor;
            context->nodeId = nodeId;
            if (!context->Node())
                return false;

            bool written = context->Write("steps", true, [&](DataValue& config)
            {
                auto steps = UiDoc::StepsOf(config);
                steps.Add(UiDoc::UiStep(step));
                UiDoc::WriteSteps(config, steps, {});
                UiDoc::SetConfigJson(config, "uiDraft", UiDoc::Json(""));
                UiDoc::SetConfigJson(config, "uiPoints", UiDoc::Json::Array());
                return true;
            });
            Refresh(nodeId);
            return written;
        }

        void SetSaveHandler(const Function<void(const Ref<PipelineEditor>& editor, const String& nodeId)>& handler)
        {
            St().save = handler;
            WireSave();
        }

        Ref<UiWsWindow> GetWindow()
        {
            return St().window;
        }

        void Reset()
        {
            auto& st = St();
            if (!st.window)
                return;

            PushEditorScopeOnStack scope;
            st.window->onClosed = Function<void()>();
            if (st.window->GetParent())
                EditorUIRoot.RemoveWidget(st.window);
            st.window = nullptr;
        }
    }
}
