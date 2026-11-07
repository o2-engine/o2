#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include <filesystem>
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/LocalAgentProvider.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/AssetsLineDeleteDlg.h"
#include "o2Editor/Windows/PipelineWindow/AssetsLineDlg.h"
#include "o2Editor/Pipeline/Providers/LocalAgentSetup.h"
#include <thread>
#include "o2Editor/Windows/PipelineWindow/PipelineWindow.h"

using namespace o2;
using namespace Editor;

// The pipeline window's side of the AssetsLine sync: a synced pipeline saves itself after an
// edit, reloads when the sync rewrites its file, and the window shows the link state

namespace
{
    const char* kFolder = "UITestSync";

    void Step(int frames = 1)
    {
        for (int i = 0; i < frames; i++)
        {
            PushEditorScopeOnStack scope;
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            auto root = EditorUIRoot.GetRootWidget();
            root->Update(1.0f / 60.0f);
            root->UpdateChildren(1.0f / 60.0f);
            root->UpdateChildrenTransforms();
            root->Draw();
            o2Render.End();
            AppTestDriver::PumpFrames(1);
        }
    }

    struct PipelineSyncUiFixture : ::testing::Test
    {
        Ref<AssetsLineSync> sync;
        Ref<PipelineWindow> window;
        String folder;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }

            PipelineUtils::SetWorkPathOverride("../../Work/Pipelines/uitest-sync/");
            folder = o2Assets.GetAssetsPath() + kFolder + "/";
            o2FileSystem.FolderCreate(folder, true);

            sync = AssetsLineSync::IsSingletonInitialzed() ? Ref(AssetsLineSync::InstancePtr()) : mmake<AssetsLineSync>();
            sync->rebuildAssets = []() {};
            sync->openUrl = [](const String&) {};
            AssetsLineConfig config;
            config.folder = kFolder;
            sync->SetConfigOverride(config);
            sync->ReloadState();

            PushEditorScopeOnStack scope;
            window = PipelineWindow::IsSingletonInitialzed() ? Ref(PipelineWindow::InstancePtr()) : mmake<PipelineWindow>();
        }

        void TearDown() override
        {
            window = nullptr;
            o2FileSystem.FolderRemove(folder, true);
            o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
            PipelineUtils::SetWorkPathOverride("");
            if (UIRoot::IsSingletonInitialzed())
                EditorUIRoot.RemoveAllWidgets();
        }

        // Links the sandbox to a server that is never asked: a stored token for the configured project
        void Link()
        {
            auto config = sync->GetConfig();
            config.enabled = true;
            config.serverUrl = "http://127.0.0.1:9";
            config.projectId = "ui-test-project";
            sync->SetConfigOverride(config);
            PipelineUtils::WriteFileBytes(PipelineUtils::GetWorkPath() + "sync/credentials.json",
                "{\"serverUrl\":\"http://127.0.0.1:9\",\"projectId\":\"ui-test-project\",\"token\":\"alt_x_y\",\"email\":\"ui@test\"}");
            sync->ReloadState();
            ASSERT_TRUE(sync->IsConnected());
        }

        Ref<PipelineAsset> OpenSynced(const String& name)
        {
            auto asset = mmake<PipelineAsset>();
            asset->SetPath(String(kFolder) + "/" + name);
            window->EditAsset(AssetRef<Asset>(asset));
            Step(2);
            return asset;
        }

        void Tick(float seconds)
        {
            for (float t = 0; t < seconds; t += 0.1f)
            {
                window->Update(0.1f);
                Step();
            }
        }

        // For a look at the layout: Work/Pipelines/shots/<name>.png when run from build/o2
        void Shot(const String& name)
        {
            Step(40);
            Ref<Bitmap> shot;
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { shot = bitmap; });
            Step();
            if (shot)
            {
                o2FileSystem.FolderCreate("../../Work/Pipelines/shots", true);
                shot->Save("../../Work/Pipelines/shots/" + name + ".png", Bitmap::ImageType::Png);
            }
        }

        Ref<Button> SyncButton()
        {
            return DynamicCast<Button>(window->GetWindow()->FindChild("settings"));
        }
    };
}

