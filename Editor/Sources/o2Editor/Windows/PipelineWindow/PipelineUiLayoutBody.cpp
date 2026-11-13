#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Assets/Assets.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAgent.h"
#include "o2Editor/Pipeline/UiDoc/UiDocExport.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineUiAi.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiDevicePicker.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiOwnGraphicsStrip.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspace.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPins.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsRefCompare.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsRunActs.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspaceModel.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsContext.h"

namespace Editor
{
    using namespace PipelineControls;

    // ---------------------------------------------------------------------------------------------------------------
    // The AI UI layout card (AssetsLine UiLayoutBody.tsx): the view, the preview screen, the comparison with the reference
    // (a menu under its button) and the workspace in the head row, the screen itself (the reference laid over it while
    // compared), the node's own graphics as a strip of cells with their output ports, a status line (blocks, kit coverage,
    // undo), Save to project with the prototype's path and its place in the Assets window, the agent's last answer (or the
    // notice of a layout in older units with the step that brings it to the kit's device), the request with its send
    // button, and the folded parameters (the agent's models, the folder the prototype goes to, the check pass). Layers,
    // inspector, text and history are in the workspace window
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
            mContext->compareNeedsBlocks = true;
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
            mDevice->Bind(mContext);
            mDevice->onCanvas = true;
            head->AddChild(mDevice);

            // Comparing with the reference: a menu under its button (shown only with a reference)
            mCompare = mmake<UiRefButton>();
            mCompare->name = "compare";
            mCompare->ctx = mContext;
            mCompare->layout->minWidth = mCompare->layout->maxWidth = 28;
            head->AddChild(mCompare);

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
            mOverlay = mmake<UiRefOverlayView>();
            mOverlay->name = "compare overlay";
            mOverlay->ctx = mContext;
            WeakRef<PipelineImageView> weakPreview(mPreview);
            mOverlay->screenRect = [weakPreview]() { auto view = weakPreview.Lock(); return view ? view->GetImageRect() : RectF(); };
            *mOverlay->layout = WidgetLayout::BothStretch();
            mPreview->AddChild(mOverlay);

            // The node's own graphics: a cell each under the screen, its output port on the cell's corner
            mOwn = mmake<UiOwnGraphicsStrip>();
            mOwn->name = "own graphics";
            mOwnCells = UiOwnCellsOf(*mNode);
            mOwn->SetCells(mOwnCells);
            AddRow(mOwn, [weakThis](float width)
            {
                auto self = weakThis.Lock();
                return self && self->mOwn ? UiOwnStripGeometry(width, self->mOwn->GetCells().Count()).height : 0.0f;
            });
            mOwnRow = mRows.Count() - 1;
            mRows[mOwnRow].gap = mOwnCells.IsEmpty() ? 0.0f : -1.0f;

            // The status line: blocks and steps, the kit coverage, undo / redo
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
            AddRow(stat, 24);

