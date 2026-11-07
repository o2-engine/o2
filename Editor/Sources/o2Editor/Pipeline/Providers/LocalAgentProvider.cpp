#include "o2Editor/stdafx.h"
#include "LocalAgentProvider.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>

#include "o2/Utils/Coroutines/Awaiters.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/AiHttp.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"

#if defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)
#include <unistd.h>
#define O2_LOCAL_AGENT_POSIX 1
#else
#define O2_LOCAL_AGENT_POSIX 0
#endif

namespace Editor::LocalAgentProvider
{
    static const char* dot = " \xC2\xB7 ";

    static String Bare(const String& model)
    {
        return model.Trimed(" \n\r\t");
    }

    bool IsLocalModel(const String& model)
    {
        return Bare(model).StartsWith(prefix);
    }

    bool IsClaudeCode(const String& model)
    {
        String id = Bare(model);
        return id == claudeCodeModel || id.StartsWith(claudeCodeModel + ":");
    }

    bool IsCodex(const String& model)
    {
        String id = Bare(model);
        return id == codexModel || id.StartsWith(codexModel + ":");
    }

    bool IsCodexImage(const String& model)
    {
        return Bare(model) == codexImageModel;
    }

    String CodexModelOf(const String& model, const PipelineSettings& settings)
    {
        String id = Bare(model);
        if (id.StartsWith(codexModel + ":"))
            return id.SubStr(codexModel.Length() + 1).Trimed(" ");

        return settings.localCodexModel.Trimed(" \n\r\t");
    }

    String ClaudeModelOf(const String& model, const PipelineSettings& settings)
    {
        String id = Bare(model);
        if (id.StartsWith(claudeCodeModel + ":"))
            return id.SubStr(claudeCodeModel.Length() + 1).Trimed(" ");

        return settings.localAgentModel.Trimed(" \n\r\t");
    }

    Vector<String> OfferedModels(const PipelineSettings& settings)
    {
        Vector<String> res;
        if (!settings.localAgentEnabled)
            return res;

        if (!FindClaude(settings.localAgentPath).IsEmpty())
        {
            res.Add(claudeCodeModel);
            for (auto& model : claudeModels)
                res.Add(claudeCodeModel + ":" + model);
        }
        if (!FindCodex(settings.localCodexPath).IsEmpty())
            res.Add(codexModel);
        if (!settings.localCommand.Trimed(" \n\r\t").IsEmpty())
            res.Add(commandModel);
        return res;
    }

    Vector<String> OfferedImageModels(const PipelineSettings& settings)
    {
        Vector<String> res;
        if (settings.localAgentEnabled && !FindCodex(settings.localCodexPath).IsEmpty())
            res.Add(codexImageModel);
        return res;
    }

    String PrettyName(const String& model)
    {
        String id = Bare(model);
        if (id == commandModel)
            return "Local command";

        if (id == claudeCodeModel)
            return String("Claude Code") + dot + "local";

        if (id == codexModel)
            return String("Codex") + dot + "local";

        if (id == codexImageModel)
            return String("Codex image") + dot + "local";

        if (IsCodex(id))
            return "Codex (" + id.SubStr(codexModel.Length() + 1) + ")" + dot + "local";

        if (IsClaudeCode(id))
        {
            String name = id.SubStr(claudeCodeModel.Length() + 1);
            bool alias = claudeModels.Contains(name);
            if (alias && !name.IsEmpty())
                name = name.SubStr(0, 1).ToUpperCase() + name.SubStr(1);
            return (alias ? "Claude Code " + name : "Claude Code (" + name + ")") + dot + "local";
        }

        return id.SubStr(prefix.Length()) + dot + "local";
    }

    static String Home()
    {
        const char* home = std::getenv("HOME");
        return home ? String(home) : String();
    }

    static bool IsExecutable(const String& path)
    {
#if O2_LOCAL_AGENT_POSIX
        std::error_code ec;
        return !path.IsEmpty() && std::filesystem::is_regular_file(path.Data(), ec) && access(path.Data(), X_OK) == 0;
#else
        return false;
#endif
    }

    static String OnPath(const String& name)
    {
        const char* path = std::getenv("PATH");
        if (!path)
            return "";

        for (auto& dir : String(path).Split(":"))
        {
            if (dir.IsEmpty())
                continue;

            String candidate = dir.EndsWith("/") ? dir + name : dir + "/" + name;
            if (IsExecutable(candidate))
                return candidate;
        }
        return "";
    }

