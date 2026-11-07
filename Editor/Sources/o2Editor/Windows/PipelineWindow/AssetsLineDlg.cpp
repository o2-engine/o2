#include "o2Editor/stdafx.h"
#include "AssetsLineDlg.h"

#include "o2Editor/Pipeline/Providers/LocalAgentProvider.h"

#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/UI/Style/EditorThemeDark.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2/Render/VectorSprite.h"

DECLARE_SINGLETON(Editor::AssetsLineDlg);

namespace Editor
{
    static const char* kDefaultServer = "https://assetsline.app";
    static const char* kBrowser = "Browser";
    static const char* kToken = "Access token";

    static const float kWidth = 500.0f;
    static const float kHeaderHeight = 48.0f;
    static const float kCaptionWidth = 92.0f;
    static const float kRowHeight = 22.0f;

    // Titles and values, and the quiet hints, in the editor theme
    static Color4 TitleColor() { return EditorTheme::IsDark() ? EditorThemeDark::textStrong : Color4(38, 50, 56, 255); }
    static Color4 HintColor() { return EditorTheme::IsDark() ? EditorThemeDark::ink : Color4(144, 164, 174, 255); }

    static String StatusTitle(AssetsLineStatus status)
    {
        switch (status)
        {
            case AssetsLineStatus::NotConnected: return "Not connected";
            case AssetsLineStatus::Connecting: return "Connecting...";
            case AssetsLineStatus::Syncing: return "Syncing...";
            case AssetsLineStatus::Synced: return "Synced";
            case AssetsLineStatus::Offline: return "Offline";
            case AssetsLineStatus::Error: return "Sync error";
            default: return "Sync is off";
        }
    }

    // Height of the rows a vertical layout stacks; a disabled row takes none
    static float StackHeight(const Ref<Widget>& widget)
    {
        auto stack = DynamicCast<VerticalLayout>(widget);
        if (!stack)
            return widget->layout->GetMinHeight();

        float height = 0;
        int shown = 0;
        for (auto& child : stack->GetChildWidgets())
        {
            if (child->IsEnabled())
            {
                height += StackHeight(child);
                shown++;
            }
        }

        return height + Math::Max(shown - 1, 0)*stack->GetSpacing();
    }

    // The colours of an agent's state
    static Color4 ReadyColor() { return Color4(56, 176, 88, 255); }
    static Color4 BusyColor() { return Color4(48, 132, 226, 255); }
    static Color4 SignedOutColor() { return Color4(226, 150, 30, 255); }
    static Color4 FailedColor() { return Color4(228, 74, 60, 255); }
    static Color4 IdleColor() { return Color4(158, 170, 176, 255); }

    static const char* dash = " \xE2\x80\x94 ";

    // The model menu of Claude Code: the value stored, then the text shown
    static const Vector<std::pair<String, String>>& ClaudeModels()
    {
        static Vector<std::pair<String, String>> models = {
            { "", "Default (Claude Code's choice)" },
            { "haiku", String("Haiku") + dash + "fastest" },
            { "sonnet", String("Sonnet") + dash + "balanced" },
            { "opus", String("Opus") + dash + "most capable" } };
        return models;
    }

    static const Vector<std::pair<String, String>>& ClaudeThinking()
    {
        static Vector<std::pair<String, String>> items = {
            { "off", String("Off") + dash + "fewest tokens" },
            { "on", String("On") + dash + "deeper answers, more tokens" } };
        return items;
    }

    static const Vector<std::pair<String, String>>& CodexEfforts()
    {
        static Vector<std::pair<String, String>> items = {
            { "minimal", String("Minimal") + dash + "fastest" },
            { "low", String("Low") + dash + "recommended" },
            { "medium", "Medium" },
            { "high", String("High") + dash + "slower" },
            { "xhigh", String("Extra high") + dash + "slowest" },
            { "", "Codex default" } };
        return items;
    }

    static const char* kClaudeSubscription = "Claude subscription";
    static const char* kClaudeKey = "Anthropic API key";
    static const char* kCodexSubscription = "ChatGPT subscription";
    static const char* kCodexKey = "OpenAI API key";

    static String TextOf(const Vector<std::pair<String, String>>& items, const String& value)
    {
        for (auto& item : items)
        {
            if (item.first == value)
                return item.second;
        }
        return items.IsEmpty() ? String() : items[0].second;
    }

    static String ValueOf(const Vector<std::pair<String, String>>& items, const String& text)
    {
        for (auto& item : items)
        {
            if (item.second == text)
                return item.first;
        }
        return items.IsEmpty() ? String() : items[0].first;
    }

    static Ref<VerticalLayout> AddSection(const Ref<VerticalLayout>& parent, const String& name)
    {
        auto section = mmake<VerticalLayout>();
        section->name = name;
        section->spacing = 6;
        section->expandWidth = true;
        section->expandHeight = false;
        section->fitByChildren = true;
        section->baseCorner = BaseCorner::Top;
        parent->AddChild(section);
        return section;
    }

