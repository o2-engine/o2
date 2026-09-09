#include "o2Editor/stdafx.h"
#include "PipelineWindow.h"

#include "o2/Assets/Assets.h"
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
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/Pipeline/PipelineImport.h"
#include "o2Editor/Windows/PipelineWindow/PipelineSettingsDlg.h"

DECLARE_SINGLETON(Editor::PipelineWindow);

namespace Editor
{
    PipelineWindow::PipelineWindow(RefCounter* refCounter):
        Singleton<PipelineWindow>(refCounter), IAssetEditorWindow(refCounter)
    {
        InitializeWindow();
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
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_graph_icon.png"));
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

        mSettingsButton = o2UI.CreateWidget<Button>("menu pipeline settings");
        mSettingsButton->name = "settings";
        mSettingsButton->onClick = THIS_FUNC(OnSettingsPressed);
        mButtonsPanel->AddChild(mSettingsButton);

        mStatusLabel = o2UI.CreateLabel("");
        mStatusLabel->name = "status";
        mStatusLabel->horAlign = HorAlign::Left;
        mStatusLabel->horOverflow = Label::HorOverflow::Dots;
        mStatusLabel->layout->minWidth = 200;
        mUpPanel->AddChild(mStatusLabel);

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
        mStatusLabel->text = "Saved";
    }

    void PipelineWindow::Update(float dt)
    {
        IAssetEditorWindow::Update(dt);

        bool running = mEditor && mEditor->IsRunning();
        mStopButton->interactable = running;
    }

    void PipelineWindow::OnImportPressed()
    {
        Map<String, String> filter = { { "AssetsLine pipeline", "*.zip;*.json" } };
        String file = GetOpenFileNameDialog("Import AssetsLine pipeline (.zip or .json)", filter, "");
        if (file.IsEmpty())
            return;

        CheckDirtyAssetAndExecute([this, file]()
        {
            String error;
            String assetPath = PipelineImport::ImportFile(file, "Pipelines/", error);
            if (assetPath.IsEmpty())
            {
                mStatusLabel->text = "Import failed: " + error;
                o2Debug.LogError("[pipeline] import failed: " + error);
                return;
            }

            mStatusLabel->text = "Imported " + assetPath;
            OpenAsset(AssetRef<Asset>(AssetRef<PipelineAsset>(assetPath)));
        });
    }

    void PipelineWindow::OnSettingsPressed()
    {
        PipelineSettingsDlg::Show();
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineWindow, Editor__PipelineWindow);
// --- END META ---
