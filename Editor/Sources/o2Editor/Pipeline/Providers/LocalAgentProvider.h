#pragma once

#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------------
    // Local agents: text and vision answers from an agent on this computer,
    // paid by its own account, not by an API key of the pipeline. Model ids:
    // local:claude-code[:<model>] runs Claude Code once (claude -p) with no
    // tools, no MCP servers, no session and a short system prompt, the
    // images inline; local:codex[:<model>] runs Codex once (codex exec) on
    // its ChatGPT login with its tools, apps, plugins and skill list off and
    // a short instruction file; local:codex-image draws with Codex's own
    // image_gen tool and takes the file it saves; local:command runs the
    // shell command of the settings with the prompt on stdin and takes
    // stdout as the answer
    // -----------------------------------------------------------------------
    namespace LocalAgentProvider
    {
        const String prefix = "local:";                     // Every local model id starts with it
        const String claudeCodeModel = "local:claude-code"; // Claude Code on the model of the settings
        const String commandModel = "local:command";        // The local command of the settings
        const String codexModel = "local:codex";             // Codex on the model of the settings
        const String codexImageModel = "local:codex-image";  // Codex drawing with its image_gen tool

        // The models Claude Code is offered with in the menus besides its default
        const Vector<String> claudeModels = { "haiku", "sonnet", "opus" };

        // How long one answer may take, and one picture
        const int timeoutSeconds = 600;
        const int imageTimeoutSeconds = 900;

        // Reasoning efforts Codex takes
        const Vector<String> codexEfforts = { "minimal", "low", "medium", "high", "xhigh" };

        // System prompt Claude Code answers with instead of its own long one
        const String systemPrompt = "You answer requests of a game asset pipeline. Do exactly what the request asks and reply "
                                    "with the answer only: no preamble, no questions back.";

        // True when the model id names a local agent
        bool IsLocalModel(const String& model);

        // True when the model id names Claude Code
        bool IsClaudeCode(const String& model);

        // True when the model id names Codex answering with text
        bool IsCodex(const String& model);

        // True when the model id names Codex drawing
        bool IsCodexImage(const String& model);

        // Returns the model of a local:codex id: its third part, else the settings' model; empty leaves the choice to Codex
        String CodexModelOf(const String& model, const PipelineSettings& settings);

        // Returns the Claude model of a local:claude-code id: its third part, else the settings' model; empty leaves the choice
        // to Claude Code
        String ClaudeModelOf(const String& model, const PipelineSettings& settings);

        // Returns the local model ids the text menus offer: none while the local agents are off, Claude Code and Codex only
        // when they are found, the command only when one is set
        Vector<String> OfferedModels(const PipelineSettings& settings);

        // Returns the local model ids the image menus offer: Codex drawing, while the local agents are on and Codex is found
        Vector<String> OfferedImageModels(const PipelineSettings& settings);

        // Returns the product name of a local model id
        String PrettyName(const String& model);

        // Returns the executables tried for Claude Code, in order: the configured one (a path, or a name looked up on PATH),
        // then claude on PATH, ~/.local/bin, ~/.claude/local, /opt/homebrew/bin and /usr/local/bin; only existing ones
        Vector<String> ClaudeCandidates(const String& configured);

        // Returns the Claude Code executable, empty when none is found
        String FindClaude(const String& configured);

        // Returns the executables tried for Codex, in order: the configured one, then codex on PATH, /opt/homebrew/bin,
        // /usr/local/bin, ~/.local/bin and ~/.npm-global/bin; only existing ones
        Vector<String> CodexCandidates(const String& configured);

        // Returns the Codex executable, empty when none is found
        String FindCodex(const String& configured);

        // Returns Codex's home folder: CODEX_HOME, else ~/.codex
        String CodexHome();

        // Returns the PATH=... entry an agent runs with: its own folder, ~/.local/bin and the Homebrew folders first (an
        // editor started from the Dock has a bare PATH, and codex is a node script that needs node on it)
        String ToolPathEnv(const String& exe);

        // A process to run for one answer
        struct Invocation
        {
            String         exe;   // Executable, an absolute path
            Vector<String> args;  // Its arguments
            Vector<String> env;   // NAME=value set over the inherited environment
            String         input; // What stdin receives before it is closed
        };

        // Builds the Claude Code call: stream-json in and out, the images as base64 blocks of the one user message; with an
        // API key of the settings it runs --bare on it, else on Claude Code's own login with ANTHROPIC_API_KEY dropped, so it
        // never runs on API prices unasked
        Invocation ClaudeInvocation(const PipelineSettings& settings, const String& exe, const String& model,
                                    const String& prompt, const Vector<AiImageRef>& images);

        // Builds the local command call: sh -c with the command, the prompt on stdin, the image files listed after it
        Invocation CommandInvocation(const PipelineSettings& settings, const String& prompt, const Vector<String>& imageFiles);

        // Builds the Codex call: codex exec --json, ephemeral, read-only, in folder, the user config and rules not loaded (so
        // the provider is its ChatGPT login, never one of the config), the tools, apps, plugins, web search and the skill list
        // off, image_gen only when it draws, instructionsFile instead of Codex's own instructions, the images as --image=, the
        // prompt on stdin. OPENAI_API_KEY is dropped and CODEX_API_KEY is the key of the settings or dropped, so it never runs
        // on API prices unasked; the folder of the executable and the Homebrew folders go first on PATH (codex is a node script)
        Invocation CodexInvocation(const PipelineSettings& settings, const String& exe, const String& model, const String& prompt,
                                   const Vector<String>& imageFiles, const String& instructionsFile, const String& folder,
                                   bool draws);

        // What Claude Code answered
        struct ClaudeAnswer
        {
            AiTextResult result;           // The answer, or why there is none
            double       costUsd = 0.0;    // What Claude Code says the call cost
            int          inputTokens = 0;  // Tokens of the request, cache reads included
            int          outputTokens = 0; // Tokens of the answer
        };

        // Reads the stream-json output of Claude Code: the result line, else the last assistant text; the exit code and the
        // stderr tail explain a missing answer
        ClaudeAnswer ParseClaudeOutput(const String& output, int exitCode, const String& stderrTail);

        // What Codex answered
        struct CodexAnswer
        {
            AiTextResult result;           // The last agent message, or why there is none
            String       threadId;         // Thread of the call: its pictures are in generated_images/<threadId>
            int          inputTokens = 0;  // Tokens of the requests, cached ones included
            int          cachedTokens = 0; // Of them read from the cache
            int          outputTokens = 0; // Tokens of the answers
        };

        // Reads the JSONL events of codex exec --json: thread.started, the agent_message items, turn.completed with its usage,
        // turn.failed and error; the exit code and the stderr tail explain a missing answer
        CodexAnswer ParseCodexOutput(const String& output, int exitCode, const String& stderrTail);

        // The request Codex draws: the prompt, then the frame, the size and the transparency asked
        String CodexImagePrompt(const String& prompt, int references, bool transparentBackground, const AiImageOptions& options);

        // Returns the newest picture Codex saved for the thread under codexHome/generated_images, empty when there is none
        String FindCodexImage(const String& codexHome, const String& threadId);

        // Answers the prompt with the local agent of the model id, the images attached
        Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                             const Vector<AiImageRef>& images);

        // Draws the picture with Codex (local:codex-image), the references attached
        Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                               const Vector<AiImageRef>& references, bool transparentBackground,
                                               const AiImageOptions& options);
    }
}
