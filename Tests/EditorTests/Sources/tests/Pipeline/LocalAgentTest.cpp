#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <thread>
#if defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)
#include <unistd.h>
#endif

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineModelMenu.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/Providers/AiHttp.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/Providers/LocalAgentProvider.h"
#include "o2Editor/Pipeline/Providers/LocalAgentSetup.h"

using namespace o2;
using namespace Editor;

namespace
{
    // Sets an environment variable for the scope, then puts the old value back
    struct EnvScope
    {
        String name, old;
        bool   had = false;

        EnvScope(const char* nameIn, const char* value): name(nameIn)
        {
            const char* current = getenv(nameIn);
            had = current != nullptr;
            old = current ? String(current) : String();
#if defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)
            if (value)
                setenv(nameIn, value, 1);
            else
                unsetenv(nameIn);
#endif
        }

        ~EnvScope()
        {
#if defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)
            if (had)
                setenv(name.Data(), old.Data(), 1);
            else
                unsetenv(name.Data());
#endif
        }
    };

    AiTextResult Ask(const PipelineSettings& settings, const String& model, const String& prompt, const Vector<AiImageRef>& images)
    {
        auto ctx = mmake<PipelineExecContext>();
        ctx->settings = settings;
        auto coroutine = AiRouter::GenerateText(ctx, model, prompt, images);
        coroutine.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 60.0f));
        return coroutine.IsDone() ? coroutine.GetResult() : AiTextResult();
    }

    AiBytesResult Draw(const PipelineSettings& settings, const String& model, const String& prompt, const Vector<AiImageRef>& references,
                       bool transparent = false, const AiImageOptions& options = AiImageOptions())
    {
        auto ctx = mmake<PipelineExecContext>();
        ctx->settings = settings;
        auto coroutine = AiRouter::GenerateImage(ctx, model, prompt, references, 0, transparent, options);
        coroutine.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 900.0f));
        return coroutine.IsDone() ? coroutine.GetResult() : AiBytesResult();
    }

    String TempFolder(const String& name)
    {
        std::error_code ec;
        String path = String(std::filesystem::temp_directory_path(ec).string().c_str());
        if (!path.EndsWith("/"))
            path += "/";
        path += name + "/";
        std::filesystem::remove_all(path.Data(), ec);
        std::filesystem::create_directories(path.Data(), ec);
        return path;
    }
}

TEST(LocalAgent, LocalIdsRouteToTheLocalAgentWithoutAKey)
{
    for (String id : { "local:claude-code", "local:claude-code:haiku", "local:command", " local:claude-code:opus ", "local:codex",
                       "local:codex:gpt-6-sol", "local:codex-image" })
    {
        EXPECT_EQ(AiRouter::FamilyOf(id), AiProvider::Local) << id.Data();
        auto routes = AiRouter::RoutesFor(id);
        ASSERT_EQ(routes.Count(), 1) << id.Data();
        EXPECT_EQ(routes[0].provider, AiProvider::Local);
        EXPECT_EQ(AiRouter::ChooseRoute(id, AiRouteKeys()).provider, AiProvider::Local) << "no key is needed";
    }

    EXPECT_EQ(AiRouter::FamilyOf("anthropic/claude-sonnet-5.5"), AiProvider::OpenRouter);
    EXPECT_EQ(AiRouter::FamilyOf("gemini-2.5-flash"), AiProvider::Gemini);
    EXPECT_EQ(AiRouter::ProviderName(AiProvider::Local), "Local agent");
}

TEST(LocalAgent, MenusOfferTheLocalAgentsOnlyWhenTurnedOn)
{
    PipelineSettings settings;
    EXPECT_TRUE(settings.localAgentEnabled) << "on by default: only the agents found are offered";
    EXPECT_EQ(settings.localCodexEffort, "low");
    settings.localAgentEnabled = false;
    EXPECT_TRUE(LocalAgentProvider::OfferedModels(settings).IsEmpty());
    EXPECT_TRUE(LocalAgentProvider::OfferedImageModels(settings).IsEmpty());

    // The agents found: the configured paths here, fakes
    String dir = TempFolder("o2-local-agent-menu");
    for (String name : { "claude", "codex" })
    {
        PipelineUtils::WriteFileBytes(dir + name, "#!/bin/sh\n");
        std::filesystem::permissions((dir + name).Data(), std::filesystem::perms::owner_all);
    }
    settings.localAgentEnabled = true;
    settings.localAgentPath = dir + "claude";
    settings.localCodexPath = dir + "codex";
    Vector<String> expected = { "local:claude-code", "local:claude-code:haiku", "local:claude-code:sonnet", "local:claude-code:opus",
                                "local:codex" };
    EXPECT_EQ(LocalAgentProvider::OfferedModels(settings), expected);
    EXPECT_EQ(LocalAgentProvider::OfferedImageModels(settings), Vector<String>({ "local:codex-image" }));

    settings.localCommand = "my-agent --answer";
    expected.Add("local:command");
    EXPECT_EQ(LocalAgentProvider::OfferedModels(settings), expected);
    std::error_code ec;
    std::filesystem::remove_all(dir.Data(), ec);

    auto group = PipelineModelMenu::GroupOf("local:claude-code:haiku");
    EXPECT_EQ(group.provider, PipelineModelProvider::Local);
    EXPECT_EQ(group.label, "Local agent");
    EXPECT_EQ(PipelineModelMenu::RowName("local:claude-code:haiku"), "Claude Code Haiku");
    EXPECT_EQ(PipelineModelMenu::RowName("local:claude-code"), "Claude Code");
    EXPECT_EQ(PipelineUtils::PrettyModelName("local:claude-code:claude-sonnet-4-5"), "Claude Code (claude-sonnet-4-5) \xC2\xB7 local");
    EXPECT_EQ(PipelineUtils::PrettyModelName("local:command"), "Local command");
    EXPECT_EQ(PipelineModelMenu::RowName("local:codex"), "Codex");
    EXPECT_EQ(PipelineModelMenu::RowName("local:codex-image"), "Codex image");
    EXPECT_EQ(PipelineUtils::PrettyModelName("local:codex:gpt-6-sol"), "Codex (gpt-6-sol) \xC2\xB7 local");
}

