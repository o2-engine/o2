#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "Network/TestHttpServer.h"
#include "o2/Network/Http/HttpCache.h"
#include "o2/Network/Http/HttpCookieJar.h"
#include "o2/Network/Http/HttpRequest.h"
#include "o2/Network/Http/HttpResponse.h"
#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/Bitmap/Bitmap.h"

using namespace o2;

namespace
{
    // Selects the HTTP backend and resets cookies and cache for the scope of one test pass
    struct HttpEnvGuard
    {
        HttpBackendType saved;

        HttpEnvGuard(HttpBackendType type): saved(o2Network.GetHttpBackendType())
        {
            o2Network.SetHttpBackendType(type);
            o2Network.GetCookies()->Clear();
            o2Network.GetCache()->Clear();
        }

        ~HttpEnvGuard()
        {
            o2Network.SetHttpBackendType(saved);
            o2Network.GetCookies()->Clear();
            o2Network.GetCache()->Clear();
        }
    };

    const HttpBackendType kBothBackends[] = { HttpBackendType::Platform, HttpBackendType::Generic };

    const char* BackendName(HttpBackendType type)
    {
        return type == HttpBackendType::Platform ? "Platform" : "Generic";
    }

    // Sends the request and pumps until the callback delivers the response
    Ref<HttpResponse> AwaitResponse(const Ref<HttpRequest>& request, float timeoutSeconds = 10.0f)
    {
        Ref<HttpResponse> response;
        o2Network.SendRequest(request, [&](const Ref<HttpResponse>& result) { response = result; });
        NetPumpUntil([&] { return (bool)response; }, timeoutSeconds);
        return response;
    }
}

TEST(Http, EchoAllMethods)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    struct MethodCase { HttpMethod method; const char* name; bool hasBody; };
    const MethodCase cases[] = {
        { HttpMethod::Get, "GET", false },
        { HttpMethod::Post, "POST", true },
        { HttpMethod::Put, "PUT", true },
        { HttpMethod::Delete, "DELETE", false },
        { HttpMethod::Patch, "PATCH", true },
        { HttpMethod::Options, "OPTIONS", false },
    };

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        for (auto& methodCase : cases)
        {
            SCOPED_TRACE(methodCase.name);

            auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/echo", methodCase.method);
            if (methodCase.hasBody)
                request->body = "payload";

            auto response = AwaitResponse(request);
            ASSERT_TRUE(response);
            EXPECT_TRUE(response->IsSuccess());
            EXPECT_EQ(response->status, 200);
            EXPECT_EQ(response->body, String(methodCase.name) + "|" + (methodCase.hasBody ? "payload" : ""));
        }
    }
}

TEST(Http, HeadRequestHasNoBody)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto response = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/echo", HttpMethod::Head));
        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 200);
        EXPECT_TRUE(response->body.IsEmpty());
    }
}

TEST(Http, RequestHeadersReachServerAndComeBack)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/echo");
        request->headers[String("X-Test")] = "custom-value";

        auto response = AwaitResponse(request);
        ASSERT_TRUE(response);
        EXPECT_EQ(response->GetHeader("X-Echo-Header"), String("custom-value"));
        EXPECT_EQ(response->GetHeader("x-echo-header"), String("custom-value"));
        EXPECT_EQ(response->GetHeader("Content-Type"), String("text/plain"));
    }
}

TEST(Http, JsonRequestAndResponse)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        // Request body from a DataDocument
        DataDocument outgoing;
        outgoing["player"] = "tester";
        outgoing["score"] = 100;

        auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/echo", HttpMethod::Post);
        request->SetBodyJson(outgoing);

        auto response = AwaitResponse(request);
        ASSERT_TRUE(response);
        EXPECT_TRUE(response->body.find("tester") != std::string::npos);

        // Response body into a DataDocument
        auto jsonResponse = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/json"));
        ASSERT_TRUE(jsonResponse);
        ASSERT_TRUE(jsonResponse->IsSuccess());

        DataDocument document;
        ASSERT_TRUE(jsonResponse->GetBodyJson(document));
        EXPECT_EQ((String)document["name"], String("o2"));
        EXPECT_EQ((int)document["value"], 42);
        EXPECT_EQ(document["items"].GetElementsCount(), 3);
    }
}

