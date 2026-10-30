#include "o2Editor/stdafx.h"
#include "UiLayoutAgent.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <set>
#include <thread>

#include "o2/EngineSettings.h"
#include "o2/Utils/Coroutines/Awaiters.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#if defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)
#define O2_AGENT_PROCESS 1
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#else
#define O2_AGENT_PROCESS 0
#endif

namespace Editor::UiLayout
{
    static const char* dot = " \xC2\xB7 ";

    static String Joined(const String& a, const String& b)
    {
        if (a.IsEmpty() || HasSuffix(a, "/"))
            return a + b;
        return a + "/" + b;
    }

    String AbsolutePath(const String& path)
    {
#if O2_AGENT_PROCESS
        char resolved[PATH_MAX];
        if (realpath(path.Data(), resolved))
            return resolved;

        if (path.StartsWith("/"))
            return path;

        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)))
            return Joined(cwd, path);
#endif
        return path;
    }

    String FindRepoPath(const String& relative)
    {
        auto exists = [](const String& path) { return o2FileSystem.IsFileExist(path) || o2FileSystem.IsFolderExist(path); };

        String fromRoot = String(GetProjectRootPath()) + relative;
        if (exists(fromRoot))
            return AbsolutePath(fromRoot);

        String up;
        for (int depth = 0; depth < 8; depth++)
        {
            if (exists(up + relative))
                return AbsolutePath(up + relative);
            up += "../";
        }
        return "";
    }

    String ImageToUiHome()
    {
        if (const char* env = getenv("O2_IMAGE_TO_UI_DIR"))
        {
            String dir = env;
            if (o2FileSystem.IsFileExist(Joined(dir, "agent/agent.py")))
                return AbsolutePath(dir);
        }

        String dir = FindRepoPath("o2/Tools/ImageToUi/agent/agent.py");
        if (dir.IsEmpty())
            dir = FindRepoPath("Tools/ImageToUi/agent/agent.py");
        if (dir.IsEmpty())
            return "";

        return o2FileSystem.GetParentPath(o2FileSystem.GetParentPath(dir));
    }

    // --- Python ----------------------------------------------------------------------------------------------

    Vector<String> PythonCandidates(const String& configured)
    {
        Vector<String> out;
#if O2_AGENT_PROCESS
        std::set<std::string> seen;
        auto add = [&](const String& path)
        {
            if (path.IsEmpty() || access(path.Data(), X_OK) != 0)
                return;

            String real = AbsolutePath(path);
            if (seen.insert(real.Data()).second)
                out.Add(path);
        };

        auto onPath = [&](const String& name)
        {
            const char* env = getenv("PATH");
            for (auto& folder : String(env ? env : "").Split(":"))
            {
                if (!folder.IsEmpty())
                    add(Joined(folder, name));
            }
        };

        String python = configured.Trimed(" \t\r\n");
        if (!python.IsEmpty())
        {
            if (python.StartsWith("~/") && getenv("HOME"))
                python = String(getenv("HOME")) + python.SubStr(1);

            if (python.Contains("/"))
                add(python);
            else
                onPath(python);

            if (out.IsEmpty())
                out.Add(python);
            return out;
        }

        onPath("python3");
        for (auto path : { "/opt/homebrew/bin/python3", "/usr/local/bin/python3", "/usr/bin/python3" })
            add(path);
#endif
        return out;
    }

    namespace
    {
        Mutex                 pythonMutex;
        Map<String, String>   foundPythons; // Python with Pillow and numpy by PATH
    }

    Coroutine<String> FindPython(String configured, Ref<PipelineExecContext> exec)
    {
        Vector<String> candidates = PythonCandidates(configured);
        if (candidates.IsEmpty())
            co_return String();

        if (!configured.Trimed(" \t\r\n").IsEmpty())
            co_return candidates[0];

        const char* env = getenv("PATH");
        String key = env ? env : "";
        {
            ScopeLock<Mutex> lock(pythonMutex);
            String found;
            if (foundPythons.TryGetValue(key, found))
                co_return found;
        }

        for (auto& python : candidates)
        {
            if (exec && exec->IsCancelled())
                break;

            AgentProcess probe;
            String error;
            if (!probe.Start(python, { "-c", "import numpy, PIL" }, "", error))
                continue;

            int code = -1;
            auto start = std::chrono::steady_clock::now();
            while (!probe.Poll(code))
            {
                probe.Pump();
                if ((exec && exec->IsCancelled()) || std::chrono::steady_clock::now() - start > std::chrono::seconds(30))
                {
                    probe.Kill();
                    break;
                }
                co_await WaitNextFrame();
            }

            if (code == 0)
            {
                ScopeLock<Mutex> lock(pythonMutex);
                foundPythons[key] = python;
                co_return python;
            }
        }
        co_return candidates[0];
    }

    // --- the process -----------------------------------------------------------------------------------------

    AgentProcess::~AgentProcess()
    {
        Kill();
        CloseAll();
    }

    void AgentProcess::CloseAll()
    {
#if O2_AGENT_PROCESS
        for (int* fd : { &mIn, &mOut, &mErr })
        {
            if (*fd >= 0)
                close(*fd);
            *fd = -1;
        }
#endif
    }

    bool AgentProcess::Start(const String& exe, const Vector<String>& args, const String& cwd, String& error)
    {
#if O2_AGENT_PROCESS
        int in[2] = { -1, -1 }, out[2] = { -1, -1 }, err[2] = { -1, -1 };
        if (pipe(in) != 0 || pipe(out) != 0 || pipe(err) != 0)
        {
            error = String("pipe: ") + strerror(errno);
            for (int fd : { in[0], in[1], out[0], out[1], err[0], err[1] })
            {
                if (fd >= 0)
                    close(fd);
            }
            return false;
        }

        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, in[0], 0);
        posix_spawn_file_actions_adddup2(&actions, out[1], 1);
        posix_spawn_file_actions_adddup2(&actions, err[1], 2);
        for (int fd : { in[0], in[1], out[0], out[1], err[0], err[1] })
        {
            if (fd > 2)
                posix_spawn_file_actions_addclose(&actions, fd);
        }
        if (!cwd.IsEmpty())
            posix_spawn_file_actions_addchdir_np(&actions, cwd.Data());

        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        sigset_t defaults, mask;
        sigemptyset(&defaults);
        sigaddset(&defaults, SIGPIPE);
        sigemptyset(&mask);
        posix_spawnattr_setsigdefault(&attr, &defaults);
        posix_spawnattr_setsigmask(&attr, &mask);
        posix_spawnattr_setpgroup(&attr, 0);
        short flags = POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETPGROUP;
