#include "o2Editor/stdafx.h"
#include "PipelineWindow.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"
#include "o2Editor/Windows/PipelineWindow/AssetsLineDlg.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/Pipeline/PipelineImport.h"
#include "o2/Render/VectorSprite.h"

DECLARE_SINGLETON(Editor::PipelineWindow);

namespace Editor
{
    PipelineWindow::PipelineWindow(RefCounter* refCounter):
        Singleton<PipelineWindow>(refCounter), IAssetEditorWindow(refCounter)
    {
        InitializeWindow();
        if (const char* file = getenv("O2_PIPELINE_IMPORT"))
            mAutoImport = file;
    }

    PipelineWindow::~PipelineWindow()
    {}

    const Type& PipelineWindow::GetAssetType() const
    {
        return TypeOf(PipelineAsset);
    }

    Ref<RefCounterable> PipelineWindow::CastToRefCounterable(const Ref<PipelineWindow>& ref)
    {
        return DynamicCast<Singleton<PipelineWindow>>(ref);
    }

    void PipelineWindow::InitializeWindow()
    {
        IAssetEditorWindow::InitializeWindow();

        mWindow->caption = "Pipeline";
        mWindow->name = "pipeline window";
        mWindow->SetIcon(mmake<VectorSprite>("ui/pipeline/window_icon.svg"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));

        mRunAllButton = o2UI.CreateWidget<Button>("menu pipeline run");
        mRunAllButton->name = "run all";
        mRunAllButton->onClick = [this]() { mEditor->RunAll(); };
        mButtonsPanel->AddChild(mRunAllButton);

        mStopButton = o2UI.CreateWidget<Button>("menu pipeline stop");
        mStopButton->name = "stop";
        mStopButton->onClick = [this]() { mEditor->StopRun(); };
        mButtonsPanel->AddChild(mStopButton);

        mFitButton = o2UI.CreateWidget<Button>("menu pipeline fit");
        mFitButton->name = "fit";
        mFitButton->onClick = [this]() { mEditor->FitView(); };
        mButtonsPanel->AddChild(mFitButton);

        mImportButton = o2UI.CreateWidget<Button>("menu pipeline import");
        mImportButton->name = "import";
        mImportButton->onClick = THIS_FUNC(OnImportPressed);
        mButtonsPanel->AddChild(mImportButton);

        mStatusLabel = o2UI.CreateLabel("");
        mStatusLabel->name = "status";
        mStatusLabel->horAlign = HorAlign::Left;
        mStatusLabel->horOverflow = Label::HorOverflow::Dots;
        mStatusLabel->layout->minWidth = 200;
        mUpPanel->AddChild(mStatusLabel);

        mSaveAllButton = o2UI.CreateWidget<Button>("menu pipeline save");
        mSaveAllButton->name = "save all";
        mSaveAllButton->onClick = [this]() { mEditor->SaveAllOutputs(); };
        mSaveAllButton->layout->minWidth = 24;
        mSaveAllButton->layout->maxWidth = 24;
        mUpPanel->AddChild(mSaveAllButton);

        // Provider keys and the AssetsLine link share one window; the dot on the gear is the state of the link
        mSettingsButton = o2UI.CreateWidget<Button>("menu pipeline settings");
        mSettingsButton->name = "settings";
        mSettingsButton->onClick = THIS_FUNC(OnSettingsPressed);
        mSettingsButton->layout->minWidth = 24;
        mSettingsButton->layout->maxWidth = 24;
        mSyncDot.hideWhenOff = true;
        mSyncDot.Attach(mSettingsButton, Vec2F(1.0f, 0.0f), Vec2F(-5.0f, 5.0f), 7.0f);
        mUpPanel->AddChild(mSettingsButton);
        AttachSync();

        mEditor = mmake<PipelineEditor>();
        *mEditor->layout = WidgetLayout::BothStretch(0, 0, 0, 20);
        mEditor->actionsListDelegate = DynamicCast<ActionsList>(Ref<IAssetEditorWindow>(this));
        mEditor->onLog = [this](const String& message)
        {
            mStatusLabel->text = message;
            o2Debug.Log("[pipeline] " + message);
        };
        mWindow->AddChild(mEditor);

        auto horScroll = o2UI.CreateHorScrollBar();
        *horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 5, 15, 10);
        mEditor->SetHorScrollbar(horScroll);