TEST(Http, ImageDownload)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto response = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/image"));
        ASSERT_TRUE(response);
        ASSERT_TRUE(response->IsSuccess());
        EXPECT_EQ(response->GetHeader("Content-Type"), String("image/png"));

        auto bitmap = response->GetBodyImage();
        ASSERT_TRUE(bitmap);
        EXPECT_EQ(bitmap->GetSize(), Vec2I(16, 16));
    }
}

TEST(Http, ErrorStatusIsNotSuccess)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto response = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/status/404"));
        ASSERT_TRUE(response);
        EXPECT_FALSE(response->IsSuccess());
        EXPECT_EQ(response->error, HttpError::None);
        EXPECT_EQ(response->status, 404);
        EXPECT_EQ(response->body, String("not found"));
    }
}

TEST(Http, InvalidUrlFails)
{
    HttpEnvGuard guard(HttpBackendType::Generic);

    auto response = AwaitResponse(mmake<HttpRequest>(String("not a url")));
    ASSERT_TRUE(response);
    EXPECT_EQ(response->error, HttpError::InvalidUrl);
    EXPECT_FALSE(response->IsSuccess());
}

TEST(Http, ConnectionRefusedFails)
{
    // Bind an ephemeral port and close it, so the request goes to a closed port
    int closedPort = 0;
    {
        auto server = mmake<TestHttpServer>();
        ASSERT_TRUE(server->Start());
        closedPort = server->GetPort();
        server->Stop();
    }

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto response = AwaitResponse(mmake<HttpRequest>("http://127.0.0.1:" + String(closedPort) + "/echo"));
        ASSERT_TRUE(response);
        EXPECT_EQ(response->error, HttpError::ConnectionFailed);
    }
}

TEST(Http, TimeoutFails)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/slow");
        request->timeout = 1.0f;

        auto response = AwaitResponse(request, 15.0f);
        ASSERT_TRUE(response);
        EXPECT_EQ(response->error, HttpError::Timeout);
    }
}

TEST(Http, RedirectIsFollowed)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto response = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/redirect"));
        ASSERT_TRUE(response);
        EXPECT_TRUE(response->IsSuccess());
        EXPECT_EQ(response->body, String("GET|"));
    }
}

TEST(Http, RedirectLoopStops)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/redirect-loop");
        request->maxRedirects = 3;

        auto response = AwaitResponse(request);
        ASSERT_TRUE(response);
        EXPECT_EQ(response->error, HttpError::TooManyRedirects);
    }
}

TEST(Http, RedirectNotFollowedWhenDisabled)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/redirect");
        request->followRedirects = false;

        auto response = AwaitResponse(request);
        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, 302);
        EXPECT_EQ(response->GetHeader("Location"), String("/echo"));
    }
}

TEST(Http, PostRedirectTurnsIntoGet)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/redirect", HttpMethod::Post);
        request->body = "post data";

        auto response = AwaitResponse(request);
        ASSERT_TRUE(response);
        EXPECT_TRUE(response->IsSuccess());
        EXPECT_EQ(response->body, String("GET|"));
        EXPECT_EQ(server->lastMethod, String("GET"));
    }
}

TEST(Http, ChunkedResponse)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto response = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/chunked"));
        ASSERT_TRUE(response);
        ASSERT_TRUE(response->IsSuccess());
        EXPECT_EQ(response->body, String("hello world"));
    }
}

TEST(Http, BigDownload)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto response = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/big"), 20.0f);
        ASSERT_TRUE(response);
        ASSERT_TRUE(response->IsSuccess());
        ASSERT_EQ((int)response->body.size(), 1024 * 1024);
        EXPECT_EQ(response->body[0], 'a');
        EXPECT_EQ(response->body[22], 'w');
    }
}