#if defined(POSIX_SPAWN_CLOEXEC_DEFAULT)
        flags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
#endif
        posix_spawnattr_setflags(&attr, flags);

        std::vector<std::string> argStrings = { exe.Data() };
        for (auto& a : args)
            argStrings.push_back(a.Data());
        std::vector<char*> argv;
        for (auto& a : argStrings)
            argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        static const char* overrides[] = { "PYTHONUNBUFFERED=1", "PYTHONIOENCODING=utf-8:replace", "PYTHONUTF8=1" };
        std::vector<std::string> envStrings;
        for (char** e = environ; e && *e; e++)
        {
            std::string entry = *e;
            bool replaced = false;
            for (auto o : overrides)
            {
                std::string name = std::string(o).substr(0, std::string(o).find('=') + 1);
                replaced = replaced || entry.compare(0, name.size(), name) == 0;
            }
            if (!replaced)
                envStrings.push_back(entry);
        }
        for (auto o : overrides)
            envStrings.push_back(o);
        std::vector<char*> envp;
        for (auto& e : envStrings)
            envp.push_back(const_cast<char*>(e.c_str()));
        envp.push_back(nullptr);

        pid_t pid = -1;
        int rc = posix_spawn(&pid, exe.Data(), &actions, &attr, argv.data(), envp.data());
        posix_spawn_file_actions_destroy(&actions);
        posix_spawnattr_destroy(&attr);
        close(in[0]);
        close(out[1]);
        close(err[1]);
        if (rc != 0)
        {
            close(in[1]);
            close(out[0]);
            close(err[0]);
            error = strerror(rc);
            return false;
        }

        mPid = pid;
        mIn = in[1];
        mOut = out[0];
        mErr = err[0];
        for (int fd : { mIn, mOut, mErr })
        {
            fcntl(fd, F_SETFD, FD_CLOEXEC);
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        }
#if defined(F_SETNOSIGPIPE)
        fcntl(mIn, F_SETNOSIGPIPE, 1);