    // A form row: a caption column, then the fields; no caption, the fields take the whole row
    static Ref<HorizontalLayout> AddRow(const Ref<VerticalLayout>& parent, const String& caption, bool captionColumn = true)
    {
        auto row = mmake<HorizontalLayout>();
        row->layout->minHeight = kRowHeight;
        row->spacing = 6;
        row->expandWidth = true;
        row->expandHeight = true;
        row->baseCorner = BaseCorner::Left;
        parent->AddChild(row);

        if (captionColumn)
        {
            auto label = o2UI.CreateLabel(caption);
            label->horAlign = HorAlign::Left;
            label->layout->minWidth = kCaptionWidth;
            label->layout->maxWidth = kCaptionWidth;
            row->AddChild(label);
        }
        return row;
    }

    static Ref<Label> AddHint(const Ref<VerticalLayout>& parent, const String& text, bool captionColumn = true)
    {
        auto row = AddRow(parent, "", captionColumn);
        row->layout->minHeight = 16;
        auto label = o2UI.CreateLabel(text);
        label->horAlign = HorAlign::Left;
        label->horOverflow = Label::HorOverflow::Dots;
        label->color = HintColor();
        row->AddChild(label);
        return label;
    }

    static Ref<EditBox> AddEdit(const Ref<HorizontalLayout>& row, const String& name)
    {
        auto edit = o2UI.CreateWidget<EditBox>("singleline");
        edit->name = name;
        row->AddChild(edit);
        return edit;
    }

    static Ref<Button> AddButton(const Ref<HorizontalLayout>& row, const String& caption, float width, const Function<void()>& onClick)
    {
        auto button = o2UI.CreateButton(caption, onClick);
        button->name = caption;
        button->layout->minWidth = width;
        button->layout->maxWidth = width;
        row->AddChild(button);
        return button;
    }

    static Ref<DropDown> AddDropDown(const Ref<HorizontalLayout>& row, const String& name, float width)
    {
        auto dropDown = o2UI.CreateDropdown();
        dropDown->name = name;
        dropDown->layout->minWidth = width;
        dropDown->layout->maxWidth = width;
        row->AddChild(dropDown);
        return dropDown;
    }

    static void AddSeparator(const Ref<VerticalLayout>& parent)
    {
        auto separator = mmake<Widget>();
        separator->layout->minHeight = 9;
        separator->AddLayer("line", mmake<VectorSprite>("ui/UI4_Separator.svg"), Layout::HorStretch(VerAlign::Middle, 0, 0, 5, 0));
        parent->AddChild(separator);
    }

    static AssetsLineDlg::Page lastPage = AssetsLineDlg::Page::Sync;

    // The ids of Codex's models, to see the catalog change
    static String CatalogKey()
    {
        String key;
        for (auto& model : LocalAgentSetup::Check::GetCodexModels())
            key += model.id + ",";
        return key;
    }

    AssetsLineDlg::AssetsLineDlg(RefCounter* refCounter):
        Singleton<AssetsLineDlg>(refCounter)
    {
        PushEditorScopeOnStack scope;
        mClaude.agent = LocalAgentSetup::Agent::ClaudeCode;
        mCodex.agent = LocalAgentSetup::Agent::Codex;
        InitializeControls();
    }

    AssetsLineDlg::~AssetsLineDlg()
    {}

    void AssetsLineDlg::InitializeControls()
    {
        mWindow = o2UI.CreateWindow("Pipeline settings");
        mWindow->name = "assetsline dialog";
        mWindow->SetIcon(mmake<VectorSprite>("ui/pipeline/window_icon.svg"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
        *mWindow->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(kWidth, 400));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 17));
        mWindow->SetViewLayout(Layout::BothStretch(0, 5, 0, 18));

