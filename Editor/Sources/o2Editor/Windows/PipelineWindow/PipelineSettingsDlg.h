#pragma once

#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"

using namespace o2;

namespace o2
{
    class EditBox;
    class Window;
}

namespace Editor
{
    // ----------------------------------------------------
    // Provider API keys dialog (Gemini, ElevenLabs, Kling)
    // ----------------------------------------------------
    class PipelineSettingsDlg : public Singleton<PipelineSettingsDlg>
    {
    public:
        // Default constructor
        PipelineSettingsDlg(RefCounter* refCounter);

        // Destructor
        ~PipelineSettingsDlg();

        // Shows the dialog with keys loaded from settings, creates it on first call
        static void Show();

        // Returns the dialog window
        const Ref<Window>& GetWindow() const { return mWindow; }

        REF_COUNTERABLE_IMPL(Singleton<PipelineSettingsDlg>);

    private:
        Ref<Window>  mWindow;          // Dialog window
        Ref<EditBox> mGeminiEdit;      // Gemini API key
        Ref<EditBox> mElevenEdit;      // ElevenLabs API key
        Ref<EditBox> mKlingAccessEdit; // Kling API key or access key
        Ref<EditBox> mKlingSecretEdit; // Kling secret key, legacy key pairs only

    private:
        // Creates window, key fields and buttons
        void InitializeControls();

        // Called when save button pressed, stores keys to settings and hides the dialog
        void OnSavePressed();
    };
}