TEST(Http, CoroutineRequests)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto coroutine = [](String baseUrl) -> Coroutine<String>
        {
            // Sequential requests written linearly with co_await
            Ref<HttpResponse> getResponse = co_await o2Network.HttpGetAsync(baseUrl + "/echo");
            if (!getResponse->IsSuccess())
                co_return String("get failed");

            Ref<HttpResponse> postResponse = co_await o2Network.HttpPostAsync(baseUrl + "/echo", "from coroutine");
            if (!postResponse->IsSuccess())
                co_return String("post failed");

            co_return getResponse->body + " + " + postResponse->body;
        }(server->GetBaseUrl());

        coroutine.Start(JobThread::Main);

        ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }, 10.0f));
        EXPECT_EQ(coroutine.GetResult(), String("GET| + POST|from coroutine"));
    }
}

TEST(Http, CookiesStoredAndSentBack)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);

        auto setResponse = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cookies/set"));
        ASSERT_TRUE(setResponse);
        ASSERT_TRUE(setResponse->IsSuccess());
        EXPECT_EQ(o2Network.GetCookies()->GetCookies().Count(), 2);

        auto showResponse = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cookies/show"));
        ASSERT_TRUE(showResponse);
        EXPECT_TRUE(showResponse->body.find("session=abc123") != std::string::npos) << showResponse->body.Data();
        EXPECT_TRUE(showResponse->body.find("theme=dark") != std::string::npos) << showResponse->body.Data();
    }
}

TEST(Http, CookiesDisabledPerRequest)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    HttpEnvGuard guard(HttpBackendType::Generic);

    AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cookies/set"));

    auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/cookies/show");
    request->useCookies = false;

    auto response = AwaitResponse(request);
    ASSERT_TRUE(response);
    EXPECT_EQ(response->body, String("-"));
}

TEST(Http, CacheServesFreshResponses)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);
        server->cacheHits = 0;

        auto first = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cache"));
        ASSERT_TRUE(first);
        ASSERT_TRUE(first->IsSuccess());
        EXPECT_FALSE(first->fromCache);
        EXPECT_EQ(server->cacheHits, 1);

        auto second = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cache"));
        ASSERT_TRUE(second);
        EXPECT_TRUE(second->fromCache);
        EXPECT_EQ(second->body, first->body);
        EXPECT_EQ(server->cacheHits, 1); // Served from the cache, the server was not asked
    }
}

TEST(Http, CacheRevalidatesWithEtag)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    for (auto backend : kBothBackends)
    {
        SCOPED_TRACE(BackendName(backend));
        HttpEnvGuard guard(backend);
        server->cacheHits = 0;

        auto first = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cache-revalidate"));
        ASSERT_TRUE(first);
        ASSERT_TRUE(first->IsSuccess());
        EXPECT_FALSE(first->fromCache);
        EXPECT_EQ(server->cacheHits, 1);

        // max-age=0: the entry is stale at once; the second request revalidates and gets 304
        auto second = AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cache-revalidate"));
        ASSERT_TRUE(second);
        EXPECT_TRUE(second->fromCache);
        EXPECT_EQ(second->body, String("revalidated content"));
        EXPECT_EQ(server->cacheHits, 2); // The server was asked and answered 304
    }
}

TEST(Http, CacheBypassPolicy)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    HttpEnvGuard guard(HttpBackendType::Generic);
    server->cacheHits = 0;

    for (int i = 0; i < 2; i++)
    {
        auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/cache");
        request->cachePolicy = HttpCachePolicy::Bypass;

        auto response = AwaitResponse(request);
        ASSERT_TRUE(response);
        EXPECT_FALSE(response->fromCache);
    }

    EXPECT_EQ(server->cacheHits, 2);
    EXPECT_EQ(o2Network.GetCache()->GetCount(), 0);
}

TEST(Http, CachePreferCachedPolicy)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    HttpEnvGuard guard(HttpBackendType::Generic);
    server->cacheHits = 0;

    // Store a stale entry, then PreferCached serves it without asking the server
    AwaitResponse(mmake<HttpRequest>(server->GetBaseUrl() + "/cache-revalidate"));
    EXPECT_EQ(server->cacheHits, 1);

    auto request = mmake<HttpRequest>(server->GetBaseUrl() + "/cache-revalidate");
    request->cachePolicy = HttpCachePolicy::PreferCached;

    auto response = AwaitResponse(request);
    ASSERT_TRUE(response);
    EXPECT_TRUE(response->fromCache);
    EXPECT_EQ(server->cacheHits, 1);
}