TEST(LocalAgent, CodexRunsOnItsLoginWithItsToolsOff)
{
    PipelineSettings settings;
    settings.localCodexModel = "gpt-6-sol";
    auto call = LocalAgentProvider::CodexInvocation(settings, "/opt/tools/codex", "local:codex", "Say \"hi\"", { "/tmp/a.png", "/tmp/b.png" },
                                                    "/tmp/call/instructions.md", "/tmp/call/", false);
    auto& a = call.args;
    auto after = [&](const String& flag) { int i = a.IndexOf(flag); return i >= 0 && i + 1 < a.Count() ? a[i + 1] : String("<none>"); };
    auto has = [&](const String& flag, const String& value)
    {
        for (int i = 0; i + 1 < a.Count(); i++)
            if (a[i] == flag && a[i + 1] == value)
                return true;
        return false;
    };
    EXPECT_EQ(a[0], "exec");
    for (String flag : { "--json", "--ephemeral", "--skip-git-repo-check", "--ignore-user-config", "--ignore-rules" })
        EXPECT_TRUE(a.Contains(flag)) << flag.Data();
    EXPECT_EQ(after("--sandbox"), "read-only");
    EXPECT_EQ(after("-C"), "/tmp/call/");
    for (String feature : { "apps", "multi_agent", "goals", "shell_tool", "plugins", "view_image", "image_generation" })
        EXPECT_TRUE(has("--disable", feature)) << feature.Data();
    EXPECT_TRUE(has("--enable", "omit_skill_descriptions"));
    EXPECT_TRUE(has("-c", "model_instructions_file=\"/tmp/call/instructions.md\""));
    EXPECT_TRUE(has("-c", "web_search=\"disabled\""));
    EXPECT_TRUE(has("-c", "model_reasoning_effort=\"low\"")) << "low by default";
    EXPECT_EQ(after("-m"), "gpt-6-sol");
    EXPECT_TRUE(a.Contains("--image=/tmp/a.png") && a.Contains("--image=/tmp/b.png")) << "one value each";
    EXPECT_FALSE(a.Contains("-") || a.Contains("-i")) << "the prompt goes on stdin";
    EXPECT_EQ(call.input, "Say \"hi\"");

    // Its ChatGPT login: no API key reaches it, and node is found from a Dock-started editor
    EXPECT_TRUE(call.env.Contains("OPENAI_API_KEY"));
    EXPECT_TRUE(call.env.Contains("CODEX_API_KEY"));
    auto path = call.env.FindOrDefault([](const String& e) { return e.StartsWith("PATH="); });
    EXPECT_TRUE(path.StartsWith("PATH=/opt/tools:")) << path.Data();
    EXPECT_TRUE(path.Contains("/.local/bin:/opt/homebrew/bin:/usr/local/bin")) << path.Data();

    // Drawing keeps image_gen; the id's model wins; a key of the settings runs it on the API; an unknown effort is left out
    settings.localCodexApiKey = "sk-test";
    settings.localCodexEffort = "turbo";
    call = LocalAgentProvider::CodexInvocation(settings, "codex", "local:codex:gpt-6-luna", "x", {}, "/i.md", "/f/", true);
    EXPECT_FALSE(call.args.Contains("image_generation"));
    EXPECT_EQ(call.args[call.args.IndexOf("-m") + 1], "gpt-6-luna");
    EXPECT_TRUE(call.env.Contains("CODEX_API_KEY=sk-test"));
    EXPECT_FALSE(call.args.Any([](const String& arg) { return arg.StartsWith("model_reasoning_effort"); }));
}

