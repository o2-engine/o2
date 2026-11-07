#include "o2Editor/stdafx.h"
#include "LocalAgentSetup.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/AiHttp.h"
#include "o2Editor/Pipeline/Providers/LocalAgentProvider.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"

namespace Editor::LocalAgentSetup
{
    static const char* dot = " \xC2\xB7 ";
    static const int checkSeconds = 20;

    static Vector<ModelInfo> codexCatalog;
    static bool              codexCatalogRead = false;

    static Function<bool(Agent, const String&)> setupOpener;

    String NameOf(Agent agent)
    {
        return agent == Agent::ClaudeCode ? "Claude Code" : "Codex";
    }

    static String LastLine(const String& text)
    {
        String last;
        for (auto& line : text.Split("\n"))
        {
            String trimmed = line.Trimed(" \t\r");
            if (!trimmed.IsEmpty())
                last = trimmed;
        }
        return last;
    }

    static String Capitalized(const String& text)
    {
        return text.IsEmpty() ? text : text.SubStr(0, 1).ToUpperCase() + text.SubStr(1);
    }

    Status ClaudeStatusOf(int exitCode, const String& output)
    {
        Status res;
        int start = output.Find("{");
        DataDocument doc;
        if (start < 0 || !doc.LoadFromData(output.SubStr(start)) || !doc.IsObject())
        {
            res.state = State::Failed;
            String last = LastLine(output);
            res.detail = "claude auth status failed" + (last.IsEmpty() ? " (exit code " + (String)exitCode + ")" : ": " + last);
            return res;
        }

        auto loggedIn = AiHttp::Member(&doc, "loggedIn");
        if (!loggedIn || !loggedIn->IsBoolean() || !(bool)*loggedIn)
        {
            res.state = State::SignedOut;
            res.detail = "Not signed in: sign in with your Claude account to use your subscription";
            return res;
        }

        res.state = State::Ready;
        String method = AiHttp::StringOf(AiHttp::Member(&doc, "authMethod"));
        String plan = AiHttp::StringOf(AiHttp::Member(&doc, "subscriptionType"));
        String email = AiHttp::StringOf(AiHttp::Member(&doc, "email"));
        if (method == "claude.ai" || method.IsEmpty())
            res.detail = "Signed in" + (plan.IsEmpty() ? String() : dot + ("Claude " + Capitalized(plan)));
        else
            res.detail = "Signed in to the Anthropic Console" + String(dot) + "API prices";
        if (!email.IsEmpty())
            res.detail += dot + email;
        return res;
    }

    Status CodexStatusOf(int exitCode, const String& output)
    {
        Status res;
        if (output.Contains("Logged in using ChatGPT"))
        {
            res.state = State::Ready;
            res.detail = "Signed in with ChatGPT";
        }
        else if (output.Contains("Logged in using an API key"))
        {
            res.state = State::Ready;
            res.detail = "Signed in with an OpenAI API key" + String(dot) + "API prices";
        }
        else if (output.Contains("Not logged in"))
        {
            res.state = State::SignedOut;
            res.detail = "Not signed in: sign in with your ChatGPT account to use your subscription";
        }
        else if (exitCode == 0)
        {
            res.state = State::Ready;
            res.detail = LastLine(output);
        }
        else
        {
            res.state = State::Failed;
            String last = LastLine(output);
            res.detail = "codex login status failed" + (last.IsEmpty() ? " (exit code " + (String)exitCode + ")" : ": " + last);
        }
        return res;
    }