TEST_F(PipelineSyncUiFixture, TheWindowShowsTheLinkState)
{
    sync->Update(0.1f);
    Tick(0.1f);
    auto button = SyncButton();
    ASSERT_TRUE(button);
    auto dot = button->GetLayer("status dot");
    ASSERT_TRUE(dot);
    EXPECT_EQ(window->GetSyncDot().GetStatus(), AssetsLineStatus::Off);
    EXPECT_EQ(dot->GetDrawable()->GetColor().a, 0) << "no badge while the link is off";

    auto config = sync->GetConfig();
    config.enabled = true;
    sync->SetConfigOverride(config);
    sync->Update(0.1f);
    Tick(0.1f);
    EXPECT_EQ(window->GetSyncDot().GetStatus(), AssetsLineStatus::NotConnected);
    EXPECT_EQ(dot->GetDrawable()->GetColor(), Color4(228, 74, 60, 255));
    EXPECT_FALSE(window->GetSyncDot().IsPulsing());
}

// Connecting and syncing pulse: the ring grows and fades, the dot breathes
TEST_F(PipelineSyncUiFixture, TheStateDotPulsesWhileInProgress)
{
    Ref<Widget> host;
    {
        PushEditorScopeOnStack scope;
        host = mmake<Widget>();
    }
    AssetsLineStatusDot dot;
    dot.Attach(host, Vec2F(0.0f, 0.5f), Vec2F(13.0f, 0.0f));
    auto pulse = host->GetLayer("status pulse");
    ASSERT_TRUE(pulse);

    dot.Update(0.1f, AssetsLineStatus::Synced);
    EXPECT_EQ(pulse->GetDrawable()->GetColor().a, 0);

    dot.Update(0.0f, AssetsLineStatus::Connecting);
    EXPECT_TRUE(dot.IsPulsing());
    EXPECT_EQ(pulse->GetDrawable()->GetColor().r, 48);
    int startAlpha = pulse->GetDrawable()->GetColor().a;
    dot.Update(0.5f, AssetsLineStatus::Connecting);
    EXPECT_LT(pulse->GetDrawable()->GetColor().a, startAlpha);

    dot.Update(0.0f, AssetsLineStatus::Syncing);
    EXPECT_TRUE(dot.IsPulsing());
    EXPECT_EQ(pulse->GetDrawable()->GetColor().g, 176);
}

TEST_F(PipelineSyncUiFixture, ASyncedPipelineSavesItselfAfterAnEdit)
{
    Link();
    auto asset = OpenSynced("auto.pipeline");
    window->GetEditor()->AddNodeAtViewCenter("sourceText");
    Step();
    ASSERT_TRUE(asset->IsDirty());

    Tick(0.5f);
    EXPECT_TRUE(asset->IsDirty()) << "saved before the edit settled";

    Tick(1.0f);
    EXPECT_FALSE(asset->IsDirty());
    String saved = PipelineUtils::ReadFileBytes(folder + "auto.pipeline");
    EXPECT_TRUE(saved.Contains("\"sourceText\"")) << saved;
    EXPECT_TRUE(saved.Contains("\"schemaVersion\""));
}

TEST_F(PipelineSyncUiFixture, APipelineOutsideTheSyncedFolderWaitsForSave)
{
    Link();
    auto asset = mmake<PipelineAsset>();
    asset->SetPath("Elsewhere/manual.pipeline");
    window->EditAsset(AssetRef<Asset>(asset));
    Step(2);
    window->GetEditor()->AddNodeAtViewCenter("sourceText");
    Tick(1.6f);
    EXPECT_TRUE(asset->IsDirty());
    asset->SetDirty(false);
}

TEST_F(PipelineSyncUiFixture, AFileTheSyncRewroteIsReloaded)
{
    Link();
    auto asset = OpenSynced("reload.pipeline");
    window->GetEditor()->AddNodeAtViewCenter("sourceText");
    Tick(1.5f);
    ASSERT_FALSE(asset->IsDirty());
    auto graph = window->GetEditor()->GetGraph();
    ASSERT_EQ(graph->nodes.Count(), 1);

    // What a pull writes: the same document with a node added elsewhere
    PipelineGraph remote;
    ASSERT_TRUE(remote.LoadFromJsonString(PipelineUtils::ReadFileBytes(folder + "reload.pipeline")));
    auto added = PipelineNodeRegistry::CreateNode("finishText", Vec2F(300, 0));
    remote.nodes.Add(added);
    PipelineUtils::WriteFileBytes(folder + "reload.pipeline", remote.ToJsonString());

    sync->onPipelineFileChanged(String(kFolder) + "/reload.pipeline");
    Tick(0.2f);
    graph = window->GetEditor()->GetGraph();
    ASSERT_EQ(graph->nodes.Count(), 2);
    EXPECT_TRUE(graph->FindNode(added->id));
    EXPECT_TRUE(window->GetEditor()->GetNodeWidget(added->id) != nullptr);
    EXPECT_FALSE(asset->IsDirty());
}

