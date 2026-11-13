#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Network/Sockets/TcpListener.h"
#include "o2/Network/Sockets/TcpSocket.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/Providers/AssetsLineAi.h"

using namespace o2;
using namespace Editor;

namespace
{
    // 4x2 lossless: red, green, blue, white over transparent black, half-alpha yellow, black, gray
    const char* gridWebP = "UklGRkAAAABXRUJQVlA4TDMAAAAvA0AAED9AkG3To598GgKCouuWEwgQnDtHIGlj8/HkDwBn0UaGewWBABmbUqLAgoj+BzYA";

    // The same picture as a PNG
    const char* gridPng = "iVBORw0KGgoAAAANSUhEUgAAAAQAAAACCAYAAAB/qH1jAAAAHUlEQVR4nGP4z8DwHwwZ/oMBAwgAqQYQ1dDQ8B8AFSQP8+yVEZEAAAAASUVORK5CYII=";

    // 16x16 lossy, one colour (40, 120, 200)
    const char* solidWebP = "UklGRj4AAABXRUJQVlA4IDIAAADQAQCdASoQABAAAMASJaACdLoB+AADsAD+8JtD/4yp8SF+JC/8ZU/998Pe+HvfD/JQAA==";

    String Bytes(const char* base64)
    {
        return PipelineUtils::Base64Decode(base64);
    }

    // The AI endpoints of AssetsLine as a connected editor sees them: records the request, answers what the test set
    struct FakeAssetsLine
    {
        struct Connection : public RefCounterable
        {
            Ref<TcpSocket> socket;
            String         buffer;
        };

        Ref<TcpListener>        listener;
        Vector<Ref<Connection>> connections;

        String method, path, body;
        Map<String, String> headers;
        int requests = 0;

        int                 status = 200;
        String              answer;
        Map<String, String> answerHeaders;

        FakeAssetsLine()
        {
            listener = mmake<TcpListener>();
            listener->onAccepted = [this](const Ref<TcpSocket>& socket)
            {
                auto connection = mmake<Connection>();
                connection->socket = socket;
                connections.Add(connection);
                Connection* raw = connection.Get();
                socket->onDataReceived = [this, raw](const String& data)
                {
                    raw->buffer.append(data.data(), data.size());
                    Process(raw);
                };
            };
            EXPECT_TRUE(listener->Listen(0));
        }

        ~FakeAssetsLine()
        {
            listener->Close();
            for (auto& connection : connections)
                connection->socket->Close();
        }

        String Url() const
        {
            return "http://127.0.0.1:" + String(listener->GetLocalPort());
        }

        String Header(const String& name) const
        {
            for (auto& kv : headers)
            {
                if (kv.first.ToLowerCase() == name.ToLowerCase())
                    return kv.second;
            }
            return "";
        }

        void Process(Connection* connection)
        {
            size_t headersEnd = connection->buffer.find("\r\n\r\n");
            if (headersEnd == std::string::npos)
                return;

            Map<String, String> parsed;
            size_t lineEnd = connection->buffer.find("\r\n");
            String requestLine = connection->buffer.substr(0, lineEnd);
            size_t begin = lineEnd + 2;
            while (begin < headersEnd)
            {
                size_t end = connection->buffer.find("\r\n", begin);
                String line = connection->buffer.substr(begin, end - begin);
                size_t colon = line.find(':');
                if (colon != std::string::npos)
                    parsed[line.substr(0, colon)] = String(line.substr(colon + 1)).Trimed(" \t");
                begin = end + 2;
            }

            int length = 0;
            for (auto& kv : parsed)
            {
                if (kv.first.ToLowerCase() == "content-length")
                    length = atoi(kv.second.Data());
            }

            if (connection->buffer.size() < headersEnd + 4 + (size_t)length)
                return;

            requests++;
            headers = parsed;
            body = connection->buffer.substr(headersEnd + 4, (size_t)length);
            size_t methodEnd = requestLine.find(' ');
            method = requestLine.substr(0, methodEnd);
            path = requestLine.substr(methodEnd + 1, requestLine.find(' ', methodEnd + 1) - methodEnd - 1);
            connection->buffer.clear();

            String response = "HTTP/1.1 " + String(status) + " X\r\nContent-Length: " + String((int)answer.size()) + "\r\n";
            for (auto& kv : answerHeaders)
                response += kv.first + ": " + kv.second + "\r\n";
            response += "\r\n";
            response.append(answer.data(), answer.size());
            connection->socket->Send(response);
        }
    };

