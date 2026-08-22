#include "o2/stdafx.h"
#include "NetworkSystem.h"

#include "o2/Network/Http/HttpBackend.h"
#include "o2/Network/Http/HttpCache.h"
#include "o2/Network/Http/HttpCookieJar.h"
#include "o2/Network/Http/HttpRequest.h"
#include "o2/Network/Http/HttpResponse.h"
#include "o2/Network/Http/SocketHttpBackend.h"
#include "o2/Network/Sockets/SocketPlatform.h"
#include "o2/Network/Sockets/TcpListener.h"
#include "o2/Network/Sockets/TcpSocket.h"
#include "o2/Network/Sockets/UdpSocket.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    DECLARE_SINGLETON(NetworkSystem);

    FORWARD_REF_IMPL(HttpCache);
    FORWARD_REF_IMPL(HttpCookieJar);
    FORWARD_REF_IMPL(HttpRequest);
    FORWARD_REF_IMPL(HttpResponse);

    // Creates the platform-native HTTP backend, or the generic one where there is none
    static IHttpBackend* CreatePlatformHttpBackend()
    {
#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)
        extern IHttpBackend* CreateAppleHttpBackend();
        return CreateAppleHttpBackend();
#elif defined(PLATFORM_ANDROID)
        extern IHttpBackend* CreateAndroidHttpBackend();
        return CreateAndroidHttpBackend();
#elif defined(PLATFORM_WASM)
        extern IHttpBackend* CreateWasmHttpBackend();
        return CreateWasmHttpBackend();
#else
        return mnew SocketHttpBackend();
#endif
    }

    // Returns the method name for the request line
    static const char* HttpMethodName(HttpMethod method)
    {
        switch (method)
        {
            case HttpMethod::Get:     return "GET";
            case HttpMethod::Post:    return "POST";
            case HttpMethod::Put:     return "PUT";
            case HttpMethod::Delete:  return "DELETE";
            case HttpMethod::Head:    return "HEAD";
            case HttpMethod::Patch:   return "PATCH";
            case HttpMethod::Options: return "OPTIONS";
        }

        return "GET";
    }

    // Splits a raw "Name: value" header line
    static bool SplitHeaderLine(const String& line, String& outName, String& outValue)
    {
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            return false;

        outName = line.substr(0, colon);

        size_t valueBegin = line.find_first_not_of(" \t", colon + 1);
        outValue = valueBegin == std::string::npos ? String() : (String)line.substr(valueBegin);
        return true;
    }

    // Returns true when the header line name matches, case-insensitive
    static bool HeaderNameIs(const String& name, const char* expected)
    {
        size_t length = strlen(expected);
        if (name.size() != length)
            return false;

        for (size_t i = 0; i < length; i++)
        {
            char a = name[i], b = expected[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b)
                return false;
        }

        return true;
    }

    NetworkSystem::NetworkSystem(RefCounter* refCounter):
        Singleton<NetworkSystem>(refCounter)
    {
        mLog = mmake<LogStream>("Network");
        o2Debug.GetLog()->BindStream(mLog);

        mCookies = mmake<HttpCookieJar>();
        mCache = mmake<HttpCache>();
    }

    NetworkSystem::~NetworkSystem()
    {
        if (mHttpBackend)
        {
            delete mHttpBackend;
            mHttpBackend = nullptr;
        }

        SocketPlatform::Deinitialize();
    }

    template<typename _socket_type>
    void NetworkSystem::PumpSockets(Vector<WeakRef<_socket_type>>& sockets, float dt)
    {
        auto pumped = sockets;
        for (auto& weakSocket : pumped)
        {
            if (auto socket = weakSocket.Lock())
                socket->UpdateSocket(dt);
        }

        sockets.RemoveAll([](const WeakRef<_socket_type>& socket) { return !socket.IsValid(); });
    }

    void NetworkSystem::Update(float dt)
    {
        PumpSockets(mTcpListeners, dt);
        PumpSockets(mTcpSockets, dt);
        PumpSockets(mUdpSockets, dt);

        UpdateHttp(dt);
    }

    void NetworkSystem::SendRequest(const Ref<HttpRequest>& request,
                                    const Function<void(const Ref<HttpResponse>&)>& onCompleted)
    {
        Url url = Url::Parse(request->url);
        if (!url.isValid)
        {
            mLog->Error("Invalid request URL '" + request->url + "'");
            CompleteHttp(onCompleted, MakeErrorResponse(HttpError::InvalidUrl));
            return;
        }

        PendingHttp pending;
        pending.request = request;
        pending.callback = onCompleted;
        pending.url = url;
        pending.method = request->method;
        pending.redirectsLeft = request->maxRedirects;

        DispatchHttp(pending);
    }

    Coroutine<Ref<HttpResponse>> NetworkSystem::RequestAsync(const Ref<HttpRequest>& request)
    {
        struct Result: public ThreadSafeRefCounterable { Ref<HttpResponse> response; };

        // The request is dispatched right away; the coroutine only awaits its completion
        Signal done;
        auto result = MakeShared<Result>();
        SendRequest(request, [done, result](const Ref<HttpResponse>& response)
        {
            result->response = response;
            done.Synchronize();
        });

        auto coroutine = [](Signal done, SharedRef<Result> result) -> Coroutine<Ref<HttpResponse>>
        {
            co_await done;
            co_return result->response;
        }(done, result);

        coroutine.Start(JobThread::Main);
        return coroutine;
    }

    void NetworkSystem::HttpGet(const String& url, const Function<void(const Ref<HttpResponse>&)>& onCompleted)
    {
        SendRequest(mmake<HttpRequest>(url), onCompleted);
    }

    Coroutine<Ref<HttpResponse>> NetworkSystem::HttpGetAsync(const String& url)
    {
        return RequestAsync(mmake<HttpRequest>(url));
    }

    void NetworkSystem::HttpPost(const String& url, const String& body,
                                 const Function<void(const Ref<HttpResponse>&)>& onCompleted)
    {
        auto request = mmake<HttpRequest>(url, HttpMethod::Post);
        request->body = body;
        SendRequest(request, onCompleted);
    }

    Coroutine<Ref<HttpResponse>> NetworkSystem::HttpPostAsync(const String& url, const String& body)
    {
        auto request = mmake<HttpRequest>(url, HttpMethod::Post);
        request->body = body;
        return RequestAsync(request);
    }

    const Ref<HttpCookieJar>& NetworkSystem::GetCookies() const
    {
        return mCookies;
    }

    const Ref<HttpCache>& NetworkSystem::GetCache() const
    {
        return mCache;
    }

    void NetworkSystem::SetHttpBackendType(HttpBackendType type)
    {
        if (mHttpBackendType == type)
            return;

        mHttpBackendType = type;

        // Active transfers keep the old backend alive inside their pending entries; here the
        // backend is replaced only when nothing is in flight, otherwise on the next creation
        if (mHttpBackend && mPendingHttp.IsEmpty())
        {
            delete mHttpBackend;
            mHttpBackend = nullptr;
        }
    }

    HttpBackendType NetworkSystem::GetHttpBackendType() const
    {
        return mHttpBackendType;
    }

    const Ref<LogStream>& NetworkSystem::GetLog() const
    {
        return mLog;
    }

    void NetworkSystem::UpdateHttp(float dt)
    {
        if (mHttpBackend)
            mHttpBackend->Update(dt);

        // Deliver queued responses over a copy: callbacks can send new requests
        auto ready = mReadyHttp;
        mReadyHttp.Clear();
        for (auto& item : ready)
            item.callback(item.response);

        // Collect finished transfers first: processing may dispatch redirect hops
        Vector<int> finishedIndexes;
        for (int i = 0; i < mPendingHttp.Count(); i++)
        {
            if (mPendingHttp[i].transfer && mPendingHttp[i].transfer->done.Load() != 0)
                finishedIndexes.Add(i);
        }

        Vector<PendingHttp> finished;
        for (int i = finishedIndexes.Count() - 1; i >= 0; i--)
        {
            finished.Add(mPendingHttp[finishedIndexes[i]]);
            mPendingHttp.RemoveAt(finishedIndexes[i]);
        }

        for (auto& pending : finished)
        {
            auto response = ProcessFinishedHttp(pending);
            if (response)
                pending.callback(response);
        }
    }

    void NetworkSystem::DispatchHttp(PendingHttp& pending)
    {
        String urlString = pending.url.ToString();

        // The cache serves GET requests: fresh entries directly, stale ones add validators
        bool cacheable = pending.method == HttpMethod::Get &&
            pending.request->cachePolicy != HttpCachePolicy::Bypass;

        String etagValidator;
        String lastModifiedValidator;

        if (cacheable)
        {
            if (auto entry = mCache->Find(urlString))
            {
                if (pending.request->cachePolicy == HttpCachePolicy::PreferCached || HttpCache::IsFresh(*entry))
                {
                    CompleteHttp(pending.callback, MakeCachedResponse(urlString));
                    return;
                }

                etagValidator = entry->etag;
                lastModifiedValidator = entry->lastModified;
            }
        }

        auto transfer = MakeShared<HttpTransfer>();
        transfer->method = HttpMethodName(pending.method);
        transfer->url = urlString;
        transfer->timeout = pending.request->timeout;

        // The body is dropped when a redirect turned the method into GET
        if (pending.method == pending.request->method)
            transfer->body = pending.request->body;

        for (auto& kv : pending.request->headers)
            transfer->headerLines.Add(kv.first + ": " + kv.second);

        if (pending.request->useCookies)
        {
            String cookieHeader = mCookies->GetCookieHeader(pending.url);
            if (!cookieHeader.IsEmpty())
                transfer->headerLines.Add("Cookie: " + cookieHeader);
        }

        if (!etagValidator.IsEmpty())
            transfer->headerLines.Add("If-None-Match: " + etagValidator);

        if (!lastModifiedValidator.IsEmpty())
            transfer->headerLines.Add("If-Modified-Since: " + lastModifiedValidator);

        pending.transfer = transfer;
        mPendingHttp.Add(pending);

        GetHttpBackend()->Perform(transfer);
    }

    Ref<HttpResponse> NetworkSystem::ProcessFinishedHttp(PendingHttp& pending)
    {
        auto transfer = pending.transfer;
        String urlString = pending.url.ToString();

        if (transfer->error != HttpError::None)
        {
            mLog->Error("HTTP " + transfer->method + " " + urlString + " failed, error " +
                        String((int)transfer->error));
            return MakeErrorResponse(transfer->error);
        }

        // Store received cookies before any redirect hop, so it sends them
        if (pending.request->useCookies)
        {
            for (auto& line : transfer->responseHeaderLines)
            {
                String name, value;
                if (SplitHeaderLine(line, name, value) && HeaderNameIs(name, "Set-Cookie"))
                    mCookies->SetFromHeader(pending.url, value);
            }
        }

        // Redirects
        bool isRedirect = transfer->status == 301 || transfer->status == 302 || transfer->status == 303 ||
            transfer->status == 307 || transfer->status == 308;

        if (isRedirect && pending.request->followRedirects)
        {
            String location;
            for (auto& line : transfer->responseHeaderLines)
            {
                String name, value;
                if (SplitHeaderLine(line, name, value) && HeaderNameIs(name, "Location"))
                {
                    location = value;
                    break;
                }
            }

            if (!location.IsEmpty())
            {
                if (pending.redirectsLeft <= 0)
                    return MakeErrorResponse(HttpError::TooManyRedirects);

                Url redirected = pending.url.ResolveRedirect(location);
                if (!redirected.isValid)
                    return MakeErrorResponse(HttpError::InvalidUrl);

                pending.redirectsLeft--;
                pending.url = redirected;

                // 303 and the historical 301/302-with-body behavior turn the request into GET
                if (transfer->status == 303 ||
                    ((transfer->status == 301 || transfer->status == 302) && pending.method != HttpMethod::Get &&
                     pending.method != HttpMethod::Head))
                {
                    pending.method = HttpMethod::Get;
                }

                pending.transfer = nullptr;
                DispatchHttp(pending);
                return nullptr;
            }
        }

        // Revalidation: 304 refreshes the entry and serves it
        if (transfer->status == 304 && pending.method == HttpMethod::Get)
        {
            mCache->Revalidate(urlString, transfer->responseHeaderLines);
            auto cached = MakeCachedResponse(urlString);
            if (cached)
                return cached;
        }

        if (pending.method == HttpMethod::Get && pending.request->cachePolicy != HttpCachePolicy::Bypass)
            mCache->Store(urlString, transfer->status, transfer->responseHeaderLines, transfer->responseBody);

        auto response = mmake<HttpResponse>();
        response->status = transfer->status;
        response->body = transfer->responseBody;

        for (auto& line : transfer->responseHeaderLines)
        {
            String name, value;
            if (SplitHeaderLine(line, name, value) && !response->headers.ContainsKey(name))
                response->headers.Add(name, value);
        }

        return response;
    }

    void NetworkSystem::CompleteHttp(const Function<void(const Ref<HttpResponse>&)>& callback,
                                     const Ref<HttpResponse>& response)
    {
        ReadyHttp ready;
        ready.callback = callback;
        ready.response = response;
        mReadyHttp.Add(ready);
    }

    Ref<HttpResponse> NetworkSystem::MakeErrorResponse(HttpError error) const
    {
        auto response = mmake<HttpResponse>();
        response->error = error;
        return response;
    }

    Ref<HttpResponse> NetworkSystem::MakeCachedResponse(const String& url) const
    {
        auto entry = mCache->Find(url);
        if (!entry)
            return nullptr;

        auto response = mmake<HttpResponse>();
        response->status = entry->status;
        response->body = entry->body;
        response->fromCache = true;

        for (auto& line : entry->headerLines)
        {
            String name, value;
            if (SplitHeaderLine(line, name, value) && !response->headers.ContainsKey(name))
                response->headers.Add(name, value);
        }

        return response;
    }

    IHttpBackend* NetworkSystem::GetHttpBackend()
    {
        if (!mHttpBackend)
        {
            if (mHttpBackendType == HttpBackendType::Generic)
                mHttpBackend = mnew SocketHttpBackend();
            else
                mHttpBackend = CreatePlatformHttpBackend();
        }

        return mHttpBackend;
    }

    void NetworkSystem::RegisterTcpSocket(const Ref<TcpSocket>& socket)
    {
        mTcpSockets.Add(WeakRef<TcpSocket>(socket));
    }

    void NetworkSystem::UnregisterTcpSocket(TcpSocket* socket)
    {
        mTcpSockets.RemoveAll([&](const WeakRef<TcpSocket>& other) { return !other.IsValid() || other.Lock().Get() == socket; });
    }

    void NetworkSystem::RegisterTcpListener(const Ref<TcpListener>& listener)
    {
        mTcpListeners.Add(WeakRef<TcpListener>(listener));
    }

    void NetworkSystem::UnregisterTcpListener(TcpListener* listener)
    {
        mTcpListeners.RemoveAll([&](const WeakRef<TcpListener>& other) { return !other.IsValid() || other.Lock().Get() == listener; });
    }

    void NetworkSystem::RegisterUdpSocket(const Ref<UdpSocket>& socket)
    {
        mUdpSockets.Add(WeakRef<UdpSocket>(socket));
    }

    void NetworkSystem::UnregisterUdpSocket(UdpSocket* socket)
    {
        mUdpSockets.RemoveAll([&](const WeakRef<UdpSocket>& other) { return !other.IsValid() || other.Lock().Get() == socket; });
    }
}
// --- META ---

ENUM_META(o2::HttpBackendType, o2__HttpBackendType)
{
    ENUM_ENTRY(Generic);
    ENUM_ENTRY(Platform);
}
END_ENUM_META;
// --- END META ---
