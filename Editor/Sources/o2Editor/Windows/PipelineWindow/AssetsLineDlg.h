#pragma once

#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"
#include "o2Editor/Pipeline/Providers/LocalAgentSetup.h"
#include "o2Editor/Windows/PipelineWindow/AssetsLineStatusDot.h"

using namespace o2;

namespace o2
{
    class Button;
    class DropDown;
    class EditBox;
    class Label;
    class Toggle;
    class VerticalLayout;
    class Widget;
    class Window;
}

namespace Editor
{
    // --------------------------------------------------------------------------------
    // Settings of the pipelines, a page each: AssetsLine (the link of the project:
    // server, connecting in the browser or with a token, the synced folder and options;
    // the header shows the live state of the sync), API keys of the providers nodes call
    // here, Local agents (Claude Code and Codex on this computer: a card each with its
    // state, Install and sign in / Sign in that open a terminal with the commands, the
    // model, thinking or effort and the account; custom paths and command folded) and
    // Editor (the view of the image nodes, Python for AI image to UI)
    // --------------------------------------------------------------------------------
    class AssetsLineDlg : public Singleton<AssetsLineDlg>
    {
    public:
        enum class Page { Sync, Keys, Agents, Editor };

    public:
        // Default constructor
        AssetsLineDlg(RefCounter* refCounter);

        // Destructor
        ~AssetsLineDlg();

        // Shows the dialog on the page shown last, creates it on first call
        static void Show();

        // Shows the dialog on the page
        static void Show(Page page);

        // Shows the page
        void ShowPage(Page page);

        // Returns the page shown
        Page GetPage() const { return mPage; }

        // Refreshes the state shown while the dialog is open and moves the state dot; called every frame
        void Update(float dt);

        // Returns the dialog window
        const Ref<Window>& GetWindow() const { return mWindow; }

        // Returns the state dot of the header
        const AssetsLineStatusDot& GetStatusDot() const { return mStatusDot; }

        REF_COUNTERABLE_IMPL(Singleton<AssetsLineDlg>);

    private:
        // A local agent's card: its state, the setup button, its options
        struct AgentCard
        {
            LocalAgentSetup::Agent agent = LocalAgentSetup::Agent::ClaudeCode;
            LocalAgentSetup::Check check;            // Background check of its state
            Ref<Widget>            dot;              // Dot coloured by the state
            Ref<Label>             state;            // Ready, Not installed, Not signed in...
            Ref<Label>             detail;           // The account, or what to do
            Ref<Button>            setup;            // Install and sign in, Sign in
            Ref<Button>            recheck;          // Checks again
            Ref<Widget>            options;          // Model, thinking or effort, account: shown once it is installed
            Ref<DropDown>          model;            // Model it runs on
            Ref<DropDown>          tuning;           // Claude: thinking; Codex: effort
            Ref<DropDown>          account;          // Subscription or API key
            Ref<Widget>            keyRow;           // API key row, shown for the API key account
            Ref<EditBox>           key;              // The API key
            Vector<String>         modelIds;         // Model ids in the order of the model items
            String                 catalog;          // Codex's catalog the model items were made from
            String                 shown;            // State shown last, to update only on change
            bool                   settingUp = false; // A setup terminal was opened and the agent is not ready yet
            float                  sinceCheck = 0;   // Seconds since the last check
            float                  setupFor = 0;     // Seconds since the setup terminal opened
            String                 setupError;       // Why the setup terminal did not open
        };

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
        Ref<Toggle>         mSideView;         // Image nodes show the input beside the result
        Ref<Toggle>         mCompareView;      // Image nodes show input and result split by a divider
        Ref<EditBox>        mGeminiEdit;       // Gemini API key
        Ref<EditBox>        mOpenAiEdit;       // OpenAI API key
        Ref<EditBox>        mOpenRouterEdit;   // OpenRouter API key
        Ref<EditBox>        mElevenEdit;       // ElevenLabs API key
        Ref<EditBox>        mKlingAccessEdit;  // Kling API key or access key
        Ref<EditBox>        mKlingSecretEdit;  // Kling secret key of a legacy key pair
        Ref<EditBox>        mPythonEdit;       // Python 3 of AI image to UI, empty to search
        Ref<Toggle>         mLocalToggle;      // The model menus offer the local agents
        AgentCard           mClaude;           // Claude Code's card
        AgentCard           mCodex;            // Codex's card
        Ref<Toggle>         mCustomToggle;     // Shows the custom paths and command
        Ref<Widget>         mCustomSection;    // Custom paths and command
        Ref<EditBox>        mLocalPathEdit;    // Claude Code executable, empty to search
        Ref<Label>          mLocalFound;       // Which Claude Code runs, or that none is found
        Ref<EditBox>        mCodexPathEdit;    // Codex executable, empty to search
        Ref<Label>          mCodexFound;       // Which Codex runs, or that none is found
        Ref<EditBox>        mLocalCommandEdit; // Shell command of local:command
        Ref<Toggle>         mTabs[4];          // The page tabs, in the order of Page
        Ref<Widget>         mPages[4];         // The pages, in the order of Page

        Page   mPage = Page::Sync; // Page shown
        String mShownStatus;       // State shown last, to refresh only on change
        bool   mReadingAgents = false; // The dropdowns are being filled: their changes are not the person's

    private:
        // Creates the window and its controls
        void InitializeControls();

        // Fills the fields from the link settings
        void ReadConfig();

        // Stores a changed folder or option
        void WriteConfig();

        // Stores the provider keys and the local agents' settings
        void WriteKeys();

        // Shows which Claude Code and which Codex run
        void ShowLocalAgentFound();

        // Builds the page of the local agents
        void BuildAgentsPage(const Ref<VerticalLayout>& page);

        // Builds an agent's card
        void BuildAgentCard(const Ref<VerticalLayout>& page, AgentCard& card);

        // Fills an agent's dropdowns from the settings
        void ReadAgent(AgentCard& card);

        // Starts the checks of both agents
        void CheckAgents();

        // Moves the checks on, re-checks while a setup runs, shows the states
        void UpdateAgents(float dt);

        // Shows an agent's state, its setup button and its options
        void ShowAgent(AgentCard& card);

        // Opens the terminal that installs the agent or signs it in
        void SetUpAgent(AgentCard& card);

        // Stores an agent's dropdowns and key
        void WriteAgent(AgentCard& card);

        // Shows the controls of the connection type chosen
        void OnConnectionTypeChanged();

        // Shows the state, the link and the controls that fit them
        void Refresh();

        // Fits the window height to the rows shown
        void FitHeight();
    };
}
// --- META ---

PRE_ENUM_META(Editor::AssetsLineDlg::Page);
// --- END META ---