    // Connects the AI calls to the fake server for the test's lifetime
    struct Connected
    {
        explicit Connected(const String& url)
        {
            AssetsLineAi::SetClientSource([url](AssetsLineClient& client)
            {
                client.serverUrl = url;
                client.token = "token-1";
                client.projectId = "project-1";
                client.clientId = "o2-test";
                return true;
            });
        }

        ~Connected() { AssetsLineAi::SetClientSource({}); }
    };

    // The provider keys of the environment stand in for the settings' ones: a test of a missing key hides them
    struct NoEnvKeys
    {
        Vector<Pair<String, String>> saved;

        NoEnvKeys()
        {
            for (auto name : { "GEMINI_API_KEY", "OPENAI_API_KEY", "OPENROUTER_API_KEY", "ELEVENLABS_API_KEY", "KLING_ACCESS_KEY" })
            {
                if (const char* value = getenv(name))
                {
                    saved.Add({ name, value });
                    Set(name, "");
                }
            }
        }

        ~NoEnvKeys()
        {
            for (auto& kv : saved)
                Set(kv.first.Data(), kv.second.Data());
        }

        static void Set(const char* name, const char* value)
        {
#if defined(PLATFORM_WINDOWS)
            _putenv_s(name, value);
#else
            if (*value)
                setenv(name, value, 1);
            else
                unsetenv(name);
#endif
        }
    };

    struct NotConnected
    {
        NotConnected() { AssetsLineAi::SetClientSource([](AssetsLineClient&) { return false; }); }
        ~NotConnected() { AssetsLineAi::SetClientSource({}); }
    };

    Ref<PipelineExecContext> Context(Vector<String>* log = nullptr)
    {
        auto ctx = mmake<PipelineExecContext>();
        ctx->pipelineId = "pipeline-1";
        ctx->pipelineName = "Wordfall main";
        ctx->nodeId = "node-1";
        ctx->nodeType = "nanoBananaGen";
        if (log)
            ctx->log = [log](const String& line) { log->Add(line); };
        return ctx;
    }

    template<typename _result>
    _result Await(Coroutine<_result> coroutine)
    {
        coroutine.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 20.0f));
        return coroutine.GetResult();
    }

    bool SamePixels(const Ref<Bitmap>& a, const Ref<Bitmap>& b)
    {
        if (!a || !b || a->GetSize() != b->GetSize())
            return false;

        int count = a->GetSize().x*a->GetSize().y*4;
        return memcmp(a->GetData(), b->GetData(), count) == 0;
    }
}

TEST(PipelineImageBytes, WebPDecodesLikeThePngOfTheSamePicture)
{
    String webp = Bytes(gridWebP);
    EXPECT_TRUE(IsWebPBytes(webp));
    EXPECT_FALSE(IsPngBytes(webp));
    EXPECT_TRUE(IsPngBytes(Bytes(gridPng)));

    auto fromWebP = DecodeImageBytes(webp);
    auto fromPng = DecodeImageBytes(Bytes(gridPng));
    ASSERT_TRUE(fromWebP);
    ASSERT_TRUE(fromPng);
    EXPECT_EQ(fromWebP->GetSize(), Vec2I(4, 2));
    EXPECT_TRUE(SamePixels(fromWebP, fromPng));

    auto solid = DecodeImageBytes(Bytes(solidWebP));
    ASSERT_TRUE(solid);
    EXPECT_EQ(solid->GetSize(), Vec2I(16, 16));
    const UInt8* p = solid->GetData() + (8*16 + 8)*4;
    EXPECT_NEAR(p[0], 40, 6);
    EXPECT_NEAR(p[1], 120, 6);
    EXPECT_NEAR(p[2], 200, 6);
    EXPECT_EQ(p[3], 255);
}