TEST(LocalAgent, ReadsCodexEventsOrSaysWhyThereIsNone)
{
    String ok = "{\"type\":\"thread.started\",\"thread_id\":\"01a1-thread\"}\n"
                "{\"type\":\"item.completed\",\"item\":{\"id\":\"item_0\",\"type\":\"error\",\"message\":\"Under-development features\"}}\n"
                "{\"type\":\"turn.started\"}\n"
                "{\"type\":\"item.completed\",\"item\":{\"id\":\"item_1\",\"type\":\"agent_message\",\"text\":\" Blue \\n\"}}\n"
                "{\"type\":\"turn.completed\",\"usage\":{\"input_tokens\":1400,\"cached_input_tokens\":600,\"output_tokens\":9}}\n";
    auto answer = LocalAgentProvider::ParseCodexOutput(ok, 0, "Reading prompt from stdin...\n");
    EXPECT_TRUE(answer.result.ok) << answer.result.error.Data();
    EXPECT_EQ(answer.result.text, "Blue");
    EXPECT_EQ(answer.threadId, "01a1-thread");
    EXPECT_EQ(answer.inputTokens, 1400);
    EXPECT_EQ(answer.cachedTokens, 600);
    EXPECT_EQ(answer.outputTokens, 9);

    String failed = "{\"type\":\"thread.started\",\"thread_id\":\"t\"}\n"
                    "{\"type\":\"error\",\"message\":\"Reconnecting... 1/5\"}\n"
                    "{\"type\":\"turn.failed\",\"error\":{\"message\":\"unexpected status 401 Unauthorized\"}}\n";
    answer = LocalAgentProvider::ParseCodexOutput(failed, 1, "");
    EXPECT_FALSE(answer.result.ok);
    EXPECT_TRUE(answer.result.error.StartsWith("Codex: unexpected status 401 Unauthorized")) << answer.result.error.Data();
    EXPECT_TRUE(answer.result.error.Contains("Local agents, Codex, Sign in")) << answer.result.error.Data();

    answer = LocalAgentProvider::ParseCodexOutput("", 127, "env: node: No such file or directory\n");
    EXPECT_FALSE(answer.result.ok);
    EXPECT_EQ(answer.result.error, "Codex ended without an answer (exit code 127): env: node: No such file or directory");
}

TEST(LocalAgent, CodexDrawsWhatIsAskedAndItsPictureIsFound)
{
    AiImageOptions options;
    options.aspectRatio = "3:2";
    String prompt = LocalAgentProvider::CodexImagePrompt(" A red gem ", 1, true, options);
    EXPECT_TRUE(prompt.StartsWith("A red gem\n\nThe attached image is the reference of this request.")) << prompt.Data();
    EXPECT_TRUE(prompt.Contains("Frame: aspect ratio 3:2."));
    EXPECT_TRUE(prompt.Contains("Background: fully transparent, keep the alpha channel."));
    EXPECT_EQ(LocalAgentProvider::CodexImagePrompt("A gem", 0, false, AiImageOptions()), "A gem");

    String home = TempFolder("o2-codex-home");
    String thread = home + "generated_images/01a1-thread/";
    std::error_code ec;
    std::filesystem::create_directories(thread.Data(), ec);
    PipelineUtils::WriteFileBytes(thread + "exec-old.png", "OLD");
    std::filesystem::last_write_time((thread + "exec-old.png").Data(),
                                     std::filesystem::file_time_type::clock::now() - std::chrono::seconds(60), ec);
    PipelineUtils::WriteFileBytes(thread + "exec-new.png", "NEW");
    PipelineUtils::WriteFileBytes(thread + "notes.txt", "x");
    EXPECT_EQ(LocalAgentProvider::FindCodexImage(home, "01a1-thread"), thread + "exec-new.png");
    EXPECT_EQ(LocalAgentProvider::FindCodexImage(home, "other"), "");
    EXPECT_EQ(LocalAgentProvider::FindCodexImage(home, "../generated_images/01a1-thread"), "") << "only a thread id";
    EXPECT_EQ(LocalAgentProvider::FindCodexImage(home, ""), "");
    std::filesystem::remove_all(home.Data(), ec);
}