    Vector<ModelInfo> CodexModelsOf(const String& json)
    {
        Vector<std::pair<double, ModelInfo>> listed;
        int start = json.Find("{");
        DataDocument doc;
        if (start < 0 || !doc.LoadFromData(json.SubStr(start)) || !doc.IsObject())
            return {};

        auto models = AiHttp::Member(&doc, "models");
        if (!models || !models->IsArray())
            return {};

        for (auto& model : *models)
        {
            if (AiHttp::StringOf(AiHttp::Member(&model, "visibility")) != "list")
                continue;

            ModelInfo info;
            info.id = AiHttp::StringOf(AiHttp::Member(&model, "slug"));
            info.name = AiHttp::StringOf(AiHttp::Member(&model, "display_name"));
            info.description = AiHttp::StringOf(AiHttp::Member(&model, "description"));
            if (info.id.IsEmpty())
                continue;
            if (info.name.IsEmpty())
                info.name = info.id;

            auto priority = AiHttp::Member(&model, "priority");
            listed.Add({ priority && priority->IsNumber() ? AiHttp::NumberOf(priority) : 1e9, info });
        }

        std::stable_sort(listed.begin(), listed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        Vector<ModelInfo> res;
        for (auto& entry : listed)
            res.Add(entry.second);
        return res;
    }

    Vector<ModelInfo> FallbackCodexModels()
    {
        return { { "gpt-6.1-sol", "GPT-6.1-Sol", "Latest workhorse model for coding and everyday work." },
                 { "gpt-6-astra", "GPT-6-Astra", "Frontier intelligence for the most demanding work." },
                 { "gpt-6-sol", "GPT-6-Sol", "Previous generation workhorse model." },
                 { "gpt-6-luna", "GPT-6-Luna", "Fast and affordable model for easier tasks." } };
    }

    String SetupScript(Agent agent)
    {
        String head =
            "#!/bin/bash\n"
            "# Set up by the o2 editor: installs the agent when it is missing, signs in when it is signed out\n"
            "export PATH=\"$HOME/.local/bin:/opt/homebrew/bin:/usr/local/bin:$PATH\"\n"
            "finish() { echo; echo \"$1\"; read -r -p \"Press Enter to close this window \"; exit \"$2\"; }\n"
            "clear\n";

        if (agent == Agent::ClaudeCode)
        {
            return head +
                "echo \"Claude Code for the o2 pipeline editor\"; echo\n"
                "if ! command -v claude >/dev/null 2>&1; then\n"
                "  echo \"Installing Claude Code (https://claude.ai/install.sh)...\"\n"
                "  curl -fsSL https://claude.ai/install.sh | bash || finish \"The installation failed, see above.\" 1\n"
                "  export PATH=\"$HOME/.local/bin:$PATH\"\n"
                "fi\n"
                "if claude auth status --json 2>/dev/null | grep -q '\"loggedIn\": *true'; then\n"
                "  echo \"Claude Code is signed in.\"\n"
                "else\n"
                "  echo \"Sign in with your Claude account in the browser that opens...\"; echo\n"
                "  claude auth login --claudeai || finish \"Signing in did not finish, see above.\" 1\n"
                "fi\n"
                "echo; claude auth status --text\n"
                "finish \"Done. Go back to the o2 editor: it notices by itself.\" 0\n";
        }

        return head +
            "echo \"Codex for the o2 pipeline editor\"; echo\n"
            "if ! command -v codex >/dev/null 2>&1; then\n"
            "  if command -v npm >/dev/null 2>&1; then\n"
            "    echo \"Installing Codex (npm install -g @openai/codex)...\"\n"
            "    npm install -g @openai/codex || finish \"The installation failed, see above.\" 1\n"
            "  elif command -v brew >/dev/null 2>&1; then\n"
            "    echo \"Installing Codex (brew install --cask codex)...\"\n"
            "    brew install --cask codex || finish \"The installation failed, see above.\" 1\n"
            "  else\n"
            "    finish \"Codex needs Node.js: install it from https://nodejs.org, then press Install in the editor again.\" 1\n"
            "  fi\n"
            "fi\n"
            "if codex login status >/dev/null 2>&1; then\n"
            "  echo \"Codex is signed in.\"\n"
            "else\n"
            "  echo \"Sign in with your ChatGPT account in the browser that opens...\"; echo\n"
            "  codex login || finish \"Signing in did not finish, see above.\" 1\n"
            "fi\n"
            "echo; codex login status\n"
            "finish \"Done. Go back to the o2 editor: it notices by itself.\" 0\n";
    }

    void SetSetupOpener(const Function<bool(Agent, const String&)>& opener)
    {
        setupOpener = opener;
    }

    static String Quoted(const String& path)
    {
        String res = "'";
        for (int i = 0; i < path.Length(); i++)
            res += path[i] == '\'' ? String("'\\''") : String(path[i]);
        return res + "'";
    }

    bool OpenSetup(Agent agent, String& error)
    {
        std::error_code ec;
        String folder = String(std::filesystem::temp_directory_path(ec).string().c_str());
        if (ec || folder.IsEmpty())
            folder = "/tmp";
        if (!folder.EndsWith("/"))
            folder += "/";
        String script = folder + (agent == Agent::ClaudeCode ? "o2-claude-code-setup.command" : "o2-codex-setup.command");
        PipelineUtils::WriteFileBytes(script, SetupScript(agent));
        std::filesystem::permissions(script.Data(), std::filesystem::perms::owner_all | std::filesystem::perms::group_read |
                                                    std::filesystem::perms::others_read, ec);

        if (setupOpener)
        {
            if (setupOpener(agent, script))
                return true;
            error = "The terminal did not open";
            return false;
        }

#if defined(PLATFORM_MAC)
        if (std::system(("/usr/bin/open -a Terminal " + Quoted(script)).Data()) == 0)
            return true;
        error = "Terminal did not open: run " + script + " in a terminal";
        return false;
#elif defined(PLATFORM_LINUX)
        for (String terminal : { "x-terminal-emulator -e", "gnome-terminal --", "konsole -e", "xfce4-terminal -x", "xterm -e" })
        {
            String exe = terminal.SubStr(0, terminal.Find(" "));
            if (std::system(("command -v " + exe + " >/dev/null 2>&1").Data()) != 0)
                continue;
            if (std::system((terminal + " bash " + Quoted(script) + " >/dev/null 2>&1 &").Data()) == 0)
                return true;
        }
        error = "No terminal found: run bash " + script + " in a terminal";
        return false;
#else
        error = "Local agents are not available on this system";
        return false;
#endif
    }

    Check::Check() = default;

    Check::~Check() = default;

    void Check::Start(Agent agent, const PipelineSettings& settings)
    {
        mAgent = agent;
        mProcess.reset();
        mStep = Step::Idle;
        mStatus = Status();
        mStatus.state = State::Checking;

        mStatus.exe = agent == Agent::ClaudeCode ? LocalAgentProvider::FindClaude(settings.localAgentPath)
                                                 : LocalAgentProvider::FindCodex(settings.localCodexPath);
        if (mStatus.exe.IsEmpty())
        {
            mStatus.state = State::NotInstalled;
            mStatus.detail = NameOf(agent) + " is not installed on this computer";
            return;
        }

        String key = (agent == Agent::ClaudeCode ? settings.localAgentApiKey : settings.localCodexApiKey).Trimed(" \n\r\t");
        if (!key.IsEmpty())
        {
            mStatus.state = State::Ready;
            mStatus.detail = "Uses your " + String(agent == Agent::ClaudeCode ? "Anthropic" : "OpenAI") + " API key" + dot +
                "billed at API prices";
            if (agent == Agent::Codex && !codexCatalogRead)
                Run(Step::Models, { "debug", "models", "--bundled" });
            return;
        }

        bool started = agent == Agent::ClaudeCode ? Run(Step::Status, { "auth", "status", "--json" })
                                                  : Run(Step::Status, { "login", "status" });
        if (!started)
        {
            mStatus.state = State::Failed;
            mStatus.detail = "Cannot run " + mStatus.exe;
        }
    }

    bool Check::Run(Step step, const Vector<String>& args)
    {
        std::error_code ec;
        String cwd = String(std::filesystem::temp_directory_path(ec).string().c_str());
        auto process = std::make_unique<UiLayout::AgentProcess>();
        String error;
        Vector<String> env = { LocalAgentProvider::ToolPathEnv(mStatus.exe), "OPENAI_API_KEY", "CODEX_API_KEY", "ANTHROPIC_API_KEY" };
        if (!process->Start(mStatus.exe, args, ec ? String("/tmp") : cwd, error, env))
            return false;

        process->CloseInput();
        mProcess = std::move(process);
        mStep = step;
        mOutput = "";
        mStarted = std::chrono::steady_clock::now();
        return true;
    }

    bool Check::Poll()
    {
        if (!mProcess)
            return true;

        mProcess->Pump();
        String line;
        while (mProcess->PopLine(line))
            mOutput += line + "\n";

        int code = -1;
        if (mProcess->Poll(code) && mProcess->IsOutputClosed())
        {
            mProcess->Pump();
            while (mProcess->PopLine(line))
                mOutput += line + "\n";
            Finish(code);
        }
        else if (std::chrono::steady_clock::now() - mStarted > std::chrono::seconds(checkSeconds))
        {
            mProcess->Kill();
            mProcess.reset();
            if (mStep == Step::Status)
            {
                mStatus.state = State::Failed;
                mStatus.detail = NameOf(mAgent) + " did not answer its status check";
            }
            mStep = Step::Idle;
        }

        return !mProcess;
    }

    void Check::Finish(int code)
    {
        String output = mOutput + mProcess->StderrTail();
        Step step = mStep;
        mProcess.reset();
        mStep = Step::Idle;

        if (step == Step::Models)
        {
            auto models = CodexModelsOf(mOutput);
            if (!models.IsEmpty())
                codexCatalog = models;
            codexCatalogRead = true;
            return;
        }

        mStatus.state = State::Checking;
        String exe = mStatus.exe;
        mStatus = mAgent == Agent::ClaudeCode ? ClaudeStatusOf(code, output) : CodexStatusOf(code, output);
        mStatus.exe = exe;

        if (mAgent == Agent::Codex && !codexCatalogRead)
            Run(Step::Models, { "debug", "models", "--bundled" });
    }

    bool Check::IsRunning() const
    {
        return mProcess != nullptr;
    }

    Vector<ModelInfo> Check::GetCodexModels()
    {
        return codexCatalog.IsEmpty() ? FallbackCodexModels() : codexCatalog;
    }
}
// --- META ---

ENUM_META(Editor::LocalAgentSetup::Agent, Editor__LocalAgentSetup__Agent)
{
    ENUM_ENTRY(ClaudeCode);
    ENUM_ENTRY(Codex);
}
END_ENUM_META;

ENUM_META(Editor::LocalAgentSetup::State, Editor__LocalAgentSetup__State)
{
    ENUM_ENTRY(Checking);
    ENUM_ENTRY(Failed);
    ENUM_ENTRY(NotInstalled);
    ENUM_ENTRY(Ready);
    ENUM_ENTRY(SignedOut);
    ENUM_ENTRY(Unknown);
}
END_ENUM_META;
// --- END META ---