#else
        signal(SIGPIPE, SIG_IGN);
#endif
        return true;
#else
        error = "starting processes is not supported on this platform";
        return false;
#endif
    }

    void AgentProcess::Pump()
    {
#if O2_AGENT_PROCESS
        char buffer[65536];
        while (mOut >= 0 && !mOutEof)
        {
            ssize_t n = read(mOut, buffer, sizeof(buffer));
            if (n > 0)
                mOutBuf.append(buffer, (size_t)n);
            else if (n == 0)
                mOutEof = true;
            else if (errno != EINTR)
                break;
        }

        while (mErr >= 0)
        {
            ssize_t n = read(mErr, buffer, sizeof(buffer));
            if (n > 0)
            {
                mErrTail.append(buffer, (size_t)n);
                if (mErrTail.size() > 8000)
                    mErrTail.erase(0, mErrTail.size() - 8000);
            }
            else if (n == 0)
            {
                close(mErr);
                mErr = -1;
            }
            else if (errno != EINTR)
                break;
        }

        while (mIn >= 0 && !mPending.empty())
        {
            ssize_t n = write(mIn, mPending.data(), mPending.size());
            if (n > 0)
                mPending.erase(0, (size_t)n);
            else if (n < 0 && errno == EINTR)
                continue;
            else if (n < 0 && errno == EAGAIN)
                break;
            else
            {
                // The agent exited first; its result line says why
                mPending.clear();
                break;
            }
        }
#endif
    }

    bool AgentProcess::PopLine(String& line)
    {
        size_t end = mOutBuf.find('\n');
        if (end == std::string::npos)
        {
            if (!mOutEof || mOutBuf.empty())
                return false;
            end = mOutBuf.size();
        }

        std::string text = mOutBuf.substr(0, end);
        mOutBuf.erase(0, Math::Min(mOutBuf.size(), end + 1));
        if (!text.empty() && text.back() == '\r')
            text.pop_back();
        line = String(text);
        return true;
    }

    void AgentProcess::Write(const String& data)
    {
        mPending.append(data.Data(), data.Length());
        Pump();
    }

    bool AgentProcess::Poll(int& exitCode)
    {
#if O2_AGENT_PROCESS
        if (!mExited && mPid > 0)
        {
            int status = 0;
            pid_t r = waitpid(mPid, &status, WNOHANG);
            if (r == mPid)
            {
                mExited = true;
                mExitCode = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
            }
            else if (r < 0 && errno == ECHILD)
                mExited = true;
        }
#endif
        exitCode = mExitCode;
        return mExited || mPid <= 0;
    }

    void AgentProcess::Kill()
    {
#if O2_AGENT_PROCESS
        if (mPid <= 0 || mExited)
            return;

        kill(-mPid, SIGTERM);
        kill(mPid, SIGTERM);
        int code = -1;
        for (int i = 0; i < 50 && !Poll(code); i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

        if (!mExited)
        {
            kill(-mPid, SIGKILL);
            kill(mPid, SIGKILL);
            int status = 0;
            waitpid(mPid, &status, 0);
            mExited = true;
            mExitCode = 128 + SIGKILL;
        }
#endif
    }

    // --- the agent -------------------------------------------------------------------------------------------

    Vector<String> AgentArguments(const String& home, const AgentRun& run)
    {
        // Free texts go as --key=value: a note or a hint that starts with a dash is still a value
        Vector<String> args = {
            Joined(home, "agent/agent.py"),
            "--design", run.design, "--assets", run.assets, "--output", run.task, "--scratch", run.scratch,
            "--skill", Joined(home, "upstream/image-to-ui"),
            "--max-turns", (String)run.maxTurns, "--name=" + run.name, "--hint=" + run.hint, "--sprites-note=" + run.note,
        };
        for (auto& font : run.fonts)
        {
            args.Add("--font");
            args.Add(font);
        }
        if (!run.traceDir.IsEmpty())
        {
            args.Add("--trace-dir");
            args.Add(run.traceDir);
        }
        if (!run.revise.IsEmpty())
        {
            args.Add("--revise");
            args.Add(run.revise);
        }
        return args;
    }

    Coroutine<ScriptResult> RunScript(String python, Vector<String> args, String cwd, Ref<PipelineExecContext> exec)
    {
        ScriptResult res;
        AgentProcess process;
        String error;
        if (!process.Start(python, args, cwd, error))
        {
            res.failure = "imageToUi: cannot start " + python + " (" + error + ")";
            co_return res;
        }

        std::string out;
        auto keep = [&](const std::string& more)
        {
            out += more;
            if (out.size() > 6000)
                out = out.substr(out.size() - 6000);
        };
        int code = -1;
        auto exitedAt = std::chrono::steady_clock::time_point();
        for (;;)
        {
            if (exec && exec->IsCancelled())
            {
                process.Kill();
                res.failure = "Cancelled";
                co_return res;
            }

            process.Pump();
            String line;
            while (process.PopLine(line))
                keep(std::string(line.Data()) + "\n");
            bool exited = process.Poll(code);
            if (exited && process.IsOutputClosed())
                break;

            if (exited)
            {
                if (exitedAt == std::chrono::steady_clock::time_point())
                    exitedAt = std::chrono::steady_clock::now();
                else if (std::chrono::steady_clock::now() - exitedAt > std::chrono::seconds(3))
                    break;
            }

            co_await WaitNextFrame();
        }

        process.Pump();
        String line;
        while (process.PopLine(line))
            keep(std::string(line.Data()) + "\n");
        keep(process.StderrTail().Data());
        res.code = code;
        res.output = String(out);
        co_return res;
    }

    static String StderrLines(const String& tail, int count)
    {
        Vector<String> lines;
        for (auto& line : tail.Split("\n"))
        {
            String trimmed = line.Trimed(" \t\r");
            if (!trimmed.IsEmpty())
                lines.Add(trimmed);
        }

        String out;
        for (int i = Math::Max(0, lines.Count() - count); i < lines.Count(); i++)
            out += (out.IsEmpty() ? "" : " | ") + lines[i];
        return out;
    }

    Coroutine<AgentResult> RunAgent(Ref<AiCtx> ai, AgentRun run)
    {
        AgentResult res;
        auto cancelled = [&]() { return ai->exec && ai->exec->IsCancelled(); };

#if !O2_AGENT_PROCESS
        res.failure = "imageToUi: the Image-To-UI agent (Python) cannot run on this platform";
        co_return res;
#else
        String home = ImageToUiHome();
        if (home.IsEmpty())
        {
            res.failure = "imageToUi: the Image-To-UI Python is missing: o2/Tools/ImageToUi (tools/sync-image-to-ui.sh copies it)";
            co_return res;
        }

        String python = co_await FindPython(run.python, ai->exec);
        if (python.IsEmpty())
        {
            res.failure = "imageToUi: no Python 3 found - install Python 3 with Pillow and numpy, or set it in the pipeline settings";
            co_return res;
        }

        Vector<String> args = AgentArguments(home, run);
        args.Insert("-B", 0);

        AgentProcess agent;
        String error;
        if (!agent.Start(python, args, run.workDir, error))
        {
            res.failure = "imageToUi: cannot start " + python + " (" + error + ") - the node needs Python 3 with Pillow and numpy";
            co_return res;
        }
        ai->Log("imageToUi" + String(dot) + "agent: " + python + ", at most " + (String)run.maxTurns + " turns");

        Json result;
        bool hasResult = false;
        int code = -1;
        auto exitedAt = std::chrono::steady_clock::time_point();
        for (;;)
        {
            if (cancelled())
            {
                agent.Kill();
                res.failure = "Cancelled";
                co_return res;
            }

            agent.Pump();
            String line;
            while (agent.PopLine(line))
            {
                if (!line.StartsWith("@@bridge "))
                    continue;

                Json msg;
                if (!Json::Parse(line.SubStr(9), msg) || !msg.IsObject())
                    continue;

                String type = msg.Get("type") ? msg.Get("type")->Str() : String();
                if (type == "event")
                {
                    ai->Log("imageToUi" + String(dot) + "agent: " + (msg.Get("message") ? msg.Get("message")->ToJsString() : String()));
                    continue;
                }

                if (type == "result")
                {
                    result = msg;
                    hasResult = true;
                    continue;
                }

                if (type != "model")
                    continue;

                Json reply = Json::Object();
                reply["id"] = msg.Get("id") ? *msg.Get("id") : Json(0);
                Vector<String> images;
                String missing;
                if (const Json* list = msg.Get("images"))
                {
                    for (auto& path : list->Items())
                    {
                        String bytes = PipelineUtils::ReadFileBytes(path.Str());
                        if (bytes.IsEmpty())
                            missing = path.Str();
                        images.Add(bytes);
                    }
                }

                if (!missing.IsEmpty())
                    reply["error"] = "the host cannot read " + missing;
                else
                {
                    String tag = msg.Get("tag") ? msg.Get("tag")->ToJsString() : String("turn");
                    String prompt = msg.Get("prompt") ? msg.Get("prompt")->Str() : String();
                    AiAnswer answer = co_await AiText(ai, prompt, images, "agent " + tag);
                    if (answer.ok)
                        reply["text"] = answer.data;
                    else
                        reply["error"] = answer.error.IsEmpty() ? String("the model call failed") : answer.error;
                }

                if (cancelled())
                {
                    agent.Kill();
                    res.failure = "Cancelled";
                    co_return res;
                }
                agent.Write(reply.Dump() + "\n");
            }

            bool exited = agent.Poll(code);
            if (exited && agent.IsOutputClosed())
                break;

            // A script the agent left behind may hold its stdout: the exit is enough after a while
            if (exited)
            {
                if (exitedAt == std::chrono::steady_clock::time_point())
                    exitedAt = std::chrono::steady_clock::now();
                else if (std::chrono::steady_clock::now() - exitedAt > std::chrono::seconds(3))
                    break;
            }

            co_await WaitNextFrame();
        }

        agent.Pump();
        res.exitCode = code;
        if (hasResult)
        {
            res.ok = result.Get("ok") && result.Get("ok")->Bool();
            res.error = result.Get("error") ? result.Get("error")->ToJsString() : String();
            res.detail = result.Get("detail") ? result.Get("detail")->ToJsString() : String();
            if (const Json* summary = result.Get("summary"))
                res.summary = *summary;
        }

        if (code == 3)
            res.failure = "imageToUi: " + (res.detail.IsEmpty() ? String("Python lacks Pillow / numpy") : res.detail);
        else if (!hasResult)
            res.failure = "imageToUi: the agent stopped (exit " + (String)code + ") " + StderrLines(agent.StderrTail(), 5);
        co_return res;
#endif
    }
}