TEST(PipelineImageBytes, AnyReadableImageBecomesAPng)
{
    String png = ToPngBytes(Bytes(gridWebP));
    ASSERT_TRUE(IsPngBytes(png));
    EXPECT_TRUE(SamePixels(DecodeImageBytes(png), DecodeImageBytes(Bytes(gridPng))));

    EXPECT_EQ(ToPngBytes(Bytes(gridPng)), Bytes(gridPng)) << "a PNG passes as it is";
    EXPECT_TRUE(ToPngBytes("not an image").IsEmpty());

    // A value labelled PNG that holds another format is re-encoded where a PNG is written
    auto value = PipelineValue::Bytes(PipelinePortType::Image, Bytes(gridWebP), "image/png");
    EXPECT_TRUE(IsPngBytes(value.GetPngBytes()));
}

// A key of this editor runs the call directly; without one a connected editor runs it on its AssetsLine account
TEST(AiRouterPlan, OwnKeysFirstThenAssetsLine)
{
    NoEnvKeys noEnvKeys;
    AiRouteKeys none;
    AiRouteKeys router = { "", "", "r" };

    auto plan = AiRouter::Plan("sourceful/riverflow-v2.5-pro", router, true);
    EXPECT_EQ(plan.kind, AiRouteKind::Direct);
    EXPECT_TRUE(plan.route == (AiRoute{ AiProvider::OpenRouter, "sourceful/riverflow-v2.5-pro" }));

    plan = AiRouter::Plan("sourceful/riverflow-v2.5-pro", none, true);
    EXPECT_EQ(plan.kind, AiRouteKind::AssetsLine);

    plan = AiRouter::Plan("sourceful/riverflow-v2.5-pro", none, false);
    EXPECT_EQ(plan.kind, AiRouteKind::None);
    EXPECT_EQ(plan.error, AiRouter::NoKeyMessage("sourceful/riverflow-v2.5-pro"));

    // The alternative route's key counts as a key of this editor
    plan = AiRouter::Plan("gemini-2.5-flash", router, true);
    EXPECT_EQ(plan.kind, AiRouteKind::Direct);
    EXPECT_TRUE(plan.route == (AiRoute{ AiProvider::OpenRouter, "google/gemini-2.5-flash" }));

    // A key that cannot render a transparent background is no key for that call
    plan = AiRouter::Plan("gpt-image-2", router, true, true);
    EXPECT_EQ(plan.kind, AiRouteKind::AssetsLine);

    // A local agent runs here, never through AssetsLine
    plan = AiRouter::Plan("local:claude-code", none, true);
    EXPECT_EQ(plan.kind, AiRouteKind::Direct);
    EXPECT_EQ(plan.route.provider, AiProvider::Local);

    EXPECT_EQ(AiRouter::PlanDirect("eleven_v3", true, true).kind, AiRouteKind::Direct);
    EXPECT_EQ(AiRouter::PlanDirect("eleven_v3", false, true).kind, AiRouteKind::AssetsLine);
    EXPECT_EQ(AiRouter::PlanDirect("eleven_v3", false, false).kind, AiRouteKind::None);
}

