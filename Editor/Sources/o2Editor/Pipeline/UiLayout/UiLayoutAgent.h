#pragma once

#include <string>

#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"

namespace Editor::UiLayout
{
    // Returns the absolute path of a file or folder of the repository: from the project root first, then searched upwards
    // from the working folder (the tests run from build/o2/Tests/...); empty when it is nowhere
    String FindRepoPath(const String& relative);

    // Returns the path made absolute against the working folder, symbolic links resolved when it exists
    String AbsolutePath(const String& path);

    // Returns the folder of the image-to-UI Python (upstream/ - XuToWei/Image-To-UI's skill, never edited - and agent/):
    // O2_IMAGE_TO_UI_DIR, else o2/Tools/ImageToUi (tools/sync-image-to-ui.sh copies it from AssetsLine); empty when missing
    String ImageToUiHome();

    // Returns the Pythons the agent may run with, in order: the configured one (a path, or a name looked up on PATH), else
    // python3 on PATH, /opt/homebrew/bin, /usr/local/bin and /usr/bin, the ones that exist, without repeats
    Vector<String> PythonCandidates(const String& configured);

    // Returns the Python the agent runs with: the configured one, else the first candidate that imports Pillow and numpy, else
    // the first candidate (the agent then says what is missing); empty when there is none
    Coroutine<String> FindPython(String configured, Ref<PipelineExecContext> exec);

    // ------------------------------------------------------------------------------------------------
    // A child process with its standard streams piped, read and written without blocking; it runs in a
    // process group of its own, so Kill stops the scripts it started too. POSIX only: elsewhere Start fails
    // ------------------------------------------------------------------------------------------------
    class AgentProcess
    {
    public:
        AgentProcess() = default;
        AgentProcess(const AgentProcess&) = delete;
        AgentProcess& operator=(const AgentProcess&) = delete;

        // Kills the process when it still runs
        ~AgentProcess();

        // Starts exe with the arguments in the folder; false with the reason when it cannot
        bool Start(const String& exe, const Vector<String>& args, const String& cwd, String& error);

        // Reads what stdout and stderr hold and writes what is queued for stdin
        void Pump();

        // Takes the next complete stdout line, without its line break
        bool PopLine(String& line);

        // Queues bytes for stdin; Pump writes them
        void Write(const String& data);

        // Returns true once stdout reached its end
        bool IsOutputClosed() const { return mOutEof; }

        // Returns true and the exit code once the process has exited (128 + signal when a signal ended it)
        bool Poll(int& exitCode);

        // Ends the process group: SIGTERM, then SIGKILL after a second
        void Kill();

        // Returns the last 8000 bytes of stderr
        String StderrTail() const { return String(mErrTail); }

    private:
        int  mPid = -1;
        int  mIn = -1, mOut = -1, mErr = -1;
        bool mOutEof = false;
        bool mExited = false;
        int  mExitCode = -1;

        std::string mOutBuf;
        std::string mErrTail;
        std::string mPending;

    private:
        void CloseAll();
    };

    // What the agent is given
    struct AgentRun
    {
        String         python;   // Configured Python, empty to search
        String         workDir;  // <work>, the agent's working folder
        String         design;   // <work>/input/design.png
        String         assets;   // <work>/sprites
        String         task;     // <work>/task
        String         scratch;  // <work>/agent-view
        String         traceDir; // <work>/trace
        int            maxTurns = 60;
        String         name;     // Canvas name
        String         hint;     // Notes from the user
        String         note;     // About the sliced assets (SpritesNote)
        Vector<String> fonts;    // fontFamily values the model may use
    };

    // How the agent ended
    struct AgentResult
    {
        String failure;  // Why the node fails without a result: not started, missing dependencies, stopped, cancelled
        bool   ok = false; // The result line says a checked structure exists
        String error;    // The agent's own error of the result line
        String detail;   // Its detail
        Json   summary;  // {complete, turns, check_count, structure_revision, target_count, status, blockers}
        int    exitCode = -1;
    };

    // The arguments of agent.py for a run, the Python and "-B" left out
    Vector<String> AgentArguments(const String& home, const AgentRun& run);

    // ------------------------------------------------------------------------------------------------
    // Runs image-to-ui/agent/agent.py and answers its model calls (PORTING.md): every stdout line
    // "@@bridge {json}" is a model request (answered on stdin with {"id", "text"} or {"id", "error"}
    // through the node's memoised AiText, tag "agent <tag>"), an event (logged) or the result (the last
    // line). Polls the pipes once a frame; a cancelled run kills the agent
    // ------------------------------------------------------------------------------------------------
    Coroutine<AgentResult> RunAgent(Ref<AiCtx> ai, AgentRun run);
}