            // Save to project: the button, the .proto it writes (or wrote) and its place in the Assets window
            auto saveRow = mmake<HorizontalLayout>();
            saveRow->name = "save row";
            saveRow->spacing = 4;
            saveRow->expandWidth = true;
            saveRow->expandHeight = true;
            saveRow->baseCorner = BaseCorner::Left;
            mSave = MakeGhostButton("Save to project");
            mSave->name = "save to project";
            mSave->layout->minWidth = mSave->layout->maxWidth = UiWsDraw::TextWidth("Save to project", 11.0f) + 18.0f;
            mSave->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    PipelineUiAi::SaveToProject(self->mEditor.Lock(), self->mNode->id);
            };
            saveRow->AddChild(mSave);
            mSaved = MakeLabel("", true);
            mSaved->name = "saved path";
            StyleLabel(mSaved, 10.5f, 400, theme.fgDim);
            mSaved->horOverflow = Label::HorOverflow::Dots;
            saveRow->AddChild(mSaved);
            mLocate = MakeIconButton("ui/pipeline/btn_locate.svg", theme.fgDim, Color4(0, 0, 0, 0));
            mLocate->name = "show in assets";
            mLocate->layout->minWidth = mLocate->layout->maxWidth = 24;
            mLocate->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    PipelineUiAi::ShowInAssets(PipelineUiAi::PrototypePath(*self->mNode));
            };
            saveRow->AddChild(mLocate);
            AddRow(saveRow, 24);

            // The answer line: an error, a layout in older units (with the button that brings it over), the kit not ready for the
            // agent, or its last words. A notice there costs the card no height: the ports below stay put
            auto replyRow = mmake<Widget>();
            replyRow->name = "reply row";
            mReply = MakeLabel("", true);
            mReply->name = "reply";
            StyleLabel(mReply, 10.5f, 400, theme.fgDim);
            mReply->verAlign = VerAlign::Top;
            mReply->horOverflow = Label::HorOverflow::Wrap;
            *mReply->layout = WidgetLayout::BothStretch();
            replyRow->AddChild(mReply);
            mBring = MakeGhostButton("Bring to the kit's device");
            mBring->name = "bring to device";
            mBringWidth = UiWsDraw::TextWidth("Bring to the kit's device", 11.0f) + 18.0f;
            *mBring->layout = WidgetLayout(Vec2F(1, 0.5f), Vec2F(1, 0.5f), Vec2F(-mBringWidth, -13), Vec2F(0, 13));
            mBring->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->mContext->Bring(); };
            mBring->SetEnabledForcible(false);
            replyRow->AddChild(mBring);
            AddRow(replyRow, 38);

            // The request with its send button inside
            auto field = mmake<Widget>();
            field->name = "request";
            // The points marked in the workspace are [1], [2]... tokens of the same text: deleting one removes its point
            mDraft = MakeEditBox(mContext->draft, true, FromReference() ?
                String("What to build or change... (empty: build the screen from the reference)") :
                String("Describe the screen, or what to change - e.g. \"a main menu: title, Play, Settings, coins in a corner\""));
            mDraft->name = "request field";
            *mDraft->layout = WidgetLayout::BothStretch();
            mDraft->onChanged = [weakThis](const WString&)
            {
                if (auto self = weakThis.Lock())
                    UiWsPins::OnFieldChanged(*self->mContext, self->mDraft.Get());
            };
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

            if (BeginParams({ PipelineUtils::PrettyModelName(UiDoc::Ws::AgentModelOf(mContext->config)),
                              UiDoc::ExportFolderOf(mNode->GetConfigString("assetPath", "")) }))
            {
                AddModelRow(PipelineTextModelPresets(), "gpt-6.1-sol", PipelineModelKind::Text, "agentModel", "Agent model", 100);
                AddModelRow(PipelineImageModelPresets(), UiDoc::UI_KIT_AI_DEFAULT_IMAGE_MODEL, PipelineModelKind::Image, "agentImageModel",
                            "Image model", 100);
                AddFolderRow();
                auto review = AddCheckbox("Check pass after each answer", "agentReview", true);
                review->name = "check pass";
                auto draw = AddCheckbox("Draw what the kit lacks", "agentDraw", true);
                draw->name = "draw missing";
                AddTurnsRow();
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
                UiWsControls::SetTextQuiet(mDraft, mNode->GetConfigString("uiDraft", ""));

            Refresh(true);
            UiWorkspace::Refresh(mNode->id);
        }

        // The own graphics' output ports sit on their cells' top-right corners; image and layout keep their port rows
        bool HasBodyPort(const String& portId) const override
        {
            return mOwn && mOwn->GetCellIndex(portId) >= 0;
        }

        bool GetBodyPortOffset(const String& portId, Vec2F& offset) const override
        {
            auto owner = mOwner.Lock();
            if (!owner || !HasBodyPort(portId))
                return false;

            float width = owner->GetCardWidth(), top = 0.0f, height = 0.0f;
            if (!GetRowPlacement(mOwn, width, owner->GetBodyAreaHeight(), top, height))
                return false;

            int index = mOwn->GetCellIndex(portId);
            UiOwnStripGeometry g(width - mPadding*2.0f, mOwn->GetCells().Count());
            Vec2F corner = g.Corner(index);
            offset = Vec2F(mPadding + corner.x, top + corner.y);
            return true;
        }

        void Update(float dt) override
        {
            PipelineNodeBody::Update(dt);
            SyncOwnCells();

            // The agent answers between frames, the kit changes upstream: the context notices
            bool busy = PipelineUiAi::IsBusy("agent", mNode->id);
            if (busy != mShownBusy || (mContext && mContext->RefreshIfChanged()))
                Refresh(true);
            else if (busy)
            {
                // The run's line follows its acts (and its clock, once a second)
                UiAgentState::LiveRun live;
                mRunClock += dt;
                if (UiAgentState::GetRun(mNode->id, live) && (live.version != mShownRun || mRunClock > 1.0f))
                {
                    mRunClock = 0.0f;
                    Refresh(false);
                }
            }
        }

    private:
        Ref<UiWsContext>      mContext;  // The node read as the workspace reads it: its layout, kit, preview screen
        Vector<Ref<Toggle>>   mViews;    // Line art | kit
        Ref<UiDevicePicker>   mDevice;   // The preview screen
        Ref<UiRefButton>      mCompare;  // The comparison with the reference and its menu
        Ref<PipelineImageView> mPreview; // The screen drawn
        Ref<UiRefOverlayView> mOverlay;  // The reference over the screen while compared
        Ref<UiOwnGraphicsStrip> mOwn;    // The node's own graphics
        Vector<UiOwnCell>     mOwnCells; // The cells shown
        int                   mOwnRow = -1;
        int                   mOwnVersion = -1; // The context version the cells were read at
        Ref<Button>           mBring;    // Brings a layout in older units to the kit's device
        float                 mBringWidth = 0.0f;
        Ref<Label>            mStat;     // Blocks and steps
        Ref<Button>           mCoverage; // How much of the layout draws with the kit
        Ref<Button>           mUndo;
        Ref<Button>           mRedo;
        Ref<Button>           mSave;     // Save to project
        Ref<Label>            mSaved;    // Where the prototype goes, whether it is there
        Ref<Button>           mLocate;   // Shows the saved prototype in the Assets window
        Ref<Label>            mReply;    // The agent's last answer
        Ref<EditBox>          mDraft;    // The request
        Ref<Button>           mSend;

        String mRendered;         // What the preview was drawn for
        bool   mShownBusy = false;
        int    mShownRun = -1;    // The live run's version the answer line shows
        float  mRunClock = 0.0f;  // Seconds since the answer line's clock was shown

        // The cells follow the node's own graphics (an answer, an undo): the strip grows or shrinks, the card with it
        void SyncOwnCells()
        {
            if (!mContext || !mOwn || mContext->version == mOwnVersion)
                return;

            mOwnVersion = mContext->version;
            auto cells = UiOwnCellsOf(*mNode);
            if (cells == mOwnCells)
                return;

            auto owner = mOwner.Lock();
            auto change = [&]()
            {
                mOwnCells = cells;
                mOwn->SetCells(cells);
                if (mOwnRow >= 0 && mOwnRow < mRows.Count())
                    mRows[mOwnRow].gap = cells.IsEmpty() ? 0.0f : -1.0f;
            };
            if (owner)
                owner->KeepAreaThrough(change);
            else
                change();
        }

        // The folder inside Assets Save to project writes into, typed or picked from the project's folders
        void AddFolderRow()
        {
            WeakRef<UiLayoutBody> weakThis(this);
            auto row = MakeRow("Folder", nullptr, 100);
            auto edit = MakeEditBox(mNode->GetConfigString("assetPath", ""), false, "Generated/UI");
            edit->name = "asset folder";
            edit->onChangeCompleted = [weakThis](const WString& text) { if (auto self = weakThis.Lock()) self->SetFolder((String)text); };
            row->AddChild(edit);
            auto browse = MakeIconButton("ui/pipeline/btn_import.svg", PipelineTheme::Get().fgDim, Color4(0, 0, 0, 0));
            browse->name = "browse folder";
            browse->layout->minWidth = browse->layout->maxWidth = 24;
            browse->onClick = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    self->ShowAssetFolderMenu([weakThis](const String& folder) { if (auto self = weakThis.Lock()) self->SetFolder(folder); });
            };
            row->AddChild(browse);
            AddRow(row, 26.85f);
        }

        void SetFolder(const String& text)
        {
            String folder = text.Trimed(" \t\r\n");
            folder.ReplaceAll("\\", "/");
            while (folder.StartsWith("/"))
                folder = folder.SubStr(1);
            while (folder.EndsWith("/"))
                folder = folder.SubStr(0, folder.Length() - 1);
            if (folder != mNode->GetConfigString("assetPath", ""))
                SetString("assetPath", folder, true);
            RebuildBodyKeepingArea();
        }

        // The turns the layout pass may take when a screen is rebuilt from its reference
        void AddTurnsRow()
        {
            auto row = mmake<HorizontalLayout>();
            row->spacing = 2;
            row->expandWidth = true;
            row->expandHeight = true;
            row->baseCorner = BaseCorner::Left;
            SkinSegmentedRow(*row);
            WeakRef<UiLayoutBody> weakThis(this);
            int turns = TurnsOf(mContext->config);
            for (int n : { 20, 40, 60, 90, 120 })
            {
                auto segment = MakeSegment((String)n, turns == n);
                segment->name = "turns " + (String)n;
                segment->onToggleByUser = [weakThis, n](bool)
                {
                    if (auto self = weakThis.Lock())
                    {
                        self->mContext->SetUi("agentTurns", UiDoc::Json(n));
                        self->RebuildBodyKeepingArea();
                    }
                };
                row->AddChild(segment);
            }
            auto block = mmake<Widget>();
            *row->layout = WidgetLayout::BothStretch(3, 3, 3, 3);
            block->AddChild(row);
            AddRow(MakeRow("Turns", block, 100), 26.85f);
        }

        // The node's agentTurns when it is one of the offered counts, else the default
        static int TurnsOf(const UiDoc::Json& config)
        {
            double n = UiDoc::JsRound(UiDoc::JsNumber(UiDoc::JsGet(config, "agentTurns")));
            for (int offered : { 20, 40, 60, 90, 120 })
            {
                if (n == offered)
                    return offered;
            }
            return UiDoc::UI_AGENT_TURNS;
        }

        // An empty layout with a reference picture: an empty request builds the screen from it
        bool FromReference() const
        {
            return mContext && mContext->reference && mContext->current.root.children.empty();
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
            if (prompt.Trimed(" \n\r\t").IsEmpty() && !FromReference())
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
            if (mCompare && mCompare->IsEnabled() != mContext->reference.has_value())
                mCompare->SetEnabledForcible(mContext->reference.has_value());

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
            RefreshSaveRow(blocks);
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
            // While the agent works: what its run is doing now
            UiAgentState::LiveRun live;
            bool older = !busy && UiDoc::Ws::InOlderUnits(mContext->current, mContext->target);
            ShowBring(older && error.IsEmpty());
            if (busy && UiAgentState::GetRun(mNode->id, live))
            {
                mReply->text = UiWsRunActs::RunNow(live.acts, live.startedMs, (double)UiDoc::NowMs());
                mReply->color = theme.fgDim;
                mShownRun = live.version;
                if (redraw)
                    Redraw();
                return;
            }
            if (older && error.IsEmpty())
            {
                mReply->text = UiDoc::Ws::OlderText(mContext->current, mContext->target);
                mReply->color = theme.yellow;
                if (redraw)
                    Redraw();
                return;
            }
            mReply->text = !error.IsEmpty() ? error :
                kitWarn ? (String)mContext->undescribed + " kit graphics have no type or description yet - describe them with the kit's AI first" :
                !reply.IsEmpty() ? reply : String("The agent's answers show here. Ask for a screen below; every answer is a step you can undo.");
            mReply->color = !error.IsEmpty() ? theme.red : kitWarn ? theme.yellow : !reply.IsEmpty() ? theme.fg : theme.fg3;

            if (redraw)
                Redraw();
        }

        // The save line: saving, the last save's error, the saved .proto, or where a save will write it
        void RefreshSaveRow(int blocks)
        {
            auto& theme = PipelineTheme::Get();
            bool saving = PipelineUiAi::IsBusy("export", mNode->id);
            String error = PipelineUiAi::LastError("export", mNode->id);
            String proto = PipelineUiAi::PrototypePath(*mNode);
            bool saved = o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + proto);

            mSave->interactable = blocks > 0 && !saving;
            mSave->transparency = mSave->interactable ? 1.0f : 0.35f;
            mLocate->interactable = saved && !saving;
            mLocate->transparency = mLocate->interactable ? 1.0f : 0.35f;
            mSaved->text = saving ? String("Saving...") : !error.IsEmpty() ? "Not saved: " + error :
                saved ? "Assets/" + proto : "Not saved yet \xC2\xB7 Assets/" + proto;
            mSaved->color = !error.IsEmpty() && !saving ? theme.red : saved ? theme.fg : theme.fg3;
        }

        // Shows the button that brings an older layout to the kit's device beside the answer line, or hides it
        void ShowBring(bool show)
        {
            if (!mBring)
                return;

            if (mBring->IsEnabled() != show)
                mBring->SetEnabledForcible(show);
            mBring->interactable = show && mContext->Editable();
            *mReply->layout = WidgetLayout::BothStretch(0, 0, show ? mBringWidth + 8.0f : 0.0f, 0);
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
