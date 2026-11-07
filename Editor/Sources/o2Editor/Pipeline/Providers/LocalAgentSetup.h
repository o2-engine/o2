#pragma once

#include <chrono>
#include <memory>

#include "o2Editor/Pipeline/PipelineSettings.h"

using namespace o2;

namespace Editor
{
    namespace UiLayout
    {
        class AgentProcess;
    }

    // -----------------------------------------------------------------------
    // Whether a local agent is ready to answer, and setting one up. A check
    // runs the agent's own status command in the background (claude auth
    // status --json, codex login status); a key in the settings needs no
    // sign-in. The setup opens a terminal with a script that installs the
    // agent when it is missing and signs in when it is signed out, so the
    // person sees every command and types nothing. Codex's models come from
    // its bundled catalog (codex debug models --bundled)
    // -----------------------------------------------------------------------
    namespace LocalAgentSetup
    {
        enum class Agent { ClaudeCode, Codex };
        enum class State { Unknown, Checking, NotInstalled, SignedOut, Ready, Failed };

        // What a check found
        struct Status
        {
            State  state = State::Unknown;
            String detail; // One line for the person: the account, or what is missing
            String exe;    // The executable found, empty when none is
        };

        // A model of the Codex catalog
        struct ModelInfo
        {
            String id;          // The slug -m takes
            String name;        // Display name
            String description; // One line
        };

        // Returns the agent's name as the window shows it
        String NameOf(Agent agent);

        // Reads claude auth status --json
        Status ClaudeStatusOf(int exitCode, const String& output);

        // Reads codex login status (it prints on stderr)
        Status CodexStatusOf(int exitCode, const String& output);

        // Reads codex debug models --bundled: the models it lists, by priority
        Vector<ModelInfo> CodexModelsOf(const String& json);

        // The models offered while the catalog is not read
        Vector<ModelInfo> FallbackCodexModels();

        // The setup script: installs the agent when it is missing, signs in when it is signed out, shows the status
        String SetupScript(Agent agent);

        // Writes the setup script and opens a terminal running it; false with the reason when no terminal opens
        bool OpenSetup(Agent agent, String& error);

        // Replaces the terminal opener (tests); it gets the agent and the path of the script. Null restores the terminal
        void SetSetupOpener(const Function<bool(Agent, const String&)>& opener);

        // ---------------------------------------------------------------------
        // A check of one agent in the background, polled every frame: whether it
        // is installed, then its status command; Codex's catalog once a session
        // ---------------------------------------------------------------------
        class Check
        {
        public:
            Check();
            ~Check();

            // Starts a check with the settings of the moment; a running one is dropped
            void Start(Agent agent, const PipelineSettings& settings);

            // Moves the check on; returns true when it is done (also when it never started)
            bool Poll();

            // Returns true while it runs
            bool IsRunning() const;

            // Returns what the last check found
            const Status& GetStatus() const { return mStatus; }

            // Returns Codex's models: the catalog once read, else the fallback list
            static Vector<ModelInfo> GetCodexModels();

        private:
            enum class Step { Idle, Status, Models };

            Agent  mAgent = Agent::ClaudeCode;
            Status mStatus;
            Step   mStep = Step::Idle;
            String mOutput;

            std::unique_ptr<UiLayout::AgentProcess>  mProcess;
            std::chrono::steady_clock::time_point     mStarted;

        private:
            // Starts the command of a step; false when it cannot
            bool Run(Step step, const Vector<String>& args);

            // Finishes the running step with what it printed
            void Finish(int code);
        };
    }
}
// --- META ---

PRE_ENUM_META(Editor::LocalAgentSetup::Agent);

PRE_ENUM_META(Editor::LocalAgentSetup::State);
// --- END META ---