TEST(LocalAgent, ClaudeCodeRunsOnceWithoutToolsAndWithItsImagesInline)
{
    EnvScope noKey("ANTHROPIC_API_KEY", nullptr);
    PipelineSettings settings;
    settings.localAgentModel = "sonnet";
    String prompt = "Say \"hi\"\nthen stop";
    auto call = LocalAgentProvider::ClaudeInvocation(settings, "/bin/claude", "local:claude-code:haiku", prompt,
                                                     { { "image/png", "PNGDATA" } });

    auto& a = call.args;
    auto after = [&](const String& flag) { int i = a.IndexOf(flag); return i >= 0 && i + 1 < a.Count() ? a[i + 1] : String("<none>"); };
    EXPECT_TRUE(a.Contains("-p"));
    EXPECT_EQ(after("--input-format"), "stream-json");
    EXPECT_EQ(after("--output-format"), "stream-json");
    EXPECT_EQ(after("--tools"), "") << "no tools at all";
    EXPECT_EQ(after("--system-prompt"), LocalAgentProvider::systemPrompt) << "a short system prompt instead of Claude Code's";
    for (String flag : { "--strict-mcp-config", "--no-session-persistence", "--disable-slash-commands", "--verbose" })
        EXPECT_TRUE(a.Contains(flag)) << flag.Data();
    EXPECT_EQ(after("--model"), "haiku") << "the id's model wins over the settings";
    EXPECT_FALSE(a.Contains("--bare")) << "no key: Claude Code's own login";
    EXPECT_TRUE(call.env.Contains("MAX_THINKING_TOKENS=0"));
    EXPECT_FALSE(call.env.Any([](const String& e) { return e.StartsWith("ANTHROPIC_API_KEY="); }));
    EXPECT_TRUE(call.env.Contains("ANTHROPIC_API_KEY")) << "the environment's key is dropped: it runs on its login";

    // One stream-json line: the image block, then the text, escaped
    EXPECT_TRUE(call.input.EndsWith("\n"));
    EXPECT_FALSE(call.input.SubStr(0, call.input.Length() - 1).Contains("\n")) << "one line";
    DataDocument doc;
    ASSERT_TRUE(doc.LoadFromData(call.input.Trimed("\n")));
    EXPECT_EQ(AiHttp::StringOf(AiHttp::Member(&doc, "type")), "user");
    auto content = AiHttp::Member(AiHttp::Member(&doc, "message"), "content");
    ASSERT_TRUE(content && content->IsArray());
    ASSERT_EQ(content->GetElementsCount(), 2);
    auto image = AiHttp::Element(content, 0);
    EXPECT_EQ(AiHttp::StringOf(AiHttp::Member(image, "type")), "image");
    EXPECT_EQ(AiHttp::StringOf(AiHttp::Member(AiHttp::Member(image, "source"), "media_type")), "image/png");
    EXPECT_EQ(PipelineUtils::Base64Decode(AiHttp::StringOf(AiHttp::Member(AiHttp::Member(image, "source"), "data"))), "PNGDATA");
    EXPECT_EQ(AiHttp::StringOf(AiHttp::Member(AiHttp::Element(content, 1), "text")), prompt);

    // The settings' model when the id names none; an API key makes it --bare on that key; thinking only when asked for
    settings.localAgentApiKey = "sk-ant-test";
    settings.localAgentThinking = true;
    call = LocalAgentProvider::ClaudeInvocation(settings, "/bin/claude", "local:claude-code", "x", {});
    EXPECT_EQ(call.args[call.args.IndexOf("--model") + 1], "sonnet");
    EXPECT_TRUE(call.args.Contains("--bare"));
    EXPECT_TRUE(call.env.Contains("ANTHROPIC_API_KEY=sk-ant-test"));
    EXPECT_FALSE(call.env.Contains("MAX_THINKING_TOKENS=0"));

    settings.localAgentModel = "";
    call = LocalAgentProvider::ClaudeInvocation(settings, "/bin/claude", "local:claude-code", "x", {});
    EXPECT_FALSE(call.args.Contains("--model")) << "Claude Code's own default";
}

TEST(LocalAgentSetup, ReadsTheStatusOfEachAgent)
{
    using namespace LocalAgentSetup;
    auto claude = ClaudeStatusOf(0, "{\"loggedIn\":true,\"authMethod\":\"claude.ai\",\"email\":\"me@test\",\"subscriptionType\":\"max\"}");
    EXPECT_EQ(claude.state, State::Ready);
    EXPECT_EQ(claude.detail, "Signed in \xC2\xB7 Claude Max \xC2\xB7 me@test");
    claude = ClaudeStatusOf(1, "{\"loggedIn\":false}");
    EXPECT_EQ(claude.state, State::SignedOut);
    claude = ClaudeStatusOf(0, "{\"loggedIn\":true,\"authMethod\":\"console\"}");
    EXPECT_TRUE(claude.detail.StartsWith("Signed in to the Anthropic Console")) << claude.detail.Data();
    claude = ClaudeStatusOf(127, "env: node: No such file or directory\n");
    EXPECT_EQ(claude.state, State::Failed);
    EXPECT_TRUE(claude.detail.Contains("No such file")) << claude.detail.Data();

    EXPECT_EQ(CodexStatusOf(0, "Logged in using ChatGPT\n").state, State::Ready);
    EXPECT_EQ(CodexStatusOf(0, "Logged in using ChatGPT\n").detail, "Signed in with ChatGPT");
    EXPECT_TRUE(CodexStatusOf(0, "Logged in using an API key - sk-***\n").detail.Contains("API key"));
    EXPECT_EQ(CodexStatusOf(1, "Not logged in\n").state, State::SignedOut);
    EXPECT_EQ(CodexStatusOf(2, "error: unexpected argument\n").state, State::Failed);

    // Codex's catalog: the listed models by priority, the hidden ones left out
    auto models = CodexModelsOf("{\"models\":[{\"slug\":\"b\",\"display_name\":\"B\",\"visibility\":\"list\",\"priority\":3},"
                                "{\"slug\":\"hidden\",\"visibility\":\"hide\",\"priority\":0},"
                                "{\"slug\":\"a\",\"display_name\":\"A\",\"description\":\"First\",\"visibility\":\"list\",\"priority\":1}]}");
    ASSERT_EQ(models.Count(), 2);
    EXPECT_EQ(models[0].id, "a");
    EXPECT_EQ(models[0].description, "First");
    EXPECT_EQ(models[1].name, "B");
    EXPECT_TRUE(CodexModelsOf("not json").IsEmpty());
    EXPECT_FALSE(FallbackCodexModels().IsEmpty());
}