        mHeader = mmake<Widget>();
        mHeader->name = "header";
        *mHeader->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, kHeaderHeight, 0);
        mHeader->AddLayer("back", mmake<VectorSprite>("ui/UI4_small_panel_back.svg"), Layout::BothStretch(-5, -5, -4, -5));
        mStatusDot.Attach(mHeader, Vec2F(0.0f, 0.5f), Vec2F(22.0f, 0.0f));
        mWindow->AddChild(mHeader);

        mStatusLabel = o2UI.CreateLabel("");
        mStatusLabel->name = "status";
        mStatusLabel->horAlign = HorAlign::Left;
        mStatusLabel->horOverflow = Label::HorOverflow::Dots;
        mStatusLabel->color = TitleColor();
        *mStatusLabel->layout = WidgetLayout::HorStretch(VerAlign::Top, 38, 12, 18, 6);
        mHeader->AddChild(mStatusLabel);

        mStatusDetail = o2UI.CreateLabel("");
        mStatusDetail->name = "status detail";
        mStatusDetail->horAlign = HorAlign::Left;
        mStatusDetail->horOverflow = Label::HorOverflow::Dots;
        *mStatusDetail->layout = WidgetLayout::HorStretch(VerAlign::Top, 38, 12, 18, 24);
        mHeader->AddChild(mStatusDetail);

        auto content = mmake<VerticalLayout>();
        content->name = "content";
        *content->layout = WidgetLayout::BothStretch(14, 10, 14, kHeaderHeight + 12);
        content->spacing = 6;
        content->expandWidth = true;
        content->expandHeight = false;
        content->fitByChildren = true;
        content->baseCorner = BaseCorner::Top;
        mContent = content;
        mWindow->AddChild(content);

        // A tab a page
        auto tabs = mmake<HorizontalLayout>();
        tabs->name = "tabs";
        tabs->layout->minHeight = 26;
        tabs->spacing = 4;
        tabs->expandWidth = false;
        tabs->expandHeight = true;
        tabs->baseCorner = BaseCorner::Left;
        content->AddChild(tabs);
        const char* tabCaptions[4] = { "AssetsLine", "API keys", "Local agents", "Editor" };
        const char* tabNames[4] = { "tab sync", "tab keys", "tab agents", "tab editor" };
        for (int i = 0; i < 4; i++)
        {
            auto tab = o2UI.CreateWidget<Toggle>("pipeline segment");
            tab->caption = tabCaptions[i];
            tab->name = tabNames[i];
            tab->layout->minWidth = 113;
            tab->layout->maxWidth = 113;
            tab->onToggleByUser = [this, i](bool) { ShowPage((Page)i); };
            tabs->AddChild(tab);
            mTabs[i] = tab;
        }

        auto syncPage = AddSection(content, "sync page");
        auto keysPage = AddSection(content, "keys page");
        auto agentsPage = AddSection(content, "agents page");
        auto editorPage = AddSection(content, "editor page");
        mPages[0] = syncPage;
        mPages[1] = keysPage;
        mPages[2] = agentsPage;
        mPages[3] = editorPage;

        // AssetsLine. Not connected: the server and the way to connect
        mConnectSection = AddSection(syncPage, "connect");
        auto connectSection = DynamicCast<VerticalLayout>(mConnectSection);

        mServerEdit = AddEdit(AddRow(connectSection, "Server"), "server");

        auto typeRow = AddRow(connectSection, "Connection");
        mConnectionType = o2UI.CreateDropdown();
        mConnectionType->name = "connection type";
        mConnectionType->AddItem(kBrowser);
        mConnectionType->AddItem(kToken);
        mConnectionType->SelectItemText(kBrowser);
        mConnectionType->layout->minWidth = 150;
        mConnectionType->layout->maxWidth = 150;
        mConnectionType->onSelectedText = [this](const WString&) { OnConnectionTypeChanged(); };
        typeRow->AddChild(mConnectionType);

        auto browserPanel = AddSection(connectSection, "browser panel");
        mBrowserPanel = browserPanel;
        AddHint(browserPanel, "Opens AssetsLine in the browser to approve this editor");
        auto browserButtons = AddRow(browserPanel, "");
        mConnectButton = AddButton(browserButtons, "Connect", 100, [this]()
        {
            o2AssetsLineSync.StartConnect((String)mServerEdit->GetText());
        });
        mCancelButton = AddButton(browserButtons, "Cancel", 80, []() { o2AssetsLineSync.CancelConnect(); });

        auto approvePanel = AddSection(browserPanel, "approve panel");
        mApprovePanel = approvePanel;
        auto codeRow = AddRow(approvePanel, "Code");
        mCodeLabel = o2UI.CreateLabel("");
        mCodeLabel->name = "code";
        mCodeLabel->horAlign = HorAlign::Left;
        mCodeLabel->color = TitleColor();
        codeRow->AddChild(mCodeLabel);
        mApproveEdit = AddEdit(AddRow(approvePanel, "Page"), "approve url");
        AddHint(approvePanel, "If the browser did not open, open this page by hand");

        auto tokenPanel = AddSection(connectSection, "token panel");
        mTokenPanel = tokenPanel;
        auto tokenRow = AddRow(tokenPanel, "Token");
        mTokenEdit = AddEdit(tokenRow, "token");
        mTokenButton = AddButton(tokenRow, "Connect", 90, [this]()
        {
            o2AssetsLineSync.ConnectWithToken((String)mServerEdit->GetText(), (String)mTokenEdit->GetText());
            mTokenEdit->SetText("");
        });
        mTokenButton->name = "use token";
        AddHint(tokenPanel, "Make one in AssetsLine: Settings, Connected editors");

        // Connected: the linked project
        mLinkedSection = AddSection(syncPage, "linked");
        auto linkedSection = DynamicCast<VerticalLayout>(mLinkedSection);
        auto projectRow = AddRow(linkedSection, "Project");
        mProjectLabel = o2UI.CreateLabel("");
        mProjectLabel->name = "project";
        mProjectLabel->horAlign = HorAlign::Left;
        mProjectLabel->horOverflow = Label::HorOverflow::Dots;
        mProjectLabel->color = TitleColor();
        projectRow->AddChild(mProjectLabel);
        mDisconnectButton = AddButton(projectRow, "Disconnect", 100, []() { o2AssetsLineSync.Disconnect(); });

        AddSeparator(syncPage);

        mFolderEdit = AddEdit(AddRow(syncPage, "Folder"), "folder");
        mFolderEdit->onChangeCompleted = [this](const WString&) { WriteConfig(); };
        AddHint(syncPage, "Pipelines inside this folder of Assets are synced");

        mEnabledToggle = o2UI.CreateToggle("Sync is on");
        mEnabledToggle->name = "enabled";
        mEnabledToggle->onToggleByUser = [this](bool) { WriteConfig(); };
        AddRow(syncPage, "")->AddChild(mEnabledToggle);

        mFinishToggle = o2UI.CreateToggle("Write results of finish nodes into the assets");
        mFinishToggle->name = "finish assets";
        mFinishToggle->onToggleByUser = [this](bool) { WriteConfig(); };
        AddRow(syncPage, "")->AddChild(mFinishToggle);

        // API keys of the providers the nodes call when they run in this editor
        AddHint(keysPage, "Used when nodes run in this editor; empty: the environment's key", false);
        auto addKey = [&](const String& caption, const String& name)
        {
            auto edit = AddEdit(AddRow(keysPage, caption), name);
            edit->onChangeCompleted = [this](const WString&) { WriteKeys(); };
            return edit;
        };
        mGeminiEdit = addKey("Gemini", "gemini key");
        mOpenAiEdit = addKey("OpenAI", "openai key");
        mOpenRouterEdit = addKey("OpenRouter", "openrouter key");
        mElevenEdit = addKey("ElevenLabs", "elevenlabs key");
        mKlingAccessEdit = addKey("Kling key", "kling key");
        mKlingSecretEdit = addKey("Kling secret", "kling secret");
        AddHint(keysPage, "Only for an old Kling key pair");

        BuildAgentsPage(agentsPage);

        // Editor: how the image-to-image nodes show their input next to their result (a local setting, not synced), and the
        // Python of AI image to UI
        auto viewRow = AddRow(editorPage, "Image nodes");
        auto addView = [&](const String& caption, const String& name, PipelineIoView view)
        {
            auto segment = o2UI.CreateWidget<Toggle>("pipeline segment");
            segment->caption = caption;
            segment->SetValue(PipelinePairLayout::GetIoView() == view);
            segment->name = name;
            segment->layout->minWidth = 100;
            segment->layout->maxWidth = 100;
            segment->onToggleByUser = [this, view](bool)
            {
                PipelinePairLayout::SetIoView(view);
                mSideView->SetValue(view == PipelineIoView::SideBySide);
                mCompareView->SetValue(view == PipelineIoView::Compare);
            };
            viewRow->AddChild(segment);
            return segment;
        };
        mSideView = addView("Side by side", "side view", PipelineIoView::SideBySide);
        mCompareView = addView("Compare", "compare view", PipelineIoView::Compare);

        mPythonEdit = AddEdit(AddRow(editorPage, "Python 3"), "image to ui python");
        mPythonEdit->onChangeCompleted = [this](const WString&) { WriteKeys(); };
        AddHint(editorPage, "For AI image to UI, with Pillow and numpy; empty: python3 on PATH");

        AddSeparator(content);

        auto actions = mmake<HorizontalLayout>();
        actions->layout->minHeight = kRowHeight;
        actions->spacing = 6;
        actions->expandWidth = false;
        actions->expandHeight = true;
        actions->baseCorner = BaseCorner::Right;
        content->AddChild(actions);
        mSyncButton = AddButton(actions, "Sync now", 100, []() { o2AssetsLineSync.SyncNow(); });
        AddButton(actions, "Close", 80, [this]() { mWindow->Hide(); });

        OnConnectionTypeChanged();
        ShowPage(Page::Sync);

        mWindow->Hide(true);
        EditorUIRoot.AddWidget(mWindow);
    }

    void AssetsLineDlg::BuildAgentsPage(const Ref<VerticalLayout>& page)
    {
        AddHint(page, "Agents on this computer answer AI nodes on your Claude or ChatGPT plan", false);

        mLocalToggle = o2UI.CreateToggle("Show the local agents in the model menus");
        mLocalToggle->name = "local agent on";
        mLocalToggle->onToggleByUser = [this](bool) { WriteKeys(); };
        AddRow(page, "", false)->AddChild(mLocalToggle);

        AddSeparator(page);
        BuildAgentCard(page, mClaude);
        AddSeparator(page);
        BuildAgentCard(page, mCodex);
        AddSeparator(page);

        // The executables found by themselves, and any other agent: folded
        mCustomToggle = o2UI.CreateToggle("Custom paths and command");
        mCustomToggle->name = "local agent custom";
        mCustomToggle->onToggleByUser = [this](bool on)
        {
            mCustomSection->enabled = on;
            FitHeight();
        };
        AddRow(page, "", false)->AddChild(mCustomToggle);

        auto custom = AddSection(page, "local agent custom section");
        mCustomSection = custom;
        auto addPath = [&](const String& caption, const String& name)
        {
            auto edit = AddEdit(AddRow(custom, caption), name);
            edit->onChangeCompleted = [this](const WString&)
            {
                WriteKeys();
                CheckAgents();
            };
            return edit;
        };
        mLocalPathEdit = addPath("Claude Code", "local agent path");
        mLocalFound = AddHint(custom, "");
        mLocalFound->name = "local agent found";
        mCodexPathEdit = addPath("Codex", "codex path");
        mCodexFound = AddHint(custom, "");
        mCodexFound->name = "codex found";
        mLocalCommandEdit = AddEdit(AddRow(custom, "Command"), "local command");
        mLocalCommandEdit->onChangeCompleted = [this](const WString&) { WriteKeys(); };
        AddHint(custom, "Any other agent: prompt on stdin, answer on stdout (Local command)");
        custom->enabled = false;
    }

    void AssetsLineDlg::BuildAgentCard(const Ref<VerticalLayout>& page, AgentCard& card)
    {
        bool claude = card.agent == LocalAgentSetup::Agent::ClaudeCode;
        String key = claude ? "claude" : "codex";
        AgentCard* cardPtr = &card;
        auto box = AddSection(page, key + " card");

        // The name, what it does, the state
        auto head = AddRow(box, "", false);
        auto name = o2UI.CreateLabel(LocalAgentSetup::NameOf(card.agent));
        name->name = key + " name";
        name->horAlign = HorAlign::Left;
        name->color = TitleColor();
        name->layout->minWidth = kCaptionWidth;
        name->layout->maxWidth = kCaptionWidth;
        head->AddChild(name);

        auto what = o2UI.CreateLabel(claude ? "Text and vision" : "Text, vision and pictures");
        what->horAlign = HorAlign::Left;
        what->horOverflow = Label::HorOverflow::Dots;
        what->color = HintColor();
        head->AddChild(what);

        card.dot = mmake<Widget>();
        card.dot->name = key + " dot";
        card.dot->layout->minWidth = 10;
        card.dot->layout->maxWidth = 10;
        card.dot->AddLayer("dot", mmake<VectorSprite>("ui/pipeline/status_dot.svg"), Layout::Based(BaseCorner::Center, Vec2F(9, 9)));
        head->AddChild(card.dot);

        card.state = o2UI.CreateLabel("Checking...");
        card.state->name = key + " state";
        card.state->horAlign = HorAlign::Left;
        card.state->color = TitleColor();
        card.state->layout->minWidth = 104;
        card.state->layout->maxWidth = 104;
        head->AddChild(card.state);

        // What it runs on, or what to do; the setup and the check
        auto line = AddRow(box, "", false);
        card.detail = o2UI.CreateLabel("");
        card.detail->name = key + " detail";
        card.detail->horAlign = HorAlign::Left;
        card.detail->horOverflow = Label::HorOverflow::Dots;
        card.detail->color = HintColor();
        line->AddChild(card.detail);
        card.setup = AddButton(line, "Install and sign in...", 150, [this, cardPtr]() { SetUpAgent(*cardPtr); });
        card.setup->name = key + " setup";
        card.recheck = AddButton(line, "Check", 64, [this, cardPtr]()
        {
            cardPtr->setupError = "";
            CheckAgents();
        });
        card.recheck->name = key + " check";

        // The options, once it is installed
        auto options = AddSection(box, key + " options");
        card.options = options;
        card.model = AddDropDown(AddRow(options, "Model"), key + " model", 250);
        card.tuning = AddDropDown(AddRow(options, claude ? "Thinking" : "Effort"), key + (claude ? " thinking" : " effort"), 250);
        card.account = AddDropDown(AddRow(options, "Account"), key + " account", 250);
        card.account->AddItem(claude ? kClaudeSubscription : kCodexSubscription);
        card.account->AddItem(claude ? kClaudeKey : kCodexKey);
        for (auto& item : claude ? ClaudeThinking() : CodexEfforts())
            card.tuning->AddItem(item.second);

        auto keyRow = AddSection(options, key + " key row");
        card.keyRow = keyRow;
        card.key = AddEdit(AddRow(keyRow, "API key"), key + " key");
        AddHint(keyRow, claude ? "Billed by Anthropic at API prices instead of your subscription"
                               : "Billed by OpenAI at API prices instead of your subscription");

        auto changed = [this, cardPtr](const WString&)
        {
            if (!mReadingAgents)
                WriteAgent(*cardPtr);
        };
        card.model->onSelectedText = changed;
        card.tuning->onSelectedText = changed;
        card.account->onSelectedText = changed;
        card.key->onChangeCompleted = changed;
    }

    void AssetsLineDlg::ReadAgent(AgentCard& card)
    {
        bool claude = card.agent == LocalAgentSetup::Agent::ClaudeCode;
        auto settings = PipelineSettings::Load();
        mReadingAgents = true;

        // The models: Claude's aliases, Codex's catalog; a model set by hand stays in the menu
        Vector<std::pair<String, String>> models;
        if (claude)
            models = ClaudeModels();
        else
        {
            models.Add({ "", "Default (Codex's choice)" });
            for (auto& model : LocalAgentSetup::Check::GetCodexModels())
                models.Add({ model.id, model.name });
            card.catalog = CatalogKey();
        }
        String current = (claude ? settings.localAgentModel : settings.localCodexModel).Trimed(" \n\r\t");
        if (!models.Any([&](const std::pair<String, String>& m) { return m.first == current; }))
            models.Add({ current, "Custom: " + current });

        for (auto& text : card.model->GetAllItemsText())
            card.model->RemoveItem(text);
        card.modelIds.Clear();
        for (auto& model : models)
        {
            card.model->AddItem(model.second);
            card.modelIds.Add(model.first);
        }
        card.model->SelectItemText(TextOf(models, current));

        if (claude)
            card.tuning->SelectItemText(TextOf(ClaudeThinking(), settings.localAgentThinking ? "on" : "off"));
        else
        {
            String effort = settings.localCodexEffort.Trimed(" \n\r\t").ToLowerCase();
            card.tuning->SelectItemText(TextOf(CodexEfforts(), effort));
        }

        String apiKey = (claude ? settings.localAgentApiKey : settings.localCodexApiKey).Trimed(" \n\r\t");
        bool keyMode = !apiKey.IsEmpty() || card.account->GetSelectedItemText() == WString(claude ? kClaudeKey : kCodexKey);
        card.account->SelectItemText(keyMode ? (claude ? kClaudeKey : kCodexKey) : (claude ? kClaudeSubscription : kCodexSubscription));
        card.key->SetText(apiKey);
        card.keyRow->enabled = keyMode;
        mReadingAgents = false;
    }

    void AssetsLineDlg::WriteAgent(AgentCard& card)
    {
        bool claude = card.agent == LocalAgentSetup::Agent::ClaudeCode;
        auto settings = PipelineSettings::Load();

        int modelIndex = card.model->FindItem(card.model->GetSelectedItemText());
        String model = modelIndex >= 0 && modelIndex < card.modelIds.Count() ? card.modelIds[modelIndex] : String();
        bool keyMode = card.account->GetSelectedItemText() == WString(claude ? kClaudeKey : kCodexKey);
        String apiKey = keyMode ? ((String)card.key->GetText()).Trimed(" \n\r\t") : String();
        String tuning = ValueOf(claude ? ClaudeThinking() : CodexEfforts(), card.tuning->GetSelectedItemText());

        String before = claude ? settings.localAgentApiKey : settings.localCodexApiKey;
        if (claude)
        {
            settings.localAgentModel = model;
            settings.localAgentThinking = tuning == "on";
            settings.localAgentApiKey = apiKey;
        }
        else
        {
            settings.localCodexModel = model;
            settings.localCodexEffort = tuning;
            settings.localCodexApiKey = apiKey;
        }
        settings.Save();

        if (!keyMode)
            card.key->SetText("");
        if (card.keyRow->enabled != keyMode)
        {
            card.keyRow->enabled = keyMode;
            FitHeight();
        }

        // The account decides whether it needs a sign-in
        if (before != apiKey)
            card.check.Start(card.agent, settings);
    }

    void AssetsLineDlg::CheckAgents()
    {
        auto settings = PipelineSettings::Load();
        for (auto card : { &mClaude, &mCodex })
        {
            card->check.Start(card->agent, settings);
            card->sinceCheck = 0;
        }
        ShowLocalAgentFound();
    }

    void AssetsLineDlg::SetUpAgent(AgentCard& card)
    {
        card.setupError = "";
        if (LocalAgentSetup::OpenSetup(card.agent, card.setupError))
        {
            card.settingUp = true;
            card.setupFor = 0;
            card.sinceCheck = 0;
        }
        card.shown = "";
    }

    void AssetsLineDlg::UpdateAgents(float dt)
    {
        if (mPage != Page::Agents)
            return;

        for (auto card : { &mClaude, &mCodex })
        {
            card->check.Poll();
            card->sinceCheck += dt;
            if (card->settingUp)
            {
                card->setupFor += dt;
                if (card->check.GetStatus().state == LocalAgentSetup::State::Ready || card->setupFor > 20*60)
                    card->settingUp = false;
                else if (!card->check.IsRunning() && card->sinceCheck > 3.0f)
                {
                    card->check.Start(card->agent, PipelineSettings::Load());
                    card->sinceCheck = 0;
                }
            }
            ShowAgent(*card);
        }

        // Codex's catalog arrives after its first check
        if (CatalogKey() != mCodex.catalog)
            ReadAgent(mCodex);
    }

    void AssetsLineDlg::ShowAgent(AgentCard& card)
    {
        using State = LocalAgentSetup::State;
        auto& status = card.check.GetStatus();
        bool claude = card.agent == LocalAgentSetup::Agent::ClaudeCode;

        String state, detail, setup;
        Color4 color = IdleColor();
        switch (status.state)
        {
            case State::NotInstalled:
                state = "Not installed";
                detail = LocalAgentSetup::NameOf(card.agent) + " is not installed on this computer";
                setup = "Install and sign in...";
                break;
            case State::SignedOut:
                state = "Not signed in";
                color = SignedOutColor();
                detail = claude ? "Sign in with your Claude account" : "Sign in with your ChatGPT account";
                setup = "Sign in...";
                break;
            case State::Ready:
                state = "Ready";
                color = ReadyColor();
                detail = status.detail;
                break;
            case State::Failed:
                state = "Check failed";
                color = FailedColor();
                detail = status.detail;
                setup = "Set up...";
                break;
            default:
                state = "Checking...";
                color = BusyColor();
                break;
        }

        if (card.settingUp)
        {
            state = "Setting up...";
            color = BusyColor();
            detail = "Finish in the Terminal window: this updates by itself";
            setup = "";
        }
        if (!card.setupError.IsEmpty())
        {
            detail = card.setupError;
            color = FailedColor();
        }

        bool known = status.state != State::Checking && status.state != State::Unknown;
        bool installed = status.state != State::NotInstalled && (known || !status.exe.IsEmpty());
        String shown = state + "|" + detail + "|" + setup + "|" + (String)installed;
        if (shown == card.shown)
            return;

        card.shown = shown;
        card.state->text = state;
        card.detail->text = detail;
        if (auto dot = card.dot->GetLayer("dot"))
            dot->GetDrawable()->SetColor(color);
        card.setup->enabled = !setup.IsEmpty();
        if (!setup.IsEmpty())
            card.setup->caption = setup;
        card.options->enabled = installed;
        FitHeight();
    }

    void AssetsLineDlg::ShowPage(Page page)
    {
        mPage = page;
        lastPage = page;
        for (int i = 0; i < 4; i++)
        {
            mPages[i]->enabled = i == (int)page;
            mTabs[i]->SetValue(i == (int)page);
        }
        mSyncButton->enabled = page == Page::Sync;
        if (page == Page::Agents)
        {
            ReadAgent(mClaude);
            ReadAgent(mCodex);
            CheckAgents();
            mClaude.shown = mCodex.shown = "";
            UpdateAgents(0.0f);
        }
        FitHeight();
    }

    void AssetsLineDlg::Show()
    {
        Show(lastPage);
    }

    void AssetsLineDlg::Show(Page page)
    {
        if (!AssetsLineSync::IsSingletonInitialzed())
            return;

        if (!IsSingletonInitialzed())
            mmake<AssetsLineDlg>();

        auto& dlg = Instance();
        if (!dlg.mWindow->GetParent())
            EditorUIRoot.AddWidget(dlg.mWindow);

        dlg.ReadConfig();
        dlg.mShownStatus = "-";
        dlg.mWindow->Show();
        dlg.ShowPage(page);
        dlg.Update(0.0f);
    }

    void AssetsLineDlg::WriteKeys()
    {
        PipelineSettings settings = PipelineSettings::Load();
        settings.geminiApiKey = ((String)mGeminiEdit->GetText()).Trimed(" \n\r\t");
        settings.openAiApiKey = ((String)mOpenAiEdit->GetText()).Trimed(" \n\r\t");
        settings.openRouterApiKey = ((String)mOpenRouterEdit->GetText()).Trimed(" \n\r\t");
        settings.elevenLabsApiKey = ((String)mElevenEdit->GetText()).Trimed(" \n\r\t");
        settings.klingAccessKey = ((String)mKlingAccessEdit->GetText()).Trimed(" \n\r\t");
        settings.klingSecretKey = ((String)mKlingSecretEdit->GetText()).Trimed(" \n\r\t");
        settings.imageToUiPython = ((String)mPythonEdit->GetText()).Trimed(" \n\r\t");
        settings.localAgentEnabled = mLocalToggle->GetValue();
        settings.localAgentPath = ((String)mLocalPathEdit->GetText()).Trimed(" \n\r\t");
        settings.localCodexPath = ((String)mCodexPathEdit->GetText()).Trimed(" \n\r\t");
        settings.localCommand = ((String)mLocalCommandEdit->GetText()).Trimed(" \n\r\t");
        settings.Save();
        ShowLocalAgentFound();
    }

    void AssetsLineDlg::ShowLocalAgentFound()
    {
        String claude = LocalAgentProvider::FindClaude(((String)mLocalPathEdit->GetText()).Trimed(" \n\r\t"));
        mLocalFound->text = claude.IsEmpty() ? String("Not found; empty: looked for by itself") : "Runs " + claude;
        String codex = LocalAgentProvider::FindCodex(((String)mCodexPathEdit->GetText()).Trimed(" \n\r\t"));
        mCodexFound->text = codex.IsEmpty() ? String("Not found; empty: looked for by itself") : "Runs " + codex;
    }

    void AssetsLineDlg::ReadConfig()
    {
        mSideView->SetValue(PipelinePairLayout::GetIoView() == PipelineIoView::SideBySide);
        mCompareView->SetValue(PipelinePairLayout::GetIoView() == PipelineIoView::Compare);

        auto keys = PipelineSettings::Load();
        mGeminiEdit->SetText(keys.geminiApiKey);
        mOpenAiEdit->SetText(keys.openAiApiKey);
        mOpenRouterEdit->SetText(keys.openRouterApiKey);
        mElevenEdit->SetText(keys.elevenLabsApiKey);
        mKlingAccessEdit->SetText(keys.klingAccessKey);
        mKlingSecretEdit->SetText(keys.klingSecretKey);
        mPythonEdit->SetText(keys.imageToUiPython);
        mLocalToggle->SetValue(keys.localAgentEnabled);
        mLocalPathEdit->SetText(keys.localAgentPath);
        mCodexPathEdit->SetText(keys.localCodexPath);
        mLocalCommandEdit->SetText(keys.localCommand);
        bool custom = !keys.localAgentPath.IsEmpty() || !keys.localCodexPath.IsEmpty() || !keys.localCommand.IsEmpty();
        if (custom)
        {
            mCustomToggle->SetValue(true);
            mCustomSection->enabled = true;
        }
        ReadAgent(mClaude);
        ReadAgent(mCodex);
        ShowLocalAgentFound();

        auto& config = o2AssetsLineSync.GetConfig();
        mServerEdit->SetText(config.serverUrl.IsEmpty() ? String(kDefaultServer) : config.serverUrl);
        mFolderEdit->SetText(config.folder);
        mEnabledToggle->SetValue(config.enabled);
        mFinishToggle->SetValue(config.writeFinishAssets);
    }

    void AssetsLineDlg::WriteConfig()
    {
        auto config = o2AssetsLineSync.GetConfig();
        String folder = ((String)mFolderEdit->GetText()).Trimed(" /\\");
        config.folder = folder.IsEmpty() ? String("Pipelines") : folder;
        config.enabled = mEnabledToggle->GetValue();
        config.writeFinishAssets = mFinishToggle->GetValue();
        o2AssetsLineSync.SetConfig(config);
        o2AssetsLineSync.SyncNow();
    }

    void AssetsLineDlg::OnConnectionTypeChanged()
    {
        bool token = mConnectionType->GetSelectedItemText() == kToken;
        mBrowserPanel->enabled = !token;
        mTokenPanel->enabled = token;
        FitHeight();
    }

    void AssetsLineDlg::FitHeight()
    {
        float height = kHeaderHeight + 12 + StackHeight(mContent) + 10 + 18 + 5;
        if (Math::Equals(mWindow->layout->GetHeight(), height))
            return;

        float top = mWindow->layout->offsetTop;
        mWindow->layout->offsetBottom = top - height;
    }

    void AssetsLineDlg::Update(float dt)
    {
        if (!mWindow || !mWindow->IsEnabled() || !AssetsLineSync::IsSingletonInitialzed())
            return;

        mStatusDot.Update(dt, o2AssetsLineSync.GetStatus());
        Refresh();
        UpdateAgents(dt);
    }

    void AssetsLineDlg::Refresh()
    {
        if (!mWindow || !mWindow->IsEnabled() || !AssetsLineSync::IsSingletonInitialzed())
            return;

        auto& sync = o2AssetsLineSync;
        auto& config = sync.GetConfig();
        auto status = sync.GetStatus();
        bool connected = sync.IsConnected();
        bool waiting = !sync.GetApproveUrl().IsEmpty();
        String shown = sync.GetStatusText() + "|" + (String)(int)status + "|" + (String)connected + "|" + (String)sync.IsBusy() +
            "|" + config.projectName + "|" + config.serverUrl + "|" + sync.GetUserEmail() + "|" + sync.GetApproveUrl();
        if (shown == mShownStatus)
            return;

        mShownStatus = shown;

        String title = StatusTitle(status);
        String detail = sync.GetStatusText();
        if (detail == title || detail.IsEmpty())
        {
            if (!connected)
                detail = "Connect the project to AssetsLine to share pipelines and results";
            else if (status == AssetsLineStatus::Off)
                detail = "Turn the sync on to share pipelines with the linked project";
            else
                detail = "";
        }
        mStatusLabel->text = title;
        mStatusDetail->text = detail;
        mStatusDetail->enabled = !detail.IsEmpty();
        *mStatusLabel->layout = detail.IsEmpty()
            ? WidgetLayout::HorStretch(VerAlign::Middle, 38, 12, 18, 0)
            : WidgetLayout::HorStretch(VerAlign::Top, 38, 12, 18, 6);

        mConnectSection->enabled = !connected;
        mLinkedSection->enabled = connected;

        String project = config.projectName.IsEmpty() ? String("Linked project") : "\"" + config.projectName + "\"";
        String account = sync.GetUserEmail().IsEmpty() ? String() : " as " + sync.GetUserEmail();
        mProjectLabel->text = project + account;

        mApprovePanel->enabled = waiting;
        mCodeLabel->text = sync.GetConnectCode();
        mApproveEdit->SetText(sync.GetApproveUrl());
        mCancelButton->enabled = waiting;
        mConnectButton->interactable = !waiting;
        mSyncButton->interactable = connected && config.enabled;
        mSyncButton->enabled = mPage == Page::Sync;

        if (connected && mEnabledToggle->GetValue() != config.enabled)
            mEnabledToggle->SetValue(config.enabled);

        FitHeight();
    }
}
// --- META ---

ENUM_META(Editor::AssetsLineDlg::Page, Editor__AssetsLineDlg__Page)
{
    ENUM_ENTRY(Agents);
    ENUM_ENTRY(Editor);
    ENUM_ENTRY(Keys);
    ENUM_ENTRY(Sync);
}
END_ENUM_META;
// --- END META ---