TEST(AssetsLineAi, TheImageBodyCarriesTheCallAndWhereItComesFromButNoKey)
{
    auto ctx = Context();
    ctx->settings.openRouterApiKey = "secret-key";
    AiImageOptions options;
    options.renderSize = "2K";
    options.aspectRatio = "9:16";
    auto body = AssetsLineAi::ImageBody(ctx, "sourceful/riverflow-v2.5-pro", "a board", { { "image/png", "abc" } }, 7, true, options);
    String text = body.SaveAsString();

    EXPECT_EQ(PipelineUtils::ValueToString(body["model"]), "sourceful/riverflow-v2.5-pro");
    EXPECT_EQ(PipelineUtils::ValueToString(body["prompt"]), "a board");
    EXPECT_EQ(PipelineUtils::ValueToString(body["references"][0]["data"]), PipelineUtils::Base64Encode("abc"));
    EXPECT_EQ(PipelineUtils::ValueToString(body["references"][0]["mimeType"]), "image/png");
    EXPECT_EQ((int)body["seed"], 7);
    EXPECT_TRUE((bool)body["transparentBackground"]);
    EXPECT_EQ(PipelineUtils::ValueToString(body["renderSize"]), "2K");
    EXPECT_EQ(PipelineUtils::ValueToString(body["aspectRatio"]), "9:16");
    EXPECT_EQ(PipelineUtils::ValueToString(body["pipeline"]), "Wordfall main");
    EXPECT_EQ(PipelineUtils::ValueToString(body["node"]["id"]), "node-1");
    EXPECT_EQ(PipelineUtils::ValueToString(body["node"]["type"]), "nanoBananaGen");
    EXPECT_FALSE(text.Contains("secret-key"));

    auto plain = AssetsLineAi::ImageBody(nullptr, "m", "p", {}, -1, false, AiImageOptions());
    EXPECT_FALSE(plain.FindMember("seed"));
    EXPECT_FALSE(plain.FindMember("references"));
    EXPECT_FALSE(plain.FindMember("transparentBackground"));
    EXPECT_FALSE(plain.FindMember("node"));
}

TEST(AssetsLineAi, ErrorAnswersReadAsTheNodeShowsThem)
{
    EXPECT_TRUE(AssetsLineAi::ErrorMessage(402, "out_of_tokens", "").StartsWith(AssetsLineAi::outOfTokens));
    EXPECT_EQ(AssetsLineAi::ErrorMessage(403, "route_blocked", "Model x has no route"), "AssetsLine: Model x has no route");
    EXPECT_EQ(AssetsLineAi::ErrorMessage(502, "provider_error", "boom"), "AssetsLine: the provider failed: boom");
    EXPECT_TRUE(AssetsLineAi::ErrorMessage(401, "", "").Contains("access token"));

    AssetsLineResponse offline;
    offline.offline = true;
    offline.error = "cannot reach";
    EXPECT_TRUE(AssetsLineAi::ReadAnswer(offline, "https://assetsline.app").error.StartsWith("AssetsLine: cannot reach https://assetsline.app"));

    AssetsLineResponse ok;
    ok.ok = true;
    ok.status = 200;
    ok.headers["content-type"] = "image/webp; q=1";
    ok.headers["x-assetsline-spent"] = "1234";
    ok.headers["x-assetsline-balance"] = "56789";
    ok.headers["x-assetsline-route"] = "OpenRouter on tokens";
    auto answer = AssetsLineAi::ReadAnswer(ok, "u");
    EXPECT_TRUE(answer.ok);
    EXPECT_EQ(answer.mimeType, "image/webp");
    EXPECT_EQ(answer.spent, 1234);
    EXPECT_EQ(answer.balance, 56789);
    EXPECT_EQ(AssetsLineAi::RouteLine(answer), "route: AssetsLine - OpenRouter on tokens (1 234 tokens, 56 789 left)");
}

