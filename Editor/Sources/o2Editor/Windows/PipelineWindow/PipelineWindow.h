#pragma once

#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Windows/IAssetEditorWindow.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"

using namespace o2;

#define o2EditorPipelineWindow PipelineWindow::Instance()

namespace o2
{
    class Button;
    class Label;
}

namespace Editor
{
    // -------------------------------------------------------------
    // Asset editor window of pipeline assets: the node graph canvas
    // with run controls and the provider settings dialog
    // -------------------------------------------------------------
    class PipelineWindow : public Singleton<PipelineWindow>, public IAssetEditorWindow
    {
    public:
        // Default constructor
        PipelineWindow(RefCounter* refCounter);

        // Destructor
        ~PipelineWindow();

        // Returns edited asset type, PipelineAsset
        const Type& GetAssetType() const override;

        // Returns node graph editor
        const Ref<PipelineEditor>& GetEditor() const { return mEditor; }

        // Updates window and the stop button availability
        void Update(float dt) override;

        // Imports the AssetsLine export as a new pipeline asset and opens it; errors go to the status line and the log
        void ImportFile(const String& file);

        // Dynamic cast to RefCounterable via Singleton
        static Ref<RefCounterable> CastToRefCounterable(const Ref<PipelineWindow>& ref);

        IOBJECT(PipelineWindow);
        REF_COUNTERABLE_IMPL(IEditorWindow, Singleton<PipelineWindow>);

    protected:
        Ref<PipelineEditor> mEditor;         // Node graph editor
        Ref<Button>         mRunAllButton;   // Runs every finish node
        Ref<Button>         mStopButton;     // Cancels the current run
        Ref<Button>         mSettingsButton; // Opens provider API keys dialog
        Ref<Button>         mFitButton;      // Fits the view to all cards
        Ref<Button>         mImportButton;   // Imports an AssetsLine pipeline file
        Ref<Label>          mStatusLabel;    // Last log line
        String              mAutoImport;     // File imported on the first update, from O2_PIPELINE_IMPORT (development aid)

    protected:
        // Creates window, toolbar buttons, status label and the editor
        void InitializeWindow() override;

        // Returns window title
        String GetWindowTitle() const override;

        // Called when asset editing started, passes it to the editor and remembers it in project config
        void OnStartEditingAsset() override;

        // Called when the import button is pressed; asks for an AssetsLine file and opens the imported asset
        void OnImportPressed();

        // Opens the freshly imported asset and shows the window
        void OpenImported(const String& assetPath);

        // Called when asset editing completed, stops the run
        void OnCompletedEditingAsset() override;

        // Returns false, pipelines have no component preview
        bool IsComponentPreviewAvailable() const override;

        // Returns true, a new asset is created when the window opens without one
        bool IsCreateNewAssetAtStartupEnabled() const override;

        // Called when asset saved, shows it in the status label
        void OnAssetSaved() override;

        // Called when settings button pressed, opens provider API keys dialog
        void OnSettingsPressed();
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineWindow)
{
    BASE_CLASS(o2::Singleton<PipelineWindow>);
    BASE_CLASS(Editor::IAssetEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineWindow)
{
    FIELD().PROTECTED().NAME(mEditor);
    FIELD().PROTECTED().NAME(mRunAllButton);
    FIELD().PROTECTED().NAME(mStopButton);
    FIELD().PROTECTED().NAME(mSettingsButton);
    FIELD().PROTECTED().NAME(mFitButton);
    FIELD().PROTECTED().NAME(mImportButton);
    FIELD().PROTECTED().NAME(mStatusLabel);
    FIELD().PROTECTED().NAME(mAutoImport);
}
END_META;
CLASS_METHODS_META(Editor::PipelineWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<PipelineEditor>&, GetEditor);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, ImportFile, const String&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<PipelineWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(String, GetWindowTitle);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnImportPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OpenImported, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsComponentPreviewAvailable);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsCreateNewAssetAtStartupEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetSaved);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSettingsPressed);
}
END_META;
// --- END META ---