        auto verScroll = o2UI.CreateVerScrollBar();
        *verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 5, 15, 10);
        mEditor->SetVerScrollbar(verScroll);
    }

    String PipelineWindow::GetWindowTitle() const
    {
        return "Pipeline";
    }

    void PipelineWindow::OnStartEditingAsset()
    {
        auto asset = DynamicCast<PipelineAsset>(mEditingAsset.Lock());
        mEditor->SetAsset(asset);
        // Only assets that live in the project are worth reopening; a fresh unsaved one is not
        if (asset && !asset->GetPath().IsEmpty() && o2Assets.IsAssetExist(asset->GetPath()) && EditorConfig::IsSingletonInitialzed())
            o2EditorConfig.projectConfig.lastPipelineAsset = asset->GetPath();
    }

    void PipelineWindow::OnCompletedEditingAsset()
    {
        mEditor->StopRun();
    }

    bool PipelineWindow::IsComponentPreviewAvailable() const
    {
        return false;
    }

    bool PipelineWindow::IsCreateNewAssetAtStartupEnabled() const
    {
        return true;
    }

    void PipelineWindow::OnAssetSaved()
    {
        if (AssetsLineSync::IsSingletonInitialzed())
            o2AssetsLineSync.NotifyLocalChange();
    }

    void PipelineWindow::AttachSync()
    {
        if (!AssetsLineSync::IsSingletonInitialzed())
            return;

        auto& sync = o2AssetsLineSync;
        mAttachedSync = &sync;
        sync.isPipelineFileBusy = [this](const String& path)
        {
            auto asset = mEditingAsset.Lock();
            return asset && asset->GetPath() == path && asset->IsDirty();
        };
        sync.onPipelineFileChanged = [this](const String& path)
        {
            auto asset = mEditingAsset.Lock();
            if (asset && asset->GetPath() == path)
                mReloadPending = true;
        };
        sync.onResultsChanged = [this](const String& pipelineId)
        {
            if (mEditor && mEditor->GetAsset() && mEditor->GetPipelineId() == pipelineId)
                mEditor->ReloadResults();
        };
    }

    bool PipelineWindow::IsSyncedPath(const String& path) const
    {
        String folder = o2AssetsLineSync.GetConfig().folder.Trimed(" /\\");
        return folder.IsEmpty() || path.StartsWith(folder + "/");
    }

    void PipelineWindow::UpdateSync(float dt)
    {
        if (!AssetsLineSync::IsSingletonInitialzed())
            return;

        auto& sync = o2AssetsLineSync;
        if (mAttachedSync != &sync)
            AttachSync();

        mSyncDot.Update(dt, sync.GetStatus());

        // A synced pipeline saves itself shortly after an edit, like the web editor does
        auto asset = DynamicCast<PipelineAsset>(mEditingAsset.Lock());
        bool synced = sync.GetConfig().enabled && sync.IsConnected() && asset && !asset->GetPath().IsEmpty() &&
            IsSyncedPath(asset->GetPath());
        if (synced && asset->IsDirty())
        {
            mUnsavedTime += dt;
            if (mUnsavedTime > 1.0f)
            {
                mUnsavedTime = 0.0f;
                SaveEditingAsset();
            }
        }
        else
            mUnsavedTime = 0.0f;

        if (mReloadPending && !sync.IsBusy())
        {
            mReloadPending = false;
            ReloadFromDisk();
        }
    }

    void PipelineWindow::ReloadFromDisk()
    {
        auto asset = DynamicCast<PipelineAsset>(mEditingAsset.Lock());
        if (!asset || asset->IsDirty())
            return;

        // Renamed by the sync: the asset lives under its new path now
        String path = o2Assets.GetAssetPath(asset->GetUID());
        if (!path.IsEmpty() && path != asset->GetPath())
        {
            OpenAsset(AssetRef<Asset>(AssetRef<PipelineAsset>(path)));
            return;
        }

        DataDocument doc;
        if (!doc.LoadFromFile(o2Assets.GetAssetsPath() + asset->GetPath()))
            return;

        asset->document = doc;
        mEditor->ReloadGraph();
        mStatusLabel->text = "Updated from AssetsLine";
    }

    void PipelineWindow::Update(float dt)
    {
        PushEditorScopeOnStack scope;
        IAssetEditorWindow::Update(dt);

        bool running = mEditor && mEditor->IsRunning();
        mStopButton->interactable = running;
        UpdateSync(dt);

        if (!mAutoImport.IsEmpty())
        {
            String file = mAutoImport;
            mAutoImport = "";
            ImportFile(file);
        }
    }

    void PipelineWindow::OnImportPressed()
    {
        Map<String, String> filter = { { "AssetsLine pipeline", "*.zip;*.json" } };
        String file = GetOpenFileNameDialog("Import AssetsLine pipeline (.zip or .json)", filter, "");
        if (file.IsEmpty())
            return;

        ImportFile(file);
    }

    void PipelineWindow::ImportFile(const String& file)
    {
        CheckDirtyAssetAndExecute([this, file]()
        {
            String error;
            String assetPath;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
            // The frame runs inside a coroutine that terminates on any escaped exception, so a broken file must not throw past here
            try
            {
                assetPath = PipelineImport::ImportFile(file, "Pipelines/", error);
                if (!assetPath.IsEmpty())
                    OpenImported(assetPath);
            }
            catch (const std::exception& e)
            {
                error = e.what();
                assetPath = "";
            }
#else
            assetPath = PipelineImport::ImportFile(file, "Pipelines/", error);
            if (!assetPath.IsEmpty())
                OpenImported(assetPath);
#endif

            if (assetPath.IsEmpty())
            {
                mStatusLabel->text = "Import failed: " + error;
                o2Debug.LogError("[pipeline] import failed: " + error);
                return;
            }

            mStatusLabel->text = "Imported " + assetPath;
        });
    }

    void PipelineWindow::OpenImported(const String& assetPath)
    {
        // Unsaved changes were asked about before the import; OpenAsset would ask again
        EditAsset(AssetRef<Asset>(AssetRef<PipelineAsset>(assetPath)));
        Show();
    }

    void PipelineWindow::OnSettingsPressed()
    {
        AssetsLineDlg::Show();
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineWindow, Editor__PipelineWindow);
// --- END META ---