// The reported case: a node with an OpenRouter model, no key in this editor, the project connected to AssetsLine
TEST(AssetsLineAi, AnImageCallWithoutAKeyRunsOnTheConnectedAccount)
{
    NoEnvKeys noEnvKeys;
    FakeAssetsLine server;
    server.answer = Bytes(gridWebP);
    server.answerHeaders["Content-Type"] = "image/webp";
    server.answerHeaders["X-AssetsLine-Spent"] = "134";
    server.answerHeaders["X-AssetsLine-Balance"] = "9000";
    server.answerHeaders["X-AssetsLine-Route"] = "OpenRouter on tokens";
    Connected connected(server.Url());

    Vector<String> log;
    auto ctx = Context(&log);
    auto result = Await(AiRouter::GenerateImage(ctx, "sourceful/riverflow-v2.5-pro", "a word game board", { { "image/png", "ref" } }, 3));

    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.mimeType, "image/webp");
    EXPECT_TRUE(SamePixels(DecodeImageBytes(result.data), DecodeImageBytes(Bytes(gridPng))));

    EXPECT_EQ(server.method, "POST");
    EXPECT_EQ(server.path, "/api/sync/ai/image");
    EXPECT_EQ(server.Header("Authorization"), "Bearer token-1");
    EXPECT_EQ(server.Header("X-Project-Id"), "project-1");
    DataDocument sent;
    ASSERT_TRUE(sent.LoadFromData(server.body)) << server.body;
    EXPECT_EQ(PipelineUtils::ValueToString(sent["model"]), "sourceful/riverflow-v2.5-pro");
    EXPECT_EQ(PipelineUtils::ValueToString(sent["prompt"]), "a word game board");
    EXPECT_EQ((int)sent["seed"], 3);
    EXPECT_TRUE(log.Contains("route: AssetsLine - OpenRouter on tokens (134 tokens, 9 000 left)")) << (log.IsEmpty() ? String() : log.Last());
}

TEST(AssetsLineAi, TextCallsAndRefusalsGoThroughTheSameWay)
{
    NoEnvKeys noEnvKeys;
    FakeAssetsLine server;
    server.answer = "{\"text\":\" four words \",\"spentTokens\":3}";
    server.answerHeaders["Content-Type"] = "application/json";
    Connected connected(server.Url());

    auto text = Await(AiRouter::GenerateText(Context(), "openai/gpt-5.5", "say it", {}));
    ASSERT_TRUE(text.ok) << text.error;
    EXPECT_EQ(text.text, "four words");
    EXPECT_EQ(server.path, "/api/sync/ai/text");

    server.status = 402;
    server.answer = "{\"error\":\"Not enough tokens\",\"code\":\"out_of_tokens\",\"topUpUrl\":\"https://assetsline.app/#/billing\"}";
    auto refused = Await(AiRouter::GenerateImage(Context(), "gemini-3.1-flash-image", "x", {}, -1));
    EXPECT_FALSE(refused.ok);
    EXPECT_TRUE(refused.error.StartsWith(AssetsLineAi::outOfTokens)) << refused.error;
    EXPECT_TRUE(refused.error.Contains("https://assetsline.app/#/billing")) << refused.error;
}

TEST(AssetsLineAi, AKeyOfThisEditorNeverSpendsTheAccount)
{
    NoEnvKeys noEnvKeys;
    FakeAssetsLine server;
    Connected connected(server.Url());
    auto ctx = Context();
    ctx->settings.elevenLabsApiKey = "own";
    EXPECT_EQ(AiRouter::Plan(ctx, "sourceful/riverflow-v2.5-pro").kind, AiRouteKind::AssetsLine);
    EXPECT_EQ(AiRouter::PlanDirect("eleven_v3", true).kind, AiRouteKind::Direct);

    ctx->settings.openRouterApiKey = "own";
    EXPECT_EQ(AiRouter::Plan(ctx, "sourceful/riverflow-v2.5-pro").kind, AiRouteKind::Direct);
    EXPECT_EQ(server.requests, 0);
}

TEST(AssetsLineAi, WithoutKeyOrConnectionTheNodeSaysWhatToDo)
{
    NoEnvKeys noEnvKeys;
    NotConnected notConnected;
    auto result = Await(AiRouter::GenerateImage(Context(), "sourceful/riverflow-v2.5-pro", "x", {}, -1));
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, AiRouter::NoKeyMessage("sourceful/riverflow-v2.5-pro"));
    EXPECT_TRUE(result.error.Contains("connect AssetsLine"));
}