TEST(LocalAgentSetup, TheSetupInstallsWhatIsMissingAndSignsIn)
{
    using namespace LocalAgentSetup;
    String claude = SetupScript(Agent::ClaudeCode);
    EXPECT_TRUE(claude.StartsWith("#!/bin/bash\n"));
    EXPECT_TRUE(claude.Contains("command -v claude"));
    EXPECT_TRUE(claude.Contains("curl -fsSL https://claude.ai/install.sh | bash")) << "the official installer";
    EXPECT_TRUE(claude.Contains("claude auth login --claudeai")) << "the subscription, not the API console";
    EXPECT_TRUE(claude.Contains("$HOME/.local/bin"));

    String codex = SetupScript(Agent::Codex);
    EXPECT_TRUE(codex.Contains("npm install -g @openai/codex"));
    EXPECT_TRUE(codex.Contains("brew install --cask codex")) << "without npm";
    EXPECT_TRUE(codex.Contains("codex login status"));
    EXPECT_TRUE(codex.Contains("\n  codex login ||"));

    // The terminal is opened with the script written next to the temp files (here a stand-in records it)
    Agent opened = Agent::ClaudeCode;
    String script;
    SetSetupOpener([&](Agent agent, const String& path) { opened = agent; script = path; return true; });
    String error;
    EXPECT_TRUE(OpenSetup(Agent::Codex, error)) << error.Data();
    SetSetupOpener(Function<bool(Agent, const String&)>());
    EXPECT_EQ(opened, Agent::Codex);
    EXPECT_TRUE(script.EndsWith("o2-codex-setup.command")) << script.Data();
    EXPECT_EQ(PipelineUtils::ReadFileBytes(script), codex);
#if defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)
    EXPECT_EQ(access(script.Data(), X_OK), 0) << "the terminal runs it";
#endif
}

TEST(LocalAgent, ReadsTheAnswerOrSaysWhyThereIsNone)
{
    String ok = "{\"type\":\"system\",\"subtype\":\"init\"}\n"
                "{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"text\",\"text\":\"draft\"}]}}\n"
                "{\"type\":\"result\",\"subtype\":\"success\",\"is_error\":false,\"result\":\" Blue \\n\",\"total_cost_usd\":0.0012,"
                "\"usage\":{\"input_tokens\":40,\"cache_read_input_tokens\":2,\"cache_creation_input_tokens\":1,\"output_tokens\":3}}\n";
    auto answer = LocalAgentProvider::ParseClaudeOutput(ok, 0, "");
    EXPECT_TRUE(answer.result.ok) << answer.result.error.Data();
    EXPECT_EQ(answer.result.text, "Blue");
    EXPECT_EQ(answer.inputTokens, 43);
    EXPECT_EQ(answer.outputTokens, 3);
    EXPECT_NEAR(answer.costUsd, 0.0012, 1e-6);

    String empty = "{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"text\",\"text\":\"From the message\"}]}}\n"
                   "{\"type\":\"result\",\"subtype\":\"success\",\"is_error\":false,\"result\":\"\"}\n";
    EXPECT_EQ(LocalAgentProvider::ParseClaudeOutput(empty, 0, "").result.text, "From the message");

    String login = "{\"type\":\"result\",\"subtype\":\"success\",\"is_error\":true,\"result\":\"Invalid API key \xC2\xB7 Please run /login\"}\n";
    answer = LocalAgentProvider::ParseClaudeOutput(login, 1, "");
    EXPECT_FALSE(answer.result.ok);
    EXPECT_TRUE(answer.result.error.StartsWith("Claude Code: Invalid API key \xC2\xB7 Please run /login")) << answer.result.error.Data();
    EXPECT_TRUE(answer.result.error.Contains("Local agents, Claude Code, Sign in"));

    answer = LocalAgentProvider::ParseClaudeOutput("not json\n", 2, "error: unknown option '--bare'\n");
    EXPECT_FALSE(answer.result.ok);
    EXPECT_TRUE(answer.result.error.Contains("exit code 2")) << answer.result.error.Data();
    EXPECT_TRUE(answer.result.error.Contains("unknown option '--bare'")) << answer.result.error.Data();
}

#if defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

