#pragma once

#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"
#include "o2Editor/Windows/PipelineWindow/AssetsLineStatusDot.h"

using namespace o2;

namespace o2
{
    class Button;
    class DropDown;
    class EditBox;
    class Label;
    class Toggle;
    class Widget;
    class Window;
}

namespace Editor
{
    // --------------------------------------------------------------------------------
    // Settings of the pipelines: the link of the project to AssetsLine (server,
    // connecting in the browser or with a token, the synced folder and options, the
    // live state of the sync) and the keys of the providers nodes call here
    // --------------------------------------------------------------------------------
    class AssetsLineDlg : public Singleton<AssetsLineDlg>
    {
    public:
        // Default constructor
        AssetsLineDlg(RefCounter* refCounter);

        // Destructor
        ~AssetsLineDlg();

        // Shows the dialog, creates it on first call
        static void Show();

        // Refreshes the state shown while the dialog is open and moves the state dot; called every frame
        void Update(float dt);

        // Returns the dialog window
        const Ref<Window>& GetWindow() const { return mWindow; }

        // Returns the state dot of the header
        const AssetsLineStatusDot& GetStatusDot() const { return mStatusDot; }

        REF_COUNTERABLE_IMPL(Singleton<AssetsLineDlg>);

    private:
        Ref<Window>         mWindow;           // Dialog window
        Ref<Widget>         mHeader;           // Band with the link state
        AssetsLineStatusDot mStatusDot;        // State dot in the header
        Ref<Label>          mStatusLabel;      // Link state
        Ref<Label>          mStatusDetail;     // What is synced, when, or what went wrong
        Ref<Widget>         mContent;          // Rows under the header
        Ref<EditBox>        mServerEdit;       // Server address
        Ref<Widget>         mConnectSection;   // How to connect, while not connected
        Ref<DropDown>       mConnectionType;   // Browser or access token
        Ref<Widget>         mBrowserPanel;     // Connecting in the browser
        Ref<Button>         mConnectButton;    // Opens the approval page
        Ref<Button>         mCancelButton;     // Stops waiting for the approval
        Ref<Widget>         mApprovePanel;     // Code and page address while waiting
        Ref<Label>          mCodeLabel;        // Code the approval page shows
        Ref<EditBox>        mApproveEdit;      // Approval page address, to open by hand
        Ref<Widget>         mTokenPanel;       // Connecting with a token
        Ref<EditBox>        mTokenEdit;        // Access token made in the AssetsLine settings
        Ref<Button>         mTokenButton;      // Connects with the token
        Ref<Widget>         mLinkedSection;    // Linked project, while connected
        Ref<Label>          mProjectLabel;     // Linked project and account
        Ref<Button>         mDisconnectButton; // Forgets the token
        Ref<EditBox>        mFolderEdit;       // Synced folder inside Assets
        Ref<Toggle>         mEnabledToggle;    // Sync on or off
        Ref<Toggle>         mFinishToggle;     // Finish results written into the assets
        Ref<Button>         mSyncButton;       // Runs a pass now
        Ref<EditBox>        mGeminiEdit;       // Gemini API key
        Ref<EditBox>        mElevenEdit;       // ElevenLabs API key
        Ref<EditBox>        mKlingAccessEdit;  // Kling API key or access key
        Ref<EditBox>        mKlingSecretEdit;  // Kling secret key of a legacy key pair

        String mShownStatus; // State shown last, to refresh only on change

    private:
        // Creates the window and its controls
        void InitializeControls();

        // Fills the fields from the link settings
        void ReadConfig();

        // Stores a changed folder or option
        void WriteConfig();

        // Stores the provider keys
        void WriteKeys();

        // Shows the controls of the connection type chosen
        void OnConnectionTypeChanged();

        // Shows the state, the link and the controls that fit them
        void Refresh();

        // Fits the window height to the rows shown
        void FitHeight();
    };
}