    // The configured executable (a path, ~/..., or a name on PATH), then name on PATH, then the folders given (~/ is home);
    // only existing ones. An editor started from the Dock does not see the PATH of a shell, hence the folders
    static Vector<String> Candidates(const String& configuredIn, const String& name, const Vector<String>& folders)
    {
        Vector<String> res;
        auto add = [&](const String& pathIn)
        {
            if (pathIn.IsEmpty() || !IsExecutable(pathIn))
                return;

            // The agent runs in a folder of its own, so a relative path is made whole
            std::error_code ec;
            auto whole = std::filesystem::absolute(pathIn.Data(), ec).lexically_normal();
            String path = ec ? pathIn : String(whole.string().c_str());
            if (!res.Contains(path))
                res.Add(path);
        };

        String home = Home();
        String configured = configuredIn.Trimed(" \n\r\t");
        if (configured.StartsWith("~/"))
            configured = home + configured.SubStr(1);
        if (!configured.IsEmpty())
            add(configured.Contains("/") ? configured : OnPath(configured));

        add(OnPath(name));
        for (auto& folder : folders)
        {
            if (!folder.StartsWith("~/"))
                add(folder + "/" + name);
            else if (!home.IsEmpty())
                add(home + folder.SubStr(1) + "/" + name);
        }
        return res;
    }

    Vector<String> ClaudeCandidates(const String& configured)
    {
        return Candidates(configured, "claude", { "~/.local/bin", "~/.claude/local", "/opt/homebrew/bin", "/usr/local/bin" });
    }

    Vector<String> CodexCandidates(const String& configured)
    {
        return Candidates(configured, "codex", { "/opt/homebrew/bin", "/usr/local/bin", "~/.local/bin", "~/.npm-global/bin" });
    }

    String FindCodex(const String& configured)
    {
        auto candidates = CodexCandidates(configured);
        return candidates.IsEmpty() ? String() : candidates[0];
    }

    String ToolPathEnv(const String& exe)
    {
        const char* path = std::getenv("PATH");
        String dir = exe.Contains("/") ? exe.SubStr(0, exe.FindLast("/")) : String();
        String home = Home();
        return "PATH=" + (dir.IsEmpty() ? String() : dir + ":") + (home.IsEmpty() ? String() : home + "/.local/bin:") +
            "/opt/homebrew/bin:/usr/local/bin" + (path && *path ? ":" + String(path) : String(":/usr/bin:/bin"));
    }

    String CodexHome()
    {
        const char* home = std::getenv("CODEX_HOME");
        if (home && *home)
            return home;

        return Home().IsEmpty() ? String() : Home() + "/.codex";
    }

    String FindClaude(const String& configured)
    {
        auto candidates = ClaudeCandidates(configured);
        return candidates.IsEmpty() ? String() : candidates[0];
    }

