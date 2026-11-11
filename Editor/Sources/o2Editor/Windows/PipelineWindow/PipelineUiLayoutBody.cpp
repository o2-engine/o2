#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineUiAi.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiDevicePicker.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspace.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspaceModel.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsContext.h"

namespace Editor
{
    using namespace PipelineControls;

    // ---------------------------------------------------------------------------------------------------------------
    // The AI UI layout card (AssetsLine UiLayoutBody.tsx): the view and the preview screen, the screen itself, a status line
    // (blocks, kit coverage, undo, download), the agent's last answer, the request with its send button, and the folded
    // parameters (the agent's model, the export target, the check pass). Layers, inspector, text and history are in the
    // workspace window
    // ---------------------------------------------------------------------------------------------------------------
    class UiLayoutBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        void Build() override
        {
            PipelineUiAi::InstallWorkspaceHandlers();

            auto& theme = PipelineTheme::Get();
            WeakRef<UiLayoutBody> weakThis(this);
            mContext = mmake<UiWsContext>();
            mContext->editor = mEditor;
            mContext->nodeId = mNode->id;
            mContext->Refresh();

            // The head: line art | kit, the preview screen, the workspace
            auto head = mmake<HorizontalLayout>();
            head->name = "layout head";
            head->spacing = 6;
            head->expandWidth = true;
            head->expandHeight = true;
            head->baseCorner = BaseCorner::Left;
            auto views = mmake<Widget>();
            views->name = "views";
            views->layout->minWidth = views->layout->maxWidth = 132;
            auto viewRow = mmake<HorizontalLayout>();
            viewRow->spacing = 2;
            viewRow->expandWidth = true;
            viewRow->expandHeight = true;
            *viewRow->layout = WidgetLayout::BothStretch(3, 3, 3, 3);
            SkinSegmentedRow(*viewRow);
            for (auto view : { "lines", "kit" })
            {
                String value = view;
                auto segment = MakeSegment(value == "lines" ? "Line art" : "Kit", mContext->view == value);
                segment->name = value;
                segment->onToggleByUser = [weakThis, value](bool) { if (auto self = weakThis.Lock()) self->SetView(value); };
                mViews.Add(segment);
                viewRow->AddChild(segment);
            }
            views->AddChild(viewRow);
            head->AddChild(views);

            mDevice = mmake<UiDevicePicker>();
            mDevice->name = "device";
            mDevice->Bind(mEditor.Lock(), mNode->id);
            head->AddChild(mDevice);

            auto open = MakeIconButton("ui/pipeline/btn_maximize.svg", theme.fgDim, Color4(0, 0, 0, 0));
            open->name = "open workspace";
            AddBorderLayer(*open, "border", theme.line, controlRadius, 1.0f, Layout::BothStretch(), -1.0f);
            open->layout->minWidth = open->layout->maxWidth = 28;
            open->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->OpenWorkspace(); };
            head->AddChild(open);
            AddRow(head, 28);

            // The screen: double-click opens the workspace
            mPreview = mmake<PipelineImageView>();
            mPreview->name = "layout preview";
            mPreview->SetHint("An empty screen - ask the agent below, or open the workspace and add blocks");
            AddFlexible(mPreview, 400.0f);
            MarkContent(mPreview);