TEST(LocalAgent, AFakeClaudeCodeGetsTheCallAndItsAnswerComesBack)
{
    EnvScope noKey("ANTHROPIC_API_KEY", nullptr);
    String dir = TempFolder("o2-local-agent-test");
    String exe = dir + "claude";
    PipelineUtils::WriteFileBytes(exe,
        "#!/bin/sh\n"
        "dir=\"$(dirname \"$0\")\"\n"
        "printf '%s\\n' \"$@\" > \"$dir/args.txt\"\n"
        "cat > \"$dir/stdin.txt\"\n"
        "echo \"thinking=${MAX_THINKING_TOKENS-unset}\" > \"$dir/env.txt\"\n"
        "pwd > \"$dir/cwd.txt\"\n"
        "echo '{\"type\":\"system\",\"subtype\":\"init\"}'\n"
        "echo '{\"type\":\"result\",\"subtype\":\"success\",\"is_error\":false,\"result\":\"FAKE ANSWER\",\"total_cost_usd\":0.0001,"
        "\"usage\":{\"input_tokens\":5,\"output_tokens\":2}}'\n");
    std::filesystem::permissions(exe.Data(), std::filesystem::perms::owner_all);

    PipelineSettings settings;
    settings.localAgentPath = exe;
    EXPECT_EQ(LocalAgentProvider::FindClaude(exe), exe);

    auto answer = Ask(settings, "local:claude-code:haiku", "Which colour?", { { "image/png", "IMG" } });
    EXPECT_TRUE(answer.ok) << answer.error.Data();
    EXPECT_EQ(answer.text, "FAKE ANSWER");

    Vector<String> args = PipelineUtils::ReadFileBytes(dir + "args.txt").Split("\n");
    EXPECT_EQ(args[args.IndexOf("--model") + 1], "haiku");
    EXPECT_EQ(args[args.IndexOf("--tools") + 1], "");
    EXPECT_TRUE(PipelineUtils::ReadFileBytes(dir + "stdin.txt").Contains("\"text\":\"Which colour?\""));
    EXPECT_TRUE(PipelineUtils::ReadFileBytes(dir + "stdin.txt").Contains(PipelineUtils::Base64Encode("IMG")));
    EXPECT_EQ(PipelineUtils::ReadFileBytes(dir + "env.txt").Trimed("\n"), "thinking=0");

    // It ran in an empty folder of its own, gone after the call
    String cwd = PipelineUtils::ReadFileBytes(dir + "cwd.txt").Trimed("\n");
    EXPECT_TRUE(cwd.Contains("o2-local-agent-")) << cwd.Data();
    EXPECT_FALSE(std::filesystem::exists(cwd.Data()));

    // A Claude Code that is not there is named in the error
    settings.localAgentPath = dir + "missing/claude";
    {
        EnvScope noPath("PATH", "/nonexistent");
        EnvScope noHome("HOME", "/nonexistent");
        if (LocalAgentProvider::FindClaude(settings.localAgentPath).IsEmpty())
        {
            answer = Ask(settings, "local:claude-code", "x", {});
            EXPECT_FALSE(answer.ok);
            EXPECT_TRUE(answer.error.Contains("Claude Code is not installed")) << answer.error.Data();
        }
    }

    std::error_code ec;
    std::filesystem::remove_all(dir.Data(), ec);
}

TEST(LocalAgent, TheLocalCommandReadsThePromptAndTheImageFiles)
{
    PipelineSettings settings;
    settings.localCommand = "cat";
    auto answer = Ask(settings, "local:command", "Describe it", { { "image/jpeg", "JPG" } });
    EXPECT_TRUE(answer.ok) << answer.error.Data();
    EXPECT_TRUE(answer.text.StartsWith("Describe it")) << answer.text.Data();
    EXPECT_TRUE(answer.text.Contains("The images of this request are these files:"));
    EXPECT_TRUE(answer.text.Contains("image-1.jpg"));

    settings.localCommand = "echo broken >&2; exit 3";
    answer = Ask(settings, "local:command", "x", {});
    EXPECT_FALSE(answer.ok);
    EXPECT_EQ(answer.error, "The local command failed (exit code 3): broken");

    settings.localCommand = "";
    answer = Ask(settings, "local:command", "x", {});
    EXPECT_FALSE(answer.ok);
    EXPECT_TRUE(answer.error.Contains("No local command is set"));

    EXPECT_FALSE(Ask(settings, "local:other", "x", {}).ok);
}

// A fake codex: the text call answers from the JSONL it prints, the drawing one finds the picture it saved under
// CODEX_HOME/generated_images/<thread>, which goes after the call; no API key of the editor reaches it
TEST(LocalAgent, AFakeCodexAnswersAndDraws)
{
    String dir = TempFolder("o2-codex-test");
    String home = dir + "home";
    EnvScope codexHome("CODEX_HOME", home.Data());
    EnvScope apiKey("OPENAI_API_KEY", "sk-from-the-editor");
    String exe = dir + "codex";
    PipelineUtils::WriteFileBytes(exe,
        "#!/bin/sh\n"
        "dir=\"$(dirname \"$0\")\"\n"
        "printf '%s\\n' \"$@\" > \"$dir/args.txt\"\n"
        "cat > \"$dir/stdin.txt\"\n"
        "echo \"openai=${OPENAI_API_KEY-unset} codex=${CODEX_API_KEY-unset}\" > \"$dir/env.txt\"\n"
        "echo '{\"type\":\"thread.started\",\"thread_id\":\"fake-thread\"}'\n"
        "case \" $* \" in *' image_generation '*) ;; *)\n"
        "  mkdir -p \"$CODEX_HOME/generated_images/fake-thread\"\n"
        "  printf 'PNGBYTES' > \"$CODEX_HOME/generated_images/fake-thread/exec-1.png\" ;;\n"
        "esac\n"
        "echo '{\"type\":\"item.completed\",\"item\":{\"id\":\"i\",\"type\":\"agent_message\",\"text\":\"FAKE CODEX\"}}'\n"
        "echo '{\"type\":\"turn.completed\",\"usage\":{\"input_tokens\":10,\"cached_input_tokens\":0,\"output_tokens\":2}}'\n");
    std::filesystem::permissions(exe.Data(), std::filesystem::perms::owner_all);

    PipelineSettings settings;
    settings.localCodexPath = exe;
    auto answer = Ask(settings, "local:codex", "Which colour?", { { "image/png", "IMG" } });
    EXPECT_TRUE(answer.ok) << answer.error.Data();
    EXPECT_EQ(answer.text, "FAKE CODEX");
    Vector<String> args = PipelineUtils::ReadFileBytes(dir + "args.txt").Split("\n");
    EXPECT_TRUE(args.Contains("image_generation")) << "a text call does not draw";
    auto image = args.FindOrDefault([](const String& arg) { return arg.StartsWith("--image="); });
    EXPECT_TRUE(image.EndsWith("image-1.png")) << image.Data();
    EXPECT_EQ(PipelineUtils::ReadFileBytes(dir + "stdin.txt"), "Which colour?");
    EXPECT_EQ(PipelineUtils::ReadFileBytes(dir + "env.txt").Trimed("\n"), "openai=unset codex=unset");
    EXPECT_FALSE(std::filesystem::exists((home + "/generated_images/fake-thread").Data())) << "a text call leaves no picture";

    auto drawn = Draw(settings, "local:codex-image", "A red gem", { { "image/png", "REF" } }, true);
    EXPECT_TRUE(drawn.ok) << drawn.error.Data();
    EXPECT_EQ(drawn.data, "PNGBYTES");
    EXPECT_EQ(drawn.mimeType, "image/png");
    EXPECT_TRUE(PipelineUtils::ReadFileBytes(dir + "stdin.txt").Contains("Background: fully transparent"));
    args = PipelineUtils::ReadFileBytes(dir + "args.txt").Split("\n");
    EXPECT_FALSE(args.Contains("image_generation"));
    EXPECT_TRUE(args.Any([](const String& arg) { return arg.StartsWith("--image=") && arg.EndsWith("reference-1.png"); }));
    EXPECT_FALSE(std::filesystem::exists((home + "/generated_images/fake-thread").Data())) << "the thread's folder goes after the call";

    // A text model does not draw, and Codex drawing does not answer text
    EXPECT_FALSE(Draw(settings, "local:codex", "A gem", {}).ok);
    EXPECT_TRUE(Ask(settings, "local:codex-image", "x", {}).error.Contains("draws pictures"));

    std::error_code ec;
    std::filesystem::remove_all(dir.Data(), ec);
}