TEST_F(PipelineSyncUiFixture, UnsavedEditsAreReportedBusyToTheSync)
{
    Link();
    auto asset = OpenSynced("busy.pipeline");
    Tick(0.1f);
    String path = String(kFolder) + "/busy.pipeline";
    EXPECT_FALSE(sync->isPipelineFileBusy(path));
    window->GetEditor()->AddNodeAtViewCenter("sourceText");
    Step();
    EXPECT_TRUE(sync->isPipelineFileBusy(path));
    EXPECT_FALSE(sync->isPipelineFileBusy(String(kFolder) + "/other.pipeline"));
    Tick(1.5f);
    EXPECT_FALSE(sync->isPipelineFileBusy(path));
}

// The provider keys live in the same window and are stored as they are edited
TEST_F(PipelineSyncUiFixture, TheSettingsWindowStoresTheProviderKeys)
{
    AssetsLineDlg::Show();
    Step(2);
    auto window = AssetsLineDlg::Instance().GetWindow();
    auto gemini = DynamicCast<EditBox>(window->FindChild("gemini key"));
    auto openAi = DynamicCast<EditBox>(window->FindChild("openai key"));
    auto openRouter = DynamicCast<EditBox>(window->FindChild("openrouter key"));
    auto kling = DynamicCast<EditBox>(window->FindChild("kling secret"));
    ASSERT_TRUE(gemini && openAi && openRouter && kling);
    gemini->SetText("test-gemini-key");
    gemini->onChangeCompleted(gemini->GetText());
    EXPECT_EQ(PipelineSettings::Load().geminiApiKey, "test-gemini-key");
    openAi->SetText("test-openai-key");
    openAi->onChangeCompleted(openAi->GetText());
    EXPECT_EQ(PipelineSettings::Load().openAiApiKey, "test-openai-key");
    openRouter->SetText("test-openrouter-key");
    openRouter->onChangeCompleted(openRouter->GetText());
    EXPECT_EQ(PipelineSettings::Load().openRouterApiKey, "test-openrouter-key");
    EXPECT_EQ(PipelineSettings::Load().openAiApiKey, "test-openai-key");
    EXPECT_EQ(PipelineSettings::Load().geminiApiKey, "test-gemini-key");

    // The view of the image nodes is a local setting: stored at once, read back when the window opens
    auto sideView = DynamicCast<Toggle>(window->FindChild("side view"));
    auto compareView = DynamicCast<Toggle>(window->FindChild("compare view"));
    ASSERT_TRUE(sideView && compareView);
    EXPECT_TRUE(sideView->GetValue());
    compareView->onToggleByUser(true);
    EXPECT_EQ(PipelinePairLayout::GetIoView(), PipelineIoView::Compare);
    EXPECT_TRUE(compareView->GetValue());
    EXPECT_FALSE(sideView->GetValue());
    EXPECT_TRUE(PipelineUtils::ReadFileBytes(PipelinePairLayout::GetViewPrefsPath()).Contains("compare"));
    sideView->onToggleByUser(true);
    EXPECT_EQ(PipelinePairLayout::GetIoView(), PipelineIoView::SideBySide);

    // The window is tall enough for the last row
    Step(2);
    auto close = window->FindChildByTypeAndName<Button>("Close");
    ASSERT_TRUE(close);
    EXPECT_GE(close->layout->GetWorldRect().bottom, window->layout->GetWorldRect().bottom);
    Shot("pipeline_settings");
    window->Hide(true);
}