            // The status line: blocks and steps, the kit coverage, undo / redo, download
            auto stat = mmake<HorizontalLayout>();
            stat->name = "status";
            stat->spacing = 4;
            stat->expandWidth = true;
            stat->expandHeight = true;
            stat->baseCorner = BaseCorner::Left;
            mStat = MakeLabel("", true);
            StyleLabel(mStat, 10.5f, 400, theme.fgDim);
            stat->AddChild(mStat);
            mCoverage = MakeGhostButton("");
            mCoverage->name = "coverage";
            mCoverage->layout->minWidth = mCoverage->layout->maxWidth = 110;
            mCoverage->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->OpenWorkspace(); };
            stat->AddChild(mCoverage);
            auto tool = [&](const String& icon, const String& name, const Function<void()>& onClick)
            {
                auto button = MakeIconButton(icon, theme.fgDim, Color4(0, 0, 0, 0));
                button->name = name;
                button->layout->minWidth = button->layout->maxWidth = 24;
                button->onClick = onClick;
                stat->AddChild(button);
                return button;
            };
            mUndo = tool("ui/pipeline/uiws_undo.svg", "undo", [weakThis]() { if (auto self = weakThis.Lock()) self->mContext->Undo(); });
            mRedo = tool("ui/pipeline/uiws_redo.svg", "redo", [weakThis]() { if (auto self = weakThis.Lock()) self->mContext->Redo(); });
            mDownload = tool("ui/pipeline/uiws_download.svg", "download", [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    PipelineUiAi::Download(self->mEditor.Lock(), self->mNode->id);
            });
            AddRow(stat, 24);

            // The answer line: an error, the kit not ready for the agent, or its last words
            mReply = MakeLabel("", true);
            mReply->name = "reply";
            StyleLabel(mReply, 10.5f, 400, theme.fgDim);
            mReply->verAlign = VerAlign::Top;
            mReply->horOverflow = Label::HorOverflow::Wrap;
            AddRow(mReply, 38);

            // The request with its send button inside
            auto field = mmake<Widget>();
            field->name = "request";
            mDraft = MakeEditBox(mContext->draft, true, "Describe the screen, or what to change - e.g. \"a main menu: title, Play, Settings, coins in a corner\"");
            mDraft->name = "request field";
            *mDraft->layout = WidgetLayout::BothStretch();
            mDraft->onChangeCompleted = [weakThis](const WString& text)
            {
                if (auto self = weakThis.Lock())
                    self->mContext->SetUi("uiDraft", UiDoc::Json((String)text));
            };
            field->AddChild(mDraft);
            mSend = MakeIconButton("ui/pipeline/uiws_send.svg", theme.accent, Color4(0, 0, 0, 0));
            mSend->name = "send";
            *mSend->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 0), Vec2F(-30, 6), Vec2F(-6, 30));
            mSend->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->Send(); };
            field->AddChild(mSend);
            AddRow(field, 62);

            if (BeginParams({ PipelineUtils::PrettyModelName(UiDoc::Ws::AgentModelOf(mContext->config)), ExportLabel(mContext->exportFor) }))
            {
                AddModelRow(PipelineTextModelPresets(), "gpt-6.1-sol", PipelineModelKind::Text, "agentModel", "Agent model", 100);
                AddExportRow();
                auto review = AddCheckbox("Check pass after each answer", "agentReview", true);
                review->name = "check pass";
            }
            EndParams();

            Refresh(true);
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            Refresh(false);
        }

        void OnConfigChanged() override
        {
            if (mDraft && !mDraft->IsFocused())
                mDraft->SetText(mNode->GetConfigString("uiDraft", ""));

            Refresh(true);
            UiWorkspace::Refresh(mNode->id);
        }

        void Update(float dt) override
        {
            PipelineNodeBody::Update(dt);

            // The agent answers between frames, the kit changes upstream: the context notices
            bool busy = PipelineUiAi::IsBusy("agent", mNode->id);
            if (busy != mShownBusy || (mContext && mContext->RefreshIfChanged()))
                Refresh(true);
        }

    private:
        Ref<UiWsContext>      mContext;  // The node read as the workspace reads it: its layout, kit, preview screen
        Vector<Ref<Toggle>>   mViews;    // Line art | kit
        Ref<UiDevicePicker>   mDevice;   // The preview screen
        Ref<PipelineImageView> mPreview; // The screen drawn
        Ref<Label>            mStat;     // Blocks and steps
        Ref<Button>           mCoverage; // How much of the layout draws with the kit
        Ref<Button>           mUndo;
        Ref<Button>           mRedo;
        Ref<Button>           mDownload;
        Ref<Label>            mReply;    // The agent's last answer
        Ref<EditBox>          mDraft;    // The request
        Ref<Button>           mSend;

        String mRendered;         // What the preview was drawn for
        bool   mShownBusy = false;

        static String ExportLabel(const String& target)
        {
            return target == "o2" ? String("o2") : target == "json" ? String("JSON") : String("Unity3D");
        }

        void AddExportRow()
        {
            auto row = mmake<HorizontalLayout>();
            row->spacing = 2;
            row->expandWidth = true;
            row->expandHeight = true;
            row->baseCorner = BaseCorner::Left;
            SkinSegmentedRow(*row);
            WeakRef<UiLayoutBody> weakThis(this);
            for (auto target : { "unity", "o2", "json" })
            {
                String value = target;
                auto segment = MakeSegment(ExportLabel(value), mContext->exportFor == value);
                segment->name = "export " + value;
                segment->onToggleByUser = [weakThis, value](bool)
                {
                    if (auto self = weakThis.Lock())
                    {
                        self->mContext->SetUi("exportFor", UiDoc::Json(value));
                        self->RebuildBodyKeepingArea();
                    }
                };
                row->AddChild(segment);
            }
            auto block = mmake<Widget>();
            *row->layout = WidgetLayout::BothStretch(3, 3, 3, 3);
            block->AddChild(row);
            AddRow(MakeRow("Export for", block, 100), 26.85f);
        }

        void SetView(const String& view)
        {
            mContext->SetUi("uiView", UiDoc::Json(view));
            Refresh(true);
        }

        void OpenWorkspace()
        {
            if (auto editor = mEditor.Lock())
                UiWorkspace::Open(editor, mNode->id);
        }

        void Send()
        {
            String prompt = mDraft ? (String)mDraft->GetText() : String();
            if (prompt.Trimed(" \n\r\t").IsEmpty())
                return;

            mContext->SetUi("uiDraft", UiDoc::Json(prompt));
            PipelineUiAi::SendAgent(mEditor.Lock(), mNode->id, prompt, mContext->selected);
            Refresh(true);
        }

        void Refresh(bool redraw)
        {
            if (!mContext || !mPreview)
                return;

            PushEditorScopeOnStack scope;
            auto& theme = PipelineTheme::Get();
            bool busy = PipelineUiAi::IsBusy("agent", mNode->id);
            mShownBusy = busy;

            for (auto& segment : mViews)
                segment->SetValue(segment->name == mContext->view);
            if (mDevice)
                mDevice->Refresh();

            const auto& doc = mContext->Doc();
            int blocks = UiDoc::CountNodes(doc);
            int steps = mContext->timeline.GetSteps().Count();
            mStat->text = (String)Math::Max(0, blocks) + " blocks \xC2\xB7 " + (String)steps + " steps" + (busy ? " \xC2\xB7 working..." : "");

            bool covered = mContext->hasKit && blocks > 0 && mContext->report.total > 0;
            mCoverage->SetEnabledForcible(covered);
            if (covered)
            {
                if (auto caption = mCoverage->GetLayerDrawable<Text>("caption"))
                    caption->text = "Kit " + (String)mContext->report.covered + "/" + (String)mContext->report.total;
            }

            mUndo->interactable = !busy && steps > 0;
            mUndo->transparency = mUndo->interactable ? 1.0f : 0.35f;
            mRedo->interactable = !busy && !mContext->redo.IsEmpty();
            mRedo->transparency = mRedo->interactable ? 1.0f : 0.35f;
            mDownload->interactable = blocks > 0 && !PipelineUiAi::IsBusy("export", mNode->id);
            mDownload->transparency = mDownload->interactable ? 1.0f : 0.35f;
            mSend->interactable = !busy;
            mSend->transparency = busy ? 0.4f : 1.0f;

            // The answer line
            String error = PipelineUiAi::LastError("agent", mNode->id);
            String reply;
            for (int i = mContext->timeline.GetSteps().Count() - 1; i >= 0; i--)
            {
                auto& step = mContext->timeline.GetSteps()[i];
                if (step.IsAgent())
                {
                    reply = step.Reply().value_or("");
                    break;
                }
            }
            bool kitWarn = reply.IsEmpty() && mContext->hasKit && mContext->undescribed > 0;
            mReply->text = !error.IsEmpty() ? error :
                kitWarn ? (String)mContext->undescribed + " kit graphics have no type or description yet - describe them with the kit's AI first" :
                !reply.IsEmpty() ? reply : String("The agent's answers show here. Ask for a screen below; every answer is a step you can undo.");
            mReply->color = !error.IsEmpty() ? theme.red : kitWarn ? theme.yellow : !reply.IsEmpty() ? theme.fg : theme.fg3;

            if (redraw)
                Redraw();
        }

        // The screen drawn at the preview frame, crisp for the card; only when what it shows changed
        void Redraw()
        {
            const auto& doc = mContext->Doc();
            String key = (String)mContext->version + "|" + mContext->view + "|" + (String)mContext->preview.frame.w + "x" +
                (String)mContext->preview.frame.h + "|" + (String)mContext->kitVersion;
            if (key == mRendered)
                return;

            mRendered = key;
            if (UiDoc::CountNodes(doc) == 0)
            {
                mPreview->SetBitmap(nullptr);
                return;
            }

            UiDoc::UiRenderOptions options;
            options.mode = mContext->view == "kit" && mContext->hasKit ? "kit" : "lines";
            options.kit = mContext->hasKit ? &mContext->kit : nullptr;
            options.width = mContext->preview.frame.w;
            options.height = mContext->preview.frame.h;
            options.rects = &mContext->rects;
            options.pixelLong = 900;
            mPreview->SetBitmap(UiDoc::RenderLayoutBitmap(doc, options));
        }
    };

    Ref<PipelineNodeBody> CreateUiLayoutNodeBody(const String& type)
    {
        return type == "uiLayout" ? mmake<UiLayoutBody>() : nullptr;
    }
}