// The checks run each agent's own status command in the background: fakes say signed in or out, an API key needs no sign-in,
// a missing agent is not installed, Codex's catalog comes after its status
TEST(LocalAgentSetup, ChecksRunTheAgentsOwnStatusCommands)
{
    using namespace LocalAgentSetup;
    String dir = TempFolder("o2-local-agent-check");
    PipelineUtils::WriteFileBytes(dir + "claude",
        "#!/bin/sh\n"
        "echo \"$@\" > \"$(dirname \"$0\")/claude-args.txt\"\n"
        "if [ -f \"$(dirname \"$0\")/signed-in\" ]; then echo '{\"loggedIn\":true,\"authMethod\":\"claude.ai\",\"subscriptionType\":\"pro\"}';\n"
        "else echo '{\"loggedIn\":false}'; exit 1; fi\n");
    PipelineUtils::WriteFileBytes(dir + "codex",
        "#!/bin/sh\n"
        "case \"$*\" in\n"
        "  'login status') echo 'Not logged in' >&2; exit 1 ;;\n"
        "  'debug models --bundled') echo '{\"models\":[{\"slug\":\"fake-1\",\"display_name\":\"Fake One\",\"visibility\":\"list\",\"priority\":1}]}' ;;\n"
        "esac\n");
    for (String name : { "claude", "codex" })
        std::filesystem::permissions((dir + name).Data(), std::filesystem::perms::owner_all);

    auto run = [](Check& check)
    {
        for (int i = 0; i < 400 && !check.Poll(); i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return check.GetStatus();
    };

    PipelineSettings settings;
    settings.localAgentPath = dir + "claude";
    settings.localCodexPath = dir + "codex";
    Check claude, codex;
    claude.Start(Agent::ClaudeCode, settings);
    EXPECT_EQ(claude.GetStatus().state, State::Checking);
    EXPECT_EQ(run(claude).state, State::SignedOut);
    EXPECT_EQ(PipelineUtils::ReadFileBytes(dir + "claude-args.txt").Trimed("\n"), "auth status --json");

    PipelineUtils::WriteFileBytes(dir + "signed-in", "");
    claude.Start(Agent::ClaudeCode, settings);
    auto status = run(claude);
    EXPECT_EQ(status.state, State::Ready);
    EXPECT_EQ(status.detail, "Signed in \xC2\xB7 Claude Pro");
    EXPECT_EQ(status.exe, dir + "claude");

    codex.Start(Agent::Codex, settings);
    EXPECT_EQ(run(codex).state, State::SignedOut) << "its status comes on stderr";
    EXPECT_EQ(Check::GetCodexModels()[0].id, "fake-1") << "the catalog read after the status";

    settings.localCodexApiKey = "sk-test";
    codex.Start(Agent::Codex, settings);
    EXPECT_EQ(codex.GetStatus().state, State::Ready) << "an API key needs no sign-in";
    EXPECT_TRUE(codex.GetStatus().detail.Contains("API key"));
    run(codex);

    settings.localCodexPath = dir + "missing/codex";
    {
        EnvScope noPath("PATH", "/nonexistent");
        EnvScope noHome("HOME", "/nonexistent");
        if (LocalAgentProvider::FindCodex(settings.localCodexPath).IsEmpty())
        {
            codex.Start(Agent::Codex, settings);
            EXPECT_EQ(codex.GetStatus().state, State::NotInstalled);
        }
    }

    std::error_code ec;
    std::filesystem::remove_all(dir.Data(), ec);
}

// The real Codex against a stand-in of the Responses API on this computer, through a wrapper that adds the stand-in as its
// provider: no tokens are spent. Gated: O2_CODEX_STUB_URL=http://127.0.0.1:<port>/v1 of a stub that answers
// O2_CODEX_STUB_ANSWER (default STUB ANSWER)
TEST(LocalAgent, RealCodexAgainstAStubApi)
{
    const char* url = getenv("O2_CODEX_STUB_URL");
    if (!url)
        GTEST_SKIP() << "O2_CODEX_STUB_URL is not set";

    String real = LocalAgentProvider::FindCodex("");
    ASSERT_FALSE(real.IsEmpty()) << "Codex is not installed";
    String dir = TempFolder("o2-codex-stub");
    String wrapper = dir + "codex";
    PipelineUtils::WriteFileBytes(wrapper, "#!/bin/sh\nexec \"" + real + "\" \"$@\" -c model_provider=stub -c 'model_providers.stub.name=\"stub\"' "
                                  "-c 'model_providers.stub.base_url=\"" + String(url) + "\"' -c 'model_providers.stub.wire_api=\"responses\"' "
                                  "-c model_providers.stub.requires_openai_auth=false\n");
    std::filesystem::permissions(wrapper.Data(), std::filesystem::perms::owner_all);

    PipelineSettings settings;
    settings.localCodexPath = wrapper;
    settings.localCodexModel = "gpt-stub";
    String png = PipelineValue::Image(PipelineImageOps::Blank(8, 8, Color4(0, 200, 0, 255))).GetPngBytes();
    const char* expected = getenv("O2_CODEX_STUB_ANSWER");
    auto answer = Ask(settings, "local:codex", "What colour is it? One word.", { { "image/png", png } });
    EXPECT_TRUE(answer.ok) << answer.error.Data();
    EXPECT_TRUE(answer.text.StartsWith(expected ? String(expected) : String("STUB ANSWER"))) << answer.text.Data();

    std::error_code ec;
    std::filesystem::remove_all(dir.Data(), ec);
}

// Real Codex on its ChatGPT login (spends the subscription's limits): it draws a button after a reference, then says what
// colour the button is. Gated: O2_CODEX_REAL=1; O2_CODEX_REAL_OUT names a file the picture is written to
TEST(LocalAgent, RealCodexDrawsAndAnswers)
{
    if (!getenv("O2_CODEX_REAL"))
        GTEST_SKIP() << "O2_CODEX_REAL is not set";

    PipelineSettings settings;
    ASSERT_FALSE(LocalAgentProvider::FindCodex("").IsEmpty()) << "Codex is not installed";
    String reference = PipelineValue::Image(PipelineImageOps::Blank(64, 64, Color4(30, 120, 220, 255))).GetPngBytes();
    auto drawn = Draw(settings, "local:codex-image", "A glossy round game button in the colour of the reference image, centred, "
                      "on a plain white background. Square image.", { { "image/png", reference } });
    ASSERT_TRUE(drawn.ok) << drawn.error.Data();
    auto bitmap = DecodeImageBytes(drawn.data);
    ASSERT_TRUE(bitmap) << "a picture Codex saved";
    EXPECT_GT(bitmap->GetSize().x, 255);
    if (const char* out = getenv("O2_CODEX_REAL_OUT"))
        PipelineUtils::WriteFileBytes(out, drawn.data);

    auto answer = Ask(settings, "local:codex", "What colour is the button in this picture? Answer with one word.",
                      { { drawn.mimeType, drawn.data } });
    ASSERT_TRUE(answer.ok) << answer.error.Data();
    EXPECT_TRUE(answer.text.ToLowerCase().Contains("blue")) << answer.text.Data();
    printf("codex answered: %s\n", answer.text.Data());
}

// The real Claude Code against a stand-in of the Anthropic API on this computer: no tokens are spent. Gated:
// O2_LOCAL_AGENT_STUB_URL=http://127.0.0.1:<port> of a stub that answers O2_LOCAL_AGENT_STUB_ANSWER (default STUB ANSWER)
TEST(LocalAgent, RealClaudeCodeAgainstAStubApi)
{
    const char* url = getenv("O2_LOCAL_AGENT_STUB_URL");
    if (!url)
        GTEST_SKIP() << "O2_LOCAL_AGENT_STUB_URL is not set";

    const char* expected = getenv("O2_LOCAL_AGENT_STUB_ANSWER");
    EnvScope base("ANTHROPIC_BASE_URL", url);
    PipelineSettings settings;
    settings.localAgentApiKey = "sk-ant-stub-000";
    ASSERT_FALSE(LocalAgentProvider::FindClaude("").IsEmpty()) << "Claude Code is not installed";

    String png = PipelineValue::Image(PipelineImageOps::Blank(8, 8, Color4(0, 200, 0, 255))).GetPngBytes();
    auto answer = Ask(settings, "local:claude-code:claude-haiku-4-5", "What colour is it? One word.", { { "image/png", png } });
    EXPECT_TRUE(answer.ok) << answer.error.Data();
    EXPECT_EQ(answer.text, expected ? String(expected) : String("STUB ANSWER"));
}

#endif