// The local agents page: a card each, its state from the agent's own status command (fakes here), Sign in opening a terminal
// (a stand-in here) and the agent ready once signed in; the dropdowns store the model, thinking or effort and the account,
// the API key row shows only for the API key account; the window stays short
TEST_F(PipelineSyncUiFixture, TheLocalAgentsPageShowsTheirStateAndSetsThemUp)
{
    using namespace LocalAgentSetup;
    String dir = PipelineUtils::GetWorkPath() + "agents/";
    o2FileSystem.FolderCreate(dir, true);
    PipelineUtils::WriteFileBytes(dir + "claude",
        "#!/bin/sh\necho '{\"loggedIn\":true,\"authMethod\":\"claude.ai\",\"subscriptionType\":\"pro\",\"email\":\"me@test\"}'\n");
    PipelineUtils::WriteFileBytes(dir + "codex",
        "#!/bin/sh\n"
        "case \"$*\" in\n"
        "  'login status') if [ -f \"$(dirname \"$0\")/signed-in\" ]; then echo 'Logged in using ChatGPT' >&2; else echo 'Not logged in' >&2; exit 1; fi ;;\n"
        "  'debug models --bundled') echo '{\"models\":[{\"slug\":\"fake-1\",\"display_name\":\"Fake One\",\"visibility\":\"list\",\"priority\":1}]}' ;;\n"
        "esac\n");
    for (String name : { "claude", "codex" })
        std::filesystem::permissions((dir + name).Data(), std::filesystem::perms::owner_all);

    auto settings = PipelineSettings::Load();
    settings.localAgentPath = dir + "claude";
    settings.localCodexPath = dir + "codex";
    settings.Save();

    Vector<Agent> opened;
    SetSetupOpener([&](Agent agent, const String&) { opened.Add(agent); return true; });
    struct Restore { ~Restore() { SetSetupOpener(Function<bool(Agent, const String&)>()); } } restore;

    AssetsLineDlg::Show(AssetsLineDlg::Page::Agents);
    auto& dlg = AssetsLineDlg::Instance();
    auto window = dlg.GetWindow();
    auto label = [&](const String& name) { return (String)DynamicCast<Label>(window->FindChild(name))->GetText(); };
    auto settle = [&]()
    {
        // The editor moves the dialog every frame; here the test does
        for (int i = 0; i < 300; i++)
        {
            dlg.Update(0.02f);
            Step(1);
            if (label("claude state") != "Checking..." && label("codex state") != "Checking...")
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        for (int i = 0; i < 20; i++)
        {
            dlg.Update(0.02f);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Step(2);
    };
    settle();

    EXPECT_EQ(dlg.GetPage(), AssetsLineDlg::Page::Agents);
    EXPECT_TRUE(window->FindChild("agents page")->IsEnabledInHierarchy());
    EXPECT_FALSE(window->FindChild("sync page")->IsEnabledInHierarchy());
    EXPECT_TRUE(DynamicCast<Toggle>(window->FindChild("tab agents"))->GetValue());
    EXPECT_TRUE(DynamicCast<Toggle>(window->FindChild("local agent on"))->GetValue()) << "on by default";

    // Claude Code is signed in; Codex is installed and signed out
    EXPECT_EQ(label("claude state"), "Ready") << label("claude state").Data() << " / " << label("claude detail").Data();
    EXPECT_EQ(label("claude detail"), "Signed in \xC2\xB7 Claude Pro \xC2\xB7 me@test") << label("claude detail").Data();
    EXPECT_FALSE(window->FindChild("claude setup")->IsEnabled());
    EXPECT_TRUE(window->FindChild("claude options")->IsEnabledInHierarchy());
    EXPECT_EQ(label("codex state"), "Not signed in") << label("codex state").Data() << " / " << label("codex detail").Data();
    auto codexSetup = DynamicCast<Button>(window->FindChild("codex setup"));
    ASSERT_TRUE(codexSetup && codexSetup->IsEnabledInHierarchy());
    EXPECT_EQ((String)codexSetup->GetCaption(), "Sign in...");
    EXPECT_TRUE(window->FindChild("codex options")->IsEnabledInHierarchy()) << "an API key can be chosen instead";
    auto codexModel = DynamicCast<DropDown>(window->FindChild("codex model"));
    ASSERT_TRUE(codexModel);
    EXPECT_TRUE(codexModel->FindItem("Fake One") >= 0) << "the models of Codex's catalog";

    Shot("pipeline_settings_agents");

    // Sign in: the terminal opens with Codex's setup; signed in there, the card is ready
    codexSetup->onClick();
    dlg.Update(0.02f);
    Step(2);
    ASSERT_EQ(opened.Count(), 1);
    EXPECT_EQ(opened[0], Agent::Codex);
    EXPECT_EQ(label("codex state"), "Setting up...");
    EXPECT_TRUE(label("codex detail").Contains("Terminal"));
    PipelineUtils::WriteFileBytes(dir + "signed-in", "");
    DynamicCast<Button>(window->FindChild("codex check"))->onClick();
    settle();
    EXPECT_EQ(label("codex state"), "Ready");
    EXPECT_EQ(label("codex detail"), "Signed in with ChatGPT");

    // The dropdowns store what they show
    DynamicCast<DropDown>(window->FindChild("claude model"))->SelectItemText("Sonnet \xE2\x80\x94 balanced");
    DynamicCast<DropDown>(window->FindChild("claude thinking"))->SelectItemText("On \xE2\x80\x94 deeper answers, more tokens");
    codexModel->SelectItemText("Fake One");
    DynamicCast<DropDown>(window->FindChild("codex effort"))->SelectItemText("Medium");
    settings = PipelineSettings::Load();
    EXPECT_EQ(settings.localAgentModel, "sonnet");
    EXPECT_TRUE(settings.localAgentThinking);
    EXPECT_EQ(settings.localCodexModel, "fake-1");
    EXPECT_EQ(settings.localCodexEffort, "medium");
    EXPECT_EQ(settings.localAgentPath, dir + "claude") << "the paths stay";

    // The API key account: its row shows, the key needs no sign-in; back on the subscription the key goes
    auto account = DynamicCast<DropDown>(window->FindChild("codex account"));
    auto keyRow = window->FindChild("codex key row");
    EXPECT_FALSE(keyRow->IsEnabledInHierarchy());
    account->SelectItemText("OpenAI API key");
    dlg.Update(0.02f);
    Step(2);
    EXPECT_TRUE(keyRow->IsEnabledInHierarchy());
    auto key = DynamicCast<EditBox>(window->FindChild("codex key"));
    key->SetText("sk-test");
    key->onChangeCompleted(key->GetText());
    settle();
    EXPECT_EQ(PipelineSettings::Load().localCodexApiKey, "sk-test");
    EXPECT_TRUE(label("codex detail").Contains("API key")) << label("codex detail").Data();
    account->SelectItemText("ChatGPT subscription");
    dlg.Update(0.02f);
    Step(2);
    EXPECT_FALSE(keyRow->IsEnabledInHierarchy());
    EXPECT_EQ(PipelineSettings::Load().localCodexApiKey, "");

    // Custom paths are set here, so their fold is open; the window stays short and fits its last row
    EXPECT_TRUE(window->FindChild("local agent custom section")->IsEnabledInHierarchy());
    EXPECT_TRUE(label("local agent found").StartsWith("Runs /") && label("local agent found").EndsWith("/agents/claude"))
        << label("local agent found").Data() << ": the path made whole";
    Step(2);
    EXPECT_LT(window->layout->GetHeight(), 760.0f);
    auto close = window->FindChildByTypeAndName<Button>("Close");
    ASSERT_TRUE(close);
    EXPECT_GE(close->layout->GetWorldRect().bottom, window->layout->GetWorldRect().bottom);

    // Off: no local agent in the menus
    auto on = DynamicCast<Toggle>(window->FindChild("local agent on"));
    on->SetValue(false);
    on->onToggleByUser(false);
    EXPECT_FALSE(PipelineSettings::Load().localAgentEnabled);

    // Back on the AssetsLine page, the sync controls show again
    DynamicCast<Toggle>(window->FindChild("tab sync"))->onToggleByUser(true);
    Step(2);
    EXPECT_TRUE(window->FindChild("sync page")->IsEnabledInHierarchy());
    EXPECT_FALSE(window->FindChild("agents page")->IsEnabledInHierarchy());
    window->Hide(true);
}

// Not linked: the connection type picks the controls shown, and the window fits them
TEST_F(PipelineSyncUiFixture, TheDialogShowsTheControlsOfTheConnectionType)
{
    AssetsLineDlg::Show();
    Step(2);
    auto window = AssetsLineDlg::Instance().GetWindow();
    ASSERT_TRUE(window->FindChild("connect")->IsEnabledInHierarchy());
    EXPECT_FALSE(window->FindChild("linked")->IsEnabledInHierarchy());

    auto type = DynamicCast<DropDown>(window->FindChild("connection type"));
    ASSERT_TRUE(type);
    EXPECT_EQ((String)type->GetSelectedItemText(), "Browser");
    EXPECT_TRUE(window->FindChild("browser panel")->IsEnabledInHierarchy());
    EXPECT_FALSE(window->FindChild("token panel")->IsEnabledInHierarchy());
    EXPECT_FALSE(window->FindChild("approve panel")->IsEnabledInHierarchy()) << "no approval is awaited";
    float browserHeight = window->layout->GetHeight();
    Shot("assetsline_dialog_browser");

    type->SelectItemText("Access token");
    Step(2);
    EXPECT_FALSE(window->FindChild("browser panel")->IsEnabledInHierarchy());
    EXPECT_TRUE(window->FindChild("token panel")->IsEnabledInHierarchy());
    EXPECT_TRUE(window->FindChild("token"));
    Shot("assetsline_dialog_token");

    type->SelectItemText("Browser");
    Step(2);
    EXPECT_NEAR(window->layout->GetHeight(), browserHeight, 0.5f);
    window->Hide(true);
}

TEST_F(PipelineSyncUiFixture, TheDialogShowsAndStoresTheLink)
{
    Link();
    AssetsLineDlg::Show();
    Step(2);
    auto& dlg = AssetsLineDlg::Instance();
    ASSERT_TRUE(dlg.GetWindow()->IsEnabled());

    auto server = DynamicCast<EditBox>(dlg.GetWindow()->FindChild("server"));
    auto project = DynamicCast<Label>(dlg.GetWindow()->FindChild("project"));
    auto folderEdit = DynamicCast<EditBox>(dlg.GetWindow()->FindChild("folder"));
    auto finish = DynamicCast<Toggle>(dlg.GetWindow()->FindChild("finish assets"));
    ASSERT_TRUE(server && project && folderEdit && finish);
    EXPECT_EQ((String)server->GetText(), "http://127.0.0.1:9");
    EXPECT_TRUE(((String)project->GetText()).Contains("ui@test")) << (String)project->GetText();
    EXPECT_EQ((String)folderEdit->GetText(), kFolder);

    finish->SetValue(false);
    finish->onToggleByUser(false);
    EXPECT_FALSE(sync->GetConfig().writeFinishAssets);

    // Linked: the project row instead of the ways to connect; no log in the dialog
    auto window = dlg.GetWindow();
    EXPECT_TRUE(window->FindChild("linked")->IsEnabledInHierarchy());
    EXPECT_FALSE(window->FindChild("connect")->IsEnabledInHierarchy());
    EXPECT_FALSE(window->FindChild("log"));
    auto status = DynamicCast<Label>(window->FindChild("status"));
    ASSERT_TRUE(status);
    EXPECT_EQ(dlg.GetStatusDot().GetStatus(), sync->GetStatus());

    Shot("assetsline_dialog");

    dlg.GetWindow()->Hide(true);
}

TEST_F(PipelineSyncUiFixture, TheDeleteQuestionAnswersAndShowsTheNextOne)
{
    window->Update(0.1f);
    EXPECT_TRUE((bool)sync->confirmDeletes) << "the window gives the sync its question";

    Vector<int> answers;
    AssetsLineDeleteDlg::Show({ "hero", "map" }, kFolder, "examples", [&](bool deleteThere) { answers.Add(deleteThere ? 1 : 0); });
    AssetsLineDeleteDlg::Show({ "boss" }, kFolder, "examples", [&](bool deleteThere) { answers.Add(deleteThere ? 1 : 0); });
    Step(2);
    ASSERT_TRUE(AssetsLineDeleteDlg::IsShown());
    EXPECT_EQ(AssetsLineDeleteDlg::GetQueuedCount(), 1);

    auto dialog = DynamicCast<o2::Window>(EditorUIRoot.GetRootWidget()->FindChild("assetsline delete question"));
    ASSERT_TRUE(dialog);
    auto message = DynamicCast<Label>(dialog->FindChild("message"));
    ASSERT_TRUE(message);
    EXPECT_EQ((String)message->GetText(), AssetsLineDeleteDlg::Message({ "hero", "map" }, kFolder, "examples"));
    Shot("assetsline_delete_question");

    auto remove = DynamicCast<Button>(dialog->FindChild("delete there"));
    ASSERT_TRUE(remove && dialog->FindChild("keep"));
    remove->onClick();
    Step(2);
    EXPECT_EQ(answers, Vector<int>({ 1 }));
    ASSERT_TRUE(AssetsLineDeleteDlg::IsShown()) << "the next question";
    EXPECT_EQ((String)message->GetText(), AssetsLineDeleteDlg::Message({ "boss" }, kFolder, "examples"));

    // Closed without an answer: kept
    dialog->Hide();
    Step(30);
    EXPECT_EQ(answers, Vector<int>({ 1, 0 }));
    EXPECT_FALSE(AssetsLineDeleteDlg::IsShown());
}