    static String JsonString(const String& text)
    {
        static const char* hex = "0123456789abcdef";
        String res = "\"";
        for (int i = 0; i < text.Length(); i++)
        {
            unsigned char c = (unsigned char)text[i];
            switch (c)
            {
                case '"': res += "\\\""; break;
                case '\\': res += "\\\\"; break;
                case '\n': res += "\\n"; break;
                case '\r': res += "\\r"; break;
                case '\t': res += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        res += "\\u00";
                        res += hex[c >> 4];
                        res += hex[c & 15];
                    }
                    else
                        res += (char)c;
            }
        }
        return res + "\"";
    }

    Invocation ClaudeInvocation(const PipelineSettings& settings, const String& exe, const String& model, const String& prompt,
                                const Vector<AiImageRef>& images)
    {
        Invocation res;
        res.exe = exe;
        res.args = { "-p", "--input-format", "stream-json", "--output-format", "stream-json", "--verbose",
                     "--tools", "", "--system-prompt", systemPrompt, "--strict-mcp-config", "--no-session-persistence",
                     "--disable-slash-commands" };

        // With an API key --bare reads only that key and skips hooks, plugins, memory and CLAUDE.md
        String apiKey = settings.GetLocalAgentApiKey();
        if (!apiKey.IsEmpty())
        {
            res.args.Add("--bare");
            res.env.Add("ANTHROPIC_API_KEY=" + apiKey);
        }
        else
            res.env.Add("ANTHROPIC_API_KEY");

        String claudeModel = ClaudeModelOf(model, settings);
        if (!claudeModel.IsEmpty())
        {
            res.args.Add("--model");
            res.args.Add(claudeModel);
        }

        if (!settings.localAgentThinking)
            res.env.Add("MAX_THINKING_TOKENS=0");
        res.env.Add("CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=1");

        String content;
        for (auto& image : images)
        {
            String mime = image.mimeType.IsEmpty() ? String("image/png") : image.mimeType;
            content += "{\"type\":\"image\",\"source\":{\"type\":\"base64\",\"media_type\":" + JsonString(mime) +
                ",\"data\":\"" + PipelineUtils::Base64Encode(image.data) + "\"}},";
        }
        content += "{\"type\":\"text\",\"text\":" + JsonString(prompt) + "}";
        res.input = "{\"type\":\"user\",\"message\":{\"role\":\"user\",\"content\":[" + content + "]}}\n";
        return res;
    }

    Invocation CodexInvocation(const PipelineSettings& settings, const String& exe, const String& model, const String& prompt,
                               const Vector<String>& imageFiles, const String& instructionsFile, const String& folder, bool draws)
    {
        Invocation res;
        res.exe = exe;
        res.args = { "exec", "--json", "--ephemeral", "--skip-git-repo-check", "--ignore-user-config", "--ignore-rules",
                     "--sandbox", "read-only", "--color", "never", "-C", folder,
                     "--disable", "apps", "--disable", "multi_agent", "--disable", "goals", "--disable", "shell_tool",
                     "--disable", "plugins", "--disable", "view_image", "--enable", "omit_skill_descriptions",
                     "-c", "suppress_unstable_features_warning=true",
                     "-c", "model_instructions_file=" + JsonString(instructionsFile),
                     "-c", "web_search=\"disabled\"", "-c", "model_reasoning_summary=\"none\"" };
        if (!draws)
        {
            res.args.Add("--disable");
            res.args.Add("image_generation");
        }

        String effort = settings.localCodexEffort.Trimed(" \n\r\t").ToLowerCase();
        if (codexEfforts.Contains(effort))
        {
            res.args.Add("-c");
            res.args.Add("model_reasoning_effort=\"" + effort + "\"");
        }

        String codexModelId = CodexModelOf(model, settings);
        if (!codexModelId.IsEmpty())
        {
            res.args.Add("-m");
            res.args.Add(codexModelId);
        }

        // One value each: -i takes every argument after it
        for (auto& file : imageFiles)
            res.args.Add("--image=" + file);

        res.env.Add("OPENAI_API_KEY");
        String apiKey = settings.localCodexApiKey.Trimed(" \n\r\t");
        res.env.Add(apiKey.IsEmpty() ? String("CODEX_API_KEY") : "CODEX_API_KEY=" + apiKey);

        res.env.Add(ToolPathEnv(exe));

        res.input = prompt;
        return res;
    }

    Invocation CommandInvocation(const PipelineSettings& settings, const String& prompt, const Vector<String>& imageFiles)
    {
        Invocation res;
        res.exe = "/bin/sh";
        res.args = { "-c", settings.localCommand.Trimed(" \n\r\t") };
        res.input = prompt;
        if (!imageFiles.IsEmpty())
        {
            res.input += "\n\nThe images of this request are these files:\n";
            for (auto& file : imageFiles)
                res.input += file + "\n";
        }
        return res;
    }

    static String LastLines(const String& text, int count)
    {
        Vector<String> lines;
        for (auto& line : text.Split("\n"))
        {
            String trimmed = line.Trimed(" \t\r");
            if (!trimmed.IsEmpty())
                lines.Add(trimmed);
        }

        String res;
        for (int i = Math::Max(0, lines.Count() - count); i < lines.Count(); i++)
            res += (res.IsEmpty() ? "" : " | ") + lines[i];
        return res;
    }

    ClaudeAnswer ParseClaudeOutput(const String& output, int exitCode, const String& stderrTail)
    {
        ClaudeAnswer res;
        bool hasResult = false, isError = false;
        String resultText, subtype, lastAssistant;
        for (auto& lineIn : output.Split("\n"))
        {
            String line = lineIn.Trimed(" \t\r");
            if (!line.StartsWith("{"))
                continue;

            DataDocument doc;
            if (!doc.LoadFromData(line) || !doc.IsObject())
                continue;

            String type = AiHttp::StringOf(AiHttp::Member(&doc, "type"));
            if (type == "assistant")
            {
                String text;
                auto content = AiHttp::Member(AiHttp::Member(&doc, "message"), "content");
                if (content && content->IsArray())
                {
                    for (auto& part : *content)
                    {
                        if (AiHttp::StringOf(AiHttp::Member(&part, "type")) == "text")
                            text += AiHttp::StringOf(AiHttp::Member(&part, "text"));
                    }
                }
                if (!text.IsEmpty())
                    lastAssistant = text;
            }
            else if (type == "result")
            {
                hasResult = true;
                subtype = AiHttp::StringOf(AiHttp::Member(&doc, "subtype"));
                resultText = AiHttp::StringOf(AiHttp::Member(&doc, "result"));
                auto error = AiHttp::Member(&doc, "is_error");
                isError = (error && error->IsBoolean() && (bool)*error) || (!subtype.IsEmpty() && subtype != "success");
                res.costUsd = AiHttp::NumberOf(AiHttp::Member(&doc, "total_cost_usd"));
                auto usage = AiHttp::Member(&doc, "usage");
                res.inputTokens = (int)(AiHttp::NumberOf(AiHttp::Member(usage, "input_tokens")) +
                                        AiHttp::NumberOf(AiHttp::Member(usage, "cache_read_input_tokens")) +
                                        AiHttp::NumberOf(AiHttp::Member(usage, "cache_creation_input_tokens")));
                res.outputTokens = (int)AiHttp::NumberOf(AiHttp::Member(usage, "output_tokens"));
            }
        }

        if (hasResult && !isError)
        {
            String text = (resultText.IsEmpty() ? lastAssistant : resultText).Trimed(" \n\r\t");
            if (!text.IsEmpty())
            {
                res.result.ok = true;
                res.result.text = text;
                return res;
            }
            res.result.error = "Claude Code answered with an empty text";
            return res;
        }

        if (hasResult)
        {
            String why = resultText.Trimed(" \n\r\t");
            res.result.error = "Claude Code: " + (why.IsEmpty() ? subtype : why);
            if (why.Contains("login") || why.Contains("API key") || why.Contains("authenticat"))
                res.result.error += " (sign in: Pipeline settings, Local agents, Claude Code, Sign in)";
            return res;
        }

        String tail = LastLines(stderrTail, 3);
        res.result.error = "Claude Code ended without an answer (exit code " + (String)exitCode + ")" +
            (tail.IsEmpty() ? String() : ": " + tail);
        return res;
    }

    CodexAnswer ParseCodexOutput(const String& output, int exitCode, const String& stderrTail)
    {
        CodexAnswer res;
        bool completed = false, failed = false;
        String lastText, failure, lastError;
        for (auto& lineIn : output.Split("\n"))
        {
            String line = lineIn.Trimed(" \t\r");
            if (!line.StartsWith("{"))
                continue;

            DataDocument doc;
            if (!doc.LoadFromData(line) || !doc.IsObject())
                continue;

            String type = AiHttp::StringOf(AiHttp::Member(&doc, "type"));
            if (type == "thread.started")
                res.threadId = AiHttp::StringOf(AiHttp::Member(&doc, "thread_id"));
            else if (type == "item.completed")
            {
                auto item = AiHttp::Member(&doc, "item");
                if (AiHttp::StringOf(AiHttp::Member(item, "type")) == "agent_message")
                    lastText = AiHttp::StringOf(AiHttp::Member(item, "text"));
            }
            else if (type == "turn.completed")
            {
                completed = true;
                auto usage = AiHttp::Member(&doc, "usage");
                res.inputTokens += (int)AiHttp::NumberOf(AiHttp::Member(usage, "input_tokens"));
                res.cachedTokens += (int)AiHttp::NumberOf(AiHttp::Member(usage, "cached_input_tokens"));
                res.outputTokens += (int)AiHttp::NumberOf(AiHttp::Member(usage, "output_tokens"));
            }
            else if (type == "turn.failed")
            {
                failed = true;
                failure = AiHttp::StringOf(AiHttp::Member(AiHttp::Member(&doc, "error"), "message"));
            }
            else if (type == "error")
                lastError = AiHttp::StringOf(AiHttp::Member(&doc, "message"));
        }

        if (completed && !failed)
        {
            String text = lastText.Trimed(" \n\r\t");
            if (!text.IsEmpty())
            {
                res.result.ok = true;
                res.result.text = text;
                return res;
            }
            res.result.error = "Codex answered with an empty text";
            return res;
        }

        String why = failed ? failure : lastError;
        if (!why.IsEmpty())
        {
            res.result.error = "Codex: " + why.Trimed(" \n\r\t");
            if (why.Contains("401") || why.Contains("Unauthorized") || why.Contains("login"))
                res.result.error += " (sign in: Pipeline settings, Local agents, Codex, Sign in)";
            return res;
        }

        String tail = LastLines(stderrTail, 3);
        res.result.error = "Codex ended without an answer (exit code " + (String)exitCode + ")" + (tail.IsEmpty() ? String() : ": " + tail);
        return res;
    }

    String CodexImagePrompt(const String& prompt, int references, bool transparentBackground, const AiImageOptions& options)
    {
        String res = prompt.Trimed(" \n\r\t");
        if (references > 0)
            res += references == 1 ? "\n\nThe attached image is the reference of this request."
                                   : "\n\nThe attached images are the references of this request, in their order.";
        if (!options.aspectRatio.IsEmpty())
            res += "\nFrame: aspect ratio " + options.aspectRatio + ".";
        if (!options.renderSize.IsEmpty())
            res += "\nResolution: about " + options.renderSize + " on the long side.";
        if (transparentBackground)
            res += "\nBackground: fully transparent, keep the alpha channel.";
        return res;
    }

    static bool IsPictureFile(const std::filesystem::path& path)
    {
        String ext = String(path.extension().string().c_str()).ToLowerCase();
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp";
    }

    String FindCodexImage(const String& codexHomeIn, const String& threadId)
    {
        String codexHome = codexHomeIn.EndsWith("/") ? codexHomeIn.SubStr(0, codexHomeIn.Length() - 1) : codexHomeIn;
        if (codexHome.IsEmpty() || threadId.IsEmpty() || threadId.Contains("/") || threadId.Contains(".."))
            return "";

        std::error_code ec;
        std::filesystem::path dir = std::filesystem::path((codexHome + "/generated_images/" + threadId).Data());
        if (!std::filesystem::is_directory(dir, ec))
            return "";

        String best;
        std::filesystem::file_time_type bestTime;
        for (auto& entry : std::filesystem::directory_iterator(dir, ec))
        {
            if (!entry.is_regular_file(ec) || !IsPictureFile(entry.path()))
                continue;

            auto time = entry.last_write_time(ec);
            if (best.IsEmpty() || time > bestTime)
            {
                best = entry.path().string().c_str();
                bestTime = time;
            }
        }
        return best;
    }

    // A folder of its own for one call, outside every project: Claude Code reads CLAUDE.md files of the folders above it
    static String MakeCallFolder()
    {
        static std::atomic<int> counter { 0 };
        std::error_code ec;
        auto base = std::filesystem::temp_directory_path(ec);
        if (ec)
            base = "/tmp";
        String path = String(base.string().c_str());
        if (!path.EndsWith("/"))
            path += "/";
        path += "o2-local-agent-" + (String)(int)std::chrono::steady_clock::now().time_since_epoch().count() + "-" +
            (String)counter.fetch_add(1);
        std::filesystem::create_directories(path.Data(), ec);
        return path;
    }

    static String ExtensionOfMime(const String& mime)
    {
        if (mime.Contains("jpeg") || mime.Contains("jpg"))
            return "jpg";
        if (mime.Contains("webp"))
            return "webp";
        if (mime.Contains("gif"))
            return "gif";
        return "png";
    }

    static const char* codexTextInstructions =
        "You answer requests of a game asset pipeline. Do exactly what the request asks and reply with the answer only: no "
        "preamble, no questions back.";
    static const char* codexImageInstructions =
        "You make images for a game asset pipeline. Call the image_gen tool exactly once with the request as its prompt "
        "(attached images are references to follow or to edit, as the request says), then reply with the single word done. "
        "Never ask questions and never call any other tool.";

    // How a call of a local agent ended
    struct CallOutcome
    {
        String error;      // Why there is no output to read: not started, cancelled, out of time
        String output;     // What stdout printed
        int    code = -1;  // Exit code
        String stderrTail; // The last lines of stderr
    };

    // Runs the call in its folder until it exits, the run is cancelled or the time is out
    static Coroutine<CallOutcome> RunCall(const Ref<PipelineExecContext>& ctx, const Invocation& call, const String& folder,
                                          int seconds)
    {
        CallOutcome res;
        UiLayout::AgentProcess process;
        String error;
        if (!process.Start(call.exe, call.args, folder, error, call.env))
        {
            res.error = "Cannot start " + call.exe + ": " + error;
            co_return res;
        }

        process.Write(call.input);
        process.CloseInput();

        auto start = std::chrono::steady_clock::now();
        auto exitedAt = std::chrono::steady_clock::time_point();
        for (;;)
        {
            if (ctx->IsCancelled() || std::chrono::steady_clock::now() - start > std::chrono::seconds(seconds))
            {
                bool cancelled = ctx->IsCancelled();
                process.Kill();
                res.error = cancelled ? String("Cancelled") : "The local agent did not answer in " + (String)(seconds/60) + " minutes";
                co_return res;
            }

            process.Pump();
            String line;
            while (process.PopLine(line))
                res.output += line + "\n";

            bool exited = process.Poll(res.code);
            if (exited && process.IsOutputClosed())
                break;

            // A child the agent left running can keep the pipe open after the agent itself exited
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
            res.output += line + "\n";
        res.stderrTail = process.StderrTail();
        co_return res;
    }

    static void RemoveFolder(const String& folder)
    {
        std::error_code ec;
        std::filesystem::remove_all(folder.Data(), ec);
    }

    static Vector<String> WriteImages(const String& folder, const Vector<AiImageRef>& images, const String& stem)
    {
        Vector<String> files;
        for (int i = 0; i < images.Count(); i++)
        {
            String file = folder + stem + "-" + (String)(i + 1) + "." + ExtensionOfMime(images[i].mimeType);
            PipelineUtils::WriteFileBytes(file, images[i].data);
            files.Add(file);
        }
        return files;
    }

    static String CodexUsage(const CodexAnswer& answer, const PipelineSettings& settings)
    {
        return (String)answer.inputTokens + " tokens in (" + (String)answer.cachedTokens + " cached), " + (String)answer.outputTokens +
            " out" + String(dot) + (settings.localCodexApiKey.Trimed(" \n\r\t").IsEmpty() ? "by its ChatGPT login" : "on the API key");
    }

    Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                         const Vector<AiImageRef>& images)
    {
        AiTextResult result;
        const PipelineSettings& settings = ctx->settings;
        bool claude = IsClaudeCode(model), codex = IsCodex(model);
        if (!claude && !codex && Bare(model) != commandModel)
        {
            result.error = IsCodexImage(model) ? Bare(model) + " draws pictures: pick a text model, such as " + codexModel
                                               : "Unknown local agent " + Bare(model) + ": use " + claudeCodeModel + ", " + codexModel +
                                                     " or " + commandModel;
            co_return result;
        }

        String folder = MakeCallFolder();
        Invocation call;
        String name;
        if (claude)
        {
            String exe = FindClaude(settings.localAgentPath);
            if (exe.IsEmpty())
            {
                RemoveFolder(folder);
                result.error = "Claude Code is not installed: Pipeline settings, Local agents, Claude Code, Install and sign in";
                co_return result;
            }

            call = ClaudeInvocation(settings, exe, model, prompt, images);
            String claudeModel = ClaudeModelOf(model, settings);
            name = "Claude Code" + (claudeModel.IsEmpty() ? String() : " (" + claudeModel + ")") +
                (settings.GetLocalAgentApiKey().IsEmpty() ? String(" on its login") : String(" --bare on the API key"));
        }
        else if (codex)
        {
            String exe = FindCodex(settings.localCodexPath);
            if (exe.IsEmpty())
            {
                RemoveFolder(folder);
                result.error = "Codex is not installed: Pipeline settings, Local agents, Codex, Install and sign in";
                co_return result;
            }

            String instructions = folder + "instructions.md";
            PipelineUtils::WriteFileBytes(instructions, codexTextInstructions);
            call = CodexInvocation(settings, exe, model, prompt, WriteImages(folder, images, "image"), instructions, folder, false);
            String codexModelId = CodexModelOf(model, settings);
            name = "Codex" + (codexModelId.IsEmpty() ? String() : " (" + codexModelId + ")");
        }
        else
        {
            if (settings.localCommand.Trimed(" \n\r\t").IsEmpty())
            {
                RemoveFolder(folder);
                result.error = "No local command is set: write it in the pipeline settings, Local agent";
                co_return result;
            }

            call = CommandInvocation(settings, prompt, WriteImages(folder, images, "image"));
            name = "the local command";
        }

        ctx->Log("local agent: asking " + name + (images.IsEmpty() ? String() : ", " + (String)images.Count() + " image(s)"));
        CallOutcome run = co_await RunCall(ctx, call, folder, timeoutSeconds);
        RemoveFolder(folder);
        if (!run.error.IsEmpty())
        {
            result.error = run.error;
            co_return result;
        }

        if (claude)
        {
            ClaudeAnswer answer = ParseClaudeOutput(run.output, run.code, run.stderrTail);
            if (answer.result.ok)
            {
                char cost[32];
                snprintf(cost, sizeof(cost), "%.4f", answer.costUsd);
                ctx->Log("local agent: Claude Code answered" + String(dot) + (String)answer.inputTokens + " tokens in, " +
                         (String)answer.outputTokens + " out" + String(dot) + "$" + String(cost) + " by its account");
            }
            co_return answer.result;
        }

        if (codex)
        {
            CodexAnswer answer = ParseCodexOutput(run.output, run.code, run.stderrTail);
            if (answer.result.ok)
                ctx->Log("local agent: Codex answered" + String(dot) + CodexUsage(answer, settings));
            co_return answer.result;
        }

        String text = run.output.Trimed(" \n\r\t");
        if (run.code != 0 || text.IsEmpty())
        {
            String tail = LastLines(run.stderrTail, 3);
            String what = run.code != 0 ? "failed (exit code " + (String)run.code + ")" : String("printed nothing");
            result.error = "The local command " + what + (tail.IsEmpty() ? String() : ": " + tail);
            co_return result;
        }

        result.ok = true;
        result.text = text;
        co_return result;
    }

    Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                           const Vector<AiImageRef>& references, bool transparentBackground,
                                           const AiImageOptions& options)
    {
        AiBytesResult result;
        const PipelineSettings& settings = ctx->settings;
        if (!IsCodexImage(model))
        {
            result.error = "A local agent answers with text: " + Bare(model) + " cannot draw, pick an image model such as " +
                codexImageModel;
            co_return result;
        }

        String exe = FindCodex(settings.localCodexPath);
        if (exe.IsEmpty())
        {
            result.error = "Codex is not installed: Pipeline settings, Local agents, Codex, Install and sign in";
            co_return result;
        }

        String folder = MakeCallFolder();
        String instructions = folder + "instructions.md";
        PipelineUtils::WriteFileBytes(instructions, codexImageInstructions);
        Invocation call = CodexInvocation(settings, exe, model, CodexImagePrompt(prompt, references.Count(), transparentBackground, options),
                                          WriteImages(folder, references, "reference"), instructions, folder, true);

        ctx->Log("local agent: Codex draws" + (references.IsEmpty() ? String() : ", " + (String)references.Count() + " reference(s)"));
        CallOutcome run = co_await RunCall(ctx, call, folder, imageTimeoutSeconds);
        RemoveFolder(folder);
        if (!run.error.IsEmpty())
        {
            result.error = run.error;
            co_return result;
        }

        CodexAnswer answer = ParseCodexOutput(run.output, run.code, run.stderrTail);
        if (!answer.result.ok)
        {
            result.error = answer.result.error;
            co_return result;
        }

        String home = CodexHome();
        String file = FindCodexImage(home, answer.threadId);
        if (file.IsEmpty())
        {
            String said = answer.result.text.Trimed(" \n\r\t");
            result.error = "Codex made no picture" + (said.IsEmpty() || said.ToLowerCase() == "done" ? String() : ": " + said.SubStr(0, 300));
            co_return result;
        }

        result.data = PipelineUtils::ReadFileBytes(file);
        String lower = file.ToLowerCase();
        result.mimeType = lower.EndsWith(".jpg") || lower.EndsWith(".jpeg") ? "image/jpeg" : lower.EndsWith(".webp") ? "image/webp" : "image/png";
        result.ok = !result.data.IsEmpty();
        if (!result.ok)
            result.error = "Codex's picture is empty: " + file;

        // The pipeline keeps the picture; the thread folder Codex made for this call goes
        RemoveFolder(file.SubStr(0, file.FindLast("/")));
        ctx->Log("local agent: Codex drew" + String(dot) + CodexUsage(answer, settings));
        co_return result;
    }
}
