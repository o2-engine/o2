#include "o2Editor/stdafx.h"
#include "AssetsLineDlg.h"

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

    static const Color4 kTitleColor(38, 50, 56, 255);
    static const Color4 kHintColor(144, 164, 174, 255);

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

    AssetsLineDlg::AssetsLineDlg(RefCounter* refCounter):
        Singleton<AssetsLineDlg>(refCounter)
    {
        PushEditorScopeOnStack scope;
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
        mStatusLabel->color = kTitleColor;
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

        auto addSection = [&](const Ref<VerticalLayout>& parent, const String& name)
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
        };

        // A form row: a caption column, then the fields
        auto addRow = [&](const Ref<VerticalLayout>& parent, const String& caption)
        {
            auto row = mmake<HorizontalLayout>();
            row->layout->minHeight = kRowHeight;
            row->spacing = 6;
            row->expandWidth = true;
            row->expandHeight = true;
            row->baseCorner = BaseCorner::Left;
            parent->AddChild(row);

            auto label = o2UI.CreateLabel(caption);
            label->horAlign = HorAlign::Left;
            label->layout->minWidth = kCaptionWidth;
            label->layout->maxWidth = kCaptionWidth;
            row->AddChild(label);
            return row;
        };

        auto addHint = [&](const Ref<VerticalLayout>& parent, const String& text)
        {
            auto row = addRow(parent, "");
            row->layout->minHeight = 16;
            auto label = o2UI.CreateLabel(text);
            label->horAlign = HorAlign::Left;
            label->horOverflow = Label::HorOverflow::Dots;
            label->color = kHintColor;
            row->AddChild(label);
            return label;
        };

        auto addEdit = [&](const Ref<HorizontalLayout>& row, const String& name)
        {
            auto edit = o2UI.CreateWidget<EditBox>("singleline");
            edit->name = name;
            row->AddChild(edit);
            return edit;
        };

        auto addButton = [&](const Ref<HorizontalLayout>& row, const String& caption, float width, const Function<void()>& onClick)
        {
            auto button = o2UI.CreateButton(caption, onClick);
            button->name = caption;
            button->layout->minWidth = width;
            button->layout->maxWidth = width;
            row->AddChild(button);
            return button;
        };

        auto addSeparator = [&]()
        {
            auto separator = mmake<Widget>();
            separator->layout->minHeight = 9;
            separator->AddLayer("line", mmake<VectorSprite>("ui/UI4_Separator.svg"), Layout::HorStretch(VerAlign::Middle, 0, 0, 5, 0));
            content->AddChild(separator);
        };

        // Not connected: the server and the way to connect
        mConnectSection = addSection(content, "connect");
        auto connectSection = DynamicCast<VerticalLayout>(mConnectSection);

        mServerEdit = addEdit(addRow(connectSection, "Server"), "server");

        auto typeRow = addRow(connectSection, "Connection");
        mConnectionType = o2UI.CreateDropdown();
        mConnectionType->name = "connection type";
        mConnectionType->AddItem(kBrowser);
        mConnectionType->AddItem(kToken);
        mConnectionType->SelectItemText(kBrowser);
        mConnectionType->layout->minWidth = 150;
        mConnectionType->layout->maxWidth = 150;
        mConnectionType->onSelectedText = [this](const WString&) { OnConnectionTypeChanged(); };
        typeRow->AddChild(mConnectionType);

        auto browserPanel = addSection(connectSection, "browser panel");
        mBrowserPanel = browserPanel;
        addHint(browserPanel, "Opens AssetsLine in the browser to approve this editor");
        auto browserButtons = addRow(browserPanel, "");
        mConnectButton = addButton(browserButtons, "Connect", 100, [this]()
        {
            o2AssetsLineSync.StartConnect((String)mServerEdit->GetText());
        });
        mCancelButton = addButton(browserButtons, "Cancel", 80, []() { o2AssetsLineSync.CancelConnect(); });

        auto approvePanel = addSection(browserPanel, "approve panel");
        mApprovePanel = approvePanel;
        auto codeRow = addRow(approvePanel, "Code");
        mCodeLabel = o2UI.CreateLabel("");
        mCodeLabel->name = "code";
        mCodeLabel->horAlign = HorAlign::Left;
        mCodeLabel->color = kTitleColor;
        codeRow->AddChild(mCodeLabel);
        mApproveEdit = addEdit(addRow(approvePanel, "Page"), "approve url");
        addHint(approvePanel, "If the browser did not open, open this page by hand");

        auto tokenPanel = addSection(connectSection, "token panel");
        mTokenPanel = tokenPanel;
        auto tokenRow = addRow(tokenPanel, "Token");
        mTokenEdit = addEdit(tokenRow, "token");
        mTokenButton = addButton(tokenRow, "Connect", 90, [this]()
        {
            o2AssetsLineSync.ConnectWithToken((String)mServerEdit->GetText(), (String)mTokenEdit->GetText());
            mTokenEdit->SetText("");
        });
        mTokenButton->name = "use token";
        addHint(tokenPanel, "Make one in AssetsLine: Settings, Connected editors");

        // Connected: the linked project
        mLinkedSection = addSection(content, "linked");
        auto linkedSection = DynamicCast<VerticalLayout>(mLinkedSection);
        auto projectRow = addRow(linkedSection, "Project");
        mProjectLabel = o2UI.CreateLabel("");
        mProjectLabel->name = "project";
        mProjectLabel->horAlign = HorAlign::Left;
        mProjectLabel->horOverflow = Label::HorOverflow::Dots;
        mProjectLabel->color = kTitleColor;
        projectRow->AddChild(mProjectLabel);
        mDisconnectButton = addButton(projectRow, "Disconnect", 100, []() { o2AssetsLineSync.Disconnect(); });

        addSeparator();

        mFolderEdit = addEdit(addRow(content, "Folder"), "folder");
        mFolderEdit->onChangeCompleted = [this](const WString&) { WriteConfig(); };
        addHint(content, "Pipelines inside this folder of Assets are synced");

        mEnabledToggle = o2UI.CreateToggle("Sync is on");
        mEnabledToggle->name = "enabled";
        mEnabledToggle->onToggleByUser = [this](bool) { WriteConfig(); };
        addRow(content, "")->AddChild(mEnabledToggle);

        mFinishToggle = o2UI.CreateToggle("Write results of finish nodes into the assets");
        mFinishToggle->name = "finish assets";
        mFinishToggle->onToggleByUser = [this](bool) { WriteConfig(); };
        addRow(content, "")->AddChild(mFinishToggle);

        addSeparator();

        // How the image-to-image nodes show their input next to their result; a local setting, not synced
        auto viewRow = addRow(content, "Image nodes");
        auto addView = [&](const String& caption, const String& name, PipelineIoView view)
        {
            auto segment = PipelineControls::MakeSegment(caption, PipelinePairLayout::GetIoView() == view);
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

        addSeparator();

        // Keys of the providers the nodes call when they run in this editor
        auto keysTitle = o2UI.CreateLabel("Provider keys");
        keysTitle->name = "provider keys";
        keysTitle->horAlign = HorAlign::Left;
        keysTitle->color = kTitleColor;
        keysTitle->layout->minHeight = 18;
        content->AddChild(keysTitle);

        auto addKey = [&](const String& caption, const String& name)
        {
            auto edit = addEdit(addRow(content, caption), name);
            edit->onChangeCompleted = [this](const WString&) { WriteKeys(); };
            return edit;
        };
        mGeminiEdit = addKey("Gemini", "gemini key");
        mOpenAiEdit = addKey("OpenAI", "openai key");
        mOpenRouterEdit = addKey("OpenRouter", "openrouter key");
        mElevenEdit = addKey("ElevenLabs", "elevenlabs key");
        mKlingAccessEdit = addKey("Kling key", "kling key");
        mKlingSecretEdit = addKey("Kling secret", "kling secret");
        addHint(content, "Only for an old Kling key pair");
        addHint(content, "Used when nodes run in this editor");

        addSeparator();

        auto actions = mmake<HorizontalLayout>();
        actions->layout->minHeight = kRowHeight;
        actions->spacing = 6;
        actions->expandWidth = false;
        actions->expandHeight = true;
        actions->baseCorner = BaseCorner::Right;
        content->AddChild(actions);
        mSyncButton = addButton(actions, "Sync now", 100, []() { o2AssetsLineSync.SyncNow(); });
        addButton(actions, "Close", 80, [this]() { mWindow->Hide(); });

        OnConnectionTypeChanged();

        mWindow->Hide(true);
        EditorUIRoot.AddWidget(mWindow);
    }

    void AssetsLineDlg::Show()
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
        settings.Save();
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

        if (connected && mEnabledToggle->GetValue() != config.enabled)
            mEnabledToggle->SetValue(config.enabled);

        FitHeight();
    }
}
